// D5 replay and counter verification. AI assistance is declared in TOOLS.md.
#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace cs253;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t number(const char* text) {
    std::string value = text;
    require(!value.empty(), "expected an unsigned decimal integer");
    for (char digit : value) {
        require(digit >= '0' && digit <= '9', "expected an unsigned decimal integer");
    }
    unsigned long long parsed = std::stoull(value);
    require(parsed <= std::numeric_limits<std::size_t>::max(), "size is too large");
    return static_cast<std::size_t>(parsed);
}

std::size_t height(std::size_t n) {
    std::size_t answer = 0;
    while (n > 1) {
        n = n / 2 + n % 2;
        answer++;
    }
    return answer;
}

// Exact maximum for A's recursive midpoint tree, including uneven sizes.
std::size_t maximum_pushes(std::size_t n) {
    if (n < 2) return 0;
    std::size_t left = n / 2 + n % 2;
    return 1 + height(left) + height(n - left);
}

bool same(const Aggregate& a, const Aggregate& b) {
    return a.length == b.length && a.sum_mod == b.sum_mod &&
           a.minimum == b.minimum && a.maximum == b.maximum;
}

Residue normalized(Value value) {
    Residue result = value % modulus;
    if (result < 0) result += modulus;
    return result;
}

void run(AdversarialTarget target, const std::string& target_name,
         const GeneratorConfig& config, std::ostream& output) {
    set_sqrt_block_size(0);
    Workload work = generate(target, config);
    LazySegmentTree a;
    SqrtDecomposition b;
    a.build(work.initial, work.mode);
    b.build(work.initial, work.mode);
    a.reset_counters();
    b.reset_counters();

    Value reset_value = 0;
    std::size_t queries = 0;
    std::uint64_t probe_pushes = maximum_pushes(config.elements);
    for (std::size_t i = 0; i < work.operations.size(); i++) {
        const Operation& operation = work.operations[i];
        if (std::holds_alternative<Update>(operation)) {
            const Update& update = std::get<Update>(operation);
            a.update(update.range, update.tag);
            b.update(update.range, update.tag);
            reset_value = update.tag.b;
        } else {
            const Query& query = std::get<Query>(operation);
            std::uint64_t before = a.counters().push_downs;
            Aggregate first = a.query(query.range);
            Aggregate second = b.query(query.range);
            require(same(first, second), "A/B mismatch at operation " + std::to_string(i));
            queries++;
            if (target == AdversarialTarget::push_downs && config.elements > 0) {
                require(a.counters().push_downs - before == probe_pushes,
                        "probe failed to attain the exact maximum");
                Aggregate expected;
                expected.length = query.range.end - query.range.begin;
                expected.sum_mod = static_cast<Residue>(
                    (static_cast<std::uint64_t>(normalized(reset_value)) *
                     (expected.length % modulus)) % modulus);
                if (work.mode == Mode::exact_extrema && expected.length > 0) {
                    expected.minimum = reset_value;
                    expected.maximum = reset_value;
                }
                require(same(first, expected), "primed probe disagrees with uniform-array formula");
            }
        }
    }

    // Full invariant audits run after replay, outside operation counter checks.
    Counters ca = a.counters();
    Counters cb = b.counters();
    require(a.check_invariants().ok, "A final invariant failure");
    require(b.check_invariants().ok, "B final invariant failure");
    require(ca.push_downs == a.counters().push_downs &&
            cb.full_blocks == b.counters().full_blocks &&
            cb.partial_elements == b.counters().partial_elements,
            "invariant checker changed counters");
    if (target == AdversarialTarget::push_downs) {
        require(ca.push_downs == (config.operations / 2) * probe_pushes,
                "wrong total push count");
    }
    if (target == AdversarialTarget::block_friendly && config.elements > 0) {
        require(cb.full_blocks == config.operations && cb.partial_elements == 0,
                "aligned workload performed partial block work");
    }

    output << target_name << ',' << (config.mode == Mode::exact_extrema ? "EXACT" : "MODULAR")
           << ',' << config.seed << ',' << config.elements << ',' << config.operations
           << ',' << config.range_width << ',' << queries << ',' << ca.nodes_visited
           << ',' << ca.push_downs << ',' << cb.full_blocks << ',' << cb.partial_elements
           << ",PASS\n";
    require(static_cast<bool>(output), "failed to write verification CSV");
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 7 && argc != 8) {
            std::cerr << "adversary_replay_check TARGET EXACT|MODULAR N Q SEED WIDTH [CSV]\n";
            return 2;
        }
        std::string name = argv[1];
        AdversarialTarget target;
        if (name == "push_downs") target = AdversarialTarget::push_downs;
        else if (name == "alternating_negative_gain") target = AdversarialTarget::alternating_negative_gain;
        else if (name == "block_friendly") target = AdversarialTarget::block_friendly;
        else if (name == "range_width_sweep") target = AdversarialTarget::range_width_sweep;
        else throw std::invalid_argument("unknown target");

        GeneratorConfig config;
        std::string mode = argv[2];
        if (mode == "EXACT") config.mode = Mode::exact_extrema;
        else if (mode == "MODULAR") config.mode = Mode::modular_sum;
        else throw std::invalid_argument("unknown mode");
        config.elements = number(argv[3]);
        config.operations = number(argv[4]);
        config.seed = number(argv[5]);
        config.range_width = number(argv[6]);
        const char* header = "target,mode,seed,n,operations,width,queries,a_nodes,a_pushes,b_full_blocks,b_partial_elements,status\n";
        if (argc == 8) {
            std::ofstream output(argv[7]);
            require(static_cast<bool>(output), "cannot open CSV");
            output << header;
            run(target, name, config, output);
            output.close();
            require(static_cast<bool>(output), "cannot close CSV");
        } else {
            std::cout << header;
            run(target, name, config, std::cout);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "D5 replay failed: " << error.what() << '\n';
        return 1;
    }
}
