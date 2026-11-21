# Reproducible Benchmark Inputs

All full-scale timings in `results/raw/full_scale_width_sweep.csv` were generated deterministically using `bench/run_full_scale.cpp`.

## Workload Specification
- **Format**: `CS253_V1` (zero-based half-open intervals `[begin, end)`)
- **Mode**: `EXACT` (`exact_extrema`, gains in `{-1, 0, 1}`)
- **Seed**: `253` (`std::mt19937_64`)
- **Scale**: `n = 1,000,000` initial elements, `q = 1,000,000` operations (50% `Update`, 50% `Query`)
- **Initial values**: uniform integers in `[-1000, 1000]`
- **Update tags**: `a` uniformly chosen from `{-1, 0, 1}`, `b` uniformly chosen in `[-100, 100]`
- **Width sweep**: `w in {1, 64, 1000, 16384, 262144, 1000000}` (`5` repetitions per width, fresh structure per trial)

## How to Reproduce the Exact Input File and CSV
Run from the repository root:

```sh
g++ -std=c++17 -O3 -Wall -Wextra -Wpedantic -Iinclude \
  src/structA/module.cpp src/structB/module.cpp bench/module.cpp bench/run_full_scale.cpp \
  -o build/run_full_scale

./build/run_full_scale --save-input
python3 results/scripts/plot_results.py results/raw/full_scale_width_sweep.csv results/figures
```

Passing `--save-input` writes the full `CS253_V1` workload to `results/raw/full_scale_seed253.txt` (not committed directly to Git due to ~25 MB size, but 100% reproducible from `seed = 253` and `bench/run_full_scale.cpp`).
