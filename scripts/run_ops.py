"""Deterministic stdin/stdout adapter for the repository CS253_V1 contract."""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ANSWER = re.compile(r"^\s*Q \d+ LEN (\d+) SUM (\d+) MIN (-?\d+|NA) MAX (-?\d+|NA)\s*$")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--structure", choices=["A", "B", "oracle"], required=True)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    args = parser.parse_args()
    name = "calibration.exe" if sys.platform == "win32" else "calibration"
    program = args.build_dir.resolve() / name
    if not program.is_file():
        parser.error(f"Build the project first; executable missing: {program}")
    # The existing app takes a path. Stream stdin to disk and app output to stdout;
    # neither the adapter nor the pipe retains all query results.
    with tempfile.TemporaryDirectory(prefix="cs253-ops-") as folder:
        workload = Path(folder) / "ops.txt"
        with workload.open("wb") as output:
            while True:
                data = sys.stdin.buffer.read(1024 * 1024)
                if not data:
                    break
                output.write(data)
        with subprocess.Popen([str(program), "--run", str(workload),
                               "--structure", args.structure],
                              stdout=subprocess.PIPE, stderr=None, text=True) as process:
            for line in process.stdout:
                match = ANSWER.fullmatch(line)
                if match:
                    print(" ".join(match.groups()))
            return process.wait()

if __name__ == "__main__":
    raise SystemExit(main())
