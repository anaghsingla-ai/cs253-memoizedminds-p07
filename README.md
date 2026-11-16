# CS253-Lazy-Propagation
CS253 DSA Lab Project 7 - Lazy Propagation and What Composes

## Team starter repository

Project 7: bulk sensor calibration x ← a*x+b, range min/max and modular checksum.
Source: CS253_MA253_Lab_Projects.docx.pdf, common rules pp. 1–4 and Project 7 pp. 13–14.
Evaluation: **17 November 2026**. Submission: **21 November 2026**.

## Current status
Structure A and Structure B implementations are present. Member 5's adversarial
generators, export CLI and module tests are implemented; see [adversary/README.md](adversary/README.md).
The application replay, production Oracle/differential harness and Analyst benchmark
remain unfinished. The calibration --all entry point still returns its explicit unfinished status.
The adversary tests use their own independent test-only array reference.
Other team member names remain to be filled in by the team.

## Six owners (fill names and freeze at first checkpoint)
| Member | Role | Owned implementation | Derivation |
|---|---|---|---|
| TBD 1 | Structure A | src/structA/, lazy_segment_tree.hpp | D1 push-down invariant |
| TBD 2 | Structure B | src/structB/, sqrt_decomposition.hpp | D2 composition, sum, extrema |
| TBD 3 | Architect | app/, shared contracts, INTERFACE.md | D3 modulo impossibility and amortised variant |
| TBD 4 | Oracle | oracle/, array_oracle.hpp, differential tests, invariant checks | D4 O(log n) visited nodes |
| Anagh Singla | Adversary | adversary/ | D5 maximum push-down sequences |
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
One-command full workflow entry point:
```sh
bash scripts/run_all.sh
```
It currently stops with exit code 2 at the explicitly unimplemented pipeline. Once implemented,
--all must run differential tests, application and EVERY benchmark, including full scale.
Never turn the placeholder into a successful no-op. The equivalent CMake target is all_checks.

## Team workflow and release
Create one shared GitHub repository; give the instructor read access from first checkpoint.
Each member works on their own branch, makes genuine commits across the project period,
and merges via a pull request reviewed by another member. Do not use bulk uploads as history.
Suggested branches: member/struct-a, member/struct-b, member/architect, member/oracle,
member/adversary, member/analyst. Repository: https://github.com/anaghsingla-ai/CS253-Lazy-Propagation.
Setup branch: setup/cpp-team-scaffold; changes await team review before merging to main.
Record names and GitHub handles in this table before work begins.
Only tag **final** after all modules, PDFs, tests and measurements are complete. Hidden tests
and the live demonstration use a fresh clone of that release; later commits are ignored.

See docs/REQUIREMENTS.md for acceptance checklist, docs/WORKFLOW.md for integration order,
REPORT.md for results template, TOOLS.md for disclosure, docs/derivations/ for D1–D6 outlines.

## Subsequent work
Use this same repository for future changes; do not recreate it from a ZIP. Fetch current branch state, inspect teammates’ changes, and work on an owned feature branch. Never overwrite another member’s code or push directly to main without approval.
