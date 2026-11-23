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


# Simple 2-variable least squares fit: y = c1 * x1 + c2 * x2
def fit_two_constants(x1_list, x2_list, y_list):
    s11 = sum(a * a for a in x1_list)
    s22 = sum(b * b for b in x2_list)
    s12 = sum(a * b for a, b in zip(x1_list, x2_list))
    s1y = sum(a * y for a, y in zip(x1_list, y_list))
    s2y = sum(b * y for b, y in zip(x2_list, y_list))

    det = s11 * s22 - s12 * s12
    if abs(det) < 1e-30:
        return 0.0, 0.0
    c1 = (s22 * s1y - s12 * s2y) / det
    c2 = (s11 * s2y - s12 * s1y) / det
    return c1, c2


def make_plots(csv_path, out_folder):
    os.makedirs(out_folder, exist_ok=True)
    rows = read_csv_data(csv_path)
    widths = sorted(list(set(int(r["width"]) for r in rows)))

    mean_a, sd_a = [], []
    mean_b, sd_b = [], []
    visits_a, push_a = [], []
    full_b, part_b = [], []

    for w in widths:
        ra = [r for r in rows if int(r["width"]) == w and "A" in r["structure"]]
        rb = [r for r in rows if int(r["width"]) == w and "B" in r["structure"]]

        ma, sa = get_mean_and_sd([float(r["elapsed_seconds"]) for r in ra])
        mb, sb = get_mean_and_sd([float(r["elapsed_seconds"]) for r in rb])
        mean_a.append(ma)
        sd_a.append(sa)
        mean_b.append(mb)
        sd_b.append(sb)

        visits_a.append(int(ra[0]["nodes_visited"]))
        push_a.append(int(ra[0]["push_downs"]))
        full_b.append(int(rb[0]["full_blocks"]))
        part_b.append(int(rb[0]["partial_elements"]))

    # Fit cost models: T_A = a_visit * V_A + a_push * P_A, T_B = b_full * F_B + b_elem * E_B
    a_visit, a_push = fit_two_constants(visits_a, push_a, mean_a)
    b_full, b_elem = fit_two_constants(full_b, part_b, mean_b)

    pred_a = [a_visit * v + a_push * p for v, p in zip(visits_a, push_a)]
    pred_b = [b_full * f + b_elem * e for f, e in zip(full_b, part_b)]

    print(f"Fitted A constants: a_visit = {a_visit * 1e9:.2f} ns/node, a_push = {a_push * 1e9:.2f} ns/push")
    print(f"Fitted B constants: b_full  = {b_full * 1e9:.2f} ns/block, b_elem = {b_elem * 1e9:.2f} ns/elem")
    print("\nWidth | A_mean (sd) | A_pred | B_mean (sd) | B_pred")
    for i, w in enumerate(widths):
        print(f"{w:7d} | {mean_a[i]:.4f} ({sd_a[i]:.4f}) | {pred_a[i]:.4f} | {mean_b[i]:.4f} ({sd_b[i]:.4f}) | {pred_b[i]:.4f}")

    # Plot 1: Measured Runtime vs Range Width
    plt.figure(figsize=(8, 5))
    plt.errorbar(widths, mean_a, yerr=sd_a, marker="o", label="Structure A (Lazy Segment Tree)")
    plt.errorbar(widths, mean_b, yerr=sd_b, marker="s", label="Structure B (Sqrt Decomposition)")
    plt.xscale("log")
    plt.xlabel("Range Width (w)")
    plt.ylabel("Elapsed Time (seconds)")
    plt.title("Full-Scale Runtime vs Range Width (n=1M, q=1M, 5 reps)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_folder, "full_scale_time.png"), dpi=150)
    plt.close()

    # Plot 2: Push-downs vs Block/Element Touches
    plt.figure(figsize=(8, 5))
    plt.plot(widths, [max(1, p) for p in push_a], marker="o", label="Structure A: Push-downs")
    plt.plot(widths, [max(1, f + e) for f, e in zip(full_b, part_b)], marker="s", label="Structure B: Full Blocks + Partial Elements")
    plt.xscale("log")
    plt.yscale("log")
    plt.xlabel("Range Width (w)")
    plt.ylabel("Counter Value (log scale)")
    plt.title("Full-Scale Operation Counters (n=1M, q=1M)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_folder, "full_scale_counters.png"), dpi=150)
    plt.close()

    # Plot 3: Predicted vs Measured Runtime
    plt.figure(figsize=(8, 5))
    plt.plot(widths, mean_a, "o-", label="A Measured")
    plt.plot(widths, pred_a, "o--", label="A Predicted")
    plt.plot(widths, mean_b, "s-", label="B Measured")
    plt.plot(widths, pred_b, "s--", label="B Predicted")
    plt.xscale("log")
    plt.xlabel("Range Width (w)")
    plt.ylabel("Elapsed Time (seconds)")
    plt.title("Predicted vs Measured Runtime (Cost Model Fit)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_folder, "predicted_vs_measured.png"), dpi=150)
    plt.close()


if __name__ == "__main__":
    in_csv = sys.argv[1] if len(sys.argv) > 1 else "results/raw/full_scale_width_sweep.csv"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "results/figures"
    make_plots(in_csv, out_dir)
    print("Plots saved in", out_dir)
