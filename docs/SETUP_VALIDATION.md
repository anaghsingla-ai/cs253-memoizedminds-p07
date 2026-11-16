# Setup validation — Architect Parts 3–4

Validated locally on 16 November 2026:

- CMake: 3.31.6
- Compiler: GNU C++ 14.2.0
- Configuration: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --parallel 4` — passed.
- Tests: `ctest --test-dir build --output-on-failure` — **8/8 passed**:
  `contract_smoke`, `structB_test`, `app_test`, `adversary_test`, `adversary_scale`,
  `adversary_cli`, `adversary_cli_invalid`, `calibration_cli`.
- Test assertions are explicitly enabled for the test targets even in Release builds.
- CLI integration checks cover `--help`, `--status`, A/B/both replay, the unavailable Oracle selector,
  and the explicit incomplete-pipeline result from `--all`.
- The Adversary generator builds as `adversary_generate`; its tests include differential checks against an independent test-only array, push-down maximality checks, block-aligned workloads and million-element generation checks.
- The Analyst benchmark implementation compiles in `support_modules`; its plotting script and 100,000-element width-sweep CSV are included, but the benchmark runner is not yet wired into `calibration --all`.
- The convenience script `scripts/run_all.sh` invokes the configure/build/test sequence before the intentionally blocked full-pipeline command.
- The sample workload returns identical query aggregates from A and B.
- `calibration --all` runs the available A/B application smoke check, then exits with status 3 because
  the Oracle differential harness is missing and the Analyst benchmark runner is not yet integrated.
  Adversary generation is available and its dedicated tests pass.

These results do not claim a one-million-operation Oracle differential campaign or full-scale
benchmark campaign. Those remain outstanding.
