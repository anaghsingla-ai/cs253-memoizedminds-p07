#include "cs253/pipeline.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef CS253_SAMPLE_WORKLOAD_PATH
#define CS253_SAMPLE_WORKLOAD_PATH "tests/cases/architect_sample.txt"
#endif

namespace {
using namespace cs253;

bool enable_terminal_colors(FILE* stream) {
#ifdef _WIN32
    const intptr_t handle_value = _get_osfhandle(_fileno(stream));
    if (handle_value == -1) return false;
    HANDLE handle = reinterpret_cast<HANDLE>(handle_value);
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) return false;
    return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return isatty(fileno(stream)) != 0;
#endif
}

bool stdout_is_terminal() {
    static const bool enabled = enable_terminal_colors(stdout);
    return enabled;
}

bool stderr_is_terminal() {
    static const bool enabled = enable_terminal_colors(stderr);
    return enabled;
}

void print_status(const char* label, const std::string& message) {
    const bool terminal = stdout_is_terminal();
    const char* color = std::string(label) == "PASS" ? "\033[32m" :
                        std::string(label) == "FAIL" ? "\033[31m" :
                        "\033[33m";
    if (terminal) std::cout << color;
    std::cout << "[" << label << "]";
    if (terminal) std::cout << "\033[0m";
    std::cout << " " << message << '\n';
}

void print_failure(const std::string& message) {
    const bool terminal = stderr_is_terminal();
    if (terminal) std::cerr << "\033[31m";
    std::cerr << "[FAIL] " << message;
    if (terminal) std::cerr << "\033[0m";
    std::cerr << '\n';
}

class Spinner {
public:
    explicit Spinner(std::string message)
        : active_(stderr_is_terminal()), message_(std::move(message)) {
        if (!active_) return;
        worker_ = std::thread([this] {
            static const char frames[] = {'-', '\\', '|', '/'};
            std::size_t frame = 0;
            while (!stop_.load()) {
                std::cerr << "\r\033[33m" << frames[frame++ % 4]
                          << "\033[0m " << message_ << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    ~Spinner() {
        stop_.store(true);
        if (worker_.joinable()) worker_.join();
        if (active_) std::cerr << "\r\033[2K" << std::flush;
    }

    Spinner(const Spinner&) = delete;
    Spinner& operator=(const Spinner&) = delete;

private:
    std::atomic<bool> stop_{false};
    bool active_;
    std::string message_;
    std::thread worker_;
};

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

const char* mode_label(Mode mode) {
    return mode == Mode::exact_extrema ? "EXACT" : "MODULAR";
}

const char* structure_label(const char* name) {
    if (std::string(name) == "A") return "LAZY SEGMENT TREE";
    if (std::string(name) == "B") return "SQUARE-ROOT DECOMPOSITION";
    return "BRUTE-FORCE ORACLE";
}

void print_result(const char* name, const ReplayResult& result,
                  const Workload& workload) {
    const double rate = result.elapsed_seconds > 0.0
        ? static_cast<double>(workload.operations.size()) / result.elapsed_seconds
        : 0.0;

    std::cout << "\n============================================================\n"
              << "  " << structure_label(name) << "\n"
              << "============================================================\n"
              << "  STRUCTURE " << (std::string(name) == "ORACLE" ? "ORACLE" : name) << "\n"
              << "  Mode: " << mode_label(workload.mode)
              << "   |   Elements: " << workload.initial.size()
              << "   |   Operations: " << workload.operations.size()
              << "   |   Queries: " << result.answers.size() << "\n";

    if (result.answers.empty()) {
        std::cout << "\n  No query operations were present in this workload.\n";
    } else {
        std::cout << "\n  QUERY RESULTS\n"
                  << "  " << std::left << std::setw(8) << "Query"
                  << std::right << std::setw(10) << "Length"
                  << std::setw(20) << "Sum"
                  << std::setw(12) << "Minimum"
                  << std::setw(12) << "Maximum" << "\n"
                  << "  " << std::string(62, '-') << "\n";
        for (std::size_t i = 0; i < result.answers.size(); ++i) {
            const Aggregate& answer = result.answers[i];
            std::cout << "  " << std::left << std::setw(8) << (i + 1)
                      << std::right << std::setw(10) << answer.length
                      << std::setw(20) << answer.sum_mod
                      << std::setw(12);
            print_optional_value(answer.minimum);
            std::cout << std::setw(12);
            print_optional_value(answer.maximum);
            std::cout << "\n"
                      << "  Q " << i << " LEN " << answer.length
                      << " SUM " << answer.sum_mod << " MIN ";
            if (answer.minimum) std::cout << *answer.minimum;
            else std::cout << "NA";
            std::cout << " MAX ";
            if (answer.maximum) std::cout << *answer.maximum << '\n';
            else std::cout << "NA\n";
        }
    }

    std::cout << "\n  PERFORMANCE\n"
              << "  Elapsed time       : " << std::fixed << std::setprecision(6)
              << result.elapsed_seconds << " s\n"
              << "  Throughput         : " << std::setprecision(2)
              << rate << " operations/s\n"
              << "  ELAPSED_SECONDS " << std::setprecision(6) << result.elapsed_seconds << "\n"
              << "  OPS_PER_SECOND " << std::setprecision(2) << rate << "\n";

    const Counters& c = result.counters;
    std::cout << "\n  INTERNAL COUNTERS\n"
              << "  Nodes visited      : " << c.nodes_visited << "\n"
              << "  Push-downs         : " << c.push_downs << "\n"
              << "  Full blocks        : " << c.full_blocks << "\n"
              << "  Partial elements   : " << c.partial_elements << "\n"
              << "  Tag compositions   : " << c.tag_compositions << "\n"
              << "  Aggregate merges   : " << c.aggregate_merges << "\n";
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
        ReplayResult result;
        { Spinner spinner("Running Structure A"); result = replay(structure, workload); }
        print_result("A", result, workload);
        return 0;
    }
    if (selected == "B") {
        SqrtDecomposition structure;
        ReplayResult result;
        { Spinner spinner("Running Structure B"); result = replay(structure, workload); }
        print_result("B", result, workload);
        return 0;
    }
    if (selected == "oracle") {
        ArrayOracle structure;
        ReplayResult result;
        { Spinner spinner("Running brute-force Oracle"); result = replay(structure, workload); }
        print_result("ORACLE", result, workload);
        return 0;
    }
    if (selected == "both") {
        LazySegmentTree a;
        SqrtDecomposition b;
        ArrayOracle oracle;
        ReplayResult ra, rb, ro;
        { Spinner spinner("Running Structure A"); ra = replay(a, workload); }
        { Spinner spinner("Running Structure B"); rb = replay(b, workload); }
        { Spinner spinner("Running brute-force Oracle"); ro = replay(oracle, workload); }
        print_result("A", ra, workload);
        print_result("B", rb, workload);
        print_result("ORACLE", ro, workload);
        if (!same_answers(ra.answers, rb.answers) ||
            !same_answers(ra.answers, ro.answers)) {
            print_status("FAIL", "Structure A, Structure B and Oracle disagree.");
            std::cerr << "ERROR: A, B and Oracle query answers differ.\n";
            return 4;
        }
        std::cout << "\n============================================================\n"
                  << "                 DIFFERENTIAL VERIFICATION\n"
                  << "============================================================\n";
        print_status("PASS", "Structure A, Structure B and Oracle agree.");
        std::cout << "  Query results checked: " << ra.answers.size() << "\n"
                  << "  All reported aggregates match for this workload.\n"
                  << "  A_B_ORACLE_QUERY_ANSWERS MATCH\n"
                  << "============================================================\n";
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
    { Spinner spinner("Running integrated benchmark"); run_benchmark(workload, std::cout); }
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
    std::cout << "\n"
              << "============================================================\n"
              << "              CS253 | CALIBRATION WORKBENCH\n"
              << "       Range affine updates  |  Query  |  Compare\n"
              << "============================================================\n"
              << "  WORKLOAD\n"
              << "   1. Load workload from file\n"
              << "   2. Generate an adversarial workload\n"
              << "   3. Save current workload\n"
              << "  EXECUTION\n"
              << "   4. Run workload (A / B / Oracle / compare all)\n"
              << "   5. Differential test against the Oracle\n"
              << "   6. Benchmark Structure A vs Structure B\n"
              << "   7. Show current workload summary\n"
              << "   0. Exit\n"
              << "------------------------------------------------------------\n";
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
                print_status("PASS", "Loaded " + std::to_string(current.initial.size()) +
                             " values and " + std::to_string(current.operations.size()) +
                             " operations.");
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
                print_status("PASS", "Generated workload in memory.");
            } else if (choice == 3) {
                if (!has_workload) throw std::logic_error("Load or generate a workload first.");
                const std::string path = ask_line("Save path: ");
                save_workload(current, path);
                print_status("PASS", "Saved workload to " + path + ".");
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
                DifferentialResult result;
                { Spinner spinner("Running differential test"); result = differential_test(make_a(), make_b(), config); }
                print_status(result.passed ? "PASS" : "FAIL",
                             "Differential test: " + result.message +
                             " checked=" + std::to_string(result.operations_checked));
                if (result.failing_case) {
                    const std::string path = ask_line("Save minimized failing workload path (blank to skip): ");
                    if (!path.empty()) save_workload(*result.failing_case, path);
                }
            } else if (choice == 6) {
                if (!has_workload) throw std::logic_error("Load or generate a workload first.");
                const std::string path = ask_line("CSV output path: ");
                std::ofstream output(path);
                if (!output) throw std::runtime_error("Cannot write CSV file: " + path);
                { Spinner spinner("Running benchmark repetitions"); run_benchmark(current, output); }
                if (!output) throw std::runtime_error("I/O error writing CSV file: " + path);
                print_status("PASS", "Benchmark complete; CSV saved to " + path + ".");
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
            print_failure(error.what());
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
        print_failure(error.what());
        return 2;
    }
}
