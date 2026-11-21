#!/usr/bin/env bash
# Execute (not just generate) all D5 families at the required full scale.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-$root/build}"
output="${2:-$root/build/validation}"
mkdir -p "$output"
for target in push_downs alternating_negative_gain block_friendly range_width_sweep; do
    for mode in EXACT MODULAR; do
        "$build/adversary_replay_check" "$target" "$mode" 1000000 1000000 253 999 \
            "$output/${target}_${mode}.csv"
    done
done
echo "All eight full-scale D5 replays passed; verification CSVs: $output"
