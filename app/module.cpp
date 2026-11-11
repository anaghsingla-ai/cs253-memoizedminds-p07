#include "cs253/pipeline.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>

namespace cs253 {
namespace {

// A resource guard for untrusted workload files; supports the lab's 10^6 scale.
// Adjust deliberately if experiments require larger datasets.
constexpr std::size_t max_elements = 5'000'000;
constexpr std::size_t max_operations = 5'000'000;

std::string next_token(std::istream& input, const char* field) {
    std::string token;
    if (!(input >> token)) {
        throw std::invalid_argument(std::string("Missing or unreadable ") + field);
    }
    return token;
}

template <typename T>
T parse_integer(std::istream& input, const char* field) {
    const std::string token = next_token(input, field);
    T value{};
    const char* const begin = token.data();
    const char* const end = begin + token.size();
    const auto parsed = std::from_chars(begin, end, value, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        throw std::invalid_argument(std::string("Invalid integer for ") + field + ": " + token);
    }
    return value;
}

std::size_t parse_count(std::istream& input, const char* field, std::size_t limit) {
    const auto value = parse_integer<std::uint64_t>(input, field);
    if (value > limit || value > std::numeric_limits<std::size_t>::max()) {
        throw std::out_of_range(std::string("Unsupported ") + field + " size");
    }
    return static_cast<std::size_t>(value);
}

void check_range(Range range, std::size_t n) {
    if (range.begin > range.end || range.end > n) {
        throw std::out_of_range("Operation range is outside [0, n]");
    }
}

void validate_workload(const Workload& workload) {
    if (workload.mode != Mode::exact_extrema && workload.mode != Mode::modular_sum) {
        throw std::invalid_argument("Unknown workload mode");
    }
    if (workload.initial.size() > max_elements || workload.operations.size() > max_operations) {
        throw std::out_of_range("Workload exceeds configured resource limits");
    }
    for (const auto& operation : workload.operations) {
        std::visit([&](const auto& value) {
            check_range(value.range, workload.initial.size());
            using OperationType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<OperationType, Update>) {
                if (workload.mode == Mode::exact_extrema &&
                    value.tag.a != -1 && value.tag.a != 0 && value.tag.a != 1) {
                    throw std::invalid_argument("EXACT mode only supports gains -1, 0, 1");
                }
            }
        }, operation);
    }
}

const char* mode_name(Mode mode) {
    if (mode == Mode::exact_extrema) return "EXACT";
    if (mode == Mode::modular_sum) return "MODULAR";
    throw std::invalid_argument("Unknown workload mode");
}

} // namespace

Workload read_workload(std::istream& input) {
    if (next_token(input, "format header") != "CS253_V1") {
        throw std::invalid_argument("Expected workload header CS253_V1");
    }

    Workload workload;
    const std::string mode = next_token(input, "mode");
    if (mode == "EXACT") workload.mode = Mode::exact_extrema;
    else if (mode == "MODULAR") workload.mode = Mode::modular_sum;
    else throw std::invalid_argument("Unknown workload mode: " + mode);

    workload.seed = parse_integer<Seed>(input, "seed");
    const std::size_t n = parse_count(input, "element count", max_elements);
    const std::size_t q = parse_count(input, "operation count", max_operations);

    workload.initial.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        workload.initial.push_back(parse_integer<Value>(input, "initial value"));
    }
    workload.operations.reserve(q);
    for (std::size_t i = 0; i < q; ++i) {
        const std::string type = next_token(input, "operation type");
        if (type != "U" && type != "Q") {
            throw std::invalid_argument("Unknown operation type: " + type);
        }
        const auto begin = parse_count(input, "range begin", n);
        const auto end = parse_count(input, "range end", n);
        const Range range{begin, end};
        check_range(range, n);
        if (type == "U") {
            const Value a = parse_integer<Value>(input, "gain");
            const Value b = parse_integer<Value>(input, "offset");
            if (workload.mode == Mode::exact_extrema && a != -1 && a != 0 && a != 1) {
                throw std::invalid_argument("EXACT mode only supports gains -1, 0, 1");
            }
            workload.operations.emplace_back(Update{range, {a, b}});
        } else {
            workload.operations.emplace_back(Query{range});
        }
    }

    std::string trailing;
    if (input >> trailing) {
        throw std::invalid_argument("Unexpected token after workload: " + trailing);
    }
    if (input.bad()) {
        throw std::runtime_error("I/O error reading workload");
    }
    return workload;
}

void write_workload(std::ostream& output, const Workload& workload) {
    validate_workload(workload); // Reject malformed objects before writing anything.

    output << "CS253_V1\n" << mode_name(workload.mode) << '\n'
           << workload.seed << '\n'
           << workload.initial.size() << ' ' << workload.operations.size() << '\n';
    for (std::size_t i = 0; i < workload.initial.size(); ++i) {
        if (i) output << ' ';
        output << workload.initial[i];
    }
    output << '\n';
    for (const Operation& operation : workload.operations) {
        std::visit([&](const auto& value) {
            using OperationType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<OperationType, Update>) {
                output << "U " << value.range.begin << ' ' << value.range.end << ' '
                       << value.tag.a << ' ' << value.tag.b << '\n';
            } else {
                output << "Q " << value.range.begin << ' ' << value.range.end << '\n';
            }
        }, operation);
    }
    if (!output) throw std::runtime_error("I/O error writing workload");
}

ReplayResult replay(RangeStructure& structure, const Workload& workload) {
    validate_workload(workload); // Validate before changing the structure's state.
    structure.build(workload.initial, workload.mode);
    structure.reset_counters();

    ReplayResult result;
    std::size_t queries = 0;
    for (const auto& operation : workload.operations) {
        if (std::holds_alternative<Query>(operation)) ++queries;
    }
    result.answers.reserve(queries);

    const auto begin = std::chrono::steady_clock::now();
    for (const auto& operation : workload.operations) {
        if (const auto* update = std::get_if<Update>(&operation)) {
            structure.update(update->range, update->tag);
        } else {
            const auto& query = std::get<Query>(operation);
            result.answers.push_back(structure.query(query.range));
        }
    }
    const auto end = std::chrono::steady_clock::now();
    result.elapsed_seconds = std::chrono::duration<double>(end - begin).count();
    result.counters = structure.counters();
    return result;
}

} // namespace cs253
