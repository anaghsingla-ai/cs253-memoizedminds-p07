# Setup validation — merged main and test-branch snapshot

Validated locally on 17 November 2026:

- CMake: GNU CMake available in the build environment.
- Compiler: GNU C++ 14.2.0.
- Configuration: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` — passed.
- Build: `cmake --build build --parallel 4` — passed.
- Tests: `ctest --test-dir build --output-on-failure` — **11/11 passed**:
  `contract_smoke`, `structB_test`, `app_test`, `adversary_test`, `adversary_scale`,
  `adversary_cli`, `adversary_cli_invalid`, `oracle_test`, `differential_harness`,
  `benchmark_smoke`, and `calibration_cli`.
- Contract tests verify that `size()` and `mode()` are not `noexcept` and throw
  `std::logic_error` before a successful build for the base interface and all concrete structures.
- Oracle tests cover exact and modular arithmetic, affine updates, invalid ranges, overflow atomicity,
  empty ranges, rebuild behavior, counters and pre-build accessors.
- Differential harness passed one million operations in EXACT mode and one million in MODULAR mode.
- Adversary tests include deterministic workload generation and the million-element scale check.
- Analyst benchmark smoke test validates the benchmark API, minimum repetition behavior and CSV header.
- CLI integration covers `--help`, `--status`, A/B/Oracle/both replay, modular workloads and the
  intentional `--all` blocker.

The benchmark implementation and plotting script are included, but the full benchmark campaign is
not yet integrated into `calibration --all`. No full-scale benchmark result is claimed.
