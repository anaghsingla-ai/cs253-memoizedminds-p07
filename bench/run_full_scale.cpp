#include "cs253/pipeline.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <variant>
#include <vector>

using namespace cs253;

// Build a deterministic workload from a fixed seed
Workload make_workload(std::size_t n, std::size_t q, Seed seed, Mode mode) {
    std::mt19937_64 rng(seed);

    Workload work;
    work.mode = mode;
    work.seed = seed;
    work.initial.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        work.initial[i] = static_cast<Value>((rng() % 2001) - 1000);
    }

    work.operations.reserve(q);
    Value gains[3] = {-1, 0, 1};
    for (std::size_t i = 0; i < q; ++i) {
        std::size_t l = rng() % n;
        if (i % 2 == 0) {
            Update u;
            u.range = {l, l + 1};
            u.tag.a = gains[rng() % 3];
            u.tag.b = static_cast<Value>((rng() % 201) - 100);
            work.operations.push_back(u);
        } else {
            Query qr;
            qr.range = {l, l + 1};
            work.operations.push_back(qr);
        }
    }
    return work;
}

// Save workload in the CS253_V1 replay format so anyone can reproduce it
void save_workload_file(const std::string& path, const Workload& work) {
    std::ofstream out(path);
    out << "CS253_V1\n";
    out << (work.mode == Mode::exact_extrema ? "EXACT" : "MODULAR") << "\n";
    out << work.seed << "\n";
    out << work.initial.size() << " " << work.operations.size() << "\n";

    for (std::size_t i = 0; i < work.initial.size(); ++i) {
        if (i > 0) out << " ";
        out << work.initial[i];
    }
    out << "\n";

    for (std::size_t i = 0; i < work.operations.size(); ++i) {
        if (std::holds_alternative<Update>(work.operations[i])) {
            const Update& u = std::get<Update>(work.operations[i]);
            out << "U " << u.range.begin << " " << u.range.end << " "
                << u.tag.a << " " << u.tag.b << "\n";
        } else {
            const Query& q = std::get<Query>(work.operations[i]);
            out << "Q " << q.range.begin << " " << q.range.end << "\n";
        }
    }
}

int main(int argc, char** argv) {
    std::size_t n = 1000000;
    std::size_t q = 1000000;
    Seed seed = 253;

    bool save_input = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--save-input") {
            save_input = true;
        }
    }

    Workload work = make_workload(n, q, seed, Mode::exact_extrema);

    if (save_input) {
        save_workload_file("results/raw/full_scale_seed253.txt", work);
        std::cout << "Saved reproducible input to results/raw/full_scale_seed253.txt\n";
    }

    BenchmarkConfig cfg;
    cfg.seed = seed;
    cfg.repetitions = 5;
    cfg.widths = {1, 64, 1000, 16384, 262144, n};

    Factory make_a = []() { return std::make_unique<LazySegmentTree>(); };
    Factory make_b = []() { return std::make_unique<SqrtDecomposition>(); };

    std::vector<Measurement> rows = benchmark(make_a, make_b, work, cfg);

    std::ofstream csv("results/raw/full_scale_width_sweep.csv");
    write_csv(csv, rows);
    std::cout << "Wrote " << rows.size() << " rows to results/raw/full_scale_width_sweep.csv\n";
    return 0;
}