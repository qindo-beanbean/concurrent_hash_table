# Concurrent Hash Table — Benchmarks, Scripts, and Results

This repo contains several concurrent hash table implementations (coarse, fine, segment-based, lock-free), a simple benchmark driver, and a scripts/ toolchain that generates the plots used in the report.

This is in fulfillment of the course requirements for Multicore Processors:Architecture & Programming
Authors: tkc8441 & qd2090

---

## Quick start

Build everything (test + benchmark + app binaries):
```bash
make
```

Run the basic unit tests:
```bash
make run_test
```

Run the core benchmark (prints tabular results and a CSV block between CSV_RESULTS_BEGIN/END):
```bash
make run_benchmark
```

Set OpenMP pinning for more stable runs:
```bash
export OMP_PROC_BIND=close
export OMP_PLACES=cores
```

Clean all build outputs and generated data/results:
```bash
make clean
```

---

## Make targets (what they do)

Core
- `test`: build unit tests in test.cpp
- `run_test`: run ./test
- `benchmark`: build the benchmark in benchmark.cpp
- `run_benchmark`: run ./benchmark (writes a CSV block to stdout)

Profiling/sanitizers
- `test_prof` / `run_prof`: gprof build and run for tests (writes profiling_results.txt)
- `benchmark_prof` / `run_benchmark_prof`: gprof build and run for benchmark (writes benchmark_profiling_results.txt)

Application scenarios (optional; one‑line overview)
- `run_all_word_count_benchmarks`, `run_all_dedup_benchmarks`, `run_all_cache_benchmarks`: generate synthetic data and run scenario benchmarks, results saved under each scenario’s results/ folder.

Utilities
- `summary`: prints a short summary from the latest runs if present
- `clean`: removes binaries and generated artifacts (including scenario data/results)

---

## scripts/ — how to turn CSV into figures

All plotting assumes you have CSVs under results/. You can capture the CSV block from `make run_benchmark` by redirecting stdout or grepping the delimited block into a file.

Common CSV schema expected by scripts:
- Required columns (most scripts): `impl, mode, mix, dist, threads, ops, bucket_count, read_ratio, p_hot, time_s, throughput_mops`
- Frequently useful: `speedup, seq_baseline_s, avg_chain, max_chain` (if available)

Recommended env for consistent labeling:
```bash
# Optional; some scripts respect these
export HEADLINE_T=16
export MIX="80/20"
```

Scripts overview (with expected inputs/outputs)

- [scripts/plot_agh_compare.py](scripts/plot_agh_compare.py)
  - Purpose: Compare S2Hash (labeled “AGH” in filenames) vs Fine and Segment across headline slices.


- [scripts/plot_matrix.py](scripts/plot_matrix.py)
  - Purpose: Render the general headline figures from a combined CSV (thread scaling, load factor, skew sweeps).


- [scripts/plot_segments_sweep.py](scripts/plot_segments_sweep.py)
  - Purpose: Segment count sweeps; generates the “seg_curve_...” plots used to pick S.


- [scripts/plot_agh_tuning.py](scripts/plot_agh_tuning.py)
  - Purpose: Tuning/diagnostics for S2Hash (AGH) — e.g., joint (S,K) sweeps, stripe count effects, bar deltas.

- [scripts/plot_speedup.py](scripts/plot_speedup.py)
  - Purpose: Speedup curves normalized to 1-thread baselines per implementation.

- [scripts/sweep_segments.sh](scripts/sweep_segments.sh)
  - Purpose: Automates segment-count sweeps for a chosen implementation; compiles/runs across several `S` and collects a CSV for plotting.
  - Typical usage: edit the S list in the script, run it; it will write a CSV under results/ and invoke `plot_segments_sweep.py`.

- [scripts/tune_agh.sh](scripts/tune_agh.sh)
  - Purpose: Automate S2Hash tuning sweeps (S and K combinations); collects CSVs and calls `plot_agh_tuning.py`.
  - Notes: Ensure the benchmark build can accept the desired S/K (either via compile-time defines or command-line configuration if present).

- [scripts/run_on_machine.sh](scripts/run_on_machine.sh)
  - Purpose: Convenience runner that sets machine-specific OpenMP env, executes a standard set of runs, and saves results to results/.

---

## File structure (short)

Core
- [benchmark.cpp](benchmark.cpp): concurrent hash table benchmark, prints table + CSV block
- [test.cpp](test.cpp): unit tests for basic API and invariants
- [bench_matrix_simple.cpp](bench_matrix_simple.cpp): simple sweeping benchmark to produce matrix CSVs

Implementations (headers)
- [sequential.h](sequential.h): single-threaded chained hash table
- [coarse_grained.h](coarse_grained.h), [coarse_grained_padded.h](coarse_grained_padded.h): global-lock variants
- [fine_grained.h](fine_grained.h), [fine_grained_padded.h](fine_grained_padded.h): per-bucket lock variants (includes `increment(key, delta)` helper)
- [segment_based.h](segment_based.h), [segment_based_padded.h](segment_based_padded.h): segment-level locking
- [lock_free.h](lock_free.h): lock-free linked-list variant (subset features)
- [agh_hash_table.h](agh_hash_table.h): experimental S2Hash-related header
- [common.h](common.h): shared types and hashing
- [hotset.h](hotset.h): hot-set skew generator

Scenarios (optional; one-line)
- [word_count/](word_count), [deduplication/](deduplication), [cache_sim/](cache_sim): simple application drivers to illustrate usage and scaling, each with a generator, a library-backed variant, a baseline/benchmark, and results/ folders.

Scripts
- See the detailed “scripts/” section above for each script’s inputs/outputs and expected filenames.

Results
- [results/](results/): place CSV inputs here; scripts will write figures to subfolders like `results/figs_agh_compare/`.

---

