import argparse
import re
import subprocess
import sys
from pathlib import Path
from export_width_workload import export_width

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description="Replay a transformed full-scale benchmark input through the evaluator")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--base-input", type=Path, required=True)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    target = build / "validation"
    target.mkdir(parents=True, exist_ok=True)
    workload = target / "width_16384.txt"
    export_width(args.base_input, workload, 16384)
    answers = []
    for structure in ("A", "B"):
        output = target / f"width_16384_{structure}.txt"
        with workload.open("rb") as inp, output.open("wb") as out:
            subprocess.run([sys.executable, str(ROOT / "scripts/run_ops.py"),
                            "--structure", structure, "--build-dir", str(build)],
                           stdin=inp, stdout=out, check=True)
        answers.append(output)
    count = 0
    with answers[0].open() as a, answers[1].open() as b:
        while True:
            left, right = a.readline(), b.readline()
            if left != right:
                raise ValueError(f"Full-scale evaluator mismatch at query {count}")
            if not left:
                break
            if not re.fullmatch(r"16384 [0-9]+ -?[0-9]+ -?[0-9]+\n", left):
                raise ValueError("Invalid deterministic evaluator row")
            count += 1
    if count != 500000:
        raise ValueError(f"Expected 500000 queries, got {count}")
    print("PASS: exact width transformation and 500000 full-scale evaluator answers agree")

if __name__ == "__main__":
    main()

