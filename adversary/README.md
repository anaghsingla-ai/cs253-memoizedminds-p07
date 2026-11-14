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
Output is the proposed CS253_V1 replay format. The module-local exporter does
not define the Architect's unfinished `write_workload` function. Omit the final
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

See `docs/derivations/D5.md` for the proof. Record the generator commit with saved
workloads. No benchmark crossover or throughput is claimed by these generators.
