# Design and experimental report

Status: template; no results collected.

## Design decisions
TODO owners: confirm modes, arithmetic bounds, half-open ranges, representation, block size.
See INTERFACE.md for proposed conventions.

## Predictions — complete and commit before measurement
TODO Analyst: derive A/B cost models, constants, width/alignment effects and expected crossover.
Record prediction commit SHA and date here before running experiments.

## Machine and method
TODO: processor, cache sizes, RAM, OS, compiler version, flags, block size, timing boundaries,
repetitions >=5, seed list, generator version and workload files.

## Correctness evidence
TODO: differential seeds and one-million-operation logs, shrink examples, invariant checks,
edge cases in both modes, full scale execution.

## Measurements and figures
TODO: raw CSV paths; means and spread, units, range widths 1..n, push-downs vs block touches,
profiling evidence for cache claims. No measured values exist yet.

## Adversarial results and gaps
TODO: all three required targets, crossover or reason none exists, predicted vs measured gaps.

## Limitations and reproducibility
TODO: arithmetic domain, peak memory, exact final release tag and fresh-clone command output.
