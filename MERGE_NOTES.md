# Remote branch integration notes

Repository: `anaghsingla-ai/CS253-Lazy-Propagation`

## Starting points preserved

- `main`: `204a70aa097987967cca88ded97c4173286a635d`
- `test-branch`: `de048eb3fae805ea982837000d27e4602d723765`

Safety references were created before integration:

- `backup/pre-integration-main-204a70a`
- `backup/pre-integration-test-branch-de048eb`

## Integration decisions

- Kept the full Architect parser/serializer/replay implementation, independent Array Oracle,
  differential/shrinking harness, Adversary generator, Analyst benchmark API and result artifacts.
- Preserved the later main-line D4 derivation and main-line project history as a merge parent.
- Repaired a malformed Structure B conflict resolution by restoring the coherent implementation
  from test-branch's immediate pre-regression parent, then added missing counter/overflow/large-value
  tests that the damaged test file had called but not defined.
- Added a stateful terminal menu for loading, generating, saving, replaying and comparing workloads;
  added differential testing against the Array Oracle and five-run benchmark CSV export.
- Wired the sample A/B/Oracle comparison and benchmark smoke campaign into `calibration --all`.
- Corrected benchmark report means and standard deviations to match the committed raw CSV and
  removed stale TODO/release-gate text. The wide-range timing campaign remains a recorded dataset,
  separate from the CI differential correctness workload.

## Validation policy

Only GitHub Actions build and CTest results for the final integration commit are reported as
verified. The differential harness is a correctness run, not a performance benchmark. The raw
width-sweep CSV contains five timing repetitions per structure and width.
