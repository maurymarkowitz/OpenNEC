#!/usr/bin/env python3
"""
plot_estimate.py — scatter plot of nec_estimate_time() vs real run time.

Usage:
    python3 test/plot_estimate.py [csv_path] [output_png]

Defaults:
    csv_path   = test/estimate_benchmark.csv
    output_png = test/estimate_plot.png
"""

import sys
import csv
import math
import pathlib

try:
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("pip install matplotlib numpy  — then re-run.")
    sys.exit(1)

# ---- load CSV ---------------------------------------------------------------
csv_path = sys.argv[1] if len(sys.argv) > 1 else "test/estimate_benchmark.csv"
out_path = sys.argv[2] if len(sys.argv) > 2 else "test/estimate_plot.png"

rows = []
with open(csv_path, newline="") as f:
    for row in csv.DictReader(f):
        try:
            T        = float(row["T"])
            elapsed  = float(row["elapsed_ms"])
            ok       = int(row["exit_code"]) == 0
            timed_out = int(row["timed_out"]) == 1
            path     = row["path"]
        except (ValueError, KeyError):
            continue
        rows.append((T, elapsed, ok, timed_out, path))

if not rows:
    print(f"No usable rows in {csv_path}")
    sys.exit(1)

print(f"Loaded {len(rows)} rows from {csv_path}")

# ---- split into buckets -----------------------------------------------------
def bucket(rows, pred):
    return [(r[0], r[1]) for r in rows if pred(r)]

pts_ok   = bucket(rows, lambda r: r[2] and not r[3] and r[0] > 0)
pts_fail = bucket(rows, lambda r: not r[2] and not r[3] and r[0] > 0)
pts_tmo  = bucket(rows, lambda r: r[3] and r[0] > 0)

def unzip(pts):
    if not pts:
        return [], []
    T, e = zip(*pts)
    return list(T), list(e)

T_ok,   e_ok   = unzip(pts_ok)
T_fail, e_fail = unzip(pts_fail)
T_tmo,  e_tmo  = unzip(pts_tmo)

# ---- plot -------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(11, 7))

ax.scatter(T_ok,   e_ok,   s=18, alpha=0.70, color="#2196F3",
           label=f"OK ({len(T_ok)})",       zorder=3)
ax.scatter(T_fail, e_fail, s=18, alpha=0.70, color="#F44336",
           label=f"Failed ({len(T_fail)})", zorder=3)
ax.scatter(T_tmo,  e_tmo,  s=18, alpha=0.70, color="#FF9800",
           label=f"Timeout ({len(T_tmo)})", zorder=3)

# ---- power-law fit on all non-timeout points --------------------------------
all_pts = [(r[0], r[1]) for r in rows if not r[3] and r[0] > 0 and r[1] > 0]
if len(all_pts) >= 10:
    log_T = np.log10([p[0] for p in all_pts])
    log_e = np.log10([p[1] for p in all_pts])
    coeffs = np.polyfit(log_T, log_e, 1)
    slope, intercept = coeffs

    x_fit = np.logspace(log_T.min() - 0.5, log_T.max() + 0.5, 300)
    y_fit = 10 ** (intercept + slope * np.log10(x_fit))
    ax.plot(x_fit, y_fit, "--", color="#444", lw=1.5,
            label=f"power-law fit  slope = {slope:.2f}")

    print(f"\nPower-law fit (non-timeout): elapsed ∝ T^{slope:.3f}")
    print(f"  (slope=1 → linear, slope=3 → cubic — expect ~1 since T² "
          f"already captures cubic scaling)")

# ---- labels -----------------------------------------------------------------
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("T  (nec_estimate_time, dimensionless complexity)", fontsize=12)
ax.set_ylabel("Actual elapsed time  (ms)", fontsize=12)
ax.set_title(
    "nec_estimate_time() correlation with actual run time\n"
    "4nec2 example models  •  log–log scale",
    fontsize=13,
)
ax.legend(fontsize=11, loc="upper left")
ax.grid(True, which="both", alpha=0.25)

# ---- Pearson r² in log space ------------------------------------------------
if len(all_pts) >= 10:
    resid  = log_e - np.polyval(coeffs, log_T)
    ss_res = np.sum(resid ** 2)
    ss_tot = np.sum((log_e - log_e.mean()) ** 2)
    r2 = 1 - ss_res / ss_tot if ss_tot > 0 else 0
    ax.text(0.97, 0.04, f"R² = {r2:.3f}", transform=ax.transAxes,
            ha="right", fontsize=11, color="#444")
    print(f"  R² (log-log) = {r2:.4f}")

plt.tight_layout()
plt.savefig(out_path, dpi=150)
print(f"\nSaved {out_path}")
plt.show()
