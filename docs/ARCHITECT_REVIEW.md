# Architect interface review — candidate v1

Date: 12 November 2026. Owner: Architect (insert name/GitHub handle).

**Status: awaiting six-member review.** This remains a proposed contract, not evidence that teammates
have agreed or that the interface is frozen. The application parser, serializer, replay engine and CLI
are now implemented in this working snapshot. The Oracle implementation, Adversary generator and Analyst benchmark implementation are present in this merged snapshot. The benchmark runner is not yet integrated into `calibration --all`. No remote Git push or team sign-off is claimed.

## Implemented by the Architect

- `read_workload()` parses and validates the bounded `CS253_V1` text format.
- `write_workload()` serializes a valid workload in the same format.
- `replay()` builds a selected `RangeStructure`, resets counters, times the operation loop, and
  returns query answers, counters and elapsed seconds.
- `calibration --run PATH --structure A|B|oracle|both` replays a workload and reports query results,
  elapsed time, throughput and counters. The Oracle selector is available; `both` compares A/B query aggregates.
- `calibration --all` runs an A/B/Oracle sample smoke comparison, then exits with status 3
  because the Analyst benchmark campaign is not yet wired into that command.
- D3 proof is written in `docs/derivations/D3.md` and exported to `D3.pdf`.

## Existing shared contract facts

- C++17; `RangeStructure` exposes `build`, `size`, `mode`, `update`, `query`, `counters`,
  `reset_counters` and `check_invariants`.
- Ranges use zero-based half-open indexing `[begin,end)`; empty ranges are valid after build.
- `EXACT` mode accepts affine gains `-1`, `0`, `1`; `MODULAR` mode accepts signed 64-bit affine
  coefficients and stores sums modulo 998244353. Modular extrema are absent.
- Composition order is `compose(newer, older) = (a2*a1, a2*b1+b2)`.

## Decisions still requiring team confirmation

| Item | Proposed v1 behavior | Why it matters | Sign-off |
|---|---|---|---|
| Before-build `size()` / `mode()` | Throw `std::logic_error`; declarations and definitions are not `noexcept` | Required by the build-state contract and now implemented/tested | Pending owner sign-off |
| Before-build `update()` / `query()` | Throw `std::logic_error`, including empty intervals | Prevent unbuilt objects from appearing valid | Pending |
| Empty range | Update is a no-op; query returns length 0, sum 0, extrema NA after build | Consistency across structures | Pending |
| Exact-mode arithmetic | Supported workloads avoid signed 64-bit overflow; implementations use checked/wide intermediates | Prevent undefined behavior | Pending |
| Timing scope | Build and parsing excluded; operation loop timed; output printed afterward | Makes A/B replay timing comparable | Pending |
| CLI structure selector | `A`, `B`, `oracle`, or `both`; Oracle selector is available | Makes current availability explicit | Pending owner sign-off |

## Owner-specific review requests

- **Structure A:** confirm empty-range behavior, before-build semantics and exact arithmetic bounds.
- **Structure B:** confirm range/mode conventions, reset behavior, counters and before-build semantics.
- **Oracle:** review independent array arithmetic, invariant access and differential/shrinking harness.
- **Adversary:** supply reproducible workload generators and save generated `CS253_V1` workloads.
- **Analyst:** review benchmark harness, CSV output, width sweep and documented counter units; coordinate `--all` integration.
- **Architect:** maintain parser/replay/CLI, D3 derivation and integration after owner modules arrive.

## Sign-off record

| Role | Name / handle | Approval date | Agreement or requested change |
|---|---|---|---|
| Structure A | TBD | TBD | Pending |
| Structure B | TBD | TBD | Pending |
| Architect | TBD | TBD | Pending |
| Oracle | TBD | TBD | Pending |
| Adversary | TBD | TBD | Pending |
| Analyst | TBD | TBD | Pending |

Do not change the status to “frozen v1” until all six owners have reviewed and approved the contract.
