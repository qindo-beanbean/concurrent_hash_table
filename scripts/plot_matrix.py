#!/usr/bin/env python3
import os
import glob
import math
import warnings
from typing import List, Set, Tuple, Dict

import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

warnings.filterwarnings("ignore", category=UserWarning)
sns.set(context="paper", style="whitegrid", font_scale=1.1)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

# Directory containing all matrix CSVs (place your coarse_matrix.csv, fine_matrix.csv etc. here).
RESULTS_DIR = "../results"
# Glob patterns (relative to RESULTS_DIR) for matrix CSVs.
PATTERNS = ["*_matrix.csv", "*matrix.csv"]

EXPECTED_COLS = {
    "impl","mode","mix","dist","threads","ops","bucket_count",
    "read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"
}

OUTDIR = "../results/figs"

def ensure_outdir(path: str):
    os.makedirs(path, exist_ok=True)

def discover_matrix_csvs() -> List[str]:
    valid = []
    invalid = []
    for pat in PATTERNS:
        full_pat = os.path.join(RESULTS_DIR, pat)
        for f in glob.glob(full_pat):
            try:
                head = pd.read_csv(f, nrows=1)
            except Exception:
                invalid.append((f, "read_error"))
                continue
            cols = set(map(str, head.columns))
            if EXPECTED_COLS.issubset(cols):
                valid.append(f)
            else:
                invalid.append((f, f"missing cols: {EXPECTED_COLS - cols}"))
    if not valid:
        msg = "No valid matrix CSVs found in 'results/'.\n"
        if invalid:
            msg += "Checked files with issues:\n" + "\n".join([f"{p} -> {reason}" for p, reason in invalid])
        raise SystemExit(msg)
    if invalid:
        print("[warn] Ignored files:")
        for p, r in invalid:
            print(f"  {p} ({r})")
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
    elements = df["ops"] * (1.0 - df["read_ratio"] / 2.0)  # N*(1 - r/2)
    df = df.copy()
    df["avg_chain"] = elements / df["bucket_count"]
    return df

def pick_top_buckets(df: pd.DataFrame) -> Tuple[int, int]:
    buckets = sorted(df["bucket_count"].unique())
    hi = buckets[-1]
    mid = buckets[-2] if len(buckets) > 1 else hi
    return hi, mid

def pick_threads(df: pd.DataFrame) -> Tuple[int, int, List[int]]:
    ts = sorted(df["threads"].unique())
    t_max = ts[-1]
    t_mid = ts[-2] if len(ts) > 1 else t_max
    return t_max, t_mid, ts

def pick_skew_phot(df: pd.DataFrame) -> float:
    vals = sorted(set([round(float(x),2) for x in df.loc[df["p_hot"] > 0,"p_hot"].unique()]))
    for pref in [0.90, 0.99, 0.70]:
        if any(abs(v - pref) < 1e-6 for v in vals):
            return pref
    return float(np.median(vals)) if vals else 0.90

def lineplot(df, x, y, hue, title, fname, xlabel=None, ylabel=None):
    if df.empty: return
    plt.figure(figsize=(6.3,3.9))
    ax = sns.lineplot(data=df, x=x, y=y, hue=hue, marker="o")
    ax.set_title(title)
    ax.set_xlabel(xlabel or x)
    ax.set_ylabel(ylabel or y)
    if ax.legend_: ax.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def barplot(df, x, y, hue, title, fname, xlabel=None, ylabel=None):
    if df.empty: return
    plt.figure(figsize=(6.3,3.9))
    ax = sns.barplot(data=df, x=x, y=y, hue=hue)
    ax.set_title(title)
    ax.set_xlabel(xlabel or x)
    ax.set_ylabel(ylabel or y)
    if hue: ax.legend(loc="best", fontsize="small")
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def heatmap(df, rows, cols, value, title, fname):
    if df.empty: return
    pivoted = df.pivot_table(index=rows, columns=cols, values=value, aggfunc="mean")
    plt.figure(figsize=(6.3,4.6))
    ax = sns.heatmap(pivoted, cmap="YlGnBu", cbar_kws={"label":value})
    ax.set_title(title)
    plt.tight_layout()
    plt.savefig(fname)
    plt.close()

def plot_all(df: pd.DataFrame, outdir: str):
    ensure_outdir(outdir)
    mix80 = "80/20" if "80/20" in df["mix"].unique() else sorted(df["mix"].unique())[0]
    mix50 = "50/50" if "50/50" in df["mix"].unique() else None
    hiB, midB = pick_top_buckets(df)
    t_max, t_mid, threads_all = pick_threads(df)
    skew_ph = pick_skew_phot(df)
    impls = sorted(df["impl"].unique())

    # Strong scaling uniform
    f1 = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==hiB)]
    lineplot(f1, "threads", "throughput_mops", "impl",
             f"Strong scaling (uniform, {mix80}, B={hiB:,})",
             os.path.join(outdir,"fig1_strong_uniform.png"),
             "Threads","Throughput (Mops/s)")

    # Strong scaling skew
    f2 = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="skew") & (df["bucket_count"]==hiB) & (np.isclose(df["p_hot"], skew_ph))]
    lineplot(f2, "threads", "throughput_mops", "impl",
             f"Strong scaling (skew p_hot={skew_ph:.2f}, {mix80}, B={hiB:,})",
             os.path.join(outdir,"fig2_strong_skew.png"),
             "Threads","Throughput (Mops/s)")

    # Weak scaling uniform
    f3 = df[(df["mode"]=="weak") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==hiB)]
    lineplot(f3, "threads", "throughput_mops", "impl",
             f"Weak scaling (uniform, {mix80}, B={hiB:,})",
             os.path.join(outdir,"fig3_weak_uniform.png"),
             "Threads","Throughput (Mops/s)")

    # Load factor uniform (T=8,16 if present)
    for T in [8,16]:
        if T in df["threads"].unique():
            lu = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["threads"]==T)].sort_values("avg_chain")
            lineplot(lu, "avg_chain", "throughput_mops", "impl",
                     f"Load factor (uniform, {mix80}, T={T})",
                     os.path.join(outdir,f"fig4_loadfactor_uniform_T{T}.png"),
                     "Average chain length","Throughput (Mops/s)")

    # Load factor skew
    for T in [8,16]:
        if T in df["threads"].unique():
            ls = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="skew") & (np.isclose(df["p_hot"], skew_ph)) & (df["threads"]==T)].sort_values("avg_chain")
            if not ls.empty:
                lineplot(ls, "avg_chain", "throughput_mops", "impl",
                         f"Load factor (skew p_hot={skew_ph:.2f}, {mix80}, T={T})",
                         os.path.join(outdir,f"fig4b_loadfactor_skew_T{T}.png"),
                         "Average chain length","Throughput (Mops/s)")

    # Skew sensitivity curves
    for B in [midB, hiB]:
        for T in [t_mid, t_max]:
            if T not in df["threads"].unique(): continue
            sc = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="skew") &
                    (df["bucket_count"]==B) & (df["threads"]==T)]
            std_ph = [ph for ph in [0.7,0.9,0.99] if any(np.isclose(sc["p_hot"], ph))]
            if std_ph:
                sc = sc[np.isclose(sc["p_hot"].values[:,None], np.array(std_ph)).any(axis=1)]
            sc = sc.sort_values("p_hot")
            lineplot(sc, "p_hot", "throughput_mops", "impl",
                     f"Skew sensitivity (T={T}, {mix80}, B={B:,})",
                     os.path.join(outdir,f"fig5_skew_T{T}_B{B}.png"),
                     "p_hot","Throughput (Mops/s)")

    # Mix impact bars
    if mix50:
        for dist, tag in [("uniform",""), ("skew", f"_skew{skew_ph:.2f}")]:
            mb = df[(df["mode"]=="strong") & (df["bucket_count"]==hiB) & (df["threads"]==t_max) & (df["mix"].isin([mix80, mix50])) & (df["dist"]==dist)]
            if dist=="skew": mb = mb[np.isclose(mb["p_hot"], skew_ph)]
            if mb.empty: continue
            agg = mb.groupby(["impl","mix"],as_index=False)["throughput_mops"].mean()
            barplot(agg,"impl","throughput_mops","mix",
                    f"Mix impact (T={t_max}, B={hiB:,}, {dist})",
                    os.path.join(outdir,f"fig6_mix_T{t_max}_B{hiB}{tag}.png"),
                    "Implementation","Throughput (Mops/s)")

    # Speedup vs threads
    sp = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==hiB)]
    lineplot(sp,"threads","speedup","impl",
             f"Speedup vs threads (uniform, {mix80}, B={hiB:,})",
             os.path.join(outdir,"fig7_speedup_uniform.png"),
             "Threads","Speedup")

    # Heatmaps (impl × dist)
    hdir = os.path.join(outdir,"heatmaps"); ensure_outdir(hdir)
    for impl in impls:
        for dist, tag in [("uniform","uniform"),("skew","skew")]:
            H = df[(df["impl"]==impl) & (df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]==dist)]
            if dist=="skew": H = H[np.isclose(H["p_hot"], skew_ph)]
            if H.empty: continue
            heatmap(H,"threads","bucket_count","throughput_mops",
                    f"{impl} heatmap ({dist}, {mix80})",
                    os.path.join(hdir,f"heatmap_{impl}_{tag}.png"))

    # Skew penalty ratio (skew/uniform)
    rdir = os.path.join(outdir,"skew_penalty"); ensure_outdir(rdir)
    for B in [midB, hiB]:
        U = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="uniform") & (df["bucket_count"]==B)]
        S = df[(df["mode"]=="strong") & (df["mix"]==mix80) & (df["dist"]=="skew") & (df["bucket_count"]==B) & (np.isclose(df["p_hot"], skew_ph))]
        keys = ["impl","threads","ops","mix","bucket_count"]
        M = pd.merge(U[keys+["throughput_mops"]].rename(columns={"throughput_mops":"thr_uniform"}),
                     S[keys+["throughput_mops"]].rename(columns={"throughput_mops":"thr_skew"}),
                     on=keys, how="inner")
        if M.empty: continue
        M["ratio"] = M["thr_skew"]/M["thr_uniform"]
        lineplot(M,"threads","ratio","impl",
                 f"Skew penalty (p_hot={skew_ph:.2f}, B={B:,})",
                 os.path.join(rdir,f"skew_penalty_B{B}.png"),
                 "Threads","Throughput ratio (skew/uniform)")

    # Padding effect (if padded variants present)
    pdir = os.path.join(outdir,"padding"); ensure_outdir(pdir)
    impl_set = set(impls)
    pairs = []
    for impl in impls:
        if impl.endswith("-Padded"):
            base = impl[:-7]
            if base in impl_set:
                pairs.append((base, impl))
    for base, padded in pairs:
        for dist, tag in [("uniform","uniform"),("skew",f"skew{skew_ph:.2f}")]:
            P = df[(df["impl"].isin([base,padded])) & (df["mode"]=="strong") & (df["bucket_count"]==hiB) & (df["threads"]==t_max) & (df["mix"]==mix80)]
            if dist=="skew":
                P = P[(P["dist"]=="skew") & (np.isclose(P["p_hot"], skew_ph))]
            else:
                P = P[P["dist"]=="uniform"]
            if P.empty: continue
            G = P.groupby("impl", as_index=False)["throughput_mops"].mean()
            barplot(G,"impl","throughput_mops",None,
                    f"Padding effect: {base} vs {padded} ({dist}, T={t_max}, B={hiB:,})",
                    os.path.join(pdir,f"padding_{base}_vs_padded_{tag}.png"),
                    "Implementation","Throughput (Mops/s)")

def main():
    print(f"[info] Searching for matrix CSVs in '{RESULTS_DIR}' with patterns {PATTERNS}")
    csvs = discover_matrix_csvs()
    print("[info] Using files:")
    for f in csvs:
        print(f"  - {f}")
    df = load_matrix(csvs)
    df = add_avg_chain(df)
    plot_all(df, OUTDIR)
    print(f"[done] Figures written to {OUTDIR}/")

if __name__ == "__main__":
    main()