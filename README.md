# CS253-Lazy-Propagation
CS253 DSA Lab Project 7 - Lazy Propagation and What Composes

## Team starter repository

Project 7: bulk sensor calibration x ← a*x+b, range min/max and modular checksum.
Source: CS253_MA253_Lab_Projects.docx.pdf, common rules pp. 1–4 and Project 7 pp. 13–14.
Evaluation: **17 November 2026**. Submission: **21 November 2026**.

## Current status
Merged snapshot: Structure A and Structure B, the Architect parser/serializer/replay CLI, and the Adversary workload generator are implemented. The Analyst benchmark function, plotting script and a 100,000-element width-sweep CSV are included; the benchmark runner is not yet wired into `calibration --all`. The Oracle implementation and differential harness remain outstanding.
The tests cover the accessor contract, Structure B, parser/serializer/replay through A and B, CLI integration, Adversary generation/differential checks, and the Adversary million-element generation scale checks. They do not replace the required one-million-operation Oracle differential campaign or full-scale benchmark campaign. The sample workload is `tests/cases/architect_sample.txt`.
Architect v1 interface review is documented in [docs/ARCHITECT_REVIEW.md](docs/ARCHITECT_REVIEW.md), but **not frozen until the six owners sign off**.
No team member names or personal role assignments have been assumed.

## Six owners (fill names and freeze at first checkpoint)
| Member | Role | Owned implementation | Derivation |
|---|---|---|---|
| TBD 1 | Structure A | src/structA/, lazy_segment_tree.hpp | D1 push-down invariant |
| TBD 2 | Structure B | src/structB/, sqrt_decomposition.hpp | D2 composition, sum, extrema |
| TBD 3 | Architect | app/, shared contracts, INTERFACE.md | D3 modulo impossibility and amortised variant |
| TBD 4 | Oracle | oracle/, array_oracle.hpp, differential tests, invariant checks | D4 O(log n) visited nodes |
| TBD 5 | Adversary | adversary/ | D5 maximum push-down sequences |
| TBD 6 | Analyst | bench/, results/, REPORT.md | D6 block size and cost models |

Read [INTERFACE.md](INTERFACE.md) before implementation, then each module's TASKS.md.
Public contract changes require Architect review and both structure owners' agreement.
The Oracle owner uses friend InvariantAccess; structure owners coordinate internal layouts.

## Build and scaffold check
Requires C++17 compiler, CMake >= 3.16; Bash for the convenience script.
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/calibration --status
```
Application examples:
```sh
./build/calibration --help
./build/calibration --status
./build/calibration --run tests/cases/architect_sample.txt --structure A
./build/calibration --run tests/cases/architect_sample.txt --structure B
./build/calibration --run tests/cases/architect_sample.txt --structure both
```

One-command full workflow entry point:
```sh
bash scripts/run_all.sh
```
The build and current tests run first. `calibration --all` then runs the available A/B application
smoke workload and exits with code 3 because the Oracle differential harness is missing and the
Analyst benchmark runner is not yet connected to the application. The Adversary generator is available
as `adversary_generate` and has dedicated tests. This is an intentional blocker, not a successful
full-pipeline run. Complete the Oracle module and wire the benchmark runner into `--all` before release.
The equivalent CMake target is `all_checks`.

## Team workflow and release
Create one shared GitHub repository; give the instructor read access from first checkpoint.
Each member works on their own branch, makes genuine commits across the project period,
and merges via a pull request reviewed by another member. Do not use bulk uploads as history.
Suggested branches: member/struct-a, member/struct-b, member/architect, member/oracle,
member/adversary, member/analyst. Repository: https://github.com/anaghsingla-ai/CS253-Lazy-Propagation.
The repository already contains setup and Structure B changes; check the current remote before merging additional work.
Record names and GitHub handles in this table, and obtain explicit v1 interface sign-off before further integration.
Only tag **final** after all modules, PDFs, tests and measurements are complete. Hidden tests
and the live demonstration use a fresh clone of that release; later commits are ignored.

See docs/REQUIREMENTS.md for acceptance checklist, docs/WORKFLOW.md for integration order,
REPORT.md for results template, TOOLS.md for disclosure, docs/derivations/ for D1–D6 outlines.

## Subsequent work
Use this same repository for future changes; do not recreate it from a ZIP. Fetch current branch state, inspect teammates’ changes, and work on an owned feature branch. Never overwrite another member’s code or push directly to main without approval.

## Architect Parts 2–4
`app_core` implements CS253_V1 parsing/serialization and replay through `RangeStructure&`. `calibration` is linked to app_core and both current structures. Its `--run` command reports query aggregates, elapsed operation-loop time, operations/second and counters. `--structure both` compares A/B query results.

`--all` currently runs the A/B sample application check and returns code 3 with an explicit blocker until the Oracle differential harness and Analyst benchmark runner are integrated. The Adversary generator is implemented and tested. It does not claim the full assignment pipeline has passed. Parser/replay resource guards limit each workload to five million elements and five million operations.

Architect D3 is fully drafted in `docs/derivations/D3.md` and exported to `docs/derivations/D3.pdf`; the author must insert their name and review the proof before submission.
