#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${1:-$root/build}"
baseline="76e81c07d9fb41ae3719f1b4ac407ae3c84293aa"
temporary_directory="$(mktemp -d)"
trap 'rm -rf "$temporary_directory"' EXIT

git -C "$root" show "$baseline:adversary/module.cpp" > "$temporary_directory/module.cpp"
git -C "$root" show "$baseline:adversary/main.cpp" > "$temporary_directory/main.cpp"
git -C "$root" show "$baseline:adversary/export.hpp" > "$temporary_directory/export.hpp"

compiler="${CXX:-c++}"
"$compiler" -std=c++17 -O2 -I"$root/include" \
    "$temporary_directory/module.cpp" "$temporary_directory/main.cpp" \
    -o "$temporary_directory/original_generate"

comparisons=0
compare_case() {
    "$temporary_directory/original_generate" "$@" > "$temporary_directory/original.txt"
    "$build_directory/adversary_generate" "$@" > "$temporary_directory/rewritten.txt"
    if ! cmp -s "$temporary_directory/original.txt" "$temporary_directory/rewritten.txt"; then
        echo "Workload changed for: $*" >&2
        diff -u "$temporary_directory/original.txt" "$temporary_directory/rewritten.txt" >&2 || true
        return 1
    fi
    comparisons=$((comparisons + 1))
}

for target in push_downs alternating_negative_gain block_friendly range_width_sweep; do
    for mode in EXACT MODULAR; do
        for elements in 0 1 2 3 5 16 17 49 50 257; do
            width=7
            if (( elements < width )); then
                width="$elements"
            fi
            for seed in 0 253 18446744073709551615; do
                compare_case "$target" "$elements" 512 "$seed" "$width" "$mode"
            done
        done
        compare_case "$target" 8 0 253 8 "$mode"
        compare_case "$target" 8 1 253 8 "$mode"
        compare_case "$target" 8 2 253 1 "$mode"
    done
done
echo "$comparisons old/new workload comparisons passed"

expect_cli_error() {
    status=0
    "$build_directory/adversary_generate" "$@" \
        > "$temporary_directory/stdout.txt" 2> "$temporary_directory/stderr.txt" || status=$?
    if (( status != 2 )); then
        echo "Expected exit 2 for: $*; got $status" >&2
        return 1
    fi
}

expect_cli_error push_downs -1 2 253 1 EXACT
expect_cli_error push_downs +8 2 253 1 EXACT
expect_cli_error push_downs 8 2 18446744073709551616 1 EXACT
expect_cli_error push_downs 8 2 253 0 EXACT
expect_cli_error unknown 8 2 253 1 EXACT
expect_cli_error push_downs 8 2 253 1 UNKNOWN
echo "6 invalid CLI cases passed"
