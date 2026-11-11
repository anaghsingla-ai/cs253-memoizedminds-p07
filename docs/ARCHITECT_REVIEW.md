# Architect interface review — candidate v1

Date: 12 November 2026. Owner: Architect (name/GitHub handle not supplied).

**Status: awaiting six-member review.** This is a proposed contract, not evidence that teammates have agreed or that the implementation is finished. The existing public C++ headers and other owners’ source files were deliberately left unchanged.

## Already declared in code

- C++17; `RangeStructure` has `build`, `size`, `mode`, `update`, `query`, `counters`, `reset_counters`, `check_invariants`.
- `[begin,end)` zero-based indexing; empty ranges valid; `Workload` holds `mode`, `seed`, initial values and `std::variant<Update,Query>` operations.
- `EXACT` mode limits `a` to `-1`, `0`, `1`; `MODULAR` mode permits arbitrary `int64_t` gain/offset; sums are modulo 998244353; nonempty exact queries return extrema.
- Composition order is `compose(newer,older)`; after the old transform `(a1,b1)` the newer `(a2,b2)` produces `(a2*a1,a2*b1+b2)`.
- Existing CLI has only `--status`; parser, serializer, replay and real `--all` orchestration do not exist yet.

## Proposed decisions requiring confirmation

| Item | Proposed v1 behavior | Why it matters | Sign-off |
|---|---|---|---|
| Before-build `size()` / `mode()` | `size()` returns 0 and `mode()` returns default exact mode; retain current `noexcept` signatures | The earlier text said these accessors require a build, but `noexcept` is incompatible with throwing; do not change headers without owner review | Pending |
| Before-build `update()` / `query()` | Throw `std::logic_error`, even for `[0,0)` | Prevent unbuilt objects from appearing valid; Structure A currently needs an owner fix | Pending |
| Empty range | Update is a no-op; query returns `length=0,sum_mod=0,extrema=NA`, if the structure has been built | Consistency across structures | Pending |
| Indexing | Zero-based `[begin,end)` | Prevent off-by-one parser errors | Pending |
| Exact-mode gain | Only `-1,0,1` allowed; reject before mutation | Keeps extrema exact and well-defined under contract | Pending |
| Modular-mode extrema | Always absent | Modular residues do not preserve integer ordering | Pending |
| Overflow domain | Supported exact workloads must avoid signed 64-bit overflow; implementations use wide arithmetic/checks | Avoid C++ undefined behavior | Pending |
| Workload text | `CS253_V1` then EXACT/MODULAR, seed, `n q`, values, `U l r a b`/`Q l r` | Enables deterministic replay | Pending |
| CLI output | Stable query rows and separate elapsed/throughput fields per `INTERFACE.md` | Supports manual demo and automated comparisons | Pending |
| Pipeline `--all` | Must fail until tests, application and benchmarks genuinely run | Avoid false pass of rubric | Pending |

## Review requests by owner

- **Structure A:** confirm after-build empty-range behavior; fix before-build `update/query` to throw; confirm bounded exact arithmetic/overflow behavior.
- **Structure B:** confirm range and mode conventions, reset behavior, counters and before-build semantics.
- **Oracle:** confirm independent reference arithmetic, exception parity, invariants access, and parser-independent differential checking.
- **Adversary:** consume and save `Workload` in `CS253_V1` format, including seeds; coordinate generators with Architect.
- **Analyst:** confirm throughput timing scope, counter units and CSV fields; do not treat application throughput as full benchmark results.
- **Architect:** implement parser/serializer/replay and CLI *after* sign-off; write application tests; own D3 proof.

## Sign-off record (fill in before marking contract frozen)

| Role | Name / handle | Approval date | Agreement or requested change |
|---|---|---|---|
| Structure A | TBD | TBD | Pending |
| Structure B | TBD | TBD | Pending |
| Architect | TBD | TBD | Pending |
| Oracle | TBD | TBD | Pending |
| Adversary | TBD | TBD | Pending |
| Analyst | TBD | TBD | Pending |

## Scope boundary

This milestone only reviews and specifies the v1 interface and application behavior. It does **not** implement the CLI, parser, replay, oracle, benchmarks or proof D3. Those belong to subsequent work. Do not edit another owner’s implementation merely to make the contract appear complete.
