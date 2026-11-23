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
The complete pipeline runs the existing driver in build/full-scale-run so it
does not overwrite committed timing measurements:

```sh
bash scripts/run_all.sh
```

The exported `build/full-scale-run/results/raw/full_scale_seed253.txt` is the
point-width base input before width adjustment. It is not the input for every
width. Export the exact stream used for a measured width with:

```sh
python3 scripts/export_width_workload.py \
  build/full-scale-run/results/raw/full_scale_seed253.txt \
  build/validation/width_16384.txt --width 16384
python3 scripts/run_ops.py --structure A < build/validation/width_16384.txt > build/validation/A.txt
python3 scripts/run_ops.py --structure B < build/validation/width_16384.txt > build/validation/B.txt
cmp build/validation/A.txt build/validation/B.txt
```

For width w (clamped to n), an operation already of width w keeps its range;
otherwise l becomes l modulo (n-w+1) and r becomes l+w. Initial values, seed,
coefficients and operation order are preserved. This matches set_workload_width
in bench/module.cpp. The pipeline checks width 16384 through both evaluators.

The new timing CSV is copied to build/validation/full_scale_reproduced.csv.
scripts/verify_benchmark_csv.py checks dimensions, modes, seed, all repetitions
and deterministic counters against the published data. Timing equality is not
required across machines. Input streams are generated files and remain ignored.

Regenerate the published figures from the published CSV without rerunning timings:
```sh
python3 scripts/render_benchmark_figures.py
```
The original Matplotlib plotting script remains available for the Analyst.
The shared renderer uses only the documented PyMuPDF dependency and labels the
two-constant fit as same-data calibration.
