#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/pipeline.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace cs253;

int main() {
    try {
        Workload work;
        work.mode = Mode::exact_extrema;
        work.seed = 253;
        work.initial = {1, 2, 3, 4, 5, 6, 7, 8};
        work.operations = {
            Update{{0, 8}, {1, 2}},
            Query{{0, 8}},
            Update{{2, 6}, {-1, 10}},
            Query{{1, 7}}
        };

        Factory a = [] { return std::make_unique<LazySegmentTree>(); };
        Factory b = [] { return std::make_unique<SqrtDecomposition>(); };
        BenchmarkConfig cfg;
        cfg.seed = 253;
        cfg.repetitions = 1; // benchmark() enforces the required minimum of five.
        cfg.widths = {1, 8};

        const auto rows = benchmark(a, b, work, cfg);
        if (rows.size() != 20) throw std::runtime_error("Unexpected benchmark row count");
        std::ostringstream csv;
        write_csv(csv, rows);
        if (csv.str().find("structure,workload,seed") != 0)
            throw std::runtime_error("CSV header missing");
        std::cout << "Benchmark smoke passed (" << rows.size() << " rows).\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark smoke failed: " << e.what() << '\n';
        return 1;
    }
}
