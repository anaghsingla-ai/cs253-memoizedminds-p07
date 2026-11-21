<<<<<<< HEAD
# Project 07: Lazy Propagation and What Composes
Team MemoizedMinds - CS 253 / MA 253, November 2026

## Team and roles
| Name | Roll no. | GitHub | Role | Module | Derivation |
|---|---|---|---|---|---|
| Naresh | 250001052 | @naresh-coder123 | Structure A | src/structA/ | D1 |
| Harshal | 250001032 | @HARSHAL-KARANJE | Structure B | src/structB/ | D2 |
| Bhuvaneesh Vijayvargiya | 250001017 | @BhuvaneeshVijayvargiya | Architect | app/, include/ | D3 |
| Arush Agrawal | 250001008 | @ArushVAgrawal | Oracle | oracle/ | D4 |
| Anagh Singla | 250001006 | @anaghsingla-ai | Adversary | adversary/ | D5 |
| Ranveer | 250041031 | @ranveer16207 | Analyst | bench/, results/ | D6 |

## Summary
We replay affine corrections x -> a*x+b on a sensor array and query its minimum,
maximum and sum checksum. Structure A is a lazy segment tree; Structure B uses
square-root blocks with affine tags. An independent plain-array oracle checks
correctness. The recorded five-run campaign at n=q=100000 favors B through width
4096 and A at width 32768 and full width; eight additional correctness runs
execute n=q=1000000, covering all four D5 families in both modes.

## Requirements
- Linux with GCC or Clang supporting C++17 **and __int128_t**; 64-bit builds recommended.
- CMake >=3.16 and Make or Ninja.
- Python >=3.9 for the evaluator adapter and submission scripts.
- Optional PDF regeneration: `python3 -m pip install -r requirements-docs.txt`.
- Windows: use a recent 64-bit MinGW-w64 GCC with __int128 support and CMake.
  MSVC alone is not supported by the current wide-arithmetic code.
- The recorded timing machine/compiler/flags are in `results/machine.md`.
  CI runs Ubuntu; CI correctness counters are not measurements on that timing machine.

## Build
From a fresh clone:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel 2
```

On Windows with MinGW, add `-G "MinGW Makefiles"` when configuring.
Executables below have a `.exe` suffix on Windows. `run.ps1` also offers Demo,
Menu and Tests modes; `./build/calibration --menu` opens the workbench.

## Quick start
```bash
python3 scripts/run_ops.py --structure A < tests/cases/architect_sample.txt
```
Expected output (sum is always modulo 998244353):
```text
4 14 1 7
4 998244351 -4 2
4 7 -4 5
```
Use `./build/calibration --run tests/cases/architect_sample.txt --structure both`
for the human-readable simulator, throughput and A/B/Oracle comparison.

## Tests
```bash
ctest --test-dir build --output-on-failure
python3 tests/evaluator_test.py --build-dir build
bash adversary/check_refactor.sh "$PWD/build"
bash scripts/verify_adversary_scale.sh "$PWD/build"
```

The CTest suite has 20 checks. The production differential harness uses 1000000
operations on 128 elements in EXACT mode (seed 253) and MODULAR mode (seed 42).
It ends with `All differential tests passed.` CTest prints
`100% tests passed out of 20`. D5 tests cover 240 small independent-array cases,
all ranges for n=1..48, repeated exact push bounds through n=16384, aligned block
counters, malformed CLI arguments, and million-element generation.
The evaluator test compares 30 CLI runs with independent Python integer arithmetic.
The last shell command **executes** all eight million-element/million-operation
workloads on A and B, checking each query and final invariants. It does not run
the O(n)-per-operation oracle at full scale.

Current full-scale evidence is in `results/validation/adversary_full_scale.csv`
and `docs/VALIDATION.md`. Verification CSVs contain operation counts, not timings.

## Evaluation interface
The two supplied course PDFs do not define an external Project 7 wire format.
The reproducible repository interface is:
=======
# CS253 Project 7 — Lazy Propagation and What Composes

**Team:** Memoized Minds 

**Course:** CS 253 / MA 253 — Data Structures and Algorithms Laboratory  

A C++17 implementation and experimental workbench for **range affine transformations** using lazy propagation and square-root decomposition.

The project models a calibration table containing sensor readings. A range update applies the transformation

$$
x \leftarrow ax+b
$$

to every element in a selected interval. The system supports range queries, compares multiple implementations against a brute-force oracle, and provides workloads for correctness testing and performance evaluation.

## 1. Problem Statement

Consider an array of up to one million sensor readings. The system must support:

- **Range updates:** Apply an affine transformation $x \leftarrow ax+b$ to every element in a range.
- **Range queries:** Retrieve the minimum, maximum, and sum over a range, where supported by the selected arithmetic mode.
- **Composition of updates:** Combine pending transformations correctly without applying every update to every element immediately.

The transformations are order-dependent. Applying one transformation followed by another is generally different from applying them in reverse order.

Let

$$
f(x)=a_1x+b_1
$$

and

$$
g(x)=a_2x+b_2.
$$

Applying $f$ first and then $g$ gives

$$
\begin{aligned}
g(f(x))
&=a_2(a_1x+b_1)+b_2 \\\\
&=a_2a_1x+a_2b_1+b_2.
\end{aligned}
$$

This composition rule is the foundation of the lazy propagation implementation.

## 2. Implementations

The project uses three implementations to evaluate correctness and performance.

| Implementation | Description |
|---|---|
| **Structure A** | Segment tree with lazy propagation |
| **Structure B** | Square-root decomposition |
| **Oracle** | Brute-force array implementation used as the correctness reference |

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

- $a=-1$: reflection or sign inversion.
- $a=0$: reset to a constant value.
- $a=1$: additive shift.

The minimum and maximum are maintained alongside the range sum.

### MODULAR Mode

Uses arithmetic modulo

$$
998244353.
$$

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

- **Language:** C++17.
- **Compiler:** GCC/MinGW, Clang, or MSVC with C++17 support.
- **Build system:** CMake 3.16 or newer.
- **Build tool:** MinGW Make, Make, Ninja, or Visual Studio build tools, depending on the selected CMake generator.
- **Dependencies:** The C++ implementation uses the standard library; no third-party C++ dependencies need to be installed.

`requirements.txt` lists required native tools. It is not a Python requirements file.

Before submission, record the exact operating system, compiler version, CMake version, and build generator used for the final clean-build verification.

## 6. Build and Run on Windows (PowerShell)

Run the following commands from the repository root.

### Default demonstration

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1
```

The documented default mode configures and builds the project, then runs the 100,000-element demonstration workload against Structure A, Structure B, and the Oracle.

### Interactive workbench

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Menu
```

### Automated tests

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Tests
```

The C++ compiler, CMake, and the selected build tool must already be installed and available on `PATH`.

## 7. Manual Build

For a manual MinGW build, run the following from the repository root:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
```

For a different installed CMake generator, omit the `-G` option or replace it with the generator installed on your system.

## 8. Workload Format and Samples

Workloads describe the initial array and the sequence of operations to perform. A workload contains:

1. The arithmetic mode.
2. The initial array size and values.
3. A sequence of range updates and queries.

Range operations use the endpoint convention defined by the workload parser. Refer to the sample workload files for concrete examples.

The repository includes:

- `tests/cases/architect_sample.txt` — a small sample demonstrating affine updates and range aggregate queries.
- `tests/cases/large_demo.txt` — a deterministic workload containing 100,000 initial elements and mixed full-range, overlapping, and partial-range updates and queries.

### Sample calculation

Consider the initial array:

$$
[2,4,1,7]
$$

Its initial aggregates are:

- Sum: $14$
- Minimum: $1$
- Maximum: $7$

After applying $x \leftarrow -x+3$ to every element, the array becomes:

$$
[1,-1,2,-4]
$$

The resulting aggregates are:

- Sum: $-2$
- Minimum: $-4$
- Maximum: $2$

After resetting the middle two elements to $5$, the array becomes:

$$
[1,5,5,-4]
$$

The resulting aggregates are:

- Sum: $7$
- Minimum: $-4$
- Maximum: $5$

These values can be used to check the corresponding sample operations.

## 9. Testing and Correctness

The project uses the brute-force Oracle as a reference implementation. Differential testing compares the results produced by the optimized implementations against the Oracle for the same workload.

The test suite is intended to cover:

- Affine transformation composition.
- Positive, negative, and zero gains.
- Overlapping range updates.
- Range minimum, maximum, and sum queries where supported.
- Boundary and single-element ranges.
- Large workloads.
- EXACT and MODULAR arithmetic modes.

Run the test mode of the PowerShell wrapper:

```powershell
Set-ExecutionPolicy -Scope Process Bypass; .\run.ps1 -Mode Tests
```

The repository also documents the following all-in-one workflow:
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)

```bash
python3 scripts/run_ops.py --structure A < ops.txt > out.txt
python3 scripts/run_ops.py --structure B < ops.txt > out.txt
```

<<<<<<< HEAD
The adapter calls the existing C++ simulator and removes headings and timings.
It never changes the algorithms. If the instructor separately publishes another
protocol, the Architect must adapt this entry point before the final tag.

Input is whitespace-separated:
```text
CS253_V1
EXACT
253
4 3
2 4 1 7
Q 0 4
U 0 4 -1 3
Q 0 4
```
Fields: header; EXACT or MODULAR; uint64 seed; n and q; n int64 initial values;
q operations. `U l r a b` updates [l,r); `Q l r` queries it. All ranges are
zero-based and half-open, with 0<=l<=r<=n. Empty arrays and ranges are legal.
The parser limit is 5000000 elements/operations. Trailing tokens are rejected.

EXACT allows a in {-1,0,1}, returns exact int64 extrema, and requires values
**and composed offsets** to fit int64. MODULAR permits any int64 a,b and returns
only the sum residue; extrema are NA. Output has one row per query:
`length sum_mod minimum maximum`. The example above prints:
```text
4 14 1 7
4 998244351 -4 2
```
An empty range prints `0 0 NA NA`. Invalid input exits nonzero and reports an
error on stderr. Use `--build-dir PATH` for a different build directory.

## Benchmarks and figures
```bash
bash scripts/run_all.sh
```
This builds, tests, runs the sample app, executes full-scale D5 verification,
runs five-repeat benchmarks through the existing workbench, regenerates figures
from the committed raw CSV, and exports report/D5/slides PDFs.
Install requirements-docs.txt first. Allow several minutes; hardware affects runtime.
Logs and replay CSVs go under build/; a new timing campaign goes to
`results/raw/reproduced_width_sweep.csv`. The historical timing CSV is preserved.

Regenerate only the committed figures and documents, with no C++ execution:
```bash
python3 scripts/export_docs.py
```
The historical plot uses 60 recorded rows (5 runs x 6 widths x 2 structures).
Its caption records the raw file and script. New timings are a separate
range_width_sweep-based campaign, not a claim to reproduce the original
unrecorded operation stream byte for byte. Original-input provenance and the
required full-scale timing campaign remain incomplete.

## Repository map
| Path | Contents |
|---|---|
| include/ | Common types, contracts and declarations |
| src/structA/, src/structB/ | Self-written structures |
| app/ | Parser, replay and calibration simulator |
| oracle/ | Independent oracle, differential test and shrinker |
| adversary/ | Four generators, exporter, CLI and replay verifier |
| bench/ | Timing harness and CSV writer |
| tests/ | Invariants, arithmetic, parser and CLI checks |
| scripts/ | Full pipeline, evaluator adapter and document export |
| results/raw/ | Historical timing CSVs and new local campaigns |
| results/validation/ | Full-scale correctness/counter evidence |
| results/figures/ | Figures reproduced from raw CSV |
| docs/derivations/ | Individual Markdown sources and PDFs |
| docs/slides/ | Shared slide source and slides.pdf |

## Results at a glance
| Case | Recorded finding |
|---|---|
| Width 1, n=q=100000 | A 0.022977 s; B 0.018803 s (means of five) |
| Width 4096 | A 0.111120 s; B 0.089405 s |
| Width 32768 | A 0.133195 s; B 0.161713 s |
| Width n=100000 | A 0.001917 s; B 0.241852 s |
| Full-scale D5 replay | All 8 target/mode pairs pass at n=q=1000000 |
| Primed push probe, n=1000000 | 39 pushes per probe; 19500000 over 500000 probes |
| Aligned B workload, q=1000000 | 1000000 full blocks; 0 partial elements |

See REPORT.md for spread, method and limits.

## Known limitations and release status
- A lifecycle guard required by INTERFACE.md is missing in update/query before build;
  This remains an unresolved API issue. Current workload entry points always build first.
- Individual PDFs need exact uppercase names; Harshal's D2 PDF is missing.
- Reviewed PRs, instructor access and the final annotated tag remain release tasks.
- The recorded timing campaign is n=q=100000, not the required full-scale campaign.
  No cache-profiler output supports causal cache claims.
- REPORT.pdf and slides.pdf are generated from checked-in sources. Individual
  derivations other than D5 remain with their named owners.
- The modulo-only variant is a D3 derivation, not a supported calibration command.

Run
`python3 scripts/submission_check.py` to check artifact paths, and add
`--require-final-tag` only when preparing the completed release.
Do not create a final tag while these blockers remain.

## Tools and references
See TOOLS.md for AI use and representative prompts. The report lists source
files and the assigned Project 7 reading; members must cite the external
sources they actually consulted. No library container replaces either structure.

=======
Run the command supported by the final checked-out repository. Record the exact passing output from the final integrated branch before submission.

### Oracle differential-test configuration

The described D4 differential-test configuration runs:

- 1,000,000 randomized operations in EXACT mode, using seed `253`.
- 1,000,000 randomized operations in MODULAR mode, using seed `42`.
- 128 initial elements in each of those test cases.

The tests also cover boundary cases, determinism, invariant checking, failure detection, and failure shrinking. Confirm these settings and the final test result on the integrated branch before treating them as the submitted test result.

## 10. Benchmarking

The project includes workload execution and benchmark result export. Benchmark results can be used to compare:

- Lazy segment tree and square-root decomposition performance.
- Different update/query distributions.
- Different workload sizes.

CSV output is provided for further analysis and visualization.

Correctness testing and performance benchmarking serve different purposes: passing differential tests establishes agreement with the reference implementation for tested workloads, while benchmarking evaluates execution cost.

Before submission, record the benchmark command, runtime, machine, raw CSV location, generated figure location, seeds, and number of repetitions. Do not claim a performance winner without supporting measurements. See `REPORT.md` for the full experimental setup and analysis, if that file is present in the final repository.

## 11. Repository Structure

The main project areas are:

| Directory / File | Purpose |
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
| `scripts/` | Build, test, and execution scripts |
| `docs/` | Project documentation and design notes |
| `docs/derivations/` | Individual derivation documents |
| `CMakeLists.txt` | CMake build configuration |
| `requirements.txt` | List of required native tools |
| `README.md` | Project overview and usage instructions |
| `TOOLS.md` | Declaration of AI and other tools |
| `REPORT.md` | Full project report, if included in the final repository |

Check this map against the final repository and remove or correct any paths that do not exist.

## 12. Team Members and Responsibilities

| Team Member | Roll number | GitHub username | Responsibility |
|---|---:|---|---|
| Naresh | 250001052 | `naresh-coder123` | Structure A — Lazy Segment Tree |
| Harshal | 250001032 | `HARSHAL-KARANJE` | Structure B — Square-Root Decomposition |
| Bhuvaneesh Vijayvargiya | 250001017 | `BhuvaneeshVijayvargiya` | Architect — application architecture, integration, interfaces, and documentation |
| Arush Agrawal | 250001008 | `ArushVAgrawal` | Oracle — brute-force reference implementation |
| Anagh Singla | 250001006 | `anaghsingla-ai` | Adversary — adversarial workload generation and correctness stress testing |
| Ranveer | 250041031 | `ranveer16207` | Analyst — benchmarking, performance analysis, and derivations |

Confirm the team name, semester, GitHub usernames, and role descriptions with the team before final submission.

## 13. Evaluation Interface (Hidden Tests)

The submission guidelines require the README to state the exact command used by the evaluator and the published input/output format. That information is not included in the README material supplied for this revision.

Before submission, fill in the following from the official Project 7 evaluation specification:

- **Evaluator command:** [Exact command]
- **Input format:** [Exact published input format]
- **Output format:** [Exact published output format]
- **Example:** [A minimal valid input and its actual expected output]

Do not substitute the sample workload command for the hidden-test interface unless the instructor's specification explicitly says they are the same.

## 14. Results at a Glance

| Item | Documented configuration/result |
|---|---|
| Update operation | $x \leftarrow ax+b$ over a selected range |
| Structure A | Lazy segment tree |
| Structure B | Square-root decomposition |
| Correctness reference | Brute-force array Oracle |
| Arithmetic modes | EXACT and MODULAR |
| Large demonstration workload | 100,000 initial elements |
| Oracle differential tests | 1,000,000 operations per mode in the described D4 test configuration |
| EXACT-mode seed | `253` |
| MODULAR-mode seed | `42` |
| Final test status | [Record the result from the final integrated branch] |
| Main performance finding | [Summarize the measured result and cite the relevant figure/table in `REPORT.md`] |

The operation counts and seeds above describe the test configuration; they are not benchmark performance results.

## 15. Known Limitations and Pre-Submission Checklist

- [ ] Add the team name and semester.
- [ ] Confirm all team details and role descriptions.
- [ ] Record the exact operating system, compiler version, CMake version, and generator used for final verification.
- [ ] Run the tests on the final integrated branch and record the actual passing output.
- [ ] Verify the all-in-one experiment command and record its runtime and output locations.
- [ ] Add the exact hidden-test evaluator command and input/output format from the official specification.
- [ ] Verify the repository map against the actual directory tree.
- [ ] Add the measured performance finding, supported by the report's figures or tables.
- [ ] List the references and assigned readings actually used by the team.

State incomplete work explicitly rather than implying that it is finished.

## 16. Tools and References

See `TOOLS.md` for the team's declaration of AI assistants and other significant tools, including the files they affected and what was changed or verified.

List the papers, books, websites, and other sources actually used by the team here and in the references section of `REPORT.md`. Include the specific assigned reading(s) from the Project 7 brief.
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)
