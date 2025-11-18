#!/usr/bin/env python3
import os, warnings, numpy as np, pandas as pd, seaborn as sns, matplotlib.pyplot as plt
warnings.filterwarnings("ignore", category=UserWarning)
sns.set(context="paper", style="whitegrid", font_scale=1.05)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

RESULTS_DIR = "../results"
OUTDIR = "../results/figs_agh_compare"
AGH_FILE = "agh_matrix.csv"
SEG_FILE = "segment_matrix.csv"

EXPECTED_COLS = {"impl","mode","mix","dist","threads","ops","bucket_count","read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"}
SEGMENT_ALIASES = {"Segment","Segment-Padded","SegmentBased","Segment-Based","segment"}

SKEW_GAIN_THRESHOLD = 0.10
UNIFORM_REGRESSION_LIMIT = 0.05

def ensure_outdir(p): os.makedirs(p, exist_ok=True)

def load_csv(path, impl_override=None):
    if not os.path.exists(path): return None
    df = pd.read_csv(path)
    if not EXPECTED_COLS.issubset(df.columns):
        raise SystemExit(f"Bad schema in {path}")
    for c in ["threads","ops","bucket_count"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    for c in ["read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    if impl_override: df["impl"] = impl_override
    df.loc[df["impl"].isin(SEGMENT_ALIASES), "impl"] = "Segment"
    if df.columns.duplicated().any():
        df = df.loc[:, ~df.columns.duplicated()]
    return df

def pick_skew_ph(df):
    vals = sorted(set(df.loc[df["dist"]=="skew","p_hot"].dropna().round(2)))
    for pref in [0.90,0.99,0.70]:
        if pref in vals: return pref
    return vals[0] if vals else 0.90

def pick_mid_high_buckets(df):
    buckets = sorted(df["bucket_count"].dropna().unique())
    if not buckets: raise SystemExit("No bucket counts")
    high = buckets[-1]; mid = buckets[-2] if len(buckets)>1 else high
    return mid, high

def slice_df(df, dist, threads, bucket, mix="80/20", skew_ph=None):
    q = (df["mode"]=="strong") & (df["mix"]==mix) & (df["threads"]==threads) & (df["bucket_count"]==bucket)
    if dist=="uniform": q &= (df["dist"]=="uniform")
    else:
        q &= (df["dist"]=="skew")
        if skew_ph is not None: q &= np.isclose(df["p_hot"], skew_ph)
    return df[q].copy()

def sanitize_numeric(col):
    return (col.astype(str)
            .str.replace(r"[^\d]", "", regex=True)
            .replace("", np.nan)
            .astype(float))

def bar_plot(sub, dist, bucket, threads, fname):
    if sub.empty:
        print(f"[warn] Empty bar slice dist={dist}, B={bucket}, T={threads}")
        return
    order = [x for x in ["Segment","AGH"] if x in sub["impl"].unique()]
    plt.figure(figsize=(5.6,3.2))
    sns.barplot(data=sub, x="impl", y="throughput_mops", order=order, color="#4C78A8")
    plt.title(f"dist={dist}, T={threads}, B={bucket:,}")
    plt.ylabel("Mops/s"); plt.xlabel("")
    for p in plt.gca().patches:
        v = p.get_height()
        plt.gca().annotate(f"{v:.2f}", (p.get_x()+p.get_width()/2., v), ha='center', va='bottom', fontsize=8,
                           xytext=(0,3), textcoords='offset points')
    plt.tight_layout(); plt.savefig(fname); plt.close(); print(f"[info] Wrote {fname}")

def line_plot(df, dist, bucket, fname):
    sub = df[(df["mode"]=="strong") & (df["mix"]=="80/20") & (df["bucket_count"]==bucket)]
    if dist=="uniform":
        sub = sub[sub["dist"]=="uniform"]
    else:
        skew_ph = pick_skew_ph(df)
        sub = sub[(sub["dist"]=="skew") & (np.isclose(sub["p_hot"], skew_ph))]
    if sub.empty:
        print(f"[warn] Empty line slice dist={dist}, B={bucket}")
        return
    if sub.columns.duplicated().any():
        sub = sub.loc[:, ~sub.columns.duplicated()]
    sub["threads"] = sanitize_numeric(sub["threads"])
    if sub["threads"].isna().any():
        print("[error] Non-numeric threads after sanitize:", sub["threads"][sub["threads"].isna()].unique())
        return
    sub["threads"] = sub["threads"].astype(int).sort_values()
    order = [x for x in ["Segment","AGH"] if x in sub["impl"].unique()]
    plt.figure(figsize=(5.6,3.2))
    for impl in order:
        r = sub[sub["impl"]==impl].sort_values("threads")
        x = r["threads"].to_numpy()
        y = r["throughput_mops"].to_numpy()
        plt.plot(x,y,marker="o",label=impl)
    plt.title(f"Throughput vs threads (dist={dist}, B={bucket:,})")
    plt.xlabel("Threads"); plt.ylabel("Mops/s"); plt.legend(fontsize="small")
    plt.tight_layout(); plt.savefig(fname); plt.close(); print(f"[info] Wrote {fname}")

def compute_deltas(df, midB, highB, skew_ph):
    rows=[]
    skew = slice_df(df,"skew",16,midB,skew_ph=skew_ph)
    if not skew.empty:
        seg=skew[skew["impl"]=="Segment"]["throughput_mops"]; agh=skew[skew["impl"]=="AGH"]["throughput_mops"]
        if not seg.empty and not agh.empty:
            rows.append(dict(dist="skew",bucket_count=midB,
                             segment_thr=seg.mean(),agh_thr=agh.mean(),
                             delta_pct=(agh.mean()-seg.mean())/seg.mean()*100.0))
    uni = slice_df(df,"uniform",16,highB)
    if not uni.empty:
        seg=uni[uni["impl"]=="Segment"]["throughput_mops"]; agh=uni[uni["impl"]=="AGH"]["throughput_mops"]
        if not seg.empty and not agh.empty:
            rows.append(dict(dist="uniform",bucket_count=highB,
                             segment_thr=seg.mean(),agh_thr=agh.mean(),
                             delta_pct=(agh.mean()-seg.mean())/seg.mean()*100.0))
    return pd.DataFrame(rows)

def decide(delta_df, midB, highB):
    if delta_df.empty:
        return "APPENDIX ONLY (no overlap)", "No slices with both Segment and AGH."
    skew_row = delta_df[(delta_df["dist"]=="skew") & (delta_df["bucket_count"]==midB)]
    uni_row  = delta_df[(delta_df["dist"]=="uniform") & (delta_df["bucket_count"]==highB)]
    include=True; notes=[]
    if skew_row.empty:
        include=False; notes.append("Missing skew slice.")
    else:
        sd=skew_row.iloc[0].delta_pct; notes.append(f"Skew mid bucket Δ={sd:.2f}% (≥{SKEW_GAIN_THRESHOLD*100:.0f}% target).")
        if sd < SKEW_GAIN_THRESHOLD*100: include=False; notes.append("Skew gain below threshold.")
    if uni_row.empty:
        notes.append("Uniform high bucket missing.")
    else:
        ud=uni_row.iloc[0].delta_pct; notes.append(f"Uniform high bucket Δ={ud:.2f}% (regression limit {UNIFORM_REGRESSION_LIMIT*100:.0f}%).")
        if ud < -UNIFORM_REGRESSION_LIMIT*100: include=False; notes.append("Uniform regression too large.")
    return ("INCLUDE in main narrative" if include else "APPENDIX ONLY (exploratory)"," ".join(notes))

def write_report(delta_df, decision, rationale):
    path=os.path.join(OUTDIR,"agh_segment_summary.md")
    lines=["# AGH vs Segment Summary",f"**Decision:** {decision}","","Rationale:","- "+rationale,""]
    if not delta_df.empty:
        lines+=["## Slice Deltas","| dist | bucket | Segment (Mops/s) | AGH (Mops/s) | Δ% |","|------|--------|-----------------|--------------|----|"]
        for _,r in delta_df.iterrows():
            lines.append(f"| {r.dist} | {r.bucket_count:,} | {r.segment_thr:.2f} | {r.agh_thr:.2f} | {r.delta_pct:.2f}% |")
    with open(path,"w",encoding="utf-8") as f: f.write("\n".join(lines))
    print(f"[info] Wrote {path}")

def main():
    seg = load_csv(os.path.join(RESULTS_DIR, SEG_FILE))
    agh = load_csv(os.path.join(RESULTS_DIR, AGH_FILE), impl_override="AGH")
    if seg is None: raise SystemExit("Missing segment_matrix.csv")
    if agh is None: raise SystemExit("Missing agh_matrix.csv")
    df = pd.concat([seg, agh], ignore_index=True)
    if "Segment" not in df["impl"].unique(): raise SystemExit("Segment missing after normalization")
    if "AGH" not in df["impl"].unique(): raise SystemExit("AGH missing after normalization")
    skew_ph = pick_skew_ph(df)
    midB, highB = pick_mid_high_buckets(df)
    ensure_outdir(OUTDIR)

    # Decision bars
    for dist in ["uniform","skew"]:
        for B in [midB, highB]:
            sub = slice_df(df, dist, 16, B, skew_ph=skew_ph if dist=="skew" else None)
            bar_plot(sub, dist, B, 16, os.path.join(OUTDIR,f"agh_decision_{dist}_B{B}.png"))

    # Thread lines
    for dist in ["uniform","skew"]:
        line_plot(df, dist, midB, os.path.join(OUTDIR,f"agh_threads_{dist}_B{midB}.png"))

    # Deltas & decision
    delta_df = compute_deltas(df, midB, highB, skew_ph)
    delta_df.to_csv(os.path.join(OUTDIR,"agh_vs_segment_delta.csv"), index=False)
    decision, rationale = decide(delta_df, midB, highB)
    write_report(delta_df, decision, rationale)
    print("[done] AGH vs Segment comparison complete.")

if __name__ == "__main__":
    main()