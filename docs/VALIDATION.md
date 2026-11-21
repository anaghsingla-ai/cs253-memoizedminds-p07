# Reproducible verification evidence
## Baseline
Main commit a33c51f7383d3f27eb56679adadd16905d8fe386 passed the original 12 checks:
https://github.com/anaghsingla-ai/cs253-memoizedminds-p07/actions/runs/44061341607

## D5 full-scale replay
Source commit: ad65d1ad608d27812bfb9298b934914b9d17e78f.
CI run: https://github.com/anaghsingla-ai/cs253-memoizedminds-p07/actions/runs/44887955330

All 20 CTest checks passed, 264 old/new generator outputs matched byte for byte,
and six invalid generator arguments returned the expected status. Eight separate
jobs executed every D5 family in EXACT and MODULAR mode at n=q=1000000,
seed 253, configured width 999. Every A/B query agreed and both final invariant
audits passed. See results/validation/adversary_full_scale.csv; these are
correctness/counter rows and contain no timing claims.

| Target | Modes | Checks beyond A/B agreement |
|---|---|---|
| push_downs | EXACT, MODULAR | Every primed query has 39 pushes and agrees with an independent constant-array sum/extrema formula |
| alternating_negative_gain | EXACT, MODULAR | Whole/partial/empty/endpoint probes, final invariant audits |
| block_friendly | EXACT, MODULAR | Exactly 1000000 B full blocks and zero partial elements |
| range_width_sweep | EXACT, MODULAR | Widths and boundary patterns; modular INT64_MIN/MAX coefficients; final invariant audits |

Reproduce after building:
```bash
bash scripts/verify_adversary_scale.sh "$PWD/build"
```

The small D5 tests additionally compare with independent wide-integer array
arithmetic, so large-scale A/B agreement is not the only correctness check.
This does not certify unseen examiner tests, unsupported overflow inputs,
or API lifecycle cases not covered by these workloads.

## Evaluator adapter
Source commit: 7d2326e5729a6e8b8d2bee85a3b8afaf0474fef9.
The independent CLI checker uses Python mathematical integers, seed 253+n,
sizes 0,1,17,50,257, both modes, 511 operations/case, and A/B/oracle.
It checks deterministic stdout and rejects malformed input.
Run python3 tests/evaluator_test.py --build-dir build.

## Timing evidence remains separate
results/raw/width_sweep.csv records 5 repetitions for each structure at 6 widths,
all n=q=100000, seed 253. The new scale checks neither replace those timing
measurements nor provide five-run full-scale timing evidence.
The reported timing machine is in results/machine.md; GitHub runners are not
that machine. REPORT.md sections 9-12 describe the remaining measurement
limitations and reproducibility requirements.
