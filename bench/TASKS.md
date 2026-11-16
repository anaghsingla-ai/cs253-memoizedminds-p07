# Analyst owner

Member: TBD. Branch: see README.md.

Write cost models BEFORE measurements; implement five-run minimum timing harness, operation counters, raw CSV, plotting scripts, machine metadata and profiler collection. Sweep widths 1..n and explain crossover or its absence.

Own derivation: D6, see docs/derivations/D6.md.
Current merged snapshot: `bench/module.cpp` implements the benchmark function and CSV writer; `results/scripts/plot_results.py`, `results/raw/width_sweep.csv`, and machine metadata are included. The benchmark runner is compiled but is not yet exposed through the application CLI.
Remaining integration: add a benchmark command or connect the harness to `calibration --all`, then collect and explain the required full-scale width sweep. Do not change another owner’s implementation without review.
