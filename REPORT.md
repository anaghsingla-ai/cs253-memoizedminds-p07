# Project 7 - Lazy Propagation and What Composes
Team MemoizedMinds | CS 253 / MA 253 | Technical report, updated 23 November 2026

## 1. Team and contributions
| Member / roll | Role | Module / derivation |
|---|---|---|
| Naresh / 250001052 | Structure A | src/structA/ - D1 |
| Harshal / 250001032 | Structure B | src/structB/ - D2 |
| Bhuvaneesh / 250001017 | Architect | app/, include/, INTERFACE.md - D3 |
| Arush / 250001008 | Oracle | oracle/ - D4 |
| Anagh / 250001006 | Adversary | adversary/ - D5 |
| Ranveer / 250041031 | Analyst | bench/, results/ - D6 |

Structure A supplies the recursive segment tree and its invariant checker.
Structure B supplies block tags, full/partial block handling and a configurable
block size. The Architect supplies the common contract, strict CS253_V1 parser,
replay engine and terminal application. The Oracle owner supplies independent
array arithmetic, seeded differential testing, shrinking and invariant access.
The Adversary supplies all required workload families and their verification.
The Analyst supplies timing, operation counters, raw CSV and the cost model.

Contribution links in this repository: Harshal PRs #2 and #8; Bhuvaneesh #5 and #9;
Arush #12 and #18; Anagh #3 and #17, with additional D5 verification in #21.
Structure A and benchmark contributions are recorded in their module/data
commit history. These links identify contributions, not evidence of peer approval.

Each member submits their derivation separately under docs/derivations/.
Section 12 records remaining implementation, measurement and submission limitations.

## 2. Problem and application
The calibration table contains n sensor readings. A correction of range [l,r)
changes each reading by x -> a*x+b. Gain -1 flips an inverted sensor, gain zero
resets it to a constant, and gain one shifts it. Queries return the range length,
a sum checksum modulo 998244353 and, in exact mode, the minimum and maximum.

The required scale is at least one million readings and one million operations.
Scanning every affected value for every wide correction would take O(n*q) work.
The project therefore compares two independently implemented ways to defer
updates: a recursive segment tree and square-root decomposition. A plain array
is retained as a correctness oracle on smaller inputs.

The application accepts CS253_V1 workloads, replays a selected implementation,
and reports query answers, elapsed time, throughput and operation counters.
The interactive menu can load, generate and save a workload, compare A/B/oracle,
run differential tests and write five-repeat benchmark CSVs. The deterministic
scripts/run_ops.py adapter provides one answer row per query for automated checks.

The delivered runtime supports affine calibration. The separate modulo-only
operation is the subject of D3's impossibility and amortisation derivation;
there is no range-modulo command in this calibration simulator.
Two arithmetic modes separate exact extrema from general modular coefficients.
This is a documented team interface choice; the Architect must reconcile any
separately announced examiner protocol before release.

## 3. Interface and invariants
Public types and operations are declared under include/cs253/. Ranges are
zero-based half-open intervals [begin,end). An empty interval is a valid update
no-op and query identity. Out-of-range endpoints must be rejected before mutation.
The API is build, update, query, counters, reset_counters and check_invariants.

EXACT permits a in {-1,0,1}; values, extrema and composed offsets must fit int64.
The sum is still modular. MODULAR permits arbitrary int64 coefficients and
initial values, reduced modulo P=998244353, with no exact extrema.
An empty result is length=0, sum_mod=0 and absent minimum/maximum in either mode.

A node in A already includes its pending tag in its own aggregates. Descendants
may lag. Pushing applies the parent tag to both children, composes it after each
child's older tag, then clears the parent's tag to (1,0). A partial update pulls
the parent from the updated children. A full-cover update stops at the node.

A block in B stores raw values, a pending affine map and logical aggregates.
Full-block updates change the logical aggregates and pending map. Partial
updates materialize and rebuild the block; partial queries apply its map to the
summary of the covered raw values. The final short block uses its actual length.

Checkers verify residues, extrema, tags and aggregate relationships without
repairing state or changing counters. Structure A now enforces the documented pre-build update/query exception and
mode validation, including empty intervals (Naresh, PR #27). INTERFACE.md is the full contract and still needs owner sign-off.

## 4. Structure A: lazy segment tree
src/structA/module.cpp implements an array-backed recursive midpoint tree.
Storage reserves 4*n+4 nodes. Each node holds a modular sum, exact extrema when
enabled and one pending affine tag. Build descends to leaves and merges upward.
The inclusive recursion [tl,tr] is an implementation detail; public ranges remain
half-open and update/query convert the right endpoint to end-1.

For a full-covered segment of length L, the sum changes to a*S+b*L modulo P.
For a=1, both extrema shift by b; for a=0 they become b; for a=-1,
new_min=-old_max+b and new_max=-old_min+b. Old extrema must both be retained
until these two values are computed. Wider intermediates detect unsupported
exact overflow and avoid negating INT64_MIN in int64 arithmetic.

When a partial request cuts a node, push propagates a nonidentity map to both
children. New updates compose after older work: (a2*a1,a2*b1+b2).
Query can therefore mutate the distribution of pending tags without changing
logical values. A full-covered query returns immediately from that node.

Build and storage are O(n). A contiguous range has at most two partial nodes
per depth, giving O(log n) update/query traversal; full-array operations use
one root visit. Counters distinguish visits, pushes, compositions and merges.
One push means one parent propagation, not two child applications.

The array layout avoids a library container substituting for the structure.
Maintaining both extrema is necessary for negative gains. Rejecting unsupported
overflow is part of the declared domain, not evidence of arbitrary-precision
support. PR #27 added the required lifecycle guards and mode validation. Unbuilt empty
operations throw; empty operations after build remain valid.

## 5. Structure B: square-root decomposition
src/structB/module.cpp stores raw values and a vector of block records. With
automatic configuration, b=ceil(sqrt(n)); there are ceil(n/b) blocks.
A block record contains its pending a,b pair, modular sum and exact extrema.
The same affine identities apply as for A, but the spatial partition is flat.

A fully covered block takes O(1) work: map its sum and extrema and compose its
pending tag. For partial updates, the current implementation materializes a pending block
tag, updates the covered raw elements, then recomputes the block aggregate. It resets the pending map to identity. Even a one-element
update can therefore cost O(b), which matters when interpreting narrow widths.

A partial query maps each covered raw value through the block's pending tag
and combines those logical values. It does not materialize
the block. Multiple covered blocks are folded into the result, respecting
length, modular sums and the enabled extrema. Short final blocks retain their
own length in all formulas.

At most two boundary blocks are partial. A worst-case operation therefore
costs O(n/b+b), with O(n) build and backing storage. The unweighted model is
minimized near sqrt(n); the actual optimum depends on relative full-block and
element costs. A public block-size setting supports experiments but must be
reset to automatic for the block-friendly generator's alignment guarantee.

The simpler contiguous loops can have favorable constants, but this report
does not claim measured cache misses or vectorization. Those claims need
profiler evidence. The D2 PDF exists. Configurable-block-size bounds and invalid-mode rejection
still require owner validation. With n=2 and block size SIZE_MAX, build now
creates one block, but the invariant checker's ceiling expression wraps to
zero and falsely reports a block-count mismatch. Build also accepts an invalid
Mode value. These are source-derived findings; the official hidden tests are unavailable.

## 6. Beyond the lecture: affine tags and modulo
Affine updates compose because applying f(x)=a1*x+b1 and then g(x)=a2*x+b2
gives g(f(x))=(a2*a1)*x+(a2*b1+b2). The identity is (1,0), and reset is (0,b).
Changing the order changes the result: adding 3 then negating gives -x-3,
whereas negating then adding 3 gives -x+3. D2 justifies these identities,
sum mapping and the negative-gain extrema swap in full.

The ordinary lazy sum summary cannot map a modulo update using only its old
sum, length and k. Arrays [1,5] and [3,3] both have sum 6 and length 2.
Modulo 4 changes their sums to 2 and 6 respectively. A function of (sum,length,k)
would receive identical inputs and need two different outputs, a contradiction.
This statement concerns that summary; it does not rule out richer structures.

For the nonnegative modulo-only variant with point assignments and range sums,
a maximum per node permits pruning when max<k. A changed value x>=k becomes
x mod k < x/2: if k<=x/2 the remainder is below k, and if k>x/2 the quotient is
one and the remainder is x-k<x/2. Each value has O(log C) such changes before
reaching zero, unless a point assignment replenishes that budget.
Charging changed leaves and boundary traversal yields the stated
O((n+m)*log(n)*log(C)) bound, with the conventional max(1,log(C)) for tiny C.

D3 is the Architect's individual proof, in docs/derivations/D3.md and the
owner's PDF. The affine simulator does not implement this modulo-only variant.
The course brief lists segment-tree and sqrt-decomposition readings; members
must state which they actually consulted rather than claim all assigned works
as implementation sources. Individual derivations remain separately assessed.

## 7. Correctness and bug log
The production oracle is an independently implemented plain array. It does not
share affine arithmetic with either optimized structure. The D4 differential
harness uses one million operations on 128 elements in each mode: seed 253 for
EXACT and 42 for MODULAR. Failure evidence includes a reproducing workload and
a shrinker; tests check that deletion of any remaining operation removes the
failure in the test scenario.

The baseline main revision passed 12 CTest checks. Eight additional replay
checks at n=4097,q=8192 bring CTest to 20 passing checks. Existing D5 tests
compare against a separate wide-integer array in 240 deterministic cases,
audit invariants and exhaust all query intervals at small n.
The new evaluator checker compares the parser/CLI output against Python
integer arithmetic and tests malformed input. See docs/VALIDATION.md for
revision-specific CI evidence rather than treating historical counts as current.
The current contract smoke test covers pre-build size/mode/update/query and
empty builds for A, B and the oracle. Extreme block sizes and the menu seed
conversion remain outside those passing checks.

| Date | Module | Symptom / discovery | Status |
|---|---|---|---|
| 21 Nov 2026 | Shared docs | D5 PDF lowercase name broke documented path; tree audit | Correct uppercase artifact restored in commit 8c32fd6 |
| 21 Nov 2026 | A | Empty update/query before build return instead of throwing; source audit | Fixed by Naresh in PR #27 |
| 21 Nov 2026 | App menu | uint64 seed limits pass through long long casts; source audit | Unresolved |
| 21 Nov 2026 | Validation | Million-element D5 check only generated data; test audit | Added actual A/B replay in ad65d1a |
| 21 Nov 2026 | Shared evaluator | Human-readable output included timings/headings | Added deterministic adapter in 7d2326e |

The log deliberately distinguishes fixes from unresolved findings. Passing the
tested workloads does not mean every public API state or hidden input is proven.

## 8. Adversarial inputs and measured counters
D5's push_downs family alternates a whole-array reset (0,c) with a deep
two-boundary query. A reset remains nonidentity even when c=0. Pushing it
overwrites any older child map with another reset, so all possible partial-node
pushes occur. Repeated probes require re-priming; a consumed lazy tag is not free
evidence of another worst-case query.

For a power-of-two n=2^h, a probe [1,n/2+1) attains 2*h-1 pushes. For arbitrary
n>=2, let L=ceil(n/2), R=floor(n/2), H(s)=ceil(log2(s)); the exact maximum is
1+H(L)+H(R). For n=1000000 this is 39. The full-scale push family has 500000
queries and exactly 19500000 pushes in each arithmetic mode.

Alternating_negative_gain exposes swapped extrema before another negation can
hide a bug; it also combines shifts, resets, overlapping intervals and empty
queries. Block_friendly uses one aligned automatic block per operation,
including a short last block. Range_width_sweep includes point/full widths,
powers of two, requested width and widths around b, at several alignments.
Modular sweeps include extreme signed coefficients.

All four families execute n=q=1000000 in both modes, seed 253, width 999.
Every query from A and B agrees and both final audits pass. The push family
also agrees with an independent constant-array formula. Block-friendly reports
1000000 full-block accesses and zero partial elements in B. The eight raw rows
are in results/validation/adversary_full_scale.csv. These are verification
counts, not wall-clock performance measurements or proof of a B timing win.

## 9. Predicted cost model and chronology
The qualitative model in REPORT.md at commit
5fcb902c3ffa02335e9bcee371cad00b78d22102 (16 November) predates the
full-scale CSV commit d33379b168558e4f406326391885ff8c7d583221 (21 November).
It postdates the older 100000-element campaign; no earlier prediction is claimed
for that historical data. The model writes A's cost as
`C_A = a_visit * V_A(n, w) + a_push * P_A(n, w)`, with `O(log n)` traversal and at most two partial nodes per level (`V_A = 1`, `P_A = 0` at full width `w = n`). It writes B's cost as `C_B = b_full * F_B(w, b) + b_elem * E_B(w, b)`, with `F_B � max(0, w / b - 1)` and partial scanning bounded by `O(b)`. Minimising `n / b + b` gives `b = ceil(sqrt(n))` (`b = 1,000` at `n = 1,000,000`).

Before the full-scale campaign, this model predicted a strong A advantage on wide and full-array ranges, with a possible B advantage on narrow ranges where per-element constants matter. The quantitative curves are calibrated after measurement: `scripts/render_benchmark_figures.py` fits the two linear constants per structure by ordinary least squares against the full-scale operation counters (`results/figures/predicted_vs_measured.png`):
- Structure A: `a_visit = 28.02 ns/node`, `a_push = 57.70 ns/push`
- Structure B: `b_full = 7.89 ns/block`, `b_elem = 1.67 ns/element`

These constants use the same six measured means shown in the curve. This is
a descriptive calibration, not an independent numerical forecast or holdout
validation. The full-width drop for A is structural; residuals alone cannot
establish cache behavior. A new unseen-width campaign would test the fit.

## 10. Experimental setup and reproducibility
The published CSV timings were collected on Intel(R) Xeon(R) 6985P-C (2 vCPUs, L1d 48 KiB, L1i 64 KiB, L2 2 MiB, L3 504 MiB), Linux 6.12.105+ x86_64 (Debian 12), GCC 12.2.0 (`-std=c++17 -O3 -Wall -Wextra -Wpedantic`), recorded in `results/machine.md`.

The full-scale timing CSV (`results/raw/full_scale_width_sweep.csv`) contains 60 rows: 6 widths (`1, 64, 1000, 16384, 262144, 1000000`), 2 structures (`structA`, `structB`), and 5 repetitions each, at `n = q = 1,000,000`, `seed = 253`, `exact_extrema` mode, and automatic `b = 1,000`. `benchmark()` validates A/B agreement before timing, builds each structure fresh before each repetition, resets counters, and surrounds only replay with `std::chrono::steady_clock`.

Reproducible input recipe: `bench/run_full_scale.cpp` deterministically generates the exact `seed = 253` workload via `std::mt19937_64` and exports the full `CS253_V1` input stream when run with `--save-input` (documented in `results/raw/workloads.md`). The earlier `n = q = 100,000` campaign (`results/raw/width_sweep.csv`) is retained as historical data.

The published full-scale data were committed in d33379b168558e4f406326391885ff8c7d583221.
Structure B was subsequently changed in 34ee633febca6edaa6c9638bde66712d4bb28a86
and merged via PR #31. Its partial-update/query loops differ from the measured
version. The published table remains evidence for that earlier implementation;
matching counters do not establish equal internal work or elapsed times.
Current-code timings are independently reproduced in the CI artifact, with its
runner/compiler metadata, rather than silently replacing the published CSV.

## 11. Results, figures and interpretation
Published campaign timings, for the implementation described in section 10
(`n = q = 1,000,000`, 5 repetitions, mean and sample SD in seconds, from `results/raw/full_scale_width_sweep.csv`):

| Width (w) | Struct A Mean (s) | Struct A SD (s) | Struct B Mean (s) | Struct B SD (s) |
| --- | --- | --- | --- | --- |
| 1 | 0.4364 | 0.0447 | 0.6297 | 0.0410 |
| 64 | 1.0668 | 0.0483 | 0.7486 | 0.0594 |
| 1,000 | 2.0254 | 0.0273 | 2.1285 | 0.1746 |
| 16,384 | 2.5096 | 0.2915 | 2.8043 | 0.0689 |
| 262,144 | 2.9543 | 0.2279 | 4.9205 | 0.0156 |
| 1,000,000 | 0.0173 | 0.0008 | 7.7966 | 0.0305 |

<!-- page -->

### Full-scale operation counts
Operation counters across the same full-scale sweep (`1,000,000` operations):

| Width (w) | A: nodes_visited | A: push_downs | B: full_blocks | B: partial_elements |
| --- | --- | --- | --- | --- |
| 1 | 20,952,012 | 0 | 0 | 500,500,000 |
| 64 | 30,984,277 | 4,546,317 | 0 | 563,466,000 |
| 1,000 | 38,886,506 | 11,703,343 | 985 | 1,498,520,000 |
| 16,384 | 46,841,714 | 19,671,587 | 15,384,733 | 1,498,394,000 |
| 262,144 | 54,238,701 | 27,371,872 | 261,145,083 | 1,498,398,000 |
| 1,000,000 | 1,000,000 | 0 | 1,000,000,000 | 0 |

<!-- page -->

### Calibrated model and interpretation
Measured versus fitted mean runtime (seconds), using constants calibrated on these same data:

| Width (w) | A Measured (s) | A Predicted (s) | B Measured (s) | B Predicted (s) |
| --- | --- | --- | --- | --- |
| 1 | 0.4364 | 0.5872 | 0.6297 | 0.8382 |
| 64 | 1.0668 | 1.1307 | 0.7486 | 0.9436 |
| 1,000 | 2.0254 | 1.7651 | 2.1285 | 2.5095 |
| 16,384 | 2.5096 | 2.4478 | 2.8043 | 2.6307 |
| 262,144 | 2.9543 | 3.0994 | 4.9205 | 4.5700 |
| 1,000,000 | 0.0173 | 0.0280 | 7.7966 | 7.8908 |

Figures (`results/figures/full_scale_time.png`, `full_scale_counters.png`, and `predicted_vs_measured.png`) and matching SVGs are regenerated by `scripts/render_benchmark_figures.py`;
`python3 scripts/export_docs.py` regenerates all figures and shared PDFs.

Interpretation:
- At `w = 1`, Structure A (`0.4364 s`) beats Structure B (`0.6297 s`) because A has `0` push-downs while B still rebuilds `b = 1,000` elements per partial update.
- At `w = 64`, Structure A triggers `4.55M` push-downs (`1.0668 s`), allowing Structure B (`0.7486 s`) to beat A because B's per-element constant (`1.67 ns`) is much smaller than A's per-push constant (`57.70 ns`).
- From `w = 1,000` onward, Structure B crosses two boundary blocks (`1.50B` partial elements) and accumulates up to `1.00B` full-block touches, so Structure A regains the lead (`2.0254 s` vs `2.1285 s` at `w = 1,000`) and reaches `0.0173 s` vs `7.7966 s` at `w = 1,000,000` (`450x` faster).
- Residual gaps show the limits of a two-constant fit. Branching, alignment, locality and fixed overhead are possible explanations; hardware cache-miss attribution remains unverified without profiler output.
- The sampled mean ordering changes between widths 1 and 64, and again between 64 and 1000. These are crossover brackets, not precise locations. At width 1000 the timing spread overlaps, so more nearby trials are required for a confident boundary.

## 12. Limitations and next measurements
- Implementation: B needs invalid-mode rejection and an overflow-safe block-count invariant; menu option 5 still passes a uint64 limit through signed long long. The CLI's seed support does not fix that menu path.
- Proof packaging: D1/D2/D3 PDFs are three pages; the guideline permits one or two. D1 lacks its roll inside the PDF. All six filenames are correct; D4/D5/D6 pass the page-count and roll checks. D2 Markdown is still unnamed/draft and D6 Markdown retains draft status; owners must finalize their own sources.
- Arithmetic: EXACT supports gains {-1,0,1} with int64 values/composed offsets. MODULAR supports general gains without exact ordered extrema.
- Measurements: the published campaign predates the latest B loops. The fitted curves use the same measured data, not held-out predictions. The sampled widths only bracket crossovers; nearby measurements and explanation of the narrow-width mismatch remain needed. Cache/vectorization claims require profiler/compiler evidence.
- Documentation: the shared summaries do not replace each member's verified sources and proof. Structure-owner interface approvals and genuine peer approvals for each member remain incomplete.
- Release: main 0cbeaa00 passed all ten CI jobs, including full-scale reproduction. The release checker still reports three proof-format blockers. No checkpoint/final tags were returned. Confirm instructor/TA access, finish owner actions, obtain review, rerun the release gate and rehearse a fresh clone of the annotated final tag before submitting its full SHA. Historical deadlines/tags must not be invented.

## 13. References, sources and tool use
Primary project requirements: CS253_MA253_Lab_Projects.docx (2).pdf,
Project 7, pp. 13-14, and common roles/scale/marking sections.
Submission format: CS253_MA253_Submission_Guidelines (2).pdf, sections 1-9.

Implementation sources for this report:
- INTERFACE.md and include/cs253/: arithmetic, ranges, invariants and counters.
- src/structA/module.cpp: recursive updates, pushes, pulls and tree checker.
- src/structB/module.cpp: full/partial blocks, materialization and block checker.
- app/module.cpp and app/main.cpp: parser, replay and workbench.
- oracle/module.cpp and tests/differential_test.cpp: oracle, shrinking and tests.
- adversary/module.cpp, replay_check.cpp and docs/derivations/D5.md: D5.
- bench/module.cpp, results/raw/width_sweep.csv and results/machine.md: timings.

Assigned Project 7 reading (each member must confirm actual use and cite it in
their own derivation where applicable):
- de Berg et al., Computational Geometry, 3rd ed., 2008, Chapter 10.
- J. L. Bentley, Decomposable searching problems, IPL 8(5), 1979.
- Halim, Halim and Effendy, Competitive Programming 4, Book 1, Chapter 2.
- cp-algorithms: Segment Tree (range updates) and Sqrt Decomposition.
- USACO Guide: Segment Tree Beats, for the amortized modulo discussion.

This list identifies the assigned reading, not a claim that every work was
consulted. TOOLS.md declares AI assistance and representative prompts. Correctness evidence comes from independent arithmetic, invariant checks,
raw-data calculations and GitHub CI. Individual proofs require their authors' sign-off.


