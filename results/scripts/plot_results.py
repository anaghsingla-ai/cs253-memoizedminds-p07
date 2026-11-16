import csv
import math
import os
import sys
import matplotlib.pyplot as plt


def read_csv_data(csv_path):
    data = []
    with open(csv_path, "r", newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            data.append(r)
    return data


def get_mean_and_sd(values):
    n = len(values)
    if n == 0:
        return 0.0, 0.0
    m = sum(values) / n
    if n < 2:
        return m, 0.0
    v = sum((x - m) ** 2 for x in values) / (n - 1)
    return m, math.sqrt(v)


def make_plots(csv_path, out_folder):
    os.makedirs(out_folder, exist_ok=True)
    rows = read_csv_data(csv_path)

    widths = sorted(list(set(int(r["width"]) for r in rows)))

    mean_a, sd_a = [], []
    mean_b, sd_b = [], []
    push_a = []
    blocks_b = []

    for w in widths:
        t_a = [float(r["elapsed_seconds"]) for r in rows if int(r["width"]) == w and "A" in r["structure"]]
        t_b = [float(r["elapsed_seconds"]) for r in rows if int(r["width"]) == w and "B" in r["structure"]]

        ma, sa = get_mean_and_sd(t_a)
        mb, sb = get_mean_and_sd(t_b)
        mean_a.append(ma)
        sd_a.append(sa)
        mean_b.append(mb)
        sd_b.append(sb)

        pa = [int(r["push_downs"]) for r in rows if int(r["width"]) == w and "A" in r["structure"]]
        fb = [int(r["full_blocks"]) + int(r["partial_elements"]) for r in rows if int(r["width"]) == w and "B" in r["structure"]]

        push_a.append(pa[0] if pa else 0)
        blocks_b.append(fb[0] if fb else 0)

    plt.figure(figsize=(8, 5))
    plt.errorbar(widths, mean_a, yerr=sd_a, marker="o", label="Structure A (Lazy Segment Tree)")
    plt.errorbar(widths, mean_b, yerr=sd_b, marker="s", label="Structure B (Sqrt Decomposition)")
    plt.xscale("log")
    plt.xlabel("Range Width (w)")
    plt.ylabel("Elapsed Time (seconds)")
    plt.title("Runtime vs Range Width (Mean of 5 Repetitions)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_folder, "width_sweep_time.png"), dpi=150)
    plt.close()

    plt.figure(figsize=(8, 5))
    plt.plot(widths, push_a, marker="o", label="Structure A: Push-downs")
    plt.plot(widths, blocks_b, marker="s", label="Structure B: Full Blocks + Partial Elements")
    plt.xscale("log")
    plt.yscale("log")
    plt.xlabel("Range Width (w)")
    plt.ylabel("Counter Value (log scale)")
    plt.title("Operation Counters Across Range Widths")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_folder, "counter_comparison.png"), dpi=150)
    plt.close()


if __name__ == "__main__":
    in_csv = sys.argv[1] if len(sys.argv) > 1 else "results/raw/width_sweep.csv"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "results/figures"
    make_plots(in_csv, out_dir)
    print("Plots saved in", out_dir)