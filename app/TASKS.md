# Architect owner

Member: TBD. Branch: see README.md.

Own the shared interface review, application, CLI orchestration and D3 derivation. Do not change
another owner's algorithm implementation without review. The public contract remains a candidate
until all six owners sign off.

## Milestone 1 — interface review
- [x] Inspect shared headers, interfaces, application placeholders and build targets.
- [x] Document proposed CLI, workload format, edge cases and output.
- [x] Create role-by-role sign-off and discrepancy list.
- [ ] Confirm six owner names/handles and obtain approval; resolve before-build semantics.
- [ ] Mark contract frozen only after all approvals.

## Milestone 2 — workload core
- [x] Implement bounded `read_workload()` validation.
- [x] Implement `write_workload()` round-trip serialization.
- [x] Implement `replay()` through `RangeStructure&`; timer excludes build and printing.
- [x] Add parser/serializer/replay tests using Structures A and B without editing their source.

## Milestone 3 — CLI and application integration
- [x] Implement `--help`, `--status`, `--run PATH --structure A|B|both` and `--all` entry points.
- [x] Link the application to `app_core`, Structure A and Structure B.
- [x] Print one stable query row per answer, then elapsed time, throughput and counters.
- [x] Compare A/B answers when `--structure both` is selected.
- [x] Make `--all` run the available A/B application smoke workload and explicitly fail while Oracle,
      Adversary and Analyst modules are unavailable.
- [x] Add CLI integration checks; remove the old test that expected the CLI to be unimplemented.
- [ ] Once owner modules arrive, integrate Oracle differential tests, adversarial workloads and all
      Analyst benchmarks into `--all`; require the full one-million-element/one-million-operation run.

## Milestone 4 — D3 proof
- [x] Prove by equal-sum/equal-length counterexample that `(sum, length, k)` cannot determine the
      post-modulo sum.
- [x] State the restricted API and nonnegative bounded-value assumptions.
- [x] Prove the halving lemma and count effective changes across assignment-separated lifetimes.
- [x] Derive the max-pruning segment-tree total-time bound.
- [x] Create `docs/derivations/D3.pdf` from the authored proof.
- [ ] Insert the responsible author's name, review the proof and explain it independently in viva.

## Remaining Architect/release actions
- [ ] Obtain interface sign-off from all six owners; do not call the interface frozen prematurely.
- [ ] Review final integration with the Oracle, Adversary and Analyst owners.
- [ ] Run the final pipeline from a fresh clone after all modules and full-scale workloads are merged.
- [ ] Commit changes on the Architect branch and merge through a reviewed pull request; no remote push
      or team approval is claimed by this local artifact.
