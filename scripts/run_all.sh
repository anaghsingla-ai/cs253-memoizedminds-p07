#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/evaluator_test.py --build-dir build
./build/calibration --all
bash scripts/verify_adversary_scale.sh "$PWD/build"
# A fresh, explicitly identified timing campaign; retain the historical CSV.
./build/adversary_generate range_width_sweep 100000 100000 253 317 EXACT build/benchmark_input.txt
printf '1\n%s\n6\n%s\n0\n' "$root/build/benchmark_input.txt" \
    "$root/results/raw/reproduced_width_sweep.csv" > build/benchmark_menu.txt
./build/calibration --menu < build/benchmark_menu.txt > build/benchmark_menu.log
python3 scripts/export_docs.py
echo "Pipeline passed. New timings: results/raw/reproduced_width_sweep.csv"
echo "Run python3 scripts/submission_check.py to list remaining release artifacts."

