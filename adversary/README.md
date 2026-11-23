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

## Source files and compatibility

- adversary/module.cpp constructs the four workload families and their ranges.
- adversary/export.hpp writes CS253_V1 workloads.
- adversary/main.cpp parses arguments and invokes generation/export.
- adversary/replay_check.cpp executes full-scale A/B checks and counter bounds.
- tests/adversary_test.cpp supplies independent-array and edge-case checks.

The shared Operation type uses std::variant. The generator, exporter and tests
use explicit Update/Query branches. Before changing the generator, run:

    bash adversary/check_refactor.sh "$PWD/build"

This compares 264 workloads byte for byte against the pinned earlier CLI and
checks six invalid CLI cases. The Actions job fetches history for that comparison.
AI assistance is declared in TOOLS.md.

## Final submission verification - 21 November 2026

The final artifact path is docs/derivations/D5_250001006.pdf (uppercase D).
Its source is D5.md; scripts/export_docs.py regenerates it.

The original --scale test checks generation. To execute all four families on
both structures at n=q=1000000, run after building:

    bash scripts/verify_adversary_scale.sh "$PWD/build"

adversary_replay_check compares every query, checks final invariants and writes
CSV counter evidence. It independently checks uniform-reset answers and the
exact maximum M(n)=1+ceil(log2(ceil(n/2)))+ceil(log2(floor(n/2))) for n>=2.
M(0)=M(1)=0. At n=1000000, M(n)=39 and 500000 primed probes yield 19500000 pushes.
The block-friendly replay must have q full blocks and zero partial elements.

The committed evidence in results/validation comes from CI, not the timing
machine. It does not replace the Analyst's five-repeat measurements.
Generation outputs are unchanged; 264 old/new outputs still match byte for byte.

