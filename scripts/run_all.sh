#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/evaluator_test.py --build-dir build
./build/calibration --all
bash scripts/verify_adversary_scale.sh "$root/build"
trial="$root/build/full-scale-run"
mkdir -p "$trial/results/raw" "$root/build/validation"
(
    cd "$trial"
    "$root/build/full_scale_benchmark" --save-input
)
python3 scripts/verify_benchmark_csv.py \
    "$trial/results/raw/full_scale_width_sweep.csv" \
    --expected results/raw/full_scale_width_sweep.csv
cp "$trial/results/raw/full_scale_width_sweep.csv" build/validation/full_scale_reproduced.csv
python3 scripts/verify_width_inputs.py --build-dir "$root/build" \
    --base-input "$trial/results/raw/full_scale_seed253.txt"
python3 scripts/export_docs.py
python3 scripts/submission_check.py --report-only > build/validation/submission_status.txt
cat build/validation/submission_status.txt
echo "Pipeline passed. Current-machine timings: build/validation/full_scale_reproduced.csv"
echo "The published raw CSV is unchanged. Remaining release blockers are listed above."


