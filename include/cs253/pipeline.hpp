#pragma once
#include "cs253/range_structure.hpp"
#include <functional>
#include <iosfwd>
#include <memory>
namespace cs253 {
using Factory = std::function<std::unique_ptr<RangeStructure>()>;
struct ReplayResult {
    std::vector<Aggregate> answers;
    Counters counters;
    double elapsed_seconds = 0;
};
// Architect: parsing and replay; no implementation supplied in scaffold.
Workload read_workload(std::istream& input);
void write_workload(std::ostream& output, const Workload& workload);
ReplayResult replay(RangeStructure& structure, const Workload& workload);
struct DifferentialConfig {
    Seed seed = 253;
    std::size_t elements = 128, operations = 1000000;
    Mode mode = Mode::exact_extrema;
};
struct DifferentialResult {
    bool passed = false;
    std::size_t operations_checked = 0;
    std::optional<Workload> failing_case;
    std::string message;
};
// Oracle owner: independent arithmetic, checks after updates AND queries.
DifferentialResult differential_test(const Factory& a, const Factory& b,
                                     const DifferentialConfig& config);
// Preserve the failure while removing operations and simplifying values/ranges.
// Document whether the result is deletion-minimal or globally minimal.
Workload shrink_failure(const Workload& failure,
                       const std::function<bool(const Workload&)>& still_fails);
struct InvariantAccess {
    static InvariantReport check(const class LazySegmentTree&);
    static InvariantReport check(const class SqrtDecomposition&);
    static InvariantReport check(const class ArrayOracle&);
};
enum class AdversarialTarget {
    push_downs, alternating_negative_gain, block_friendly, range_width_sweep
};
struct GeneratorConfig {
    Seed seed = 253;
    std::size_t elements = 1000000, operations = 1000000;
    std::size_t range_width = 1;
    Mode mode = Mode::exact_extrema;
};
Workload generate(AdversarialTarget target, const GeneratorConfig& config);
struct BenchmarkConfig {
    Seed seed = 253;
    std::size_t repetitions = 5;
    std::vector<std::size_t> widths; // Must include 1 and n.
};
struct Measurement {
    std::string structure, workload;
    Seed seed = 0;
    std::size_t n = 0, operations = 0, width = 0, repetition = 0;
    double elapsed_seconds = 0;
    Counters counters;
};
// Analyst: build/reset before timing; return each run, not only averages.
std::vector<Measurement> benchmark(const Factory& a, const Factory& b,
                                 const Workload& workload,
                                 const BenchmarkConfig& config);
void write_csv(std::ostream& output, const std::vector<Measurement>& rows);
} // namespace cs253
