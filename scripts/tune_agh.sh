#!/usr/bin/env bash
set -euo pipefail

# AGH tuning (threads ≤ 16), concise search space.
# Parameters in original AGH:
#   -DSB_DEFAULT_SEGMENTS / -DAGH_DEFAULT_SEGMENTS => S (fixed at 512 by default)
#   -DAGH_MAX_STRIPES => M in {8,16,32}
#   -DAGH_STRIPE_FACTOR => F in {1,2,3}
#
# Slices we run:
#   mode=strong, mix=80/20
#   threads in {8,16}
#   bucket_count in {262144,1048576}
#   dist in {uniform, skew @ p_hot=0.90}
#
# Output:
#   ../results/agh_tuning/agh_tune.csv (filtered rows + S,M,F metadata)
#   ../results/agh_tuning/AGH_raw_S${S}_M${M}_F${F}.csv (full AGH CSV block per build)
#
# Override via env if needed:
#   SEG_LIST="512"           # keep 512 by default
#   MAX_LIST="8 16 32"
#   FACTOR_LIST="1 2 3"
#   THREADS_LIST="8 16"
#   BUCKETS_LIST="262144 1048576"
#   STRONG_OPS=2000000

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${ROOT_DIR}/bench_matrix_simple.cpp"
BIN_DIR="${ROOT_DIR}/build"
OUTDIR="${ROOT_DIR}/results/agh_tuning"
BIN="${BIN_DIR}/bench_matrix_simple"

mkdir -p "${BIN_DIR}" "${OUTDIR}"

SEG_LIST=(${SEG_LIST:-512})
MAX_LIST=(${MAX_LIST:-8 16 32})
FACTOR_LIST=(${FACTOR_LIST:-1 2 3})
THREADS_LIST=(${THREADS_LIST:-8 16})
BUCKETS_LIST=(${BUCKETS_LIST:-262144 1048576})
MIX="${MIX:-80/20}"
PH="${PH:-0.90}"

export OMP_PROC_BIND=close
export OMP_PLACES=cores

if [[ ! -f "${SRC}" ]]; then
  echo "[error] Missing ${SRC}"
  exit 1
fi

echo "[info] Building bench object once..."
g++ -std=c++17 -O3 -fopenmp -c "${SRC}" -o "${BIN_DIR}/bench_matrix_simple.o"

COMBINED="${OUTDIR}/agh_tune.csv"
if [[ ! -f "${COMBINED}" ]]; then
  echo "impl,mode,mix,dist,threads,ops,bucket_count,read_ratio,p_hot,time_s,throughput_mops,speedup,seq_baseline_s,S,MAX_STRIPES,STRIPE_FACTOR" > "${COMBINED}"
fi

build_and_extract () {
  local S="$1" M="$2" F="$3"
  local TAG="S${S}_M${M}_F${F}"
  echo "[build] ${TAG}"
  g++ -std=c++17 -O3 -fopenmp \
      -DSB_DEFAULT_SEGMENTS="${S}" \
      -DAGH_DEFAULT_SEGMENTS="${S}" \
      -DAGH_MAX_STRIPES="${M}" \
      -DAGH_STRIPE_FACTOR="${F}" \
      "${BIN_DIR}/bench_matrix_simple.o" -o "${BIN}"

  local LOG="${OUTDIR}/${TAG}.log"
  local RAW="${OUTDIR}/AGH_raw_${TAG}.csv"

  echo "[run] ${TAG}"
  "${BIN}" --impl=agh > "${LOG}"

  # Extract the CSV block
  awk '/CSV_RESULTS_BEGIN/{f=1;next}/CSV_RESULTS_END/{f=0}f' "${LOG}" > "${RAW}"

  # Filter to the concise slice and append metadata
  awk -F, -v S="${S}" -v M="${M}" -v F="${F}" -v MIX="${MIX}" -v PH="${PH}" '
    NR==1 { next } # skip header in RAW
    $1=="AGH" && $2=="strong" && $3==MIX &&
    ( $4=="uniform" || ($4=="skew" && $9==PH) ) &&
    ( $5=="8" || $5=="16" ) &&
    ( $7=="262144" || $7=="1048576" ) {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
             $1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,S,M,F
    }' "${RAW}" >> "${COMBINED}"

  echo "[done] ${TAG} (raw CSV: ${RAW})"
}

for S in "${SEG_LIST[@]}"; do
  for M in "${MAX_LIST[@]}"; do
    for F in "${FACTOR_LIST[@]}"; do
      build_and_extract "${S}" "${M}" "${F}"
    done
  done
done

echo "[summary] Combined tuning CSV: ${COMBINED}"
echo "[next] Run: python scripts/plot_agh.py"