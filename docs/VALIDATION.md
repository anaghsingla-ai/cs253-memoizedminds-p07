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

## Full-scale benchmark reproduction
The Analyst's `results/raw/full_scale_width_sweep.csv` records 60 rows at
n=q=1000000, seed 253: six widths, A/B, five trials. Machine details are in
results/machine.md. The older width_sweep.csv is a separate 100000-element campaign.

The shared pipeline builds full_scale_benchmark from the existing Analyst source,
runs it in build/full-scale-run, and checks its deterministic counters against
the committed CSV. Timings depend on the runner and are not expected to match.
CI artifacts record that runner and compiler separately from the Analyst machine.
The width-1 base input is transformed using the exact benchmark range mapping;
width 16384 is replayed through the evaluator for A/B and all 500000 answer rows
are compared. Independent arithmetic remains covered by the small Python tests.

Figures and REPORT.pdf/slides.pdf regenerate from the committed raw CSV. The
runtime model is calibrated on the same data, explicitly labeled in the plot;
no independent numerical prediction is claimed. See REPORT.md for the earlier
qualitative-model commit and full-scale measurement chronology.

The pipeline invokes submission_check.py --report-only so that known individual
proof packaging blockers are recorded without pretending the release is ready.
Run the checker without that flag for the release gate. Instructor access,
peer approvals, interface sign-offs and final-tag submission remain manual checks.
