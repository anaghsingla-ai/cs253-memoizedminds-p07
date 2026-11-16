# Setup validation — Architect Parts 3–4

Validated locally on 14 November 2026:

- CMake: 3.31.6
- Compiler: GNU C++ 14.2.0
- Configuration: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --parallel 4` — passed.
- Tests: `ctest --test-dir build --output-on-failure` — **4/4 passed**:
  `contract_smoke`, `structB_test`, `app_test`, `calibration_cli`.
- Test assertions are explicitly enabled for the test targets even in Release builds.
- CLI integration checks cover `--help`, `--status`, A/B/both replay, the unavailable Oracle selector,
  and the explicit incomplete-pipeline result from `--all`.
- The convenience script `scripts/run_all.sh` has LF line endings and invokes the same configure/build/test
  sequence before the intentionally blocked full-pipeline command.
- The sample workload returns identical query aggregates from A and B.
- `calibration --all` runs the available A/B application smoke check, then exits with status 3 because
  Oracle differential testing, Adversary generators and Analyst benchmarks are still missing.

These results do not claim a one-million-operation differential campaign, adversarial workload
coverage or full-scale benchmarks. Those remain blocked on the respective team owners' modules.
