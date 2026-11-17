# CS253 Project 7 — Lazy Propagation and What Composes

A C++17 implementation and experimental workbench for **range affine transformations** using lazy propagation and square-root decomposition.

The project models a calibration table containing sensor readings. A range update applies the transformation

\[
x \leftarrow ax+b
\]

to every element in a selected interval. The system supports range queries, compares multiple implementations against a brute-force oracle, and provides workloads for correctness testing and performance evaluation.

---

## 1. Problem Statement

Consider an array of up to one million sensor readings. The system must support:

- **Range updates:** Apply an affine transformation \(x \leftarrow ax+b\) to every element in a range.
- **Range queries:** Retrieve the minimum, maximum, and sum over a range.
- **Composition of updates:** Correctly combine multiple pending transformations without applying each update individually.

The transformations are order-dependent. Applying one transformation followed by another is generally different from applying them in reverse order.

If

\[
f(x)=a_1x+b_1
\]

and

\[
g(x)=a_2x+b_2,
\]

then applying \(f\) first and \(g\) second gives

\[
g(f(x))=a_2a_1x+a_2b_1+b_2.
\]

This composition rule is the foundation of the lazy propagation implementation.

## 2. Implementations

The project uses three implementations to evaluate correctness and performance.

| Implementation | Description |
|---|---|
| Structure A | Segment tree with lazy propagation |
| Structure B | Square-root decomposition |
| Oracle | Brute-force array implementation used as the correctness reference |

### Structure A — Lazy Segment Tree

Maintains range aggregates and pending affine transformations. Lazy tags are composed so that an update can be applied to an entire segment without immediately visiting every element.

### Structure B — Square-Root Decomposition

Divides the array into blocks and uses block-level information to process range updates and queries.

### Oracle — Brute Force

Applies updates directly to individual elements. It is not intended to be the fastest implementation; it serves as a reference for differential testing.

## 3. Arithmetic Modes

The project supports two arithmetic modes.

### EXACT Mode

Uses exact integer arithmetic for the restricted calibration transformations:

- \(a=-1\): reflection or sign inversion.
- \(a=0\): reset to a constant value.
- \(a=1\): additive shift.

The minimum and maximum are maintained alongside the range sum.

### MODULAR Mode

Uses arithmetic modulo

\[
998244353.
\]

This mode supports general affine coefficients. Minimum and maximum queries are not meaningful under modular arithmetic, so the supported aggregate is the range sum.

## 4. Features

- Range affine updates with lazy composition.
- Range minimum, maximum, and sum queries where supported by the selected arithmetic mode.
- Segment tree and square-root decomposition implementations.
- Brute-force oracle for correctness verification.
- Differential testing between implementations.
- Workload parsing and replay.
- Interactive terminal menu.
- Generated and saved test workloads.
- Benchmark execution and CSV result export.
- Support for large workloads, including tests involving up to one million elements and operations.

## 5. Requirements

- C++17 compiler (GCC/MinGW, Clang, or MSVC)
- CMake 3.16 or newer
- Compatible build tool (MinGW Make, Make, Ninja, or Visual Studio build tools)

The project uses only the C++ standard library; no third-party C++ dependencies need to be installed. `requirements.txt` lists the required native tools. It is not a Python requirements file.

## 6. Build and Run on Windows (PowerShell)

From the repository root, run:
# CS253 Project 7 — Lazy Propagation and What Composes

**Team:** [Team name]  
**Course:** CS 253 / MA 253 — Data Structures and Algorithms Laboratory  
**Semester:** [Semester]

## Team and roles

| Name | Roll number | GitHub username | Role | Module folder | Derivation |
|---|---:|---|---|---|---|
| Naresh | 250001052 | naresh-coder123 | Structure A — Lazy Segment Tree | `src/structA/` | D1 |
| Harshal | 250001032 | HARSHAL-KARANJE | Structure B — Square-Root Decomposition | `src/structB/` | D2 |
| Bhuvaneesh Vijayvargiya | 250001017 | BhuvaneeshVijayvargiya | Architect — application architecture, integration, interfaces and documentation | `app/`, `include/` | D3 |
| Arush Agrawal | 250001008 | ArushVAgrawal | Oracle — brute-force reference implementation | `oracle/` | D4 |
| Anagh Singla | 250001006 | anaghsingla-ai | Adversary — adversarial workload generation and correctness stress testing | `adversary/` | D5 |
| Ranveer | 250041031 | ranveer16207 | Analyst — benchmarking, performance analysis and derivations | `bench/`, `results/` | D6 |

Complete the missing roll numbers, GitHub usernames, derivation numbers, team name and semester before submission.

## Summary

This project implements and evaluates range affine transformations on an array of sensor readings. Each update applies \(x \leftarrow ax+b\) to every element in a selected interval, and the project compares two implementations: a lazy segment tree (Structure A) and square-root decomposition (Structure B). A brute-force array Oracle serves as the correctness reference, while workload replay, differential testing and benchmarking support correctness and performance evaluation. The project supports exact and modular arithmetic modes; the measured comparison and main finding are reported in [REPORT.md](REPORT.md).

## Requirements

- **Language:** C++17.
- **Build system:** CMake 3.16 or newer.
- **Compiler:** GCC/MinGW, Clang or MSVC with C++17 support. Record the exact compiler and version used for the final build.
- **Build tool:** MinGW Make, Make, Ninja or Visual Studio build tools, depending on the selected CMake generator.
- **Operating system:** The provided wrapper instructions use Windows PowerShell. The manual CMake commands may also work on other supported platforms, but test the final build on the platform used for evaluation.
- **Dependencies:** The project README states that the C++ implementation uses the standard library and has no third-party C++ dependencies. `requirements.txt` lists native tools; it is not a Python requirements file.

Before final submission, record the exact OS, compiler version, CMake version and generator used for the clean-clone verification.

## Build

On Windows PowerShell, run the project wrapper from the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1
```

The script configures the build, compiles the project, and runs the 100,000-element demonstration workload against Structure A, Structure B, and the Oracle.

To open the interactive workbench:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Menu
```

To build and run the automated tests:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Tests
```

The C++ compiler, CMake, and build tool must already be installed and available on `PATH`.

## 7. Manual Build

From the repository root:
The documented default mode configures and builds the project, then runs the 100,000-element demonstration workload against Structure A, Structure B and the Oracle.

For a manual MinGW build:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
```

For generators other than MinGW Makefiles, omit the `-G` option or select the generator installed on your system.

## 9. Workload Format

Workloads describe the initial array and the sequence of operations to perform.

A workload contains:

1. The arithmetic mode.
2. The initial array size and values.
3. A sequence of range updates and queries.

Range operations use interval endpoints as defined by the workload parser. Refer to the sample workload files in `tests/cases/` for concrete examples.

### Sample Workload

The repository includes:

```text
tests/cases/architect_sample.txt
```

The small sample demonstrates affine updates and range aggregate queries. For a larger demonstration, use:

```text
tests/cases/large_demo.txt
```

This deterministic workload contains 100,000 initial elements and mixed full-range, overlapping, and partial-range updates and queries. It is designed to make the A/B/Oracle comparison more representative than the tiny correctness sample.

For an initial array:
If using a different installed CMake generator, omit `-G "MinGW Makefiles"` or replace it with the generator installed on your system.

## Quick start

To run the documented demonstration:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1
```

To open the interactive terminal workbench:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Menu
```

The repository includes these sample workload files:

- `tests/cases/architect_sample.txt` — a small sample for affine updates and range aggregate queries.
- `tests/cases/large_demo.txt` — a deterministic workload with 100,000 initial elements and mixed full-range, overlapping and partial-range updates and queries.

The following calculation illustrates the expected aggregates for the sample operations.

Initial array:

\[
[2,4,1,7]
\]

the initial aggregates are:

- Sum = 14
- Minimum = 1
- Maximum = 7

After applying \(x \leftarrow -x+3\), the array becomes:
Initial aggregates: sum \(=14\), minimum \(=1\), maximum \(=7\).

After applying \(x \leftarrow -x+3\):

\[
[1,-1,2,-4]
\]

The resulting aggregates are:

- Sum = -2
- Minimum = -4
- Maximum = 2

After resetting the middle range to 5, the array becomes:
Aggregates: sum \(=-2\), minimum \(=-4\), maximum \(=2\).

After resetting the middle two elements to \(5\):

\[
[1,5,5,-4]
\]

The resulting aggregates are:

- Sum = 7
- Minimum = -4
- Maximum = 5

These values can be used to verify the correctness of the sample workload execution.

## 10. Testing and Correctness

The project uses the brute-force Oracle as a reference implementation.

Differential testing compares the results produced by the optimized implementations against the Oracle for the same workload.

The test suite is intended to cover:

- Affine transformation composition.
- Positive, negative, and zero gains.
- Overlapping range updates.
- Range minimum, maximum, and sum queries.
- Boundary and single-element ranges.
- Large workloads.
- EXACT and MODULAR arithmetic modes.

Run the available test workflow using:
Aggregates: sum \(=7\), minimum \(=-4\), maximum \(=5\).

**Expected result:** the application should report these aggregates for the corresponding operations. Add a captured terminal output from the final build before submission; the values above are the mathematical expected results, not a transcript of the program.

## Tests

Run the test mode of the PowerShell wrapper:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Tests
```

The repository also documents this test-and-project workflow:

```bash
bash scripts/run_all.sh
```

## 11. Benchmarking

The project includes workload execution and benchmark result export.

Benchmark results can be used to compare:

- Segment tree performance.
- Square-root decomposition performance.
- Effects of different update and query distributions.
- Performance under different workload sizes.

CSV output is provided for further analysis and visualization.

Correctness testing and performance benchmarking serve different purposes: passing differential tests establishes agreement with the reference implementation for tested workloads, while benchmarking evaluates execution cost.

## 12. Repository Structure

The main project areas are:

| Directory / File | Purpose |
|---|---|
| `src/` | Application and implementation source code |
| `include/` | Shared interfaces and declarations |
| `tests/` | Test cases and testing support |
| `scripts/` | Build, test, and execution scripts |
| `docs/` | Project documentation and design notes |
| `README.md` | Project overview and usage instructions |
| `CMakeLists.txt` | CMake build configuration |

## 13. Team Members and Responsibilities

The project is divided into the following responsibilities:

| Team Member | Responsibility |
|---|---|
| Naresh | Structure A — Lazy Segment Tree |
| Harshal | Structure B — Square-Root Decomposition |
| Bhuvaneesh | Architect — Application architecture, integration, interfaces, and documentation |
| Arush | Oracle — Brute-force reference implementation |
| Anagh | Adversary — Adversarial workload generation and correctness stress testing |
| Ranveer | Analyst — Benchmarking, performance analysis, and derivations |

## 14. Project Documentation

Additional design and implementation documentation is available in the `docs/` directory.

The documentation complements the source code and describes the architecture, implementation decisions, and analysis associated with the project.

---

## 15. Conclusion

This project demonstrates how lazy propagation can efficiently support range updates when the updates form a composable family of transformations.

By implementing affine transformations, explicitly handling their composition order, and comparing optimized data structures against a brute-force reference, the project explores both the mathematical foundations and practical engineering considerations of range-update data structures.
Use the command supported by the final checked-out repository. The differential test configuration described for the Oracle work runs one million randomized operations in exact mode (seed `253`) and one million in modular mode (seed `42`), with 128 initial elements in those test cases. The tests also cover boundary cases, determinism, invariant checking, failure detection and failure shrinking.

A successful run must be recorded from the final integrated branch. Add the exact passing terminal message and confirm the final test command and seed values after running the tests from a fresh clone.

## Evaluation interface (hidden tests)

The submission guidelines require the README to state the exact command used by the evaluator and the published input/output format. That information is not present in the README text supplied for this revision.

Before submission, fill in all of the following from the official Project 7 evaluation specification:

- **Evaluator command:** [Exact command]
- **Input format:** [Exact published input format]
- **Output format:** [Exact published output format]
- **Example:** [A minimal valid input and its actual expected output]

Do not substitute the sample workload command for the hidden-test interface unless the instructor's specification explicitly says they are the same.

## Benchmarks and figures

The project includes workload execution and CSV result export for comparing:

- Lazy segment tree and square-root decomposition.
- Different update/query distributions.
- Different workload sizes.

The submission guidelines require one command that reruns every experiment and regenerates every figure. The project README supplied for this revision documents `bash scripts/run_all.sh` as the all-in-one workflow:

```bash
bash scripts/run_all.sh
```

Before submission, verify that this script actually builds, tests, runs the application and benchmarks, and regenerates all figures on the final branch. Record:

- Approximate end-to-end runtime and the machine used.
- Raw CSV output location.
- Generated figure location.
- Benchmark seeds and number of repetitions.

Do not claim a performance winner without supporting measurements. See [REPORT.md](REPORT.md) for the full experimental setup, predicted-cost model, figures and analysis.

## Repository map

The following is the intended project layout based on the supplied project README and submission structure. Check it against the final repository and update any path that differs.

| Path | Purpose |
|---|---|
| `src/` | Application and implementation source code |
| `src/structA/` | Structure A — lazy segment tree |
| `src/structB/` | Structure B — square-root decomposition |
| `include/` | Shared interfaces and declarations |
| `app/` | Application and terminal interface |
| `oracle/` | Brute-force Oracle and differential-testing code |
| `adversary/` | Adversarial workload generators |
| `bench/` | Benchmark harness and counters |
| `tests/` | Unit tests and testing support |
| `tests/cases/` | Sample workload files |
| `scripts/` | Build, test and execution scripts |
| `results/raw/` | Raw benchmark CSV logs, if present |
| `results/figures/` | Generated benchmark figures, if present |
| `docs/` | Project documentation and design notes |
| `docs/derivations/` | Individual derivation documents |
| `docs/slides/` | Presentation slides |
| `CMakeLists.txt` | CMake build configuration |
| `requirements.txt` | List of required native tools, as described by the project |
| `README.md` | Build, run, test and project overview |
| `TOOLS.md` | Declaration of AI and other tools |
| `REPORT.md` | Full project report |

## Results at a glance

| Item | Current documented configuration/result |
|---|---|
| Update operation | \(x \leftarrow ax+b\) over a selected range |
| Structure A | Lazy segment tree |
| Structure B | Square-root decomposition |
| Correctness reference | Brute-force array Oracle |
| Arithmetic modes | Exact and modular |
| Large demonstration workload | 100,000 initial elements |
| Oracle differential tests | 1,000,000 operations per mode in the described D4 test configuration |
| Exact-mode seed | `253` |
| Modular-mode seed | `42` |
| Final test status | [Record result from the final integrated branch] |
| Main performance finding | [Summarize the measured result and cite the relevant figure/table in REPORT.md] |

The configured operation counts and seeds describe the test setup; they are not benchmark performance results.

## Known limitations

- The exact hidden-test evaluator command and input/output format must be copied from the official Project 7 specification.
- The README needs the actual passing output from the final integrated test run.
- The benchmark regeneration command, output paths and runtime must be verified on the final branch.
- The exact operating system, compiler version, CMake version and build generator used for final verification must be recorded.
- The repository map must be checked against the actual final directory tree.
- The team table needs the remaining roll numbers, GitHub usernames and derivation numbers.

Add any other known incomplete feature or outstanding bug here. State incomplete work explicitly rather than implying that it is finished.

## Tools and references

See [TOOLS.md](TOOLS.md) for the team's declaration of AI assistants and other significant tools, the files they affected, what was changed or verified, and representative prompts.

List the papers, books, websites and other sources actually used by the team here and in the references section of [REPORT.md](REPORT.md). Add the specific assigned reading(s) from the Project 7 brief; they were not identified in the supplied README text.
