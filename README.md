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

```bash
python3 scripts/run_ops.py --structure A < ops.txt > out.txt
python3 scripts/run_ops.py --structure B < ops.txt > out.txt
```

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
unrecorded operation stream byte for byte. Ranveer must resolve that provenance
gap and add the required full-scale timing campaign.

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
  Naresh must fix and commit it. Current workload entry points always build first.
- Individual PDFs need exact uppercase names; Harshal's D2 PDF is missing.
- Reviewed PRs, instructor access and the final annotated tag remain release tasks.
- The recorded timing campaign is n=q=100000, not the required full-scale campaign.
  No cache-profiler output supports causal cache claims.
- REPORT.pdf and slides.pdf are generated from checked-in sources. Individual
  derivations other than D5 remain with their named owners.
- The modulo-only variant is a D3 derivation, not a supported calibration command.

`docs/SUBMISSION_AUDIT.md` gives the owner-by-owner handoff. Run
`python3 scripts/submission_check.py` to check artifact paths, and add
`--require-final-tag` only when preparing the completed release.
Do not create a final tag while these blockers remain.

## Tools and references
See TOOLS.md for AI use and representative prompts. The report lists source
files and the assigned Project 7 reading; members must cite the external
sources they actually consulted. No library container replaces either structure.

