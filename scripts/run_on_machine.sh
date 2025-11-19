#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <impl: coarse|fine|segment|lockfree|agh>" >&2
  exit 1
fi

IMPL="$1"

g++ -std=c++17 -O3 -fopenmp ../bench_matrix_simple.cpp -o ../bench_matrix_simple

export OMP_PROC_BIND=close
export OMP_PLACES=cores

OUT="../results/${IMPL}_matrix.out"
CSV="../results/${IMPL}_matrix.csv"

./../bench_matrix_simple --impl="${IMPL}" | tee "${OUT}"

awk '/CSV_RESULTS_BEGIN/{f=1;next}/CSV_RESULTS_END/{f=0}f' "${OUT}" > "${CSV}"
echo "Wrote ${CSV} (full log ${OUT})"