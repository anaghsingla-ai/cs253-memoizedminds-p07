"""Check final submission paths without inventing individual deliverables."""
import argparse
from pathlib import Path
import subprocess
import pymupdf

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
    parser.add_argument("--report-only", action="store_true", help="Report release blockers without failing a development pipeline")
    args = parser.parse_args()
    def exact_file(name):
        path = ROOT / name
        return path.is_file() and path.name in {p.name for p in path.parent.iterdir()}
    missing = [name for name in REQUIRED if not exact_file(name)]
    for name in REQUIRED:
        print(("MISSING " if name in missing else "OK      ") + name)
    for name in REQUIRED:
        if name.startswith("docs/derivations/") and name not in missing:
            with pymupdf.open(ROOT / name) as pdf:
                pages = len(pdf)
                roll = Path(name).stem.split("_")[1]
                content = "".join(page.get_text() for page in pdf)
                valid = 1 <= pages <= 2 and roll in content
            print(("OK      " if valid else "BLOCKER ") + f"{name}: {pages} pages; roll {'present' if roll in content else 'absent'}")
            if not valid:
                missing.append(name + " content/page requirements")
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
    print(f"Release blockers found: {len(missing)}")
    return bool(missing) and not args.report_only

if __name__ == "__main__":
    raise SystemExit(main())


