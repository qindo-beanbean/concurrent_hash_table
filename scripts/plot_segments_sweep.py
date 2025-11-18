#!/usr/bin/env python3
import os
import glob
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

sns.set(context="paper", style="whitegrid", font_scale=1.1)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

def ensure_outdir(path: str):
    os.makedirs(path, exist_ok=True)

def find_file(default_paths, must_exist=True):
    for p in default_paths:
        if os.path.exists(p):
            return p
    # fallback: search recursively
    for pattern in ["**/summary.csv", "**/avg_report.csv"]:
        for f in glob.glob(pattern, recursive=True):
            if os.path.exists(f):
                return f
    if must_exist:
        raise SystemExit(f"Could not find any of: {default_paths}")
    return ""

def read_summary(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    # Normalize S column ("S256" -> 256)
    def to_int_s(x):
        s = str(x)
        return int(s[1:]) if s.startswith("S") else int(s)
    df["S"] = df["S"].apply(to_int_s)
    # Types
    df["threads"] = df["threads"].astype(int)
    df["bucket_count"] = df["bucket_count"].astype(int)
    df["throughput_mops"] = df["throughput_mops"].astype(float)
    return df

def compute_avg_from_summary(summary: pd.DataFrame) -> pd.DataFrame:
    # Balanced slice as discussed
    sl = summary[
        (summary["mode"] == "strong")
        & (summary["mix"].isin(["80/20","50/50"]))
        & ((summary["dist"] == "uniform") | ((summary["dist"] == "skew") & (abs(summary["p_hot"] - 0.90) < 1e-9)))
        & (summary["threads"].isin([8,16]))
        & (summary["bucket_count"].isin([65536, 262144]))
    ].copy()
    if sl.empty:
        raise SystemExit("Filtered slice is empty. Ensure your summary.csv matches the sweep script’s filters.")
    avg = sl.groupby("S", as_index=False)["throughput_mops"].mean().rename(columns={"throughput_mops":"avg_throughput_mops"})
    uni = sl[sl["dist"]=="uniform"].groupby("S", as_index=False)["throughput_mops"].mean().rename(columns={"throughput_mops":"avg_uniform_mops"})
    skew = sl[sl["dist"]=="skew"].groupby("S", as_index=False)["throughput_mops"].mean().rename(columns={"throughput_mops":"avg_skew_mops"})
    out = avg.merge(uni, on="S", how="left").merge(skew, on="S", how="left")
    out["rows_count"] = sl.groupby("S")["throughput_mops"].count().values
    return out

def plot_overall_avg(df_avg: pd.DataFrame, outdir: str):
    best_idx = df_avg["avg_throughput_mops"].idxmax()
    best_row = df_avg.loc[best_idx]
    best_S = int(best_row["S"])
    best_val = float(best_row["avg_throughput_mops"])

    plt.figure(figsize=(6.3, 3.9))
    ax = sns.barplot(data=df_avg.sort_values("S"), x="S", y="avg_throughput_mops", color="#4C78A8")
    ax.set_title(f"Average throughput vs segments (chosen S={best_S}, ~{best_val:.2f} Mops/s)")
    ax.set_xlabel("Number of segments (S)")
    ax.set_ylabel("Average throughput (Mops/s)")
    # annotate chosen bar
    xs = list(df_avg.sort_values("S")["S"])
    xpos = xs.index(best_S)
    ax.annotate("chosen", xy=(xpos, best_val), xytext=(0, 10),
                textcoords="offset points", ha="center", va="bottom",
                fontsize=9, color="crimson")
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, "seg_overall_avg.png"))
    plt.close()
    return best_S, best_val

def slice_and_plot(summary: pd.DataFrame, outdir: str):
    # Curves vs S for dist in {uniform, skew@0.90}, T in {8,16}, B in {65536, 262144}, averaged across mixes
    for dist in ["uniform", "skew"]:
        for B in [65536, 262144]:
            for T in [8, 16]:
                df = summary[
                    (summary["mode"]=="strong")
                    & (summary["mix"].isin(["80/20","50/50"]))
                    & (summary["dist"]==dist)
                    & (summary["bucket_count"]==B)
                    & (summary["threads"]==T)
                ].copy()
                if dist=="skew":
                    df = df[abs(df["p_hot"] - 0.90) < 1e-9]
                if df.empty:
                    continue
                g = df.groupby("S", as_index=False)["throughput_mops"].mean()
                plt.figure(figsize=(6.3, 3.4))
                ax = sns.lineplot(data=g.sort_values("S"), x="S", y="throughput_mops", marker="o", color="#4C78A8")
                ax.set_title(f"Throughput vs segments (dist={dist}, T={T}, B={B:,}, avg over mixes)")
                ax.set_xlabel("Number of segments (S)")
                ax.set_ylabel("Throughput (Mops/s)")
                plt.tight_layout()
                plt.savefig(os.path.join(outdir, f"seg_curve_{dist}_T{T}_B{B}.png"))
                plt.close()

def main():
    outdir = "../results/segments/figs_segments"
    ensure_outdir(outdir)

    # Auto-discover files
    summary_path = find_file([
        "../results/segments/summary.csv",
    ])
    # avg_report is optional; we can compute from summary
    avg_path_candidates = [
        "../results/segments/avg_report.csv",
    ]
    avg_path = None
    for p in avg_path_candidates:
        if os.path.exists(p):
            avg_path = p
            break

    summary = read_summary(summary_path)
    if avg_path and os.path.exists(avg_path):
        df_avg = pd.read_csv(avg_path)
        df_avg["S"] = df_avg["S"].astype(int)
    else:
        df_avg = compute_avg_from_summary(summary)

    best_S, best_val = plot_overall_avg(df_avg, outdir)
    slice_and_plot(summary, outdir)

    # Save decision CSV for the appendix text
    pd.DataFrame([{"best_S": int(best_S), "avg_throughput_mops": float(best_val)}]).to_csv(
        os.path.join(outdir, "chosen_segments.csv"), index=False
    )

    print(f"[done] Segment sweep figures written to {outdir}/")
    print(f"[chosen] SB_DEFAULT_SEGMENTS={best_S} (avg throughput ~ {best_val:.2f} Mops/s)")

if __name__ == "__main__":
    main()