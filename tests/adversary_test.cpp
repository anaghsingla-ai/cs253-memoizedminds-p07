// Adversary module tests. Tool assistance is declared in TOOLS.md.
#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include "../adversary/export.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace cs253;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

const AdversarialTarget targets[] = {
    AdversarialTarget::push_downs,
    AdversarialTarget::alternating_negative_gain,
    AdversarialTarget::block_friendly,
    AdversarialTarget::range_width_sweep
};

Mode test_mode(int index) {
    if (index == 0) {
        return Mode::exact_extrema;
    } else {
        return Mode::modular_sum;
    }
}

bool same(const Aggregate& first, const Aggregate& second) {
    if (first.length != second.length) {
        return false;
    }
    if (first.sum_mod != second.sum_mod) {
        return false;
    }
    if (first.minimum != second.minimum) {
        return false;
    }
    if (first.maximum != second.maximum) {
        return false;
    }
    return true;
}

Range operation_range(const Operation& operation) {
    if (std::holds_alternative<Update>(operation)) {
        const Update& update = std::get<Update>(operation);
        return update.range;
    } else {
        const Query& query = std::get<Query>(operation);
        return query.range;
    }
}

// A small independent array for these tests, not the production Oracle.
// Wide arithmetic prevents the reference itself from overflowing.
struct Reference {
    std::vector<Value> values;
    Mode mode;

    static Residue reduce(__int128_t value) {
        Residue remainder = static_cast<Residue>(value % 998244353);
        if (remainder < 0) {
            remainder = remainder + 998244353;
        }
        return remainder;
    }

    explicit Reference(const Workload& workload) {
        values = workload.initial;
        mode = workload.mode;
        if (mode == Mode::modular_sum) {
            for (std::size_t i = 0; i < values.size(); i++) {
                values[i] = reduce(values[i]);
            }
        }
    }

    void update(const Update& update) {
        __int128_t gain = update.tag.a;
        __int128_t offset = update.tag.b;
        if (mode == Mode::modular_sum) {
            gain = reduce(gain);
            offset = reduce(offset);
        }

        for (std::size_t i = update.range.begin; i < update.range.end; i++) {
            __int128_t new_value = gain * values[i] + offset;
            if (mode == Mode::exact_extrema) {
                require(new_value >= std::numeric_limits<Value>::min() &&
                        new_value <= std::numeric_limits<Value>::max(),
                        "exact overflow in test reference");
                values[i] = static_cast<Value>(new_value);
            } else {
                values[i] = reduce(new_value);
            }
        }
    }

    Aggregate query(Range range) const {
        Aggregate answer;
        answer.length = range.end - range.begin;
        __int128_t sum = 0;

        for (std::size_t i = range.begin; i < range.end; i++) {
            sum = sum + values[i];
            if (mode == Mode::exact_extrema) {
                if (!answer.minimum.has_value()) {
                    answer.minimum = values[i];
                } else if (values[i] < answer.minimum.value()) {
                    answer.minimum = values[i];
                }
                if (!answer.maximum.has_value()) {
                    answer.maximum = values[i];
                } else if (values[i] > answer.maximum.value()) {
                    answer.maximum = values[i];
                }
            }
        }
        answer.sum_mod = reduce(sum);
        return answer;
    }
};

void validate(const Workload& workload, const GeneratorConfig& config) {
    require(workload.initial.size() == config.elements, "wrong initial size");
    require(workload.operations.size() == config.operations, "wrong operation count");
    require(workload.seed == config.seed, "wrong seed");
    require(workload.mode == config.mode, "wrong mode");

    for (std::size_t i = 0; i < workload.operations.size(); i++) {
        const Operation& operation = workload.operations[i];
        Range range = operation_range(operation);
        require(range.begin <= range.end && range.end <= config.elements,
                "invalid generated range");

        if (std::holds_alternative<Update>(operation)) {
            const Update& update = std::get<Update>(operation);
            if (config.mode == Mode::exact_extrema) {
                require(update.tag.a >= -1 && update.tag.a <= 1,
                        "invalid exact gain");
            }
        }
    }
}

void differential(const Workload& workload) {
    LazySegmentTree tree;
    SqrtDecomposition blocks;
    tree.build(workload.initial, workload.mode);
    blocks.build(workload.initial, workload.mode);
    Reference reference(workload);

    std::size_t operation_index = 0;
    try {
        for (std::size_t i = 0; i < workload.operations.size(); i++) {
            const Operation& operation = workload.operations[i];
            if (std::holds_alternative<Update>(operation)) {
                const Update& update = std::get<Update>(operation);
                tree.update(update.range, update.tag);
                blocks.update(update.range, update.tag);
                reference.update(update);
            } else {
                const Query& query = std::get<Query>(operation);
                Aggregate expected = reference.query(query.range);
                require(same(tree.query(query.range), expected), "A query mismatch");
                require(same(blocks.query(query.range), expected), "B query mismatch");
            }

            // Audits must not push tags or change the next operation's state.
            require(tree.check_invariants().ok, "A invariant failure");
            require(blocks.check_invariants().ok, "B invariant failure");
            operation_index++;
        }

        Range whole;
        whole.begin = 0;
        whole.end = workload.initial.size();
        Aggregate expected = reference.query(whole);
        require(same(tree.query(whole), expected), "A final mismatch");
        require(same(blocks.query(whole), expected), "B final mismatch");
    } catch (const std::exception& error) {
        std::ofstream output("adversary_failure.txt");
        if (output) {
            adversary::export_workload(output, workload);
            output.close();
        }

        std::cerr << "seed=" << workload.seed;
        std::cerr << " operation=" << operation_index << ": " << error.what();
        if (output) {
            std::cerr << "; workload saved to adversary_failure.txt\n";
        } else {
            std::cerr << "; could not save adversary_failure.txt\n";
        }
        throw;
    }
}

void exact_push_counts() {
    // Compare the generated probe with every possible range for small n.
    for (std::size_t n = 1; n <= 48; n++) {
        GeneratorConfig config;
        config.elements = n;
        config.operations = 2;
        Workload workload = generate(AdversarialTarget::push_downs, config);
        Range probe = std::get<Query>(workload.operations[1]).range;

        LazySegmentTree tree;
        tree.build(workload.initial, config.mode);
        tree.update({0, n}, {0, 7});
        tree.reset_counters();
        tree.query(probe);
        std::uint64_t generated_pushes = tree.counters().push_downs;

        std::uint64_t best = 0;
        for (std::size_t begin = 0; begin <= n; begin++) {
            for (std::size_t end = begin; end <= n; end++) {
                tree.update({0, n}, {0, 7});
                tree.reset_counters();
                tree.query({begin, end});
                std::uint64_t pushes = tree.counters().push_downs;
                if (pushes > best) {
                    best = pushes;
                }
            }
        }
        require(generated_pushes == best,
                "push range not maximal for n=" + std::to_string(n));
    }

    // n = 1, 2, 4, ... 16384. Check repeated generated queries.
    std::size_t n = 1;
    for (unsigned height = 0; height <= 14; height++) {
        GeneratorConfig config;
        config.elements = n;
        config.operations = 10;
        Workload workload = generate(AdversarialTarget::push_downs, config);

        std::uint64_t expected_pushes = 0;
        if (height > 0) {
            expected_pushes = 2 * height - 1;
        }

        LazySegmentTree tree;
        tree.build(workload.initial, config.mode);
        for (std::size_t i = 0; i < workload.operations.size(); i++) {
            const Operation& operation = workload.operations[i];
            if (std::holds_alternative<Update>(operation)) {
                const Update& update = std::get<Update>(operation);
                tree.update(update.range, update.tag);
            } else {
                const Query& query = std::get<Query>(operation);
                tree.reset_counters();
                tree.query(query.range);
                require(tree.counters().push_downs == expected_pushes,
                        "wrong exact query push bound");
            }
        }

        // A partial update follows the same boundary paths.
        if (height > 0) {
            Range probe = std::get<Query>(workload.operations[1]).range;
            for (int repeat = 0; repeat < 3; repeat++) {
                tree.update({0, n}, {0, 7});
                tree.reset_counters();
                tree.update(probe, {-1, 3});
                require(tree.counters().push_downs == expected_pushes,
                        "partial update missed the maximum push count");
            }
        }
        n = n * 2;
    }
}

void block_counts() {
    const std::size_t sizes[] = {1, 2, 17, 49, 50, 64, 1000};
    for (int mode_index = 0; mode_index < 2; mode_index++) {
        for (int size_index = 0; size_index < 7; size_index++) {
            GeneratorConfig config;
            config.elements = sizes[size_index];
            config.operations = 128;
            config.mode = test_mode(mode_index);
            Workload workload = generate(AdversarialTarget::block_friendly, config);

            SqrtDecomposition blocks;
            blocks.build(workload.initial, config.mode);
            for (std::size_t i = 0; i < workload.operations.size(); i++) {
                blocks.reset_counters();
                const Operation& operation = workload.operations[i];
                if (std::holds_alternative<Update>(operation)) {
                    const Update& update = std::get<Update>(operation);
                    blocks.update(update.range, update.tag);
                } else {
                    const Query& query = std::get<Query>(operation);
                    blocks.query(query.range);
                }

                Counters counts = blocks.counters();
                require(counts.full_blocks == 1 && counts.partial_elements == 0,
                        "block-friendly operation did partial work");
            }
        }
    }
}

void scale() {
    for (int target_index = 0; target_index < 4; target_index++) {
        for (int mode_index = 0; mode_index < 2; mode_index++) {
            GeneratorConfig config;
            config.mode = test_mode(mode_index);
            config.range_width = 999;
            Workload workload = generate(targets[target_index], config);
            validate(workload, config);
            require(workload.initial.size() == 1000000 &&
                    workload.operations.size() == 1000000, "scale mismatch");
        }
    }
}

void expect_rejected(AdversarialTarget target, const GeneratorConfig& config) {
    bool rejected = false;
    try {
        generate(target, config);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "invalid config accepted");
}

void errors() {
    GeneratorConfig config;
    config.elements = 5;
    config.operations = 1;

    config.range_width = 0;
    expect_rejected(targets[0], config);
    config.range_width = 6;
    expect_rejected(targets[0], config);
    config.range_width = 1;
    expect_rejected(static_cast<AdversarialTarget>(99), config);
    config.mode = static_cast<Mode>(99);
    expect_rejected(targets[0], config);

    // Check that the arithmetic guard rejects before any huge allocation.
    std::uint64_t safe_operations = static_cast<std::uint64_t>(
        (std::numeric_limits<Value>::max() - 1000) / 31);
    if (std::numeric_limits<std::size_t>::max() > safe_operations) {
        config.mode = Mode::exact_extrema;
        config.operations = static_cast<std::size_t>(safe_operations + 1);
        bool correct_error = false;
        try {
            generate(targets[0], config);
        } catch (const std::length_error& error) {
            if (std::string(error.what()) == "operation count exceeds exact arithmetic bound") {
                correct_error = true;
            }
        }
        require(correct_error, "missing early arithmetic safety guard");
    }
}

void negative_example() {
    GeneratorConfig config;
    config.elements = 2;
    config.operations = 2;
    Workload workload = generate(AdversarialTarget::alternating_negative_gain, config);
    require(std::holds_alternative<Update>(workload.operations[0]) &&
            std::holds_alternative<Query>(workload.operations[1]),
            "missing first-negation probe");

    Reference reference(workload);
    reference.update(std::get<Update>(workload.operations[0]));
    Aggregate result = reference.query(std::get<Query>(workload.operations[1]).range);
    require(result.minimum == -30 && result.maximum == 26,
            "wrong negative extrema example");
}

void width_coverage() {
    GeneratorConfig config;
    config.elements = 257;
    config.operations = 512;
    config.range_width = 7;
    Workload workload = generate(AdversarialTarget::range_width_sweep, config);

    bool single = false;
    bool whole = false;
    bool requested = false;
    for (std::size_t i = 0; i < workload.operations.size(); i++) {
        Range range = operation_range(workload.operations[i]);
        std::size_t width = range.end - range.begin;
        if (width == 1) {
            single = true;
        }
        if (width == 257) {
            whole = true;
        }
        if (width == 7) {
            requested = true;
        }
    }
    require(single && whole && requested, "width sweep missed required widths");

    for (int i = 0; i < 4; i++) {
        config.operations = 0;
        require(generate(targets[i], config).operations.empty(), "q=0 failed");
    }
}

std::size_t reproducibility_cases() {
    const std::size_t sizes[] = {0, 1, 2, 3, 5, 16, 17, 49, 50, 257};
    const Seed seeds[] = {0, 253, std::numeric_limits<Seed>::max()};
    std::size_t runs = 0;

    for (int target_index = 0; target_index < 4; target_index++) {
        for (int mode_index = 0; mode_index < 2; mode_index++) {
            for (int size_index = 0; size_index < 10; size_index++) {
                for (int seed_index = 0; seed_index < 3; seed_index++) {
                    GeneratorConfig config;
                    config.elements = sizes[size_index];
                    config.operations = 512;
                    config.seed = seeds[seed_index];
                    config.mode = test_mode(mode_index);
                    config.range_width = 7;
                    if (config.elements < 7) {
                        config.range_width = config.elements;
                    }

                    Workload first = generate(targets[target_index], config);
                    validate(first, config);

                    std::ostringstream first_output;
                    std::ostringstream second_output;
                    adversary::export_workload(first_output, first);
                    Workload second = generate(targets[target_index], config);
                    adversary::export_workload(second_output, second);
                    require(first_output.str() == second_output.str(),
                            "non-reproducible output");

                    differential(first);
                    runs++;
                }
            }
        }
    }
    return runs;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            if (std::string(argv[1]) == "--scale") {
                scale();
                std::cout << "8 million-operation generation cases passed\n";
                return 0;
            }
        }

        set_sqrt_block_size(0);
        std::size_t runs = reproducibility_cases();
        exact_push_counts();
        block_counts();
        errors();
        negative_example();
        width_coverage();

        std::cout << runs;
        std::cout << " differential/reproducibility cases; push-down maxima and block counters passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "adversary_test: " << error.what() << '\n';
        return 1;
    }
}
