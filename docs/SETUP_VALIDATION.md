# Setup validation — integrated remote branches

Validated by GitHub Actions on 17 November 2026.

- Repository: `anaghsingla-ai/CS253-Lazy-Propagation`
- Branch under test: `test-branch`
- Validated source commit: `8eb79dcd55d130858e7e558d21bb0376fa081025`
- Workflow: `Adversary checks` (Ubuntu 26.04, GNU C++ 15.2.0, CMake Release build)
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` — passed.
- Build: `cmake --build build --parallel 2` — passed.
- Tests: `ctest --test-dir build --output-on-failure` — **12/12 passed**:
  `contract_smoke`, `structB_test`, `app_test`, `adversary_test`,
  `adversary_scale`, `adversary_cli`, `adversary_cli_invalid`,
  `oracle_test`, `differential_harness`, `benchmark_smoke`,
  `calibration_cli`, and `calibration_menu`.
- The differential harness completed its one-million-operation EXACT and one-million-operation
  MODULAR checks against the independent Array Oracle.
- The adversary scale test generated the million-element workloads.
- The menu test exercised file loading, A/B/Oracle comparison, five-run benchmark CSV export,
  retained session state and summary display.
- The CLI test exercised `--help`, `--status`, A/B/Oracle/both replay, modular replay,
  invalid selectors, missing files and the integrated `--all` path.

A later documentation-only commit added the tool-disclosure note; the same source tree was
rebuilt and retested in the subsequent GitHub Actions run. The final merge commit will also
receive a fresh CI run after both branch refs are advanced.

## Not claimed

- The differential and adversary scale checks are correctness/generation tests, not timing campaigns.
- The performance dataset is the separate five-repetition 100,000-element width sweep in
  `results/raw/width_sweep.csv`.
- Peak-memory measurement, Architect v1 sign-off and final release tagging remain outstanding.
