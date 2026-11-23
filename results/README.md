# Reproducible results

`raw/full_scale_width_sweep.csv` is the published full-scale campaign:
1,000,000 elements, 1,000,000 operations, seed 253, six widths, A/B and five
repetitions per point (60 rows). `raw/width_sweep.csv` retains the earlier
100,000-element campaign. The recorded timing machine is in `machine.md`.

From the repository root, run:

```sh
bash scripts/run_all.sh
```

The pipeline builds and tests the project, executes eight full-scale D5 replays,
runs the existing full-scale benchmark, checks deterministic counters against the
published CSV and compares 500000 exact-width evaluator answers. New timings
are written to `build/validation/full_scale_reproduced.csv`; the published raw
CSV is preserved. See `raw/workloads.md` for exact per-width input export.

To regenerate every figure and the shared PDFs from committed data:

```sh
python3 scripts/export_docs.py
```

The full-scale SVG/PNG figures come from `scripts/render_benchmark_figures.py`.
Historical SVG figures come from `scripts/export_docs.py`. The Analyst's
`results/scripts/plot_results.py` remains available as an alternative renderer.
Model constants fitted to these same data are descriptive calibration, not an
independent prediction. Captions record the source CSV, script, units and spread.

Timings and implementation revisions must be kept together: the published
campaign predates the Structure B changes merged through PR #31. Current CI
reproduction records current-code timings on a separately recorded runner;
deterministic counter agreement does not establish identical elapsed times or
identical internal work. Use those new logs for claims about the current code.
