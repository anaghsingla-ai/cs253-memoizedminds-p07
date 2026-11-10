# Reproducible results (not yet collected)

raw/: preserve per-run CSV logs with seed and workload. figures/: plots. scripts/: plotting programs
(to be written by Analyst). machine.md: machine/compiler/cache/profiler metadata (to be completed).
CSV proposed columns:
structure,workload,seed,n,operations,width,repetition,elapsed_seconds,nodes_visited,push_downs,full_blocks,partial_elements,tag_compositions,aggregate_merges

Record >=5 repetitions; report mean plus chosen spread statistic (e.g. sample standard deviation),
not only best time. Preserve query checksum, mode, generator commit and exact workload separately
alongside each CSV. Do not put correctness audits or logging inside timed loops.
No measurements or plots are supplied in this scaffold.
