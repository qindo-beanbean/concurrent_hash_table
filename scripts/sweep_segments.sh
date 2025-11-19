#!/usr/bin/env bash
set -euo pipefail

# Balanced selector for segment count S:
# - Builds bench per S with -DSB_DEFAULT_SEGMENTS
# - Runs full segment matrix (your bench isn't granular, that's fine)
# - Filters to a representative slice and averages throughput
#   Slice: strong mode; mixes {80/20,50/50}; dist {uniform,skew @ p_hot=0.90};
#   threads {8,16}; bucket_count {65536,262144}
#
# Prereqs:
# - bench_matrix_simple.cpp (per-config baseline variant)
# - segment_based.h supports SB_DEFAULT_SEGMENTS macro
#
# Usage:
#   ./sweep_segments_avg.sh
#   # outputs results/segments/summary.csv and prints best S

S_LIST=("16" "32" "64" "128" "256" "512")
OUTDIR="../results/segments"
mkdir -p "${OUTDIR}"

export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Build, run, and extract CSV for each S
for S in "${S_LIST[@]}"; do
  BIN="bench_segment_S${S}"
  LOG="${OUTDIR}/segment_S${S}.out"
  CSV="${OUTDIR}/segment_S${S}.csv"

  echo "[build] SB_DEFAULT_SEGMENTS=${S}"
  g++ -std=c++17 -O3 -fopenmp -DSB_DEFAULT_SEGMENTS=${S} ../bench_matrix_simple.cpp -o "${BIN}"

  echo "[run]   ${BIN} --impl=segment"
  "./${BIN}" --impl=segment | tee "${LOG}" >/dev/null

  awk '/CSV_RESULTS_BEGIN/{f=1;next}/CSV_RESULTS_END/{f=0}f' "${LOG}" > "${CSV}"
done

# Summarize filtered rows and compute averages per S
SUMMARY="${OUTDIR}/summary.csv"
echo "S,impl,mode,mix,dist,threads,ops,bucket_count,read_ratio,p_hot,time_s,throughput_mops,speedup" > "${SUMMARY}"

for S in "${S_LIST[@]}"; do
  CSV="${OUTDIR}/segment_S${S}.csv"
  awk -F, -v S="${S}" '
    NR>1 && $1=="Segment" && $2=="strong" && ($3=="80/20" || $3=="50/50") &&
    ($4=="uniform" || ($4=="skew" && $9=="0.90")) &&
    ($5=="8" || $5=="16") &&
    ($7=="65536" || $7=="262144") {
      printf "S%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
             S,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12
    }' "${CSV}" >> "${SUMMARY}"
done

# Compute averages per S (overall, and separated by uniform/skew for visibility)
REPORT="${OUTDIR}/avg_report.csv"
echo "S,avg_throughput_mops,avg_uniform_mops,avg_skew_mops,rows_count" > "${REPORT}"

for S in "${S_LIST[@]}"; do
  CSV="${SUMMARY}"
  # overall
  overall=$(awk -F, -v S="S${S}" 'NR>1 && $1==S {sum+=$12; n++} END{if(n>0) printf "%.6f,%d", sum/n, n; else print "0,0"}' "${CSV}")
  # uniform-only
  uni=$(awk -F, -v S="S${S}" 'NR>1 && $1==S && $5=="uniform" {sum+=$12; n++} END{if(n>0) printf "%.6f", sum/n; else printf "0"}' "${CSV}")
  # skew-only
  skew=$(awk -F, -v S="S${S}" 'NR>1 && $1==S && $5=="skew"    {sum+=$12; n++} END{if(n>0) printf "%.6f", sum/n; else printf "0"}' "${CSV}")
  echo "${S},${overall%,*},${uni},${skew},${overall#*,}" >> "${REPORT}"
done

# Pick best S by highest overall average throughput
BEST=$(awk -F, 'NR>1 { if ($2>best) {best=$2; s=$1} } END{if(s!="") print s","best; else print "NA,0"}' "${REPORT}")
BEST_S="${BEST%%,*}"
BEST_THR="${BEST##*,}"

echo "[summary] Wrote ${SUMMARY}"
echo "[report ] Wrote ${REPORT}"
if [ "${BEST_S}" != "NA" ]; then
  echo "[best] SB_DEFAULT_SEGMENTS=${BEST_S} (avg throughput ~ ${BEST_THR} Mops/s) over the selected slice"
else
  echo "[best] No matching rows; check ${SUMMARY}"
fi