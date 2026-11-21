"""Check final submission paths without inventing individual deliverables."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "README.md", "REPORT.md", "REPORT.pdf", "INTERFACE.md", "TOOLS.md",
    "CMakeLists.txt", "scripts/run_all.sh", "docs/slides/slides.pdf",
    "docs/derivations/D1_250001052.pdf",
    "docs/derivations/D2_250001032.pdf",
    "docs/derivations/D3_250001017.pdf",
    "docs/derivations/D4_250001008.pdf",
    "docs/derivations/D5_250001006.pdf",
    "docs/derivations/D6_250041031.pdf",
]
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-final-tag", action="store_true")
    args = parser.parse_args()
    missing = [name for name in REQUIRED if not (ROOT / name).is_file()]
    for name in REQUIRED:
        print(("MISSING " if name in missing else "OK      ") + name)
    for folder, pattern in [("results/raw", "*.csv"), ("results/figures", "*")]:
        entries = [p for p in (ROOT / folder).glob(pattern) if p.is_file() and not p.name.startswith(".")]
        if not entries:
            missing.append(folder)
        print(("OK      " if entries else "MISSING ") + folder)
    if args.require_final_tag:
        result = subprocess.run(["git", "cat-file", "-t", "refs/tags/final"], cwd=ROOT,
                                text=True, capture_output=True)
        annotated = result.returncode == 0 and result.stdout.strip() == "tag"
        print(("OK      " if annotated else "MISSING ") + "annotated final tag")
        if not annotated:
            missing.append("annotated final tag")
    print("Manual review: report content, official evaluator format, five-run measurements,")
    print("prediction history, instructor access, member attribution and approved PRs.")
    return bool(missing)

if __name__ == "__main__":
    raise SystemExit(main())

