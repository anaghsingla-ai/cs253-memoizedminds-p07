
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

- C++17 compatible compiler (GCC/MinGW, Clang, or MSVC)
- CMake 3.16 or newer
- A compatible build tool (MinGW Make, Make, Ninja, or Visual Studio build tools)
- No third-party C++ libraries are required; the implementation uses the C++ standard library.

The repository includes a `requirements.txt` file as a convenient checklist of these **native system requirements**. It is not a Python dependency file, so do not run `pip install -r requirements.txt`.

## 6. One-Command Build and Run (Windows)

Open PowerShell in the repository root and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\\run.ps1
```

This automatically configures CMake (using MinGW Make when available), builds the project, and runs the 100,000-element demonstration workload against Structure A, Structure B, and the Oracle.

Other modes:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\\run.ps1 -Mode Menu
Set-ExecutionPolicy -Scope Process Bypass; .\\run.ps1 -Mode Tests
```

The first run requires the native tools listed above to already be installed and available on PATH. The script does not install a C++ compiler or CMake for you.

## 7. Manual Build Instructions

From the repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

The executable is generated in the `build` directory.

## 8. Running the Application

### Interactive Menu

Start the terminal application:

```bash
./build/calibration --menu
```

The interactive menu provides access to the application's workload, execution, comparison, and benchmark functionality.

### Command-Line Help

```bash
./build/calibration --help
```

### Application Status

```bash
./build/calibration --status
```

### Run a Workload

The application accepts a workload file and allows execution using the available implementations.

```bash
./build/calibration --run tests/cases/architect_sample.txt --structure A
```

Other supported structure selections include:

```bash
./build/calibration --run tests/cases/architect_sample.txt --structure B
./build/calibration --run tests/cases/architect_sample.txt --structure oracle
./build/calibration --run tests/cases/architect_sample.txt --structure both
```

### Run the Combined Workflow

```bash
./build/calibration --all
```

This runs the application's combined workflow. It is a smoke-test workflow and should not be interpreted as a substitute for the full performance evaluation.

### Run the Test Script

```bash
bash scripts/run_all.sh
```

Refer to `--help` for the exact command-line options supported by the current build.

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

\[
[2,4,1,7]
\]

the initial aggregates are:

- Sum = 14
- Minimum = 1
- Maximum = 7

After applying \(x \leftarrow -x+3\), the array becomes:

\[
[1,-1,2,-4]
\]

The resulting aggregates are:

- Sum = -2
- Minimum = -4
- Maximum = 2

After resetting the middle range to 5, the array becomes:

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
