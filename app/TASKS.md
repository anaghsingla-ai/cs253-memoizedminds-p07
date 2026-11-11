# Architect owner

Member: TBD. Branch: see README.md.

Review/freeze shared interfaces; implement parser, replay, CLI selection and --all orchestration. Report query results and throughput. D3 requires proof of modulo variant, not an additional mandatory implemented structure.

Own derivation: D3, see docs/derivations/D3.md.
Do not change another owner’s implementation without review. The module.cpp is a placeholder only.
Read INTERFACE.md before coding; implementation and algorithm tests remain your work.


## Architect milestone 1 — interface review (12 November 2026)
- [x] Inspect existing shared headers, interface, application placeholders and build targets.
- [x] Document proposed CLI syntax, replay format, edge cases and output in INTERFACE.md.
- [x] Create a role-by-role sign-off and discrepancy list in docs/ARCHITECT_REVIEW.md.
- [ ] Confirm six actual owner names/handles (not supplied in snapshot).
- [ ] Obtain six-member approval and resolve `noexcept`/before-build contradiction.
- [ ] Mark contract `frozen v1` only after approvals (not yet done).
- [ ] Create/push Architect feature branch in the shared remote (requires user authentication; not done here).

Next milestone: implement `read_workload`, `write_workload`, `replay` and application tests; leave other owners' code untouched.

## Architect milestone 2 — workload core (12 November 2026)
- [x] Implement `read_workload()` with CS253_V1 validation and bounded allocations.
- [x] Implement `write_workload()` with round-trip preservation.
- [x] Implement `replay()` via `RangeStructure&`, excluding build and printing from its timer.
- [x] Add `tests/app_test.cpp` and register CTest target (`app_test`).
- [x] Exercise replay through the existing A and B implementations without editing their code.
- [ ] Next: implement real CLI arguments/output in `app/main.cpp`, link application core and structures into `calibration`, and coordinate --all with other owners.
