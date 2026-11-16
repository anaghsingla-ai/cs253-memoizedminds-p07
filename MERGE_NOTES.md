# Merge notes

This source tree combines the uploaded `CS253-Lazy-Propagation-main(1).zip` and
`CS253-Lazy-Propagation-test-branch.zip`.

## Merge choices

- Used the test-branch tree as the base because it contains the fuller Architect application,
  CLI integration, adversary implementation, Analyst benchmark implementation, result artifacts,
  and the accessor-contract correction.
- Added the main archive's independent Array Oracle implementation, Oracle unit tests, and
  differential/shrinking harness.
- Kept the branch's newer `RangeStructure` contract: `size()` and `mode()` are not `noexcept`
  and throw `std::logic_error` before a successful `build()`.
- Updated the Oracle implementation and differential-test fault-injection class to match that
  exception specification. Added Oracle pre-build accessor tests.
- Connected the Oracle to the calibration CLI and added a benchmark smoke test.
- Reconciled CMake targets and updated documentation that previously described Oracle as missing.

## Validation

- Release configuration and build succeeded with GNU C++ 14.2.0.
- CTest: 11/11 tests passed.
- Differential harness: one million EXACT operations and one million MODULAR operations passed.
- `calibration --all` compares A, B and Oracle on the sample workload, then returns status 3
  because the full benchmark campaign is not yet integrated into that command. This is an
  intentional remaining release gate, not a build or test failure.

No remote Git merge, push, or teammate approval is implied by this ZIP merge.
