# Member 5 - adversarial workload generation

Implements the existing `generate(AdversarialTarget, GeneratorConfig)` API in
`pipeline.hpp`. Shared headers and teammate algorithms are unchanged.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/adversary_generate push_downs 1000000 1000000 253 1 EXACT push.txt
./build/adversary_generate alternating_negative_gain 257 512 253 7 EXACT negative.txt
./build/adversary_generate block_friendly 1000000 1000000 253 1 EXACT blocks.txt
./build/adversary_generate range_width_sweep 1000000 1000000 253 999 MODULAR widths.txt
```

Windows multi-configuration generators may place executables under `build/Release/`.
Output uses the shared CS253_V1 replay format. The module-local exporter keeps
the generator CLI independent of the application library. Omit the final
filename to use stdout. Errors return 2. Numbers must be unsigned decimal.

| Target | Pattern | Purpose |
|---|---|---|
| push_downs | Alternate whole-array resets and two-boundary queries | Maximum pushes on each primed probe; exact 2h-1 for n=2^h |
| alternating_negative_gain | Negations, shifts, resets, overlaps, boundary queries | Extrema swap and noncommuting tag order |
| block_friendly | One automatically aligned block per operation | Candidate B advantage; one full block, zero partial elements |
| range_width_sweep | Eight operations per selected width | Endpoints, shifted ranges, seeded positions, 1 through n scale |

`range_width` must be in [1,n] for nonempty arrays. It controls a probe in the
negative family and is explicitly included in the sweep. The push family uses
derived worst-case boundaries; the block family uses ceil(sqrt(n)) alignment.
With short q the sweep is a prefix: use at least 8 times the number of selected
widths to cover all widths. A full million-operation workload covers them all.
Set `set_sqrt_block_size(0)` before building B for the block-friendly family.

Empty n gives q empty queries. q=0 gives initial data only. Invalid enums,
invalid nonempty widths and unsafe exact operation counts throw before generation.
All exact values and pending offsets fit int64 by the documented bound.

Tests cover 240 seeded target/mode/size combinations, array differential checks
against A and B, invariant audits after each operation, deterministic exported
bytes, exhaustive small-size push maxima, exact repeated power-of-two pushes,
B alignment counters, width coverage and invalid inputs. `adversary_scale`
generates all four targets in both modes at one million elements and operations.
It validates generation only; timings belong to the Analyst. On differential
failure, `adversary_failure.txt` preserves the full replay and stderr records
the seed and operation index. Member 4 retains ownership of shrinking.

See `docs/derivations/D5.md` and `docs/derivations/D5_250001006.pdf` for the proof.
Record the generator commit with saved
workloads. No benchmark crossover or throughput is claimed by these generators.

## Review on 16 November 2026

Owner: Anagh Singla, roll number 250001006. All three Project 7 adversarial
families are implemented; the width sweep is an additional family. The generator
uses ordinary loops, explicit if/else branches, and the existing shared structs.
`maximum_push_range` chooses the probe; `deepest_boundary` follows left children
until two leaves remain. No teammate algorithm is changed.

The C++ tests also check partial **updates**, after re-priming, against the same
`2*h-1` maximum. Generation at one million elements and one million operations is
checked in all four families and both modes. This is generation validation,
not evidence that the whole team's unfinished pipeline runs at that scale.

The final derivation is `docs/derivations/D5_250001006.pdf`; its Markdown source
is `docs/derivations/D5.md`. The optional PDF export script was removed.
Changes must be merged through a pull request approved by another member.
Include the adversarial measurements in the final team report.

## Readable C++ version - 17 November 2026

The public generate() API, four workload families, seeded random draws, range
convention and CS253_V1 output format are retained. Shared headers and teammate
algorithms are not edited.

Read the files in this order:

1. adversary/module.cpp: add_update and add_query build instructions using named
   fields; the block and boundary helpers choose ranges; generate selects the
   family using if/else. add_negative_operation spells out the 16-step cycle.
2. adversary/export.hpp: an ordinary indexed loop writes each update or query.
3. adversary/main.cpp: read arguments, select the target and mode, generate,
   then write to a file or stdout.
4. tests/adversary_test.cpp: the independent reference array, validation,
   differential cases, push maxima, block counters and edge cases.

The shared Operation type still uses std::variant because it is part of the
team interface. The exporter and tests use holds_alternative/get with explicit
Update and Query names rather than pointer-based get_if shorthand. Safe numeric
types and wide reference arithmetic are retained.

check_refactor.sh compiles the previous committed CLI and compares 264 exported
workloads byte for byte with the rewritten CLI (all targets/modes, three seeds,
empty/uneven arrays and short operation counts). It also checks six invalid CLI
cases. Run after building:

    bash adversary/check_refactor.sh "$PWD/build"

The Actions workflow fetches history so the pinned comparison source is available.
New checks cover both modes for aligned-block counters and the early arithmetic
guard. Failure export now reports a write failure instead of always claiming
that a replay file was saved.

AI assistance remains declared in TOOLS.md. The simpler syntax is intended to
help the owner read, explain and maintain the code.
