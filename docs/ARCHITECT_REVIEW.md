# Architect review — shared interface and application integration

**Owner:** Bhuvaneesh (Architect)  
**Review date:** 17 November 2026  
**Interface status:** proposed v1; team sign-off is still pending.

This document records the Architect-owned contract and application integration decisions. It is not a replacement for the six owners' reviews, module proofs, test evidence, or final performance report.

## Architect-owned deliverables

The Architect owns the workload format, parser/serializer, replay pipeline, public interface coordination, terminal application, and D3 derivation.

- `app/module.cpp`: `read_workload()`, `write_workload()`, and `replay()`.
- `app/main.cpp`: command-line entry points and the interactive Sensor Calibration Workbench.
- `include/cs253/pipeline.hpp`: pipeline contracts used by the application and the other modules.
- `INTERFACE.md`: shared data-structure and workload contracts.
- `docs/derivations/D3.md` and `docs/derivations/D3.pdf`: Architect's derivation deliverables; author review is still required.

The terminal workbench supports loading and saving workloads, generating adversarial workloads through the generator interface, running A/B/Oracle, comparing query aggregates, running differential tests, and exporting five-repetition benchmark CSV. It consumes the other owners' modules through their public interfaces; this review does not claim ownership of their algorithms or experimental results.

## Application commands

```bash
calibration --help
calibration --status
calibration --menu
calibration --run WORKLOAD --structure A|B|oracle|both
calibration --all
```

- `--menu` is the interactive workflow for loading/generating/saving workloads, replaying a chosen implementation, running differential checks, and exporting benchmark CSV.
- `--run` replays one `CS253_V1` workload. The `both` selector runs A, B, and Oracle and compares every query aggregate.
- `--all` runs the included sample through A/B/Oracle, then runs the integrated five-repetition benchmark smoke campaign. It is an integration check, not the required full-scale benchmark campaign.
- Replay prints a machine-readable row for each query, followed by elapsed operation-loop time, throughput, and instrumentation counters. Parsing, build, and output are outside the timed operation loop.

## Shared interface facts

- C++17; `RangeStructure` exposes `build`, `size`, `mode`, `update`, `query`, `counters`, `reset_counters`, and `check_invariants`.
- Ranges are zero-based and half-open: `[begin,end)`.
- Empty ranges are valid after build. An empty query returns length 0, sum 0, and absent extrema.
- EXACT mode restricts gain to -1, 0, or 1; it reports exact min/max and modular sum.
- MODULAR mode accepts signed 64-bit affine coefficients, reduces arithmetic modulo 998244353, and does not report extrema.
- An older pending affine tag followed by a newer tag composes as `compose(newer, older) = (a2*a1, a2*b1+b2)`.
- Build replaces the current dataset and resets counters. Before-build access/update/query behavior is specified in `INTERFACE.md`; structure owners must confirm their implementations match it.

## Workload format and validation

The parser accepts a whitespace-tokenized `CS253_V1` stream:

```text
CS253_V1
EXACT
seed
n q
<n initial values>
U begin end gain offset
Q begin end
```

There are exactly `n` initial values and exactly `q` operations. Unknown tokens, invalid ranges, malformed/truncated records, invalid EXACT gains, and trailing non-whitespace input are rejected before replay. The current defensive resource limit is five million elements and five million operations. The serializer emits the same format and is intended to round-trip every valid workload.

## Decisions and owner confirmations

| Decision | Proposed v1 contract | Confirmation |
|---|---|---|
| Before-build `size()` / `mode()` | Throw `std::logic_error` | Pending Structure A and B owners |
| Before-build `update()` / `query()`, including empty ranges | Throw `std::logic_error` | Pending Structure A and B owners |
| Empty update after build | No-op | Pending Structure A and B owners |
| Empty query after build | Length 0, sum 0, extrema absent | Pending Structure A and B owners |
| Exact arithmetic | Supported workloads avoid signed 64-bit overflow; implementations use safe intermediates for supported inputs | Pending Structure A and B owners |
| Timing scope | Build/parsing/output excluded; operation loop timed | Analyst review pending |
| Replay equivalence | A, B, and Oracle return identical query aggregates | Oracle and structure-owner review pending |
| Counter units | As specified in `INTERFACE.md` | Analyst and structure-owner review pending |

The Architect maintains the public contract and coordinates changes. Changes affecting the two data structures require agreement from both structure owners. Oracle and Analyst owners retain responsibility for their own implementation and evidence.

## Review requests by owner

- **Naresh — Structure A:** verify lazy-tag invariant, empty ranges, before-build exceptions, and exact arithmetic bounds; complete D1.
- **Harshal — Structure B:** verify block-tag invariant, partial-block handling, empty ranges, counters, and exact arithmetic bounds; complete D2.
- **Bhuvaneesh — Architect:** maintain parser/serializer/replay/CLI, shared interface coordination, and D3.
- **Arush — Oracle:** verify independent array arithmetic, invariant checks, differential testing, and failure shrinking; complete D4.
- **Anagh — Adversary:** verify reproducible adversarial families and saved workload fixtures; complete D5.
- **Ranveer — Analyst:** verify benchmark methodology, raw CSV, repetitions/spread, machine metadata, profiler evidence, and report; complete D6.

## Sign-off record

| Role | Owner | Interface approval | Date / requested changes |
|---|---|---|---|
| Structure A | Naresh | Pending | Pending |
| Structure B | Harshal | Pending | Pending |
| Architect | Bhuvaneesh | Pending | Pending |
| Oracle | Arush | Pending | Pending |
| Adversary | Anagh | Pending | Pending |
| Analyst | Ranveer | Pending | Pending |

**Do not label the interface frozen v1 until all six owners have reviewed and approved it.** A populated team roster is not evidence of technical sign-off.
