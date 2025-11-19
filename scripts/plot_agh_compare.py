#!/usr/bin/env python3
"""
Compare AGH against Fine and Segment.

NOTE: Only change requested — plot labels show 'S2Hash' instead of 'AGH'.
All filenames, CSV schema, and internal canonical name remain 'AGH'.

What it generates (in ../results/figs_agh_compare):
- agh_bar_uniform_T{T}_B{high}.png        : Bars @ T for uniform, high bucket (AGH vs Fine vs Segment)
- agh_bar_skew_T{T}_B{mid}.png            : Bars @ T for skew (p_hot=chosen), mid bucket
- agh_threads_uniform_B{high}.png         : Throughput vs threads (uniform, high bucket)
- agh_threads_skew_B{mid}.png             : Throughput vs threads (skew p_hot=chosen, mid bucket)
- agh_speedup_uniform_B{high}.png         : Speedup vs threads (uniform, high bucket) [recomputed]
- agh_speedup_skew_B{mid}.png             : Speedup vs threads (skew p_hot=chosen, mid bucket) [recomputed]
- agh_skew_sweep_T{T}_B{bucket}.png       : Throughput vs p_hot @ T for each impl (one per bucket present)
- agh_buckets_uniform_T{T}.png            : Throughput vs bucket_count @ T (uniform)
- agh_buckets_skew_T{T}_ph{ph}.png        : Throughput vs bucket_count @ T (skew at chosen p_hot)
- agh_vs_baselines_delta.csv              : Delta (%) of AGH vs Segment and vs Fine on two headline slices
- agh_compare_summary.md                  : Human-readable decision + deltas

Usage:
- Place CSVs under ../results:
  - agh_matrix.csv          (impl will be normalized to "AGH")
  - segment_matrix.csv      (impl normalized to "Segment")
  - fine_matrix.csv         (impl normalized to "Fine")
- Optionally set environment variables:
  - HEADLINE_T (default 16)       : thread count for bar/bucket/skew-sweep plots
  - MIX (default "80/20")
"""
import os, warnings
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

warnings.filterwarnings("ignore", category=UserWarning)
sns.set(context="paper", style="whitegrid", font_scale=1.05)
plt.rcParams["figure.dpi"] = 140
plt.rcParams["savefig.bbox"] = "tight"

RESULTS_DIR = "../results"
OUTDIR = "../results/figs_agh_compare"

AGH_FILE = "agh_matrix.csv"
SEG_FILE = "segment_matrix.csv"
FINE_FILE = "fine_matrix.csv"

EXPECTED_COLS = {
    "impl","mode","mix","dist","threads","ops","bucket_count",
    "read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"
}
IMPL_ALIASES = {
    "Segment": {"Segment","Segment-Padded","SegmentBased","Segment-Based","segment"},
    "Fine": {"Fine","Fine-Grained","PerBucket","fine"},
    "AGH": {"AGH","agh","Agh"}
}

# Mapping for display labels (graph legend / axis) ONLY
LABEL_MAP = {"AGH": "S2Hash"}

SKEW_GAIN_THRESHOLD = 0.10     # >= +10% vs Segment on skew mid bucket
UNIFORM_REG_LIMIT   = -0.05    # >= -5% vs Fine on uniform high bucket

HEADLINE_T = int(os.getenv("HEADLINE_T", "16"))
MIX = os.getenv("MIX", "80/20")

def ensure_outdir(p): os.makedirs(p, exist_ok=True)

def normalize_impl(df):
    df = df.copy()
    for canonical, aliases in IMPL_ALIASES.items():
        df.loc[df["impl"].isin(aliases), "impl"] = canonical
    return df

def load_csv(path, impl_override=None):
    if not os.path.exists(path): return None
    df = pd.read_csv(path)
    df.columns = df.columns.str.strip()
    if not EXPECTED_COLS.issubset(df.columns):
        missing = EXPECTED_COLS - set(df.columns)
        raise SystemExit(f"Bad schema in {path}, missing: {missing}")
    for c in ["threads","ops","bucket_count"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    for c in ["read_ratio","p_hot","time_s","throughput_mops","speedup","seq_baseline_s"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    if impl_override:
        df["impl"] = impl_override
    df = normalize_impl(df)
    if df.columns.duplicated().any():
        df = df.loc[:, ~df.columns.duplicated()]
    return df

def pick_skew_ph(df):
    vals = sorted(set(df.loc[df["dist"]=="skew","p_hot"].dropna()))
    for pref in [0.90, 0.99, 0.70]:
        if any(np.isclose(val, pref) for val in vals):
            return pref
    return vals[-1] if vals else 0.90

def pick_mid_high_buckets(df):
    bucks = sorted(set(df["bucket_count"].dropna().astype(int)))
    if not bucks:
        raise SystemExit("No bucket counts found.")
    high = bucks[-1]
    mid = bucks[-2] if len(bucks) > 1 else high
    return mid, high

def sanitize_threads(col):
    return (col.astype(str)
            .str.extract(r"(\d+)", expand=False)
            .astype(float))

def strong_slice(df, dist, bucket, mix=MIX, p_hot=None):
    sub = (df["mode"]=="strong") & (df["mix"]==mix) & (df["bucket_count"]==bucket)
    if dist == "uniform":
        sub &= (df["dist"]=="uniform")
    else:
        sub &= (df["dist"]=="skew")
        if p_hot is not None:
            sub &= np.isclose(df["p_hot"], p_hot)
    out = df[sub].copy()
    out["threads"] = sanitize_threads(out["threads"]).astype("Int64")
    out = out.dropna(subset=["threads","throughput_mops"])
    out["threads"] = out["threads"].astype(int)
    return out

def at_T(df, dist, bucket, T=HEADLINE_T, mix=MIX, p_hot=None):
    sub = strong_slice(df, dist, bucket, mix, p_hot)
    return sub[sub["threads"]==T].copy()

def recompute_speedup(df_slice):
    rows = []
    for impl in sorted(df_slice["impl"].unique()):
        sub = df_slice[df_slice["impl"]==impl].sort_values("threads")
        base = sub[sub["threads"]==1]
        if base.empty:
            print(f"[warn] Missing 1-thread baseline for {impl} in slice; skipping it for speedup.")
            continue
        b_thr = base["throughput_mops"].iloc[0]
        for r in sub.itertuples():
            sp = r.throughput_mops / b_thr if b_thr > 0 else np.nan
            rows.append({"impl":impl, "threads":r.threads, "speedup":sp})
    return pd.DataFrame(rows)

def display_label(impl):
    return LABEL_MAP.get(impl, impl)

def bar_plot_T(sub, title, fname, order=("Fine","Segment","AGH")):
    if sub.empty:
        print(f"[warn] Empty bar slice for {fname}")
        return
    use_order = [x for x in order if x in sub["impl"].unique()]
    # Use underlying impl for order; then relabel x tick text
    plt.figure(figsize=(6.0,3.4))
    ax = sns.barplot(data=sub, x="impl", y="throughput_mops", order=use_order, palette="tab10")
    ax.set_title(title); ax.set_xlabel(""); ax.set_ylabel("Throughput (Mops/s)")
    # Annotate heights
    for p in ax.patches:
        v = p.get_height()
        ax.annotate(f"{v:.2f}", (p.get_x()+p.get_width()/2., v), ha='center', va='bottom',
                    fontsize=8, xytext=(0,3), textcoords='offset points')
    # Relabel x-axis ticks
    ax.set_xticklabels([display_label(t.get_text()) for t in ax.get_xticklabels()])
    plt.tight_layout(); plt.savefig(fname); plt.close(); print(f"[info] Wrote {fname}")

def line_plot_throughput(df_slice, title, fname, order=("Fine","Segment","AGH")):
    if df_slice.empty:
        print(f"[warn] Empty line slice for {fname}")
        return
    plt.figure(figsize=(6.6,3.6))
    for impl in order:
        sub = df_slice[df_slice["impl"]==impl].sort_values("threads")
        if sub.empty: continue
        plt.plot(sub["threads"], sub["throughput_mops"], marker="o", label=display_label(impl))
    plt.title(title); plt.xlabel("Threads"); plt.ylabel("Throughput (Mops/s)")
    plt.legend(fontsize="small"); plt.tight_layout(); plt.savefig(fname); plt.close()
    print(f"[info] Wrote {fname}")

def line_plot_speedup(df_slice, title, fname, order=("Fine","Segment","AGH")):
    sp = recompute_speedup(df_slice)
    if sp.empty:
        print(f"[warn] Empty speedup slice for {fname}")
        return
    plt.figure(figsize=(6.6,3.6))
    for impl in order:
        sub = sp[sp["impl"]==impl].sort_values("threads")
        if sub.empty: continue
        plt.plot(sub["threads"], sub["speedup"], marker="o", label=display_label(impl))
    plt.title(title); plt.xlabel("Threads"); plt.ylabel("Speedup (×)")
    plt.legend(fontsize="small"); plt.tight_layout(); plt.savefig(fname); plt.close()
    print(f"[info] Wrote {fname}")

def skew_sweep_plot(df_all, bucket, T=HEADLINE_T, mix=MIX, fname="agh_skew_sweep.png", order=("Fine","Segment","AGH")):
    sub = df_all[(df_all["mode"]=="strong") & (df_all["mix"]==mix) & (df_all["bucket_count"]==bucket) &
                 (df_all["threads"]==T) & (df_all["dist"]=="skew")].copy()
    if sub.empty:
        print(f"[warn] Empty skew sweep for B={bucket}, T={T}")
        return
    plt.figure(figsize=(6.6,3.6))
    for impl in order:
        r = sub[sub["impl"]==impl].sort_values("p_hot")
        if r.empty: continue
        plt.plot(r["p_hot"], r["throughput_mops"], marker="o", label=display_label(impl))
    plt.title(f"Skew sensitivity (T={T}, B={bucket:,}, mix={mix})")
    plt.xlabel("p_hot"); plt.ylabel("Throughput (Mops/s)")
    plt.legend(fontsize="small"); plt.tight_layout(); plt.savefig(fname); plt.close()
    print(f"[info] Wrote {fname}")

def buckets_scaling_plot(df_all, dist, T=HEADLINE_T, mix=MIX, p_hot=None, fname="agh_buckets.png", order=("Fine","Segment","AGH")):
    sub = (df_all["mode"]=="strong") & (df_all["mix"]==mix) & (df_all["threads"]==T)
    if dist=="uniform":
        sub &= (df_all["dist"]=="uniform")
    else:
        sub &= (df_all["dist"]=="skew")
        if p_hot is not None:
            sub &= np.isclose(df_all["p_hot"], p_hot)
    d = df_all[sub].copy()
    if d.empty:
        print(f"[warn] Empty bucket scaling slice for {dist}, T={T}")
        return
    plt.figure(figsize=(6.8,3.8))
    for impl in order:
        r = d[d["impl"]==impl].sort_values("bucket_count")
        if r.empty: continue
        plt.plot(r["bucket_count"], r["throughput_mops"], marker="o", label=display_label(impl))
    ttl = f"Throughput vs buckets (dist={dist}, T={T}, mix={mix}"
    if dist=="skew" and p_hot is not None: ttl += f", p_hot={p_hot:.2f}"
    ttl += ")"
    plt.title(ttl); plt.xlabel("bucket_count"); plt.ylabel("Throughput (Mops/s)")
    plt.legend(fontsize="small"); plt.tight_layout(); plt.savefig(fname); plt.close()
    print(f"[info] Wrote {fname}")

def compute_deltas(df_all, midB, highB, skew_ph, T=HEADLINE_T, mix=MIX):
    rows=[]
    skewT = at_T(df_all, "skew", midB, T=T, mix=mix, p_hot=skew_ph)
    if not skewT.empty:
        def val(impl):
            s = skewT[skewT["impl"]==impl]["throughput_mops"]
            return np.nan if s.empty else float(s.mean())
        agh = val("AGH"); seg = val("Segment"); fin = val("Fine")
        if not np.isnan(agh):
            if not np.isnan(seg):
                rows.append(dict(slice="skew_midB", cmp="AGH vs Segment", delta_pct=(agh-seg)/seg*100.0, agh=agh, base=seg))
            if not np.isnan(fin):
                rows.append(dict(slice="skew_midB", cmp="AGH vs Fine", delta_pct=(agh-fin)/fin*100.0, agh=agh, base=fin))
    uniT = at_T(df_all, "uniform", highB, T=T, mix=mix)
    if not uniT.empty:
        def val2(impl):
            s = uniT[uniT["impl"]==impl]["throughput_mops"]
            return np.nan if s.empty else float(s.mean())
        agh = val2("AGH"); seg = val2("Segment"); fin = val2("Fine")
        if not np.isnan(agh):
            if not np.isnan(seg):
                rows.append(dict(slice="uniform_highB", cmp="AGH vs Segment", delta_pct=(agh-seg)/seg*100.0, agh=agh, base=seg))
            if not np.isnan(fin):
                rows.append(dict(slice="uniform_highB", cmp="AGH vs Fine", delta_pct=(agh-fin)/fin*100.0, agh=agh, base=fin))
    return pd.DataFrame(rows)

def decide(delta_df):
    include = False
    notes=[]
    if delta_df.empty:
        return include, "No comparable slices with AGH + Fine + Segment."
    skew_row = delta_df[(delta_df["slice"]=="skew_midB") & (delta_df["cmp"]=="AGH vs Segment")]
    uni_row  = delta_df[(delta_df["slice"]=="uniform_highB") & (delta_df["cmp"]=="AGH vs Fine")]
    if not skew_row.empty:
        sd = float(skew_row.iloc[0].delta_pct)
        notes.append(f"Skew midB Δ(AGH vs Segment)={sd:.2f}% (target ≥ {SKEW_GAIN_THRESHOLD*100:.0f}%).")
    else:
        notes.append("Missing skew_midB comparison vs Segment.")
    if not uni_row.empty:
        ud = float(uni_row.iloc[0].delta_pct)
        notes.append(f"Uniform highB Δ(AGH vs Fine)={ud:.2f}% (limit ≥ {UNIFORM_REG_LIMIT*100:.0f}%).")
    else:
        notes.append("Missing uniform_highB comparison vs Fine.")
    include = (not skew_row.empty and sd >= SKEW_GAIN_THRESHOLD*100) and (not uni_row.empty and ud >= UNIFORM_REG_LIMIT*100)
    return include, " ".join(notes)

def write_summary(delta_df, include, rationale):
    path = os.path.join(OUTDIR, "agh_compare_summary.md")
    lines = [
        "# AGH vs Fine and Segment — Summary",
        f"**Decision:** {'INCLUDE in main narrative' if include else 'APPENDIX ONLY (exploratory)'}",
        "",
        "Rationale:",
        f"- {rationale}",
        ""
    ]
    if not delta_df.empty:
        lines += [
            "## Headline slice deltas @ T={}".format(HEADLINE_T),
            "| slice | comparison | AGH (Mops/s) | baseline (Mops/s) | Δ% |",
            "|-------|------------|--------------|--------------------|----|",
        ]
        for _,r in delta_df.iterrows():
            lines.append(f"| {r['slice']} | {r['cmp']} | {r['agh']:.2f} | {r['base']:.2f} | {r['delta_pct']:.2f}% |")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"[info] Wrote {path}")

def main():
    ensure_outdir(OUTDIR)
    seg = load_csv(os.path.join(RESULTS_DIR, SEG_FILE))
    fine = load_csv(os.path.join(RESULTS_DIR, FINE_FILE))
    agh = load_csv(os.path.join(RESULTS_DIR, AGH_FILE), impl_override="AGH")
    if seg is None: raise SystemExit("Missing segment_matrix.csv")
    if fine is None: raise SystemExit("Missing fine_matrix.csv")
    if agh is None: raise SystemExit("Missing agh_matrix.csv")

    df = pd.concat([seg, fine, agh], ignore_index=True)
    df = df[df["impl"].isin(["Segment","Fine","AGH"])].copy()

    skew_ph = pick_skew_ph(df)
    midB, highB = pick_mid_high_buckets(df)

    # Bars @ T
    uni_T = at_T(df, "uniform", highB, T=HEADLINE_T, mix=MIX)
    bar_plot_T(
        uni_T,
        f"Uniform (mix={MIX}), T={HEADLINE_T}, B={highB:,}",
        os.path.join(OUTDIR, f"agh_bar_uniform_T{HEADLINE_T}_B{highB}.png")
    )
    skew_T = at_T(df, "skew", midB, T=HEADLINE_T, mix=MIX, p_hot=skew_ph)
    bar_plot_T(
        skew_T,
        f"Skew (p_hot={skew_ph:.2f}, mix={MIX}), T={HEADLINE_T}, B={midB:,}",
        os.path.join(OUTDIR, f"agh_bar_skew_T{HEADLINE_T}_B{midB}.png")
    )

    # Throughput vs threads
    uni_slice = strong_slice(df, "uniform", highB, mix=MIX, p_hot=None)
    line_plot_throughput(
        uni_slice,
        f"Throughput vs threads (uniform, B={highB:,}, mix={MIX})",
        os.path.join(OUTDIR, f"agh_threads_uniform_B{highB}.png")
    )
    skew_slice = strong_slice(df, "skew", midB, mix=MIX, p_hot=skew_ph)
    line_plot_throughput(
        skew_slice,
        f"Throughput vs threads (skew p_hot={skew_ph:.2f}, B={midB:,}, mix={MIX})",
        os.path.join(OUTDIR, f"agh_threads_skew_B{midB}.png")
    )

    # Speedup vs threads
    line_plot_speedup(
        uni_slice,
        f"Speedup vs threads (uniform, B={highB:,}, mix={MIX})",
        os.path.join(OUTDIR, f"agh_speedup_uniform_B{highB}.png")
    )
    line_plot_speedup(
        skew_slice,
        f"Speedup vs threads (skew p_hot={skew_ph:.2f}, B={midB:,}, mix={MIX})",
        os.path.join(OUTDIR, f"agh_speedup_skew_B{midB}.png")
    )

    # Skew sensitivity sweeps
    buckets = sorted(set(df["bucket_count"].dropna().astype(int)))
    for B in buckets:
        skew_sweep_plot(
            df, B, T=HEADLINE_T, mix=MIX,
            fname=os.path.join(OUTDIR, f"agh_skew_sweep_T{HEADLINE_T}_B{B}.png")
        )

    # Bucket scaling
    buckets_scaling_plot(
        df, "uniform", T=HEADLINE_T, mix=MIX,
        fname=os.path.join(OUTDIR, f"agh_buckets_uniform_T{HEADLINE_T}.png")
    )
    buckets_scaling_plot(
        df, "skew", T=HEADLINE_T, mix=MIX, p_hot=skew_ph,
        fname=os.path.join(OUTDIR, f"agh_buckets_skew_T{HEADLINE_T}_ph{str(skew_ph).replace('.','')}.png")
    )

    # Deltas and decision
    delta_df = compute_deltas(df, midB, highB, skew_ph, T=HEADLINE_T, mix=MIX)
    delta_csv = os.path.join(OUTDIR, "agh_vs_baselines_delta.csv")
    delta_df.to_csv(delta_csv, index=False)
    print(f"[info] Wrote {delta_csv}")

    include, rationale = decide(delta_df)
    write_summary(delta_df, include, rationale)

    print("[done] AGH vs Fine and Segment comparison complete.")

if __name__ == "__main__":
    main()