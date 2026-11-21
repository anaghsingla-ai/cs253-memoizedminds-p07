# Project 7 - Lazy Propagation and What Composes
Team MemoizedMinds | CS 253 / MA 253 | Submission review, 21 November 2026

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
Arush #12 and #18; Anagh #3 and #17, with this submission audit in #21.
Naresh and Ranveer have implementation/data commits, but must provide their own
reviewed PR links rather than treating another member's integration PR as theirs.
These references describe observed history; they are not claims of peer approval.

This report packages shared work. D1, D2, D3, D4 and D6 remain the named owners'
individual submissions. Anagh's branch leaves their implementations unchanged.
See docs/SUBMISSION_AUDIT.md for exact outstanding actions and filename fixes.
The final tag is intentionally withheld until those blockers are resolved.

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
repairing state or changing counters. The documented pre-build update/query
exception is not consistently enforced by A; this source-review finding is
assigned to Naresh and is a release blocker, even though the tested replay paths
always call build first. INTERFACE.md is the full contract and still needs owner sign-off.

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
support. A's owner must add lifecycle guards and consistent mode validation.

## 5. Structure B: square-root decomposition
src/structB/module.cpp stores raw values and a vector of block records. With
automatic configuration, b=ceil(sqrt(n)); there are ceil(n/b) blocks.
A block record contains its pending a,b pair, modular sum and exact extrema.
The same affine identities apply as for A, but the spatial partition is flat.

A fully covered block takes O(1) work: map its sum and extrema and compose its
pending tag. For partial updates, one fused pass materializes every raw value
in the boundary block, applies the requested update where covered, and rebuilds
the block aggregate. It resets the pending map to identity. Even a one-element
update can therefore cost O(b), which matters when interpreting narrow widths.

A partial query scans only the covered raw elements, combines their summary
and applies the block's pending map to that summary. It does not materialize
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
profiler evidence. Harshal must finish the named D2 PDF and configuration audit.

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

The baseline main revision passed 12 CTest checks. This audit adds eight replay
checks at n=4097,q=8192, bringing CTest to 20 passing checks. Existing D5 tests
compare against a separate wide-integer array in 240 deterministic cases,
audit invariants and exhaust all query intervals at small n.
The new evaluator checker compares the parser/CLI output against Python
integer arithmetic and tests malformed input. See docs/VALIDATION.md for
revision-specific CI evidence rather than treating historical counts as current.

| Date | Module | Symptom / discovery | Status |
|---|---|---|---|
| 21 Nov 2026 | Shared docs | D5 PDF lowercase name broke documented path; tree audit | Correct uppercase artifact restored on this PR |
| 21 Nov 2026 | A | Empty update/query before build return instead of throwing; source audit | Naresh must fix in his own PR |
| 21 Nov 2026 | App menu | uint64 seed limits pass through long long casts; source audit | Architect must test and fix |
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
The inherited premeasurement model writes A's cost as
C_A=c_node*V(n,w)+c_push*P(n,w), with O(log n) traversal and at most two partial
nodes per level. For full width, V=1 and P=0. It writes B's cost as
C_B=c_full*F(w,b)+c_elem*E(w,b), with F about w/b and partial scanning bounded
by O(b). The familiar worst-case n/b+b model gives b near sqrt(n).

This predicts a strong A advantage for full-array operations and a possible B
advantage on narrow/aligned ranges where constants matter. The inherited
report suggested a crossover when widths span dozens of blocks, around 16384.
The six recorded widths only bracket a crossover between 4096 and 32768;
they do not measure its precise location or establish a universal threshold.

A formal prediction must predate the measurements it predicts. The course
guidelines explicitly check commit chronology. This review preserves the
existing model but does not claim that restating it now is a new prediction.
Ranveer must link the actual earlier model commit and subsequent data commit.
If that sequence cannot be demonstrated, report the limitation honestly.

No coefficients have been fitted after seeing data and relabeled as predictions.
The current figures show observed timing means with spread and separate
operation counts. A calibrated predicted-time curve on the same axes is still
the Analyst's task, with a stated fitting method and independent evaluation.
The code's asymptotic bounds do not predict exact seconds without constants.
D6 contains the individual minimization and model derivation.

## 10. Experimental setup and reproducibility
The historical timing record reports Intel Xeon 6985P-C, two vCPUs, Linux
6.12.105+ x86_64/Debian 12, GCC 12.2.0 and -std=c++17 -O3 -Wall -Wextra
-Wpedantic. Reported cache capacities are L1d 48 KiB, L1i 64 KiB,
L2 2 MiB and L3 504 MiB. This metadata is retained from results/machine.md;
it is not claimed as the hardware used by the new GitHub correctness jobs.

The timing CSV contains 60 rows: six widths, two structures and five
repetitions. All use n=q=100000, seed 253, exact_extrema mode and automatic
b=317. benchmark() validates A/B agreement before repetitions, builds each
structure before timing and resets counters. A steady clock surrounds replay.
The harness runs A then B in each repetition, rather than randomizing order;
that ordering is a possible measurement bias.

The precise original operation stream and command are not recorded with the
CSV. Thus the original figures can be regenerated, but a fresh byte-for-byte
input reconstruction cannot currently be certified. The shared pipeline runs
a clearly identified new range_width_sweep campaign through the existing
workbench and saves a separate reproduced_width_sweep.csv.

scripts/run_all.sh builds, tests, runs the sample, executes full-scale D5
verification, runs five-repeat timings and regenerates all committed figures
and PDFs. The C++ structures use no third-party dependency. PDF export uses
PyMuPDF and figure SVG generation uses only Python's standard library.
Build outputs and large inputs remain ignored. Re-running on another machine
will change timings; retain machine metadata with any new campaign.

## 11. Results, figures and interpretation
The following table is recalculated from the committed timing CSV; spread is
sample standard deviation. Times cover all 100000 operations, not one operation.

| Width (w) | Struct A Mean Time (s) | Struct A SD (s) | Struct B Mean Time (s) | Struct B SD (s) |
| --- | --- | --- | --- | --- |
| 1 | 0.02298 | 0.00036 | 0.01880 | 0.00007 |
| 16 | 0.04383 | 0.00040 | 0.02124 | 0.00012 |
| 317 | 0.08326 | 0.00096 | 0.05928 | 0.00014 |
| 4,096 | 0.11112 | 0.00141 | 0.08941 | 0.00056 |
| 32,768 | 0.13320 | 0.00092 | 0.16171 | 0.00205 |
| 100,000 | 0.00192 | 0.00007 | 0.24185 | 0.00170 |

Operation counters across the same width sweep (`100,000` operations):

| Width (w) | A: nodes_visited | A: push_downs | B: full_blocks | B: partial_elements |
| --- | --- | --- | --- | --- |
| 1 | 1,768,850 | 0 | 0 | 15,887,444 |
| 16 | 2,386,392 | 196,391 | 0 | 17,381,153 |
| 317 | 3,232,133 | 870,453 | 323 | 47,383,109 |
| 4,096 | 3,947,302 | 1,582,362 | 1,192,499 | 47,374,717 |
| 32,768 | 4,502,104 | 2,143,376 | 10,237,157 | 47,414,149 |
| 100,000 | 100,000 | 0 | 31,600,000 | 0 |


Figure 1: results/figures/width_sweep_time.svg. Generated by scripts/export_docs.py
from results/raw/width_sweep.csv; horizontal axis is logarithmic range width
(elements), vertical axis is elapsed seconds, and bars show one sample SD over
five runs per structure. B is faster through width 4096; A is faster at 32768.
The sampled data brackets a crossover but does not locate it.

Figure 2: results/figures/width_sweep_counters.svg. Same script and raw file.
Separate counter series retain their units; tree nodes, pushes, block accesses
and raw-element scans are not interchangeable work units.
At full width A visits exactly one root per operation and does not push.
B handles 316 blocks per operation at n=100000,b=317, or 31600000 accesses.

Sequential block scans are a plausible explanation for narrow-width constants,
but no profiler output proves a cache or vectorization cause. The older prose
contained numbers inconsistent with its own table; this edition uses the raw
five-run means throughout. Full-scale replay establishes tested correctness,
not full-scale throughput or comparative benchmark speed.

## 12. Limitations and next measurements
The submission is not ready for the final tag until the individual handoffs
and reviewed PR requirements in docs/SUBMISSION_AUDIT.md are complete.
A has a pre-build lifecycle mismatch, the menu needs uint64 seed checks,
D2 lacks its required PDF, and several individual PDFs have lowercase names.
Each owner should commit and explain their own fixes.

Exact arithmetic is limited to gains -1,0,1 and int64 values/composed offsets.
Overflow is outside the workload contract; callers should not rely on recovery
after a failed overflowing update. General modular gains cannot return exact
ordered extrema. The application parser caps n and q at five million.

The recorded performance campaign is one-tenth of the specified full scale.
No peak memory, cache-profiler output, precise crossover search, mixed-machine
comparison or calibrated predicted-time curve is provided. The Analyst should
run n=q=1000000 with at least five repetitions, record every seed, sample more
widths between 4096 and 32768, preserve actual input recipes, and compare model
predictions without retrospectively changing the prediction.

All eight full-scale D5 correctness replays pass, but they are not official
hidden tests. Large A/B agreement is supplemented by small independent-array
checks and production oracle tests; shared implementation mistakes remain a
general limitation of differential testing. Invariant audits must stay outside
timed loops. A final fresh-clone run from the completed annotated tag is still
necessary, along with instructor access and the portal's required commit SHA.
No checkpoint history is fabricated or backdated in this review.

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
consulted. TOOLS.md declares AI assistance and representative prompts. The
2026-11-21 shared audit uses code review, raw-data arithmetic and GitHub CI.
Teammate-owned proofs and source modules require their own authors' sign-off.

