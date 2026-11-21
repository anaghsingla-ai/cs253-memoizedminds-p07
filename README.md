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
correctness. The full-scale five-run campaign uses n=q=1000000: A has the
lower measured mean at width 1 and from width 1000 onward; B wins at width 64.
Eight correctness runs cover all four D5 families in both modes at that scale.

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
This builds all targets, runs 20 CTests and independent evaluator tests,
executes all eight full-scale D5 replays, and runs the full-scale benchmark
(six widths, A/B, five repetitions). It verifies deterministic counters against
the published CSV, then exports the exact width-16384 input and compares all
500000 A/B evaluator answers. Finally it regenerates every shared figure and PDF.
Install requirements-docs.txt first; allow several minutes.

New timing rows are saved to `build/validation/full_scale_reproduced.csv`.
The original published timing CSV remains unchanged. The saved seed-253 input
is a width-1 base: `scripts/export_width_workload.py` applies the same range
mapping as the benchmark to export any measured width. See results/raw/workloads.md.
CI uploads CSVs, runner/compiler details, test logs and a release-status report.
A passing development pipeline does not mean the release checker has no blockers.

Regenerate figures and documents without running C++:
```bash
python3 scripts/export_docs.py
```
The full-scale renderer creates SVG/PNG runtime, counter and calibrated-model
figures from the committed 60-row CSV. It preserves zero counter values and
shows sample standard deviations. The model constants are fitted to the same
data; its curve is not an independent prediction. The earlier 100000-element
CSV and its two SVG plots remain historical evidence; its original operation
stream was not recorded, so exact stream reproduction is not claimed.

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
| Width 1, n=q=1000000 | A 0.4364 s; B 0.6297 s (means of five) |
| Width 64 | A 1.0668 s; B 0.7486 s |
| Width 1000 | A 2.0254 s; B 2.1285 s; spreads overlap |
| Width n=1000000 | A 0.0173 s; B 7.7966 s |
| Full-scale D5 replay | All 8 target/mode pairs pass at n=q=1000000 |
| Primed push probe, n=1000000 | 39 pushes per probe; 19500000 over 500000 probes |
| Aligned B workload, q=1000000 | 1000000 full blocks; 0 partial elements |

See REPORT.md for spread, method and limits.

## Known limitations and release status
- A lifecycle/mode validation was fixed by Naresh in PR #27.
- B still needs configurable-block-size overflow and mode validation checks.
  The menu still needs uint64 seed parsing; CLI seed support is separate.
- Individual proofs other than D5 exceed the one-to-two-page guideline.
  D1 lacks its name/roll inside the PDF; D2/D3/D4 also need uppercase filenames.
- Every member needs a genuine peer-approved PR; interface sign-offs,
  instructor access and the final annotated tag remain release tasks.
- Full-scale five-repeat timings are recorded. The fitted curve uses those
  same data; cache claims still need profiler evidence.
- REPORT.pdf and slides.pdf are generated from checked-in sources. Individual
  derivations other than D5 remain with their named owners.
- The modulo-only variant is a D3 derivation, not a supported calibration command.

Run
`python3 scripts/submission_check.py` to check exact artifact paths and proof page counts/roll numbers, and add
`--require-final-tag` only when preparing the completed release.
Do not create a final tag while these blockers remain.

## Tools and references
See TOOLS.md for AI use and representative prompts. The report lists source
files and the assigned Project 7 reading; members must cite the external
sources they actually consulted. No library container replaces either structure.


