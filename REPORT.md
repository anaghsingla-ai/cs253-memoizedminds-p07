# Design and Experimental Report

## Design decisions
- **Range convention**: Half-open zero-based ranges `[begin, end)` where `0 <= begin <= end <= n`. Empty ranges (`begin == end`) are valid no-ops on updates and return `length = 0, sum_mod = 0` with empty `minimum` and `maximum` on queries.
- **Operating modes**:
  - `exact_extrema`: Supports `a` in `{-1, 0, 1}`. Tracks exact 64-bit signed `minimum` and `maximum` (checked via `__int128_t`) alongside `sum_mod` modulo `P = 998244353`. When `a = -1`, the new minimum comes from `-old_max + b` and the new maximum comes from `-old_min + b`.
  - `modular_sum`: Supports arbitrary 64-bit signed `a, b` reduced modulo `P = 998244353`. Extrema are always absent (`std::nullopt`) because modulo arithmetic does not preserve ordering.
- **Tag composition**: Applying a newer tag `g(x) = a2 * x + b2` after an older pending tag `f(x) = a1 * x + b1` gives `compose(newer, older) = (a2 * a1, a2 * b1 + b2)` with identity `(1, 0)`.
- **Block size for Structure B**: We choose `b = ceil(sqrt(n))` (for `n = 100,000`, `b = 317`; for `n = 1,000,000`, `b = 1000`).

## Predictions — complete and commit before measurement
- **Structure A (Lazy Segment Tree) cost model**:
  - Each range update or query of width `w` visits at most `4 * log2(n) + O(1)` nodes:
    `C_A(n, w) = c_node * V(n, w) + c_push * P(n, w)`
  - At `w = 1`, updates and queries follow a single root-to-leaf path (`~2 * log2(n)` nodes) and never stop at internal nodes, so internal tags rarely accumulate. At `w = n`, every operation covers the root `[0, n)` in `1` node visit with `0` push-downs.
- **Structure B (Sqrt Decomposition) cost model**:
  - For block size `b = ceil(sqrt(n))` and range width `w`:
    - Number of fully covered blocks is roughly `F(w) = max(0, w / b - 1)`.
    - Number of partial blocks is at most `2`. A partial update scans the whole block of `b` elements (fused materialize + update + rebuild), whereas a partial query scans only the overlapped elements (`<= 2 * b`).
    - Cost per operation: `C_B(n, w) = c_full * (w / b) + c_elem * b`
- **Expected crossover**:
  - For small range widths (`w <= b`), Structure B touches only 1 or 2 contiguous blocks in L1/L2 cache with sequential array loops, while Structure A jumps across tree nodes in a `4n`-element array. Thus Structure B can match or beat Structure A on narrow ranges.
  - As `w` grows toward `n`, `w / b` grows up to `sqrt(n)` full-block touches per operation, while Structure A stays `O(log n)` (and drops to `O(1)` at `w = n`). We predict a clear crossover where Structure A overtakes Structure B once `w` spans dozens of blocks (`w >= 16,384`).

## Machine and method
- **Processor**: Intel(R) Xeon(R) 6985P-C (x86_64, 2 vCPUs)
- **Cache sizes**: L1d 48 KiB, L1i 64 KiB, L2 2 MiB, L3 504 MiB
- **OS & Compiler**: Linux 6.12.105+ x86_64, `g++ (Debian 12.2.0-14+deb12u1) 12.2.0` with `-std=c++17 -O3 -Wall -Wextra -Wpedantic`
- **Method**:
  - Before timing each width, `benchmark()` runs an untimed validation pass confirming that `structA` and `structB` return identical answers on every query.
  - Each structure is built from scratch and `reset_counters()` is called before starting `std::chrono::steady_clock::now()`.
  - Every width is measured for `5` repetitions (`seed = 253`). All query answers are XOR-accumulated into a checksum so the compiler cannot optimize queries away.

## Correctness evidence
- Both `structA` (`LazySegmentTree`) and `structB` (`SqrtDecomposition`) pass `contract_smoke`, `structB_test`, and the pre-timing validation check in `benchmark()` across all widths (`w in {1, 16, 317, 4096, 32768, 100000}`) with zero mismatches in `length`, `sum_mod`, `minimum`, and `maximum`.
- Edge cases tested: empty ranges `[i, i)`, single-element ranges (`w = 1`), block-sized ranges (`w = 317`), full-array updates/queries (`w = n`), and negative gain (`a = -1`) extrema swapping.

## Measurements and figures
Measured on `n = 100,000` elements and `q = 100,000` mixed operations (50% updates, 50% queries, `exact_extrema` mode, `seed = 253`, 5 repetitions per width). Raw logs are saved in `results/raw/width_sweep.csv` and plots in `results/figures/`.

| Width (w) | Struct A Mean Time (s) | Struct A SD (s) | Struct B Mean Time (s) | Struct B SD (s) |
| --- | --- | --- | --- | --- |
| 1 | 0.02684 | 0.00008 | 0.01882 | 0.00012 |
| 16 | 0.05246 | 0.00054 | 0.02131 | 0.00011 |
| 317 | 0.09849 | 0.00087 | 0.05996 | 0.00043 |
| 4,096 | 0.13065 | 0.00376 | 0.08954 | 0.00089 |
| 32,768 | 0.14805 | 0.00099 | 0.16180 | 0.00023 |
| 100,000 | 0.00183 | 0.00004 | 0.24582 | 0.00122 |

Operation counters across the same width sweep (`100,000` operations):

| Width (w) | A: nodes_visited | A: push_downs | B: full_blocks | B: partial_elements |
| --- | --- | --- | --- | --- |
| 1 | 1,768,850 | 0 | 0 | 15,887,444 |
| 16 | 2,386,392 | 196,391 | 0 | 17,381,153 |
| 317 | 3,232,133 | 870,453 | 323 | 47,383,109 |
| 4,096 | 3,947,302 | 1,582,362 | 1,192,499 | 47,374,717 |
| 32,768 | 4,502,104 | 2,143,376 | 10,237,157 | 47,414,149 |
| 100,000 | 100,000 | 0 | 31,600,000 | 0 |

## Adversarial results and gaps
- **Crossover analysis**: On narrow and medium widths (`w <= 4,096`), Structure B is faster (`0.0188s` vs `0.0268s` at `w = 1`) because its partial block pass is a contiguous loop over `b = 317` elements that the compiler vectorizes easily, while Structure A performs branching recursive tree traversals and `push_downs`. Between `w = 4,096` and `w = 32,768`, `full_blocks` in Structure B jumps from `1.19M` to `10.24M`, causing Structure B's runtime to overtake Structure A (`0.1618s` vs `0.1481s`). At `w = n = 100,000`, Structure A finishes in `0.00183s` (`1` root visit per op) while Structure B takes `0.2458s` (`31.6M` full-block updates/queries).
- **Predicted vs measured gap**: Asymptotic big-O counts alone suggest `O(log n)` beats `O(sqrt(n))` at every width, but constant factors (sequential array scan vs pointer-chasing tree nodes and `push_downs`) favor Structure B until `w / b` becomes large.

## Limitations and reproducibility
- **Arithmetic domain**: In `exact_extrema` mode, gains are restricted to `{-1, 0, 1}` and intermediate values are checked against 64-bit signed integer overflow using `__int128_t`.
- **Reproducibility**: Run `python3 results/scripts/plot_results.py results/raw/width_sweep.csv results/figures` to regenerate all figures from the recorded CSV logs.

## Adversarial results and gaps
TODO: all three required targets, crossover or reason none exists, predicted vs measured gaps.

## Limitations and reproducibility
TODO: arithmetic domain, peak memory, exact final release tag and fresh-clone command output.

## Member 5: adversarial inputs and verification (16 November 2026)

Anagh Singla (250001006) owns `adversary/` and D5. The `generate` function emits
all three required families plus a range-width sweep. Maximum-push probes are
preceded by a whole-array reset; negative-gain probes observe extrema before a
second negation could hide a missing swap; block-friendly operations cover one
aligned block, including the last short block. See `adversary/README.md` for the
commands and `docs/derivations/D5_250001006.pdf` for the proof.

Deterministic counter evidence from `tests/adversary_test.cpp` (GCC 14.2.0,
CMake 4.4.5, Release build, seed 253):

| Case | Predicted count | Verified count |
|---|---:|---:|
| n=16, primed partial query or update | 7 pushes | 7 pushes |
| n=1024, primed partial query or update | 19 pushes | 19 pushes |
| n=16384, primed partial query or update | 27 pushes | 27 pushes |
| n=50, one aligned B block per operation | 1 full block, 0 partial elements | 1 full block, 0 partial elements |

The `2*h-1` prediction was already recorded in D5 on the branch before this
review. Exhaustive ranges for sizes 1 through 48 confirm the generated query
achieves the maximum for those sizes. Repeated power-of-two checks reach
n=16384. The counter unit is one parent tag propagated to both children.

All 8 CTest checks passed. This includes 240 small differential/reproducibility
cases against independent test-only array arithmetic, invariant audits, CLI
checks, and generation of eight workloads with n=1000000 and q=1000000
(four families, two modes). The scale check validates generation and ranges;
it does not replay one million operations against the production Oracle.

These are correctness/counter checks, not timing benchmarks. The Analyst must
still measure A/B timings over at least five runs, report spread, and establish
whether the block-friendly pattern wins on the stated machine. Team-wide
pipeline work and final report export remain with their respective owners.
