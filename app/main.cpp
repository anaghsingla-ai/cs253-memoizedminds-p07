#include "cs253/pipeline.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
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
        << "  calibration --menu\n"
        << "  calibration --run WORKLOAD --structure A|B|oracle|both\n"
        << "  calibration --all\n\n"
        << "WORKLOAD uses CS253_V1; ranges are zero-based [begin,end).\n"
        << "A = lazy segment tree; B = square-root decomposition. 'both' compares A, B and Oracle.\n"
        << "--menu opens a stateful terminal workflow for load/generate/save/run/test/benchmark.\n";
}

void print_status() {
    std::cout
        << "parser=ready replay=ready cli=ready interactive_menu=ready workload_save=ready\n"
        << "structure_A=ready structure_B=ready oracle=ready differential_harness=ready\n"
        << "adversary_generator=ready benchmark_runner=ready full_pipeline=ready\n";
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
        std::cout << "Q " << i << " LEN " << answer.length
                  << " SUM " << answer.sum_mod << " MIN ";
        print_optional_value(answer.minimum);
        std::cout << " MAX ";
        print_optional_value(answer.maximum);
        std::cout << '\n';
    }
    std::cout << std::setprecision(10)
              << "ELAPSED_SECONDS " << result.elapsed_seconds << '\n';
    if (result.elapsed_seconds > 0.0) {
        std::cout << "OPS_PER_SECOND "
                  << static_cast<double>(workload.operations.size()) /
                         result.elapsed_seconds << '\n';
    } else {
        std::cout << "OPS_PER_SECOND NA\n";
    }
    const Counters& c = result.counters;
    std::cout << "COUNTERS nodes_visited=" << c.nodes_visited
              << " push_downs=" << c.push_downs
              << " full_blocks=" << c.full_blocks
              << " partial_elements=" << c.partial_elements
              << " tag_compositions=" << c.tag_compositions
              << " aggregate_merges=" << c.aggregate_merges << '\n';
}

bool same_answers(const std::vector<Aggregate>& a,
                  const std::vector<Aggregate>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].length != b[i].length || a[i].sum_mod != b[i].sum_mod ||
            a[i].minimum != b[i].minimum || a[i].maximum != b[i].maximum) return false;
    }
    return true;
}

Workload load_workload(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open workload file: " + path);
    return read_workload(input);
}

void save_workload(const Workload& workload, const std::string& path) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("Cannot write workload file: " + path);
    write_workload(output, workload);
    if (!output) throw std::runtime_error("I/O error writing workload file: " + path);
}

int run_selected(const Workload& workload, const std::string& selected) {
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
    if (selected == "oracle") {
        ArrayOracle structure;
        print_result("ORACLE", replay(structure, workload), workload);
        return 0;
    }
    if (selected == "both") {
        LazySegmentTree a;
        SqrtDecomposition b;
        ArrayOracle oracle;
        const ReplayResult ra = replay(a, workload);
        const ReplayResult rb = replay(b, workload);
        const ReplayResult ro = replay(oracle, workload);
        print_result("A", ra, workload);
        print_result("B", rb, workload);
        print_result("ORACLE", ro, workload);
        if (!same_answers(ra.answers, rb.answers) ||
            !same_answers(ra.answers, ro.answers)) {
            std::cerr << "ERROR: A, B and Oracle query answers differ.\n";
            return 4;
        }
        std::cout << "A_B_ORACLE_QUERY_ANSWERS MATCH\n";
        return 0;
    }
    throw std::invalid_argument("Unknown structure '" + selected +
                                "'; choose A, B, oracle or both.");
}

Factory make_a() {
    return [] { return std::make_unique<LazySegmentTree>(); };
}
Factory make_b() {
    return [] { return std::make_unique<SqrtDecomposition>(); };
}

std::vector<std::size_t> default_widths(std::size_t n) {
    if (n == 0) return {0};
    const std::size_t root = static_cast<std::size_t>(
        std::ceil(std::sqrt(static_cast<double>(n))));
    std::vector<std::size_t> widths{1, root, std::max<std::size_t>(1, n / 4),
                                    std::max<std::size_t>(1, n / 2), n};
    for (auto& width : widths) width = std::min(width, n);
    std::sort(widths.begin(), widths.end());
    widths.erase(std::unique(widths.begin(), widths.end()), widths.end());
    return widths;
}

void run_benchmark(const Workload& workload, std::ostream& output) {
    BenchmarkConfig config;
    config.seed = workload.seed;
    config.repetitions = 5;
    config.widths = default_widths(workload.initial.size());
    const auto rows = benchmark(make_a(), make_b(), workload, config);
    write_csv(output, rows);
}

int run_available_all() {
    const Workload workload = load_workload(CS253_SAMPLE_WORKLOAD_PATH);
    const int comparison = run_selected(workload, "both");
    if (comparison != 0) return comparison;
    std::cout << "BENCHMARK_CSV_BEGIN\n";
    run_benchmark(workload, std::cout);
    std::cout << "BENCHMARK_CSV_END\n";
    std::cout << "FULL_PIPELINE PASS: sample replay, A/B/Oracle comparison and five-run benchmark completed.\n";
    return 0;
}

std::string ask_line(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) throw std::runtime_error("Input closed.");
    return value;
}

template <typename T>
T ask_integer(const std::string& prompt, T minimum, T maximum) {
    const std::string line = ask_line(prompt);
    std::istringstream input(line);
    long long value = 0;
    char extra = 0;
    if (!(input >> value) || (input >> extra) ||
        value < static_cast<long long>(minimum) ||
        value > static_cast<long long>(maximum)) {
        throw std::invalid_argument("Enter a whole number in the permitted range.");
    }
    return static_cast<T>(value);
}

Mode ask_mode() {
    const std::string mode = ask_line("Mode [1=EXACT, 2=MODULAR]: ");
    if (mode == "1" || mode == "EXACT") return Mode::exact_extrema;
    if (mode == "2" || mode == "MODULAR") return Mode::modular_sum;
    throw std::invalid_argument("Choose EXACT or MODULAR.");
}

AdversarialTarget ask_target() {
    std::cout << "Generator target: 1=push_downs 2=alternating_negative_gain\n"
              << "                 3=block_friendly 4=range_width_sweep\n";
    switch (ask_integer<int>("Target: ", 1, 4)) {
        case 1: return AdversarialTarget::push_downs;
        case 2: return AdversarialTarget::alternating_negative_gain;
        case 3: return AdversarialTarget::block_friendly;
        default: return AdversarialTarget::range_width_sweep;
    }
}

void print_menu() {
    std::cout << "\n=== CS253 Calibration Workbench ===\n"
              << "1. Load workload from file\n"
              << "2. Generate workload (adversarial families)\n"
              << "3. Save current workload\n"
              << "4. Run current workload (A / B / Oracle / compare all)\n"
              << "5. Differential test A and B against the Array Oracle\n"
              << "6. Benchmark A vs B (five repetitions, CSV)\n"
              << "7. Show current workload summary\n"
              << "0. Exit\n";
}

int run_menu() {
    Workload current;
    bool has_workload = false;
    for (;;) {
        print_menu();
        const int choice = ask_integer<int>("Select: ", 0, 7);
        try {
            if (choice == 0) return 0;
            if (choice == 1) {
                const std::string path = ask_line("Workload path: ");
                Workload candidate = load_workload(path);
                current = std::move(candidate);
                has_workload = true;
                std::cout << "Loaded " << current.initial.size() << " values and "
                          << current.operations.size() << " operations.\n";
            } else if (choice == 2) {
                GeneratorConfig config;
                const AdversarialTarget target = ask_target();
                config.elements = ask_integer<std::size_t>("Element count: ", 0, 5000000);
                config.operations = ask_integer<std::size_t>("Operation count: ", 0, 5000000);
                config.seed = ask_integer<Seed>("Seed: ", 0, static_cast<Seed>(std::numeric_limits<std::int64_t>::max()));
                config.range_width = ask_integer<std::size_t>("Range width: ", 0,
                                                               std::max<std::size_t>(1, config.elements));
                config.mode = ask_mode();
                Workload candidate = generate(target, config);
                current = std::move(candidate);
                has_workload = true;
                std::cout << "Generated workload in memory.\n";
            } else if (choice == 3) {
                if (!has_workload) throw std::logic_error("Load or generate a workload first.");
                const std::string path = ask_line("Save path: ");
                save_workload(current, path);
                std::cout << "Saved workload to " << path << ".\n";
            } else if (choice == 4) {
                if (!has_workload) throw std::logic_error("Load or generate a workload first.");
                std::cout << "Structure: 1=A 2=B 3=Oracle 4=Compare A/B/Oracle\n";
                const int selected = ask_integer<int>("Choice: ", 1, 4);
                const char* name = selected == 1 ? "A" : selected == 2 ? "B" :
                                   selected == 3 ? "oracle" : "both";
                (void)run_selected(current, name);
            } else if (choice == 5) {
                DifferentialConfig config;
                config.elements = ask_integer<std::size_t>("Array size: ", 0, 1000000);
                config.operations = ask_integer<std::size_t>("Operations: ", 0, 1000000);
                config.seed = ask_integer<Seed>("Seed: ", 0, std::numeric_limits<Seed>::max());
                config.mode = ask_mode();
                const DifferentialResult result = differential_test(make_a(), make_b(), config);
                std::cout << (result.passed ? "DIFFERENTIAL PASS: " : "DIFFERENTIAL FAIL: ")
                          << result.message << " checked=" << result.operations_checked << '\n';
                if (result.failing_case) {
                    const std::string path = ask_line("Save minimized failing workload path (blank to skip): ");
                    if (!path.empty()) save_workload(*result.failing_case, path);
                }
            } else if (choice == 6) {
                if (!has_workload) throw std::logic_error("Load or generate a workload first.");
                const std::string path = ask_line("CSV output path: ");
                std::ofstream output(path);
                if (!output) throw std::runtime_error("Cannot write CSV file: " + path);
                run_benchmark(current, output);
                if (!output) throw std::runtime_error("I/O error writing CSV file: " + path);
                std::cout << "Benchmark complete; CSV saved to " << path << ".\n";
            } else if (choice == 7) {
                if (!has_workload) {
                    std::cout << "No workload loaded.\n";
                } else {
                    std::cout << "mode=" << (current.mode == Mode::exact_extrema ? "EXACT" : "MODULAR")
                              << " seed=" << current.seed
                              << " elements=" << current.initial.size()
                              << " operations=" << current.operations.size() << '\n';
                }
            }
        } catch (const std::exception& error) {
            std::cerr << "ERROR: " << error.what() << '\n';
        }
    }
}
} // namespace

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
        if (argc == 2 && std::string(argv[1]) == "--all") return run_available_all();
        if (argc == 2 && std::string(argv[1]) == "--menu") return run_menu();
        if (argc == 5 && std::string(argv[1]) == "--run" &&
            std::string(argv[3]) == "--structure") {
            return run_selected(load_workload(argv[2]), argv[4]);
        }
        print_usage(std::cerr);
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
