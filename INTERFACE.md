# Shared contract — review before implementation

Status: **Architect v1 review candidate (12 November 2026)**. Existing structure owners must approve before this can be called frozen. See `docs/ARCHITECT_REVIEW.md` for open decisions and sign-off. Written before structures.
Language C++17. Public headers: include/cs253/. Ownership is listed in README.md.

## Values, ranges and modes
- Range is zero-based [begin,end), with 0 <= begin <= end <= size(). Invalid ranges throw std::out_of_range before mutation. Empty builds and empty ranges are supported.
- Values and tag coefficients are signed 64-bit. Residues are canonical [0,998244353).
- exact_extrema mode permits a only in {-1,0,1}; reject other gains with std::invalid_argument before mutation. Values/min/max remain mathematical signed integers; sum is modular. Caller/generators must ensure all exact results and composed offsets fit int64; overflow is outside the supported workload domain. Use checked or wider intermediates, including negation of INT64_MIN. Document implementation bounds before benchmarking.
- modular_sum mode permits arbitrary int64 a,b and initial values, reduced modulo the prime BEFORE arithmetic. Minimum and maximum are always absent: modular residues do not retain exact ordering. No midstream mode switch: build starts a new dataset.
- Empty result: length=0, sum_mod=0, minimum/maximum absent in either mode. Nonempty exact result has both extrema. build replaces all state and resets counters. **Open contract discrepancy:** `size()` and `mode()` are declared `noexcept`, but the original text required a successful build before calling them. The Architect proposes that `size()` return 0 and `mode()` return the default mode before build; confirm with both structure owners. `update()` and `query()` (including empty intervals) must throw `std::logic_error` before `build()`.
- Separate modes are a team design choice resolving the PDF's distinct sum/extrema guarantees. Confirm against any examiner-provided input format; no such format is specified in the PDF.

## Operations and composition
build(initial,mode), update(range,tag), query(range), counters(), reset_counters(), check_invariants().
Query returns all supported aggregates together. Query may push tags, so it is not const.
Old pending tag f(x)=a1*x+b1 followed by new g(x)=a2*x+b2 combines as
**compose(newer,older)=(a2*a1,a2*b1+b2)**. Identity=(1,0); reset=(0,b).
Example: +3 then negate gives -x-3; negate then +3 gives -x+3.
For length L: sum' = a*sum+b*L modulo P. Normalize negative residues.
For a >= 0: min'=a*min+b, max'=a*max+b. For a < 0 use OLD max for new min
and OLD min for new max. a=0 resets both extrema to b.
These are contracts and derivation targets, not supplied arithmetic implementations.

## Structure A invariant (owner D1)
A node's aggregates ALREADY include its own pending tag. That tag is owed only to descendants;
never apply it to the node again on push. Children may lag until pushed. Push applies the parent's
tag to each child aggregate, composes it AFTER the child's pending tag, then resets the parent tag.
After partial updates pull from current children. Full cover can stop at the node.
Storage must be a self-written segment tree; O(n) build/space, O(log n) update/query.

## Structure B invariant (owner D2)
Each block stores raw elements, an outstanding affine tag and logical aggregates that include it.
Full-block updates change the tag and aggregates; partial operations materialize pending work as
needed, then rebuild aggregates. Never apply a block's pending tag twice. Last short block uses
its actual length. Target O(n/b+b) per operation; choose and record b near sqrt(n).
Storage is self-written square-root decomposition. std::vector is backing storage, not a substitute
structure. Library maps/heaps/sets cannot stand in for either studied structure.

## Oracle and invariant checking
Oracle is an independently implemented plain array, same modes and exceptions. Do not share update
arithmetic with structures: shared bugs invalidate differential tests. InvariantAccess is a friend
of each concrete class. Oracle owner defines its overloads; check_invariants delegates to them.
Checks are const and must not repair or mutate state, alter counters, or hide pending-tag bugs.
Check effective aggregates against leaves/raw values, coverage, lengths, tags, mode rules and
canonical residues. Full audits are expensive: use on small differential workloads, never inside timings.

## Instrumentation
Counters reset separately from state. nodes_visited counts every tree-node entry in update/query;
push_downs counts nonidentity parent tags propagated to children (one per parent event, not per child).
full_blocks counts fully covered B blocks processed; partial_elements counts raw entries read/written
in partial-block processing INCLUDING materialization/rebuild; tag_compositions counts actual
composition calls; aggregate_merges counts actual merge calls. Unused fields stay zero.
Counters cover updates and queries after build, not build, audit, parsing or output.
Analyst must document these units and separate profiling from timing if overhead matters.

## Replay format (Architect v1 candidate, parser and replay implemented)
Whitespace tokens, no comments: CS253_V1; mode EXACT or MODULAR; seed; n q; n initial values;
then q operations, each either U begin end a b or Q begin end. Reject unknown tokens,
invalid values, truncated records and trailing input. Operation ranges use [begin,end).
Replay output: query index, length, canonical sum and extrema (NA when absent); throughput separately.
All structures replay the same initial values and operation sequence. Persist seeds plus workloads:
a seed alone does not guarantee reproduction across generator versions.

## Pipeline contracts
pipeline.hpp declares replay, differential_test, shrink_failure, generate, benchmark and CSV output.
Factories return fresh structures. Differential tests compare both structures with ArrayOracle,
check after queries as well as updates, record seed/index and shrink the first failure.
Benchmark runs on fresh state for >=5 repetitions, performs correctness validation outside timed
regions, consumes answers, records every run and counters; timings exclude build unless explicitly
reported as a separate metric. widths includes 1..n sweep with aligned/unaligned ranges.
CSV schema is in results/README.md. No timing/throughput values may be fabricated by stubs.


## Application CLI proposal (Architect-owned; not implemented)
- `calibration --help` prints usage and exits 0; `calibration --status` accurately identifies what is implemented.
- `calibration --run PATH --structure A|B|oracle` reads one workload from PATH and selects a structure. `oracle` is available only after the Oracle owner supplies its implementation.
- `calibration --all` runs the complete reproducible tests, application and benchmarks. Until the pipeline actually exists, it must fail nonzero, never report fabricated success.
- Exactly one command is accepted per invocation. Invalid flags, missing file names and unknown structures produce an informative message on stderr and a nonzero exit code.
- Query result output (proposed stable machine-readable format): `Q <zero-based-query-number> LEN <length> SUM <canonical-residue> MIN <integer|NA> MAX <integer|NA>`. One line per query, in order. Print `ELAPSED_SECONDS` and `OPS_PER_SECOND` separately *after* queries; execution time excludes parsing, build and output, unless separately documented.
- No CLI output or timing is computed by this contract document; it specifies behavior for the next implementation milestone.

## Parser and serializer edge cases (Architect-owned; implemented in Part 2)
- Text format is whitespace-tokenized, has no comments, uses the exact `CS253_V1` magic token and `EXACT` or `MODULAR` mode. All integer fields must be parsed completely and within their declared types; reject negative `n`, `q`, indices and malformed tokens before conversion to unsigned values.
- `n=0` and `q=0` are valid, as are operations on empty ranges `[i,i)` for `0<=i<=n`. A query on an empty interval reports length 0, sum 0 and extrema `NA`.
- Validate `0 <= begin <= end <= n` for **each** operation before replay; in `EXACT` mode reject `a` outside `{-1,0,1}` before applying it. Coefficients and initial values must fit signed 64-bit; seed must fit unsigned 64-bit.
- Reject incomplete `n`-value data, incomplete `q`-operation records, extra records and nonwhitespace trailing input. Reject unreasonably large allocations through a stated implementation resource limit rather than relying on unchecked capacity calculations. Part 2 sets 5,000,000 elements and 5,000,000 operations as a defensive parser and replay limit; the lab-required 1,000,000 scale is supported.
- `write_workload` uses the same format, and `read_workload(write_workload(w))` must preserve mode, seed, initial values, operation kinds/order, ranges and coefficients for valid workloads.
- Format handling and CLI errors should not silently mutate the running structure; parse the whole input successfully before calling `replay`.

## Architect Part 2 implementation status (12 November 2026)
- `app/module.cpp` now implements `read_workload`, `write_workload`, and `replay` under the proposed CS253_V1 contract, pending team approval.
- Replay validates the full workload before touching the selected structure, builds it, resets counters, times only the operation loop and captures query answers and counters. It does not fabricate timing or query data.
- `tests/app_test.cpp` verifies valid input, serialization round trips, parser rejection, exact and modular replay on both existing implementations, and empty intervals.
- The CLI in `app/main.cpp` is **still a placeholder**. CLI implementation and final --all orchestration are Part 3; this stage only links app_core to the new test.
- Sign-off of the proposed interface is still pending.
