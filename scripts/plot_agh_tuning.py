#!/usr/bin/env python3
import os, warnings
import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

warnings.filterwarnings("ignore", category=UserWarning)
sns.set(context="paper", style="whitegrid", font_scale=1.05)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

IN_CSV = "../results/agh_tuning/agh_tune.csv"
OUTDIR = "../results/figs_agh_tuning"

TARGET_THREADS = 16
SKEW_PH = 0.90
SKEW_BUCKET = 262144
UNIFORM_BUCKET = 1048576

def load():
    if not os.path.exists(IN_CSV):
        raise SystemExit(f"{IN_CSV} not found. Run scripts/tune_agh.sh first.")
    df = pd.read_csv(IN_CSV)
    for c in ["threads","bucket_count","throughput_mops","S","MAX_STRIPES","STRIPE_FACTOR","p_hot"]:
        if c in df.columns:
            df[c] = pd.to_numeric(df[c], errors="coerce")
    df = df.dropna(subset=["threads","bucket_count","throughput_mops"])
    return df

def slice_decision(df):
    skew = df[(df["dist"]=="skew") &
              (np.isclose(df["p_hot"], SKEW_PH)) &
              (df["bucket_count"]==SKEW_BUCKET) &
              (df["threads"]==TARGET_THREADS)]
    uni  = df[(df["dist"]=="uniform") &
              (df["bucket_count"]==UNIFORM_BUCKET) &
              (df["threads"]==TARGET_THREADS)]
    return skew, uni

def aggregate(slice_df):
    if slice_df.empty: return pd.DataFrame()
    cols = ["S","MAX_STRIPES","STRIPE_FACTOR"]
    return (slice_df.groupby(cols, as_index=False)["throughput_mops"].mean()
            .sort_values("throughput_mops", ascending=False))

def cfg_label(r):
    return f"S{int(r.S)}-M{int(r.MAX_STRIPES)}-F{int(r.STRIPE_FACTOR)}"

def plot_combined(skew_agg, uni_agg, out_path):
    if skew_agg.empty and uni_agg.empty:
        print("[warn] No data to plot.")
        return
    skew_agg = skew_agg.copy(); skew_agg["slice"]="skew"
    uni_agg  = uni_agg.copy();  uni_agg["slice"]="uniform"
    merged = pd.concat([skew_agg, uni_agg], ignore_index=True)
    merged["config"] = merged.apply(cfg_label, axis=1)
    order = list(skew_agg["S"].astype(int).astype(str) + "-M" + skew_agg["MAX_STRIPES"].astype(int).astype(str) + "-F" + skew_agg["STRIPE_FACTOR"].astype(int).astype(str))
    plt.figure(figsize=(10,4.6))
    ax1 = plt.subplot(1,2,1)
    sns.barplot(data=merged[merged["slice"]=="skew"], x="config", y="throughput_mops", color="#4C78A8", ax=ax1, order=order)
    ax1.set_title(f"Skew (T={TARGET_THREADS}, B={SKEW_BUCKET:,}, p_hot={SKEW_PH})")
    ax1.set_xlabel("Config"); ax1.set_ylabel("Throughput (Mops/s)")
    ax1.tick_params(axis='x', rotation=35)
    for p in ax1.patches:
        v = p.get_height()
        ax1.annotate(f"{v:.2f}", (p.get_x()+p.get_width()/2., v), ha='center', va='bottom', fontsize=7, xytext=(0,2), textcoords='offset points')
    ax2 = plt.subplot(1,2,2)
    sns.barplot(data=merged[merged["slice"]=="uniform"], x="config", y="throughput_mops", color="#F58518", ax=ax2, order=order)
    ax2.set_title(f"Uniform (T={TARGET_THREADS}, B={UNIFORM_BUCKET:,})")
    ax2.set_xlabel("Config"); ax2.set_ylabel("Throughput (Mops/s)")
    ax2.tick_params(axis='x', rotation=35)
    for p in ax2.patches:
        v = p.get_height()
        ax2.annotate(f"{v:.2f}", (p.get_x()+p.get_width()/2., v), ha='center', va='bottom', fontsize=7, xytext=(0,2), textcoords='offset points')
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"[info] Wrote {out_path}")

def plot_scaling(df, topN, out_path):
    rows=[]
    top = []
    # Select topN by skew at target slice
    skew_slice, _ = slice_decision(df)
    skew_agg = aggregate(skew_slice)
    for _, r in skew_agg.head(topN).iterrows():
        top.append((int(r.S), int(r.MAX_STRIPES), int(r.STRIPE_FACTOR)))
    if not top:
        print("[warn] No top configs found for scaling.")
        return
    for (S, M, F) in top:
        sub = df[(df["S"]==S) & (df["MAX_STRIPES"]==M) & (df["STRIPE_FACTOR"]==F)]
        for dist, B in [("uniform", UNIFORM_BUCKET), ("skew", SKEW_BUCKET)]:
            cur = sub[(sub["dist"]==dist) & ((dist=="uniform") | (np.isclose(sub["p_hot"], SKEW_PH))) & (sub["bucket_count"]==B)]
            cur = cur.groupby("threads", as_index=False)["throughput_mops"].mean().sort_values("threads")
            for _, rr in cur.iterrows():
                rows.append({"config": f"S{S}-M{M}-F{F}", "slice": dist, "threads": int(rr["threads"]), "throughput_mops": rr["throughput_mops"]})
    plot_df = pd.DataFrame(rows)
    if plot_df.empty:
        print("[warn] Empty scaling data.")
        return
    plt.figure(figsize=(8.0,4.2))
    sns.lineplot(data=plot_df, x="threads", y="throughput_mops", hue="config", style="slice", marker="o")
    plt.title("Top AGH configs: throughput vs threads (≤16)")
    plt.xlabel("Threads"); plt.ylabel("Throughput (Mops/s)")
    plt.tight_layout(); plt.savefig(out_path); plt.close()
    print(f"[info] Wrote {out_path}")

def write_summary(df, out_path):
    skew_slice, uni_slice = slice_decision(df)
    skew_agg = aggregate(skew_slice)
    uni_agg  = aggregate(uni_slice)
    lines = ["# AGH Tuning Summary (threads ≤ 16)"]
    if skew_agg.empty:
        lines.append("No skew decision slice at target threads.")
    else:
        best = skew_agg.iloc[0]
        lines.append("## Recommended Configuration")
        lines.append(f"- S={int(best.S)}, MAX_STRIPES={int(best.MAX_STRIPES)}, STRIPE_FACTOR={int(best.STRIPE_FACTOR)}")
        lines.append(f"- Skew (T={TARGET_THREADS}, B={SKEW_BUCKET:,}): {best.throughput_mops:.2f} Mops/s")
        # Uniform value for same config
        m = uni_agg[(uni_agg["S"]==best.S) & (uni_agg["MAX_STRIPES"]==best.MAX_STRIPES) & (uni_agg["STRIPE_FACTOR"]==best.STRIPE_FACTOR)]
        if not m.empty:
            lines.append(f"- Uniform (T={TARGET_THREADS}, B={UNIFORM_BUCKET:,}): {m.iloc[0].throughput_mops:.2f} Mops/s")
        lines.append("\n## Top by Skew")
        lines.append("| Rank | Config | Throughput (Mops/s) |")
        lines.append("|------|--------|---------------------|")
        for i, r in enumerate(skew_agg.head(8).itertuples(), start=1):
            lines.append(f"| {i} | S{int(r.S)}-M{int(r.MAX_STRIPES)}-F{int(r.STRIPE_FACTOR)} | {r.throughput_mops:.2f} |")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"[info] Wrote {out_path}")

def main():
    os.makedirs(OUTDIR, exist_ok=True)
    df = load()
    skew_slice, uni_slice = slice_decision(df)
    skew_agg = aggregate(skew_slice)
    uni_agg  = aggregate(uni_slice)
    plot_combined(skew_agg, uni_agg, os.path.join(OUTDIR, "agh_combined_T16.png"))
    plot_scaling(df, topN=3, out_path=os.path.join(OUTDIR, "agh_scaling_le16.png"))
    write_summary(df, os.path.join(OUTDIR, "agh_summary_T16.md"))
    print("[done] AGH tuning figures written.")

if __name__ == "__main__":
    main()