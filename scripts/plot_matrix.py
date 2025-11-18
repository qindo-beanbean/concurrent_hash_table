#!/usr/bin/env python3
import os
import glob
import math
import warnings
from typing import List, Tuple, Dict

import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

warnings.filterwarnings("ignore", category=UserWarning)
sns.set(context="paper", style="whitegrid", font_scale=1.1)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

RESULTS_DIR = "../results"
PATTERNS = ["*_matrix.csv", "*matrix.csv"]
OUTDIR = "../results/figs"

EXPECTED_COLS = {
    "impl","mode","mix","dist","threads","ops","bucket_count",
    "read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"
}

# Configuration for headline vs appendix figures
EXCLUDE_FOR_HEADLINE = {"Coarse", "Coarse-Padded"}  # remove coarse variants from main plots
LOG_SCALE_FIGS = True
BROKEN_AXIS_FIG = True
NORMALIZED_FIGS = True
FACET_FIGS = True

def ensure_outdir(path: str):
    os.makedirs(path, exist_ok=True)

def discover_matrix_csvs() -> List[str]:
    valid, invalid = [], []
    for pat in PATTERNS:
        for f in glob.glob(os.path.join(RESULTS_DIR, pat)):
            try:
                head = pd.read_csv(f, nrows=1)
            except Exception:
                invalid.append((f,"read_error")); continue
            if EXPECTED_COLS.issubset(set(head.columns)):
                valid.append(f)
            else:
                invalid.append((f,"missing_cols"))
    if not valid:
        raise SystemExit("No valid matrix CSVs found.")
    if invalid:
        print("[warn] Ignored files:")
        for p,r in invalid: print(f"  {p} ({r})")
    return sorted(valid)

def load_matrix(csvs: List[str]) -> pd.DataFrame:
    frames = []
    for f in csvs:
        df = pd.read_csv(f)
        for c in ["threads","ops","bucket_count"]:
            df[c] = df[c].astype(int)
        for c in ["read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"]:
            df[c] = df[c].astype(float)
        frames.append(df)
    df = pd.concat(frames, ignore_index=True).drop_duplicates()
    return df

def add_avg_chain(df: pd.DataFrame) -> pd.DataFrame:
    elements = df["ops"] * (1.0 - df["read_ratio"] / 2.0)
    df = df.copy()
    df["avg_chain"] = elements / df["bucket_count"]
    return df

def pick_top_buckets(df: pd.DataFrame) -> Tuple[int, int]:
    buckets = sorted(df["bucket_count"].unique())
    hi = buckets[-1]
    mid = buckets[-2] if len(buckets) > 1 else hi
    return hi, mid

def pick_skew_ph(df: pd.DataFrame) -> float:
    vals = sorted(set(df.loc[df["dist"]=="skew","p_hot"].round(2)))
    for target in [0.90,0.99,0.70]:
        if target in vals: return target
    return float(np.median(vals)) if vals else 0.90

def lineplot(df, x, y, hue, title, fname, xlabel=None, ylabel=None, logy=False):
    if df.empty: return
    plt.figure(figsize=(6.3,3.9))
    ax = sns.lineplot(data=df, x=x, y=y, hue=hue, marker="o")
    ax.set_title(title)
    ax.set_xlabel(xlabel or x)
    ax.set_ylabel(ylabel or y)
    if logy:
        ax.set_yscale("log")
    if ax.legend_: ax.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def lineplot_broken_axis(df, x, y, hue, title, fname, lower_max, upper_min):
    """
    Create a broken y-axis figure to show low (coarse) and high (fine/lock-free) ranges distinctly.
    lower_max: upper bound of lower subplot
    upper_min: lower bound of upper subplot
    """
    if df.empty: return
    impls = sorted(df[hue].unique())
    fig, (ax_low, ax_high) = plt.subplots(2,1,sharex=True, figsize=(6.3,4.8), gridspec_kw={'height_ratios':[1,2]})
    sns.lineplot(data=df, x=x, y=y, hue=hue, marker="o", ax=ax_low, legend=False)
    sns.lineplot(data=df, x=x, y=y, hue=hue, marker="o", ax=ax_high)
    ax_low.set_ylim(0, lower_max)
    ax_high.set_ylim(upper_min, df[y].max()*1.05)
    ax_low.spines['bottom'].set_visible(False)
    ax_high.spines['top'].set_visible(False)
    ax_low.tick_params(labelbottom=False)
    d = .5
    kwargs = dict(transform=ax_low.transAxes, color='k', clip_on=False)
    ax_low.plot((-d,+d),(-0.02,-0.02), **kwargs)
    ax_low.plot((1-d,1+d),(-0.02,-0.02), **kwargs)
    kwargs.update(transform=ax_high.transAxes)
    ax_high.plot((-d,+d),(0,0), **kwargs)
    ax_high.plot((1-d,1+d),(0,0), **kwargs)
    ax_high.set_title(title)
    ax_high.set_xlabel("Threads")
    ax_high.set_ylabel("Throughput (Mops/s)")
    ax_high.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def facet_plot(df, fname, title):
    if df.empty: return
    g = sns.FacetGrid(df, col="impl", col_wrap=3, sharey=False, height=3.0)
    g.map_dataframe(sns.lineplot, x="threads", y="throughput_mops", marker="o")
    g.set_axis_labels("Threads", "Mops/s")
    g.fig.subplots_adjust(top=0.87)
    g.fig.suptitle(title)
    plt.savefig(fname)
    plt.close()

def normalized_plot(df, fname, title):
    if df.empty: return
    # Normalize throughput to max per thread across impls
    df = df.copy()
    max_per_thread = df.groupby("threads")["throughput_mops"].transform("max")
    df["norm_throughput"] = df["throughput_mops"] / max_per_thread
    plt.figure(figsize=(6.3,3.9))
    ax = sns.lineplot(data=df, x="threads", y="norm_throughput", hue="impl", marker="o")
    ax.set_title(title)
    ax.set_xlabel("Threads")
    ax.set_ylabel("Normalized throughput (fraction of per-thread max)")
    ax.set_ylim(0,1.05)
    if ax.legend_: ax.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def efficiency_plot(df, fname, title):
    if df.empty: return
    # Scaling efficiency = speedup / threads
    df = df.copy()
    df["efficiency"] = df["speedup"] / df["threads"]
    plt.figure(figsize=(6.3,3.9))
    ax = sns.lineplot(data=df, x="threads", y="efficiency", hue="impl", marker="o")
    ax.set_title(title)
    ax.set_xlabel("Threads")
    ax.set_ylabel("Scaling efficiency (speedup / threads)")
    ax.set_ylim(0,1.1)
    if ax.legend_: ax.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def main():
    print("[info] Loading matrix CSVs.")
    csvs = discover_matrix_csvs()
    df = load_matrix(csvs)
    df = add_avg_chain(df)

    ensure_outdir(OUTDIR)
    mix80 = "80/20" if "80/20" in df["mix"].unique() else sorted(df["mix"].unique())[0]
    hiB, midB = pick_top_buckets(df)
    skew_ph = pick_skew_ph(df)

    # Strong scaling uniform (FULL including coarse)
    strong_full = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==hiB)]
    lineplot(strong_full, "threads", "throughput_mops", "impl",
             f"Strong scaling (uniform, {mix80}, B={hiB:,})",
             os.path.join(OUTDIR,"fig1_strong_uniform_all.png"))

    # Headline variant excluding coarse
    strong_headline = strong_full[~strong_full["impl"].isin(EXCLUDE_FOR_HEADLINE)]
    lineplot(strong_headline, "threads", "throughput_mops", "impl",
             f"Strong scaling (uniform, {mix80}, B={hiB:,}) [no coarse]",
             os.path.join(OUTDIR,"fig1_strong_uniform_headline.png"))

    # Optional log-scale
    if LOG_SCALE_FIGS:
        lineplot(strong_full, "threads", "throughput_mops", "impl",
                 f"Strong scaling (uniform, log y) B={hiB:,}",
                 os.path.join(OUTDIR,"fig1_strong_uniform_log.png"),
                 logy=True)

    # Optional broken axis (choose thresholds heuristically)
    if BROKEN_AXIS_FIG and not strong_full.empty:
        low_max = strong_full["throughput_mops"].quantile(0.25) * 1.2
        high_min = strong_full["throughput_mops"].median() * 0.9
        lineplot_broken_axis(strong_full, "threads", "throughput_mops", "impl",
                             f"Strong scaling broken axis (uniform, {mix80}, B={hiB:,})",
                             os.path.join(OUTDIR,"fig1_strong_uniform_broken.png"),
                             lower_max=low_max, upper_min=high_min)

    # Skew strong scaling (full + headline)
    skew_full = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="skew") &
                   (df["bucket_count"]==hiB) & (np.isclose(df["p_hot"], skew_ph))]
    lineplot(skew_full, "threads", "throughput_mops", "impl",
             f"Strong scaling (skew p_hot={skew_ph:.2f}, B={hiB:,})",
             os.path.join(OUTDIR,"fig2_strong_skew_all.png"))
    skew_headline = skew_full[~skew_full["impl"].isin(EXCLUDE_FOR_HEADLINE)]
    lineplot(skew_headline, "threads", "throughput_mops", "impl",
             f"Strong scaling (skew p_hot={skew_ph:.2f}, B={hiB:,}) [no coarse]",
             os.path.join(OUTDIR,"fig2_strong_skew_headline.png"))
    if LOG_SCALE_FIGS:
        lineplot(skew_full, "threads", "throughput_mops", "impl",
                 f"Strong scaling skew (log y) p_hot={skew_ph:.2f}",
                 os.path.join(OUTDIR,"fig2_strong_skew_log.png"),
                 logy=True)

    # Weak scaling uniform
    weak_full = df[(df["mode"]=="weak") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==hiB)]
    lineplot(weak_full, "threads", "throughput_mops", "impl",
             f"Weak scaling (uniform, {mix80}, B={hiB:,})",
             os.path.join(OUTDIR,"fig3_weak_uniform_all.png"))
    weak_headline = weak_full[~weak_full["impl"].isin(EXCLUDE_FOR_HEADLINE)]
    lineplot(weak_headline, "threads", "throughput_mops", "impl",
             f"Weak scaling (uniform, {mix80}, B={hiB:,}) [no coarse]",
             os.path.join(OUTDIR,"fig3_weak_uniform_headline.png"))
    if LOG_SCALE_FIGS:
        lineplot(weak_full, "threads", "throughput_mops", "impl",
                 f"Weak scaling (uniform, log y) B={hiB:,}",
                 os.path.join(OUTDIR,"fig3_weak_uniform_log.png"),
                 logy=True)

    # Efficiency plots (include Coarse to show scaling inefficiency starkly)
    efficiency_plot(strong_full, os.path.join(OUTDIR,"fig_efficiency_strong_uniform.png"),
                    f"Scaling efficiency (uniform, B={hiB:,})")
    efficiency_plot(skew_full, os.path.join(OUTDIR,"fig_efficiency_strong_skew.png"),
                    f"Scaling efficiency (skew p_hot={skew_ph:.2f}, B={hiB:,})")

    # Normalized throughput (show relative position without axis compression)
    if NORMALIZED_FIGS:
        normalized_plot(strong_full, os.path.join(OUTDIR,"fig_norm_strong_uniform.png"),
                        f"Normalized strong scaling (uniform, B={hiB:,})")
        normalized_plot(skew_full, os.path.join(OUTDIR,"fig_norm_strong_skew.png"),
                        f"Normalized strong scaling (skew p_hot={skew_ph:.2f})")

    # Facet small multiples (each impl its own scale)
    if FACET_FIGS:
        facet_plot(strong_full, os.path.join(OUTDIR,"fig_facet_strong_uniform.png"),
                   f"Strong scaling facets (uniform, B={hiB:,})")
        facet_plot(skew_full, os.path.join(OUTDIR,"fig_facet_strong_skew.png"),
                   f"Strong scaling facets (skew p_hot={skew_ph:.2f}, B={hiB:,})")

    # Speedup plot (already normalized per config baseline)
    lineplot(strong_full, "threads", "speedup", "impl",
             f"Speedup vs threads (uniform, B={hiB:,})",
             os.path.join(OUTDIR,"fig_speedup_strong_uniform_all.png"))
    lineplot(strong_headline, "threads", "speedup", "impl",
             f"Speedup vs threads (uniform, B={hiB:,}) [no coarse]",
             os.path.join(OUTDIR,"fig_speedup_strong_uniform_headline.png"))

    print(f"[done] Figures written to {OUTDIR}/")

if __name__ == "__main__":
    main()