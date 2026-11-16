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

## Member 5: adversarial inputs and verification (16 November 2026)

Anagh Singla (250001006) owns `adversary/` and D5. The `generate` function emits
all three required families plus a range-width sweep. Maximum-push probes are
preceded by a whole-array reset; negative-gain probes observe extrema before a
second negation could hide a missing swap; block-friendly operations cover one
aligned block, including the last short block. See `adversary/README.md` for the
commands and `docs/derivations/D5_250001006.pdf` for the proof.

Deterministic counter evidence from `tests/adversary_test.cpp` (GCC 14.2.0,
CMake 4.4.5, Release build, seed 253):

| Case | Predicted count | Verified count |
|---|---:|---:|
| n=16, primed partial query or update | 7 pushes | 7 pushes |
| n=1024, primed partial query or update | 19 pushes | 19 pushes |
| n=16384, primed partial query or update | 27 pushes | 27 pushes |
| n=50, one aligned B block per operation | 1 full block, 0 partial elements | 1 full block, 0 partial elements |

The `2*h-1` prediction was already recorded in D5 on the branch before this
review. Exhaustive ranges for sizes 1 through 48 confirm the generated query
achieves the maximum for those sizes. Repeated power-of-two checks reach
n=16384. The counter unit is one parent tag propagated to both children.

All 8 CTest checks passed. This includes 240 small differential/reproducibility
cases against independent test-only array arithmetic, invariant audits, CLI
checks, and generation of eight workloads with n=1000000 and q=1000000
(four families, two modes). The scale check validates generation and ranges;
it does not replay one million operations against the production Oracle.

These are correctness/counter checks, not timing benchmarks. The Analyst must
still measure A/B timings over at least five runs, report spread, and establish
whether the block-friendly pattern wins on the stated machine. Team-wide
pipeline work and final report export remain with their respective owners.
