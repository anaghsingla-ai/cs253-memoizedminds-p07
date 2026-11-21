"""Hidden-test-style checks of stdin/stdout, parsing and arithmetic."""
import argparse
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
P = 998244353

def run(text, structure, build):
    return subprocess.run([sys.executable, str(ROOT / "scripts/run_ops.py"),
                           "--build-dir", str(build), "--structure", structure],
                          input=text, capture_output=True, text=True, timeout=60)

def case(n, mode):
    rng = random.Random(253 + n)
    values = [rng.randint(-1000, 1000) for _ in range(n)]
    initial = values[:]
    ops, expected = [], []
    for i in range(511):
        left = rng.randrange(n + 1)
        right = rng.randrange(left, n + 1)
        if i % 3 == 0:
            total = sum(values[left:right]) % P
            lo = str(min(values[left:right])) if left < right and mode == "EXACT" else "NA"
            hi = str(max(values[left:right])) if left < right and mode == "EXACT" else "NA"
            expected.append(f"{right-left} {total} {lo} {hi}\n")
            ops.append(f"Q {left} {right}")
        else:
            if mode == "EXACT":
                a, b = rng.choice([-1, 0, 1]), rng.randint(-31, 31)
            else:
                a = rng.choice([-(1 << 63), (1 << 63)-1, -1, 0, 1, 998244353])
                b = rng.randint(-(1 << 63), (1 << 63)-1)
            ops.append(f"U {left} {right} {a} {b}")
            for j in range(left, right):
                values[j] = a * values[j] + b
                if mode == "MODULAR":
                    values[j] %= P
    text = f"CS253_V1\n{mode}\n253\n{n} {len(ops)}\n"
    text += " ".join(map(str, initial)) + "\n" + "\n".join(ops) + "\n"
    return text, "".join(expected)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    args = parser.parse_args()
    count = 0
    for n in [0, 1, 17, 50, 257]:
        for mode in ["EXACT", "MODULAR"]:
            text, expected = case(n, mode)
            for structure in ["A", "B", "oracle"]:
                result = run(text, structure, args.build_dir)
                if result.returncode or result.stdout != expected:
                    raise AssertionError(f"{structure} {mode} n={n}: {result.stderr}\n{result.stdout[:200]}")
                count += 1
    invalid = [
        "CS253_V1 EXACT 253 1 1 7 Q 0 2",
        "CS253_V1 EXACT 253 1 1 7 Q 1 0",
        "CS253_V1 EXACT 253 1 1 7 U 0 1 2 0",
        "CS253_V1 EXACT 253 1 0 7 trailing",
        "CS253_V1 MODULAR 253 1 1 7 Q -1 0",
        "CS253_V1 MODULAR 253 1 1 7 U 0 1 9223372036854775808 0",
    ]
    for text in invalid:
        for structure in ["A", "B"]:
            result = run(text, structure, args.build_dir)
            assert result.returncode != 0, (structure, text)
    print(f"PASS: {count} evaluator/reference cases and 12 malformed-input rejections")

if __name__ == "__main__":
    main()
