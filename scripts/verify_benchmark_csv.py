import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path

COUNTERS = ["nodes_visited", "push_downs", "full_blocks", "partial_elements",
            "tag_compositions", "aggregate_merges"]

def load_campaign(path):
    with Path(path).open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"Empty campaign: {path}")
    groups = defaultdict(list)
    for row in rows:
        if row["structure"] not in ("structA", "structB"):
            raise ValueError("Unknown structure")
        if int(row["n"]) != 1000000 or int(row["operations"]) != 1000000:
            raise ValueError("Expected n=q=1000000")
        if int(row["seed"]) != 253 or row["workload"] != "exact_extrema":
            raise ValueError("Unexpected seed or arithmetic mode")
        elapsed = float(row["elapsed_seconds"])
        if not math.isfinite(elapsed) or elapsed <= 0:
            raise ValueError("Invalid timing")
        groups[(row["structure"], int(row["width"]))].append(row)
    widths = {1, 64, 1000, 16384, 262144, 1000000}
    if set(groups) != {(s, w) for s in ("structA", "structB") for w in widths}:
        raise ValueError("Missing structure/width pair")
    for key, group in groups.items():
        if len(group) != 5 or {int(r["repetition"]) for r in group} != set(range(1, 6)):
            raise ValueError(f"Expected five distinct repetitions: {key}")
        for counter in COUNTERS:
            values = {int(r[counter]) for r in group}
            if len(values) != 1 or min(values) < 0:
                raise ValueError(f"Counter changed across repetitions: {key}, {counter}")
    return groups

def main():
    parser = argparse.ArgumentParser(description="Validate a full-scale timing campaign and deterministic counters")
    parser.add_argument("csv", type=Path)
    parser.add_argument("--expected", type=Path)
    args = parser.parse_args()
    groups = load_campaign(args.csv)
    if args.expected:
        expected = load_campaign(args.expected)
        for key in groups:
            for counter in COUNTERS:
                if groups[key][0][counter] != expected[key][0][counter]:
                    raise ValueError(f"Counter mismatch for {key}, {counter}")
    print(f"PASS: 60 full-scale rows, five repetitions per point" +
          ("; all deterministic counters match the committed campaign" if args.expected else ""))

if __name__ == "__main__":
    main()

