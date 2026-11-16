#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef CS253_SAMPLE_WORKLOAD_PATH
#define CS253_SAMPLE_WORKLOAD_PATH "tests/cases/architect_sample.txt"
#endif

namespace {

using namespace cs253;

void print_usage(std::ostream& out) {
    out << "CS253 calibration simulator\n"
        << "Usage:\n"
        << "  calibration --help\n"
        << "  calibration --status\n"
        << "  calibration --run WORKLOAD --structure A|B|both\n"
        << "  calibration --all\n\n"
        << "WORKLOAD uses the CS253_V1 text format. Ranges are zero-based [begin,end).\n"
        << "A = lazy segment tree; B = square-root decomposition; both runs and compares A/B.\n"
        << "--all runs the available A/B application smoke check and reports missing team modules.\n";
}

void print_status() {
    std::cout
        << "parser=ready replay=ready cli=ready structure_A=ready structure_B=ready\n"
        << "oracle=missing adversary=ready benchmarks=implemented_not_integrated\n"
        << "full_pipeline=blocked_until_oracle_and_benchmark_runner_integration\n";
}

void print_optional_value(const std::optional<Value>& value) {
    if (value) std::cout << *value;
    else std::cout << "NA";
}

void print_result(const char* name, const ReplayResult& result,
                  const Workload& workload) {
    std::cout << "STRUCTURE " << name << '\n';
    for (std::size_t i = 0; i < result.answers.size(); ++i) {
        const Aggregate& answer = result.answers[i];
        std::cout << "Q " << i
                  << " LEN " << answer.length
                  << " SUM " << answer.sum_mod
                  << " MIN ";
        print_optional_value(answer.minimum);
        std::cout << " MAX ";
        print_optional_value(answer.maximum);
        std::cout << '\n';
    }

    std::cout << std::setprecision(10)
              << "ELAPSED_SECONDS " << result.elapsed_seconds << '\n';
    if (result.elapsed_seconds > 0.0) {
        const double rate = static_cast<double>(workload.operations.size()) /
                            result.elapsed_seconds;
        std::cout << "OPS_PER_SECOND " << rate << '\n';
    } else {
        std::cout << "OPS_PER_SECOND NA\n";
    }

    const Counters& c = result.counters;
    std::cout << "COUNTERS"
              << " nodes_visited=" << c.nodes_visited
              << " push_downs=" << c.push_downs
              << " full_blocks=" << c.full_blocks
              << " partial_elements=" << c.partial_elements
              << " tag_compositions=" << c.tag_compositions
              << " aggregate_merges=" << c.aggregate_merges
              << '\n';
}

bool same_answers(const std::vector<Aggregate>& a,
                  const std::vector<Aggregate>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].length != b[i].length ||
            a[i].sum_mod != b[i].sum_mod ||
            a[i].minimum != b[i].minimum ||
            a[i].maximum != b[i].maximum) {
            return false;
        }
    }
    return true;
}

Workload load_workload(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open workload file: " + path);
    return read_workload(input);
}

int run_selected(const std::string& path, const std::string& selected) {
    const Workload workload = load_workload(path);

    if (selected == "A") {
        LazySegmentTree structure;
        print_result("A", replay(structure, workload), workload);
        return 0;
    }
    if (selected == "B") {
        SqrtDecomposition structure;
        print_result("B", replay(structure, workload), workload);
        return 0;
    }
    if (selected == "both") {
        LazySegmentTree a;
        SqrtDecomposition b;
        const ReplayResult result_a = replay(a, workload);
        const ReplayResult result_b = replay(b, workload);
        print_result("A", result_a, workload);
        print_result("B", result_b, workload);
        if (!same_answers(result_a.answers, result_b.answers)) {
            std::cerr << "ERROR: Structure A and Structure B returned different query answers.\n";
            return 4;
        }
        std::cout << "A_B_QUERY_ANSWERS MATCH\n";
        return 0;
    }

    if (selected == "oracle") {
        throw std::invalid_argument(
            "The Oracle module is not implemented in this repository snapshot; choose A, B or both.");
    }
    throw std::invalid_argument("Unknown structure '" + selected + "'; choose A, B or both.");
}

int run_available_all() {
    const Workload workload = load_workload(CS253_SAMPLE_WORKLOAD_PATH);
    LazySegmentTree a;
    SqrtDecomposition b;
    const ReplayResult result_a = replay(a, workload);
    const ReplayResult result_b = replay(b, workload);

    print_result("A", result_a, workload);
    print_result("B", result_b, workload);
    if (!same_answers(result_a.answers, result_b.answers)) {
        std::cerr << "AVAILABLE_APP_SMOKE FAIL: A/B query answers differ.\n";
        return 4;
    }

    std::cout << "AVAILABLE_APP_SMOKE PASS\n"
              << "BLOCKED: full --all requires the Oracle differential harness "
              << "and integration of the Analyst benchmark runner.\n"
              << "The Adversary workload generator is available as adversary_generate "
              << "and its tests are included in CTest.\n"
              << "No full-pipeline pass is claimed. Complete the Oracle module and "
              << "wire the benchmark runner into --all before final release.\n";
    return 3;
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            print_usage(std::cout);
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--status") {
            print_status();
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--all") {
            return run_available_all();
        }
        if (argc == 5 && std::string(argv[1]) == "--run" &&
            std::string(argv[3]) == "--structure") {
            return run_selected(argv[2], argv[4]);
        }

        print_usage(std::cerr);
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
