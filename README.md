# CS253 Project 7 — Lazy Propagation and What Composes

A C++17 sensor-calibration workbench for bulk affine range updates, where each selected reading is transformed by \(x \leftarrow ax+b\). The repository compares a lazy segment tree (Structure A) and square-root decomposition (Structure B), with an independent plain-array Oracle for correctness checking.

The deliverable is intentionally a terminal application rather than a web/desktop product. Its purpose is to make the algorithms demonstrable: create or load a workload, apply range corrections, inspect query aggregates, compare implementations, run differential tests, and export benchmark data.

## What the application does

Build the project, then start the interactive workbench:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/calibration --menu
```

The menu supports:

1. Load a workload from a `CS253_V1` file.
2. Generate a workload from the available adversarial families: push-downs, alternating negative gain, block-friendly, or range-width sweep.
3. Save the current workload so it can be replayed.
4. Run it using Structure A, Structure B, the Array Oracle, or all three.
5. Differential-test A and B against the independent Oracle.
6. Benchmark A against B and export five-repetition CSV results.
7. Show the current workload's mode, seed, number of elements, and number of operations.

For each query, replay prints its range length, sum modulo 998244353, and (in EXACT mode) minimum and maximum. It also prints operation-loop elapsed time, throughput, and instrumentation counters. Build, parsing, and output are excluded from the replay operation-loop timing.

## Example workload and expected result

The included exact-mode example is `tests/cases/architect_sample.txt`:

```text
CS253_V1
EXACT
253
4 5
2 4 1 7
Q 0 4
U 0 4 -1 3
Q 0 4
U 1 3 0 5
Q 0 4
```

The sequence starts with `[2, 4, 1, 7]`, queries the full range, applies `x <- -x + 3` to the full range, resets the middle two values to 5, and queries again. Expected query aggregates:

| Query state | Logical values | Sum mod 998244353 | Minimum | Maximum |
|---|---|---:|---:|---:|
| Initial | [2, 4, 1, 7] | 14 | 1 | 7 |
| After full-range affine update | [1, -1, 2, -4] | 998244351 | -4 | 2 |
| After middle-range reset | [1, 5, 5, -4] | 7 | -4 | 5 |

Run the workload through all implementations:

```bash
./build/calibration --run tests/cases/architect_sample.txt --structure both
```

A successful run prints the results for A, B, and Oracle and confirms that all query aggregates match. Timing values are measured at runtime and are not hard-coded.

## Commands

```bash
./build/calibration --help
./build/calibration --status
./build/calibration --menu
./build/calibration --run tests/cases/architect_sample.txt --structure A
./build/calibration --run tests/cases/architect_sample.txt --structure B
./build/calibration --run tests/cases/architect_sample.txt --structure oracle
./build/calibration --run tests/cases/architect_sample.txt --structure both
./build/calibration --all
bash scripts/run_all.sh
```

`--run` replays one workload. The `both` selector runs A, B, and Oracle and compares every query aggregate. `--all` runs the included sample comparison and an integrated five-repetition benchmark smoke campaign. It is an integration check, **not** a replacement for the required full-scale performance campaign. `scripts/run_all.sh` configures and builds the project, runs CTest, and invokes `--all`.

## Affine update model

A range update applies `x <- gain*x + offset` to every element in a half-open range `[begin,end)`.

- Gain 1 shifts each selected reading.
- Gain -1 reverses its sign and adds the offset.
- Gain 0 resets each selected reading to the offset.
- EXACT mode supports gains -1, 0, and 1, with exact min/max and modular sum.
- MODULAR mode supports arbitrary signed 64-bit affine coefficients and reports the modular sum only; extrema are absent because modular residues do not preserve ordering.

Updates are order-sensitive. If the older operation is \(f(x)=a_1x+b_1\) and the newer operation is \(g(x)=a_2x+b_2\), their composition is \((g\circ f)(x)=(a_2a_1)x+(a_2b_1+b_2)\). The sum is maintained modulo 998244353.

## Workload format

The parser accepts a whitespace-tokenized `CS253_V1` stream:

- Format marker: `CS253_V1`.
- Mode: `EXACT` or `MODULAR`.
- Reproducibility seed.
- Element count `n` and operation count `q`.
- Exactly `n` initial signed 64-bit values.
- Exactly `q` operations: `U begin end gain offset` or `Q begin end`.

Ranges are zero-based and half-open: `[begin,end)`. Malformed, truncated, invalid, out-of-range, or trailing input is rejected before replay. The current defensive parser/replay limit is five million elements and five million operations.

## Repository map

| Path | Responsibility |
|---|---|
| `src/structA/` and `include/cs253/lazy_segment_tree.hpp` | Structure A: lazy segment tree |
| `src/structB/` and `include/cs253/sqrt_decomposition.hpp` | Structure B: square-root decomposition |
| `oracle/` and `include/cs253/array_oracle.hpp` | Independent array Oracle and invariant access |
| `app/` and `include/cs253/pipeline.hpp` | Parser, serializer, replay engine, terminal workbench |
| `adversary/` | Reproducible adversarial workload generation |
| `bench/` | Benchmark harness and CSV output |
| `tests/` | Contract, module, differential, CLI, and menu tests |
| `docs/derivations/` | D1–D6 derivation drafts and PDFs |
| `results/raw/` | Raw benchmark measurements |
| `results/scripts/` | Plotting scripts |
| `REPORT.md` | Experimental report |

See [INTERFACE.md](INTERFACE.md) for the shared API, arithmetic rules, invariants, range conventions, and instrumentation counter definitions.

## Team and ownership

The member order and role mapping below follow the team's supplied order.

| Order | Member | Role | Main ownership | Derivation |
|---:|---|---|---|---|
| 1 | Naresh | Structure A | `src/structA/`, segment-tree implementation | D1 |
| 2 | Harshal | Structure B | `src/structB/`, square-root decomposition | D2 |
| 3 | Bhuvaneesh | Architect | `app/`, shared interface and integration | D3 |
| 4 | Arush | Oracle | `oracle/`, differential checks and invariants | D4 |
| 5 | Anagh | Adversary | `adversary/`, adversarial workload families | D5 |
| 6 | Ranveer | Analyst | `bench/`, `results/`, measurements and report | D6 |

Each owner is responsible for their own implementation, genuine commits, derivation, and pull request reviewed by another member. This roster does not imply that an owner's work has been reviewed or signed off.

## Tests and validation

Run:

```bash
ctest --test-dir build --output-on-failure
```

The suite covers the shared contract, Structure B, application parsing/replay, adversarial generation, Oracle behavior, differential testing, benchmark smoke tests, CLI behavior, and a scripted menu session.

A GitHub Actions run recorded 12/12 tests passing on `test-branch` at commit `8eb79dcd55d130858e7e558d21bb0376fa081025`. It included one-million-operation EXACT and MODULAR differential checks. Run CI again on the final branch/release before submission; the historical run is not a claim that every later commit has already been tested.

## Benchmark evidence and remaining work

The repository contains a five-repetition width-sweep dataset in `results/raw/width_sweep.csv` and its analysis in `REPORT.md`. The recorded campaign uses 100,000 elements and 100,000 operations. The one-million-operation differential checks are correctness tests, not a one-million-element/one-million-operation timing campaign.

Before final submission, the team must still verify and document the full required scale, record machine/compiler details and timing spread, measure adversarial and block-friendly workloads, and provide profiler evidence for cache-related claims. Keep raw runs and derived plots reproducible.

## Derivations and release checklist

The assignment requires six separately authored derivations, D1–D6, as PDFs under `docs/derivations/`. The repository currently contains a D3 PDF and a D5 PDF. D1, D2, D4, and D6 still need their final authored PDF deliverables; Markdown drafts do not replace the required PDFs.

Before creating the final release:

- [ ] All six members have reviewed and signed off the shared interface.
- [ ] Each owner has completed their implementation and derivation PDF.
- [ ] Full-scale correctness and benchmark campaigns are complete and reproducible.
- [ ] The report agrees with committed raw data and profiler evidence.
- [ ] Every member has genuine commits and at least one pull request reviewed by another member.
- [ ] The instructor has repository read access.
- [ ] A fresh clone builds, passes tests, and completes the demonstration workflow.
- [ ] Presentation and AI/tool-use disclosure are complete.

Do not mark the interface frozen or the project complete until the corresponding reviews, tests, and deliverables are actually finished.
