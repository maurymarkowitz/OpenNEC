#!/usr/bin/env python3
"""
plot_estimate.py — scatter plot of nec_estimate_time() vs real run time.

Usage:
    python3 test/plot_estimate.py [csv_path] [output_png]

Defaults:
    csv_path   = test/estimate_benchmark.csv
    output_png = test/estimate_plot.png

Two series are plotted:
  • sim_ms   — nec_run_simulation() only (what T is supposed to predict)
  • total_ms — full ./onec subprocess (shows the constant-overhead floor)
"""

import sys
import csv
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
            T         = float(row["T"])
            sim_ms    = float(row["sim_ms"])
            total_ms  = float(row["total_ms"])
            ok        = int(row["exit_code"]) == 0
            timed_out = int(row["timed_out"]) == 1
            path      = row["path"]
        except (ValueError, KeyError):
            continue
        rows.append(dict(T=T, sim_ms=sim_ms, total_ms=total_ms,
                         ok=ok, timed_out=timed_out, path=path))

if not rows:
    print(f"No usable rows in {csv_path}")
    sys.exit(1)

print(f"Loaded {len(rows)} rows from {csv_path}")

# ---- helper -----------------------------------------------------------------
def split_series(rows, y_col):
    """Return (T, y) arrays for ok / fail / timeout, filtering T>0 and y>0."""
    ok   = [(r["T"], r[y_col]) for r in rows if r["ok"] and not r["timed_out"]
            and r["T"] > 0 and r[y_col] > 0]
    fail = [(r["T"], r[y_col]) for r in rows if not r["ok"] and not r["timed_out"]
            and r["T"] > 0 and r[y_col] > 0]
    tmo  = [(r["T"], r[y_col]) for r in rows if r["timed_out"] and r["T"] > 0]
    def unzip(pts):
        if not pts: return [], []
        a, b = zip(*pts); return list(a), list(b)
    return unzip(ok), unzip(fail), unzip(tmo)

def power_fit(all_pts):
    if len(all_pts) < 10:
        return None
    log_T = np.log10([p[0] for p in all_pts])
    log_y = np.log10([p[1] for p in all_pts])
    coeffs = np.polyfit(log_T, log_y, 1)
    slope, intercept = coeffs
    resid  = log_y - np.polyval(coeffs, log_T)
    ss_res = np.sum(resid ** 2)
    ss_tot = np.sum((log_y - log_y.mean()) ** 2)
    r2 = 1 - ss_res / ss_tot if ss_tot > 0 else 0
    x_fit = np.logspace(log_T.min() - 0.5, log_T.max() + 0.5, 300)
    y_fit = 10 ** (intercept + slope * np.log10(x_fit))
    return dict(slope=slope, r2=r2, x_fit=x_fit, y_fit=y_fit,
                log_T=log_T, log_y=log_y, coeffs=coeffs)

# ---- two-subplot figure -----------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7),
                                sharex=False, sharey=False)

for ax, y_col, title_suffix, fit_color, ok_color, fail_color in [
    (ax1, "sim_ms",   "sim only  (nec_run_simulation)",  "#0D47A1", "#2196F3", "#EF9A9A"),
    (ax2, "total_ms", "total process  (./onec overhead included)", "#1B5E20", "#4CAF50", "#EF9A9A"),
]:
    (T_ok, y_ok), (T_fail, y_fail), (T_tmo, y_tmo) = split_series(rows, y_col)

    ax.scatter(T_ok,   y_ok,   s=18, alpha=0.70, color=ok_color,
               label=f"OK ({len(T_ok)})",        zorder=3)
    ax.scatter(T_fail, y_fail, s=18, alpha=0.70, color=fail_color,
               label=f"Failed ({len(T_fail)})",  zorder=3)
    if T_tmo:
        ax.scatter(T_tmo, [1]*len(T_tmo), s=18, alpha=0.50, color="#FF9800",
                   label=f"Timeout ({len(T_tmo)})", zorder=3, marker="^")

    # power-law fit on OK + failed (non-timeout) rows with valid values
    all_pts = [(r["T"], r[y_col]) for r in rows
               if not r["timed_out"] and r["T"] > 0 and r[y_col] > 0]
    fit = power_fit(all_pts)
    if fit:
        ax.plot(fit["x_fit"], fit["y_fit"], "--", color=fit_color, lw=1.8,
                label=f"fit  slope={fit['slope']:.2f}  R²={fit['r2']:.3f}")
        label = "sim_ms" if y_col == "sim_ms" else "total_ms"
        print(f"\n[{label}]  slope={fit['slope']:.3f}  R²={fit['r2']:.4f}")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("T  (nec_estimate_time, dimensionless complexity)", fontsize=11)
    ax.set_ylabel("Time  (ms)", fontsize=11)
    ax.set_title(f"T vs {title_suffix}", fontsize=12)
    ax.legend(fontsize=10, loc="upper left")
    ax.grid(True, which="both", alpha=0.25)

fig.suptitle(
    "nec_estimate_time() correlation  •  4nec2 example models  •  log–log scale",
    fontsize=13, y=1.01,
)
plt.tight_layout()
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"\nSaved {out_path}")
