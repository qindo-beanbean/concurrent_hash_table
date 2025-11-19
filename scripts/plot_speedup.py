#!/usr/bin/env python3
import os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

sns.set(context="paper", style="whitegrid", font_scale=1.1)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

RESULTS_DIR = "../results"
OUTDIR = "../results/figs"

IMPL_FILES = {
    "coarse_matrix.csv": "Coarse",
    "fine_matrix.csv": "Fine",
    "segment_matrix.csv": "Segment",
    "lockfree_matrix.csv": "Lock-Free",
}

def load():
    frames=[]
    for fname, label in IMPL_FILES.items():
        path=os.path.join(RESULTS_DIR,fname)
        if not os.path.exists(path):
            print(f"[warn] Missing {fname}")
            continue
        df=pd.read_csv(path)
        # Standard numeric coercions
        num_cols=["threads","bucket_count","throughput_mops","p_hot","read_ratio"]
        for c in num_cols:
            if c in df.columns:
                df[c]=pd.to_numeric(df[c], errors="coerce")
        df=df.dropna(subset=["threads","bucket_count","throughput_mops"])
        df["threads"]=df["threads"].astype(int)
        df["impl"]=label
        frames.append(df)
    if not frames:
        raise SystemExit("No CSVs found.")
    big=pd.concat(frames, ignore_index=True)
    return big

def recompute_speedup(df_slice):
    # df_slice contains multiple impls & threads for identical (mode,mix,dist,bucket,p_hot) slice
    rows=[]
    for impl in sorted(df_slice["impl"].unique()):
        sub=df_slice[df_slice["impl"]==impl].sort_values("threads")
        base=sub[sub["threads"]==1]
        if base.empty:
            print(f"[warn] Missing 1-thread baseline for {impl} slice; skipping speedup curve.")
            continue
        b_thr=base["throughput_mops"].iloc[0]
        for r in sub.itertuples():
            speedup = r.throughput_mops / b_thr if b_thr>0 else np.nan
            efficiency = speedup / r.threads if r.threads>0 else np.nan
            rows.append({
                "impl": impl,
                "threads": r.threads,
                "throughput_mops": r.throughput_mops,
                "speedup": speedup,
                "efficiency": efficiency
            })
    return pd.DataFrame(rows)

def lineplot(df, y, title, fname, logy=False):
    if df.empty:
        print(f"[warn] Empty data for {fname}")
        return
    plt.figure(figsize=(6.8,3.8))
    sns.lineplot(data=df, x="threads", y=y, hue="impl", marker="o")
    plt.title(title)
    plt.xlabel("Threads")
    ylabel = "Speedup (×)" if y=="speedup" else "Efficiency"
    plt.ylabel(ylabel)
    if logy:
        plt.yscale("log")
    plt.tight_layout()
    out=os.path.join(OUTDIR,fname)
    plt.savefig(out)
    plt.close()
    print(f"[info] Wrote {out}")

def main():
    os.makedirs(OUTDIR, exist_ok=True)
    df=load()

    mix = "80/20" if "80/20" in df["mix"].unique() else df["mix"].unique()[0]
    buckets = sorted(df["bucket_count"].unique())
    bigB = 1048576 if 1048576 in buckets else buckets[-1]
    skew_ph = 0.90 if (df["dist"]=="skew").any() and (np.isclose(df["p_hot"],0.90)).any() else None

    # Uniform strong scaling slice
    u_slice = df[(df["mode"]=="strong") & (df["mix"]==mix) & (df["dist"]=="uniform") & (df["bucket_count"]==bigB)]
    u_sp = recompute_speedup(u_slice)
    lineplot(u_sp, "speedup", f"Speedup vs threads (uniform, {mix}, B={bigB:,})", "fig_speedup_uniform_fixed.png")
    lineplot(u_sp, "speedup", f"Speedup vs threads (uniform, log y)", "fig_speedup_uniform_log.png", logy=True)
    # Optional efficiency
    lineplot(u_sp, "efficiency", f"Efficiency vs threads (uniform, {mix}, B={bigB:,})", "fig_efficiency_uniform_recalc.png")

    # Skew strong scaling slice
    if skew_ph is not None:
        s_slice = df[(df["mode"]=="strong") & (df["mix"]==mix) & (df["dist"]=="skew") &
                     (np.isclose(df["p_hot"], skew_ph)) & (df["bucket_count"]==bigB)]
        s_sp = recompute_speedup(s_slice)
        lineplot(s_sp, "speedup", f"Speedup vs threads (skew p_hot={skew_ph:.2f}, {mix}, B={bigB:,})", "fig_speedup_skew_fixed.png")
        lineplot(s_sp, "speedup", f"Speedup vs threads (skew, log y)", "fig_speedup_skew_log.png", logy=True)
        lineplot(s_sp, "efficiency", f"Efficiency vs threads (skew p_hot={skew_ph:.2f})", "fig_efficiency_skew_recalc.png")

if __name__=="__main__":
    main()