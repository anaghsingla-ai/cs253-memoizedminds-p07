# Reproducible results

This merged snapshot includes `raw/width_sweep.csv`, a plotting script at
`scripts/plot_results.py`, and machine/compiler metadata in `machine.md`. The supplied CSV
contains 100,000-element / 100,000-operation measurements with five repetitions per width.
It is not the required full-scale one-million-element / one-million-operation campaign.

Preserve per-run CSV logs with seed and workload. Record at least five repetitions and report
mean plus a spread statistic; do not report only the best time. Preserve query checksums, mode,
generator revision and exact workload alongside each CSV. Correctness validation and logging
should remain outside timed loops.

The benchmark function currently compiles in `support_modules`, but a CLI/application integration
that invokes it from `calibration --all` remains outstanding.
