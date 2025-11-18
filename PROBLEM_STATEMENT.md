# Problem Statement: Lock Granularity and Cache-Aware Optimization for a Hash-Table–Centric OpenMP Workload

## Goal (Category 1: Parallelizing an Application)
Parallelize and optimize a key/value analytics workload whose dominant kernel is a shared hash table. Quantify how lock granularity and cache-aware layout impact strong/weak scaling under uniform and skewed access, using OpenMP to generate concurrency.

## Core Question
How do lock granularity (global vs per-bucket vs segmented) and cache-aware layout (e.g., cache-line padding) affect throughput, speedup, and predictability of a hash-table–centric workload as threads and workload characteristics vary?

## Implementations Under Test
- SequentialHashTable (baseline, no synchronization)
- CoarseGrainedHashTable (global lock)
- FineGrainedHashTable (per-bucket locks)
- SegmentBasedHashTable (segment-scoped locking; tunable number of segments)
- Padded variants (alignas(64) on lock holders) as a cache-aware tweak

Optional: lock striping (M locks < buckets), batch inserts.

## Workload Model (the “application”)
Two-phase pipeline:
1) Build phase: bulk inserts
2) Mixed phase: reads and writes with configurable mix (80/20, 50/50)

Access distributions:
- Uniform
- Skewed hot-set (e.g., 90% requests to 10% keys), with skew sweep (p_hot ∈ {0.7, 0.9, 0.99})

Problem sizes and scaling:
- Strong scaling: fixed total ops, threads ∈ {1,2,4,8,16}
- Weak scaling: ops per thread fixed (e.g., 250k × threads)
- Load factor sweep via bucket_count ∈ {8K, 16K, 64K}

## Hypotheses (grounded in literature)
- H1: Finer-grained locking improves strong scaling until memory/allocator limits dominate; segmentation lands between coarse and fine if segments are sufficiently many.
- H2: Skew amplifies contention, widening the gap between coarse and finer-grained designs; segmentation improves predictability under skew.
- H3: Cache-aware padding helps only when false sharing is present; otherwise the effect is neutral or negative.
- H4: Lower load factor reduces time spent in critical sections (shorter chains), improving parallel throughput.

Reference motivation: SEPH (OSDI’23) emphasizes predictable scalability via segmented structures and minimal writes; Vec-HT (ECOOP’23) shows batch/locality benefits (conceptual guidance)  
- SEPH: https://www.usenix.org/conference/osdi23/presentation/wang-chao  
- Vec-HT: https://drops.dagstuhl.de/storage/00lipics/lipics-vol263-ecoop2023/LIPIcs.ECOOP.2023.27/LIPIcs.ECOOP.2023.27.pdf

## Metrics and Methods
- Throughput (ops/s), speedup vs sequential baseline, scaling curves (strong/weak)
- Skew sensitivity; load-factor sensitivity (avg/max bucket length)
- Profiling: gprof (sequential hotspots); perf stat (cycles, instructions, LLC misses) for select runs

## Success Criteria (Deliverables)
- Clear speedup over sequential and rigorous comparisons among parallel designs
- Strong/weak scaling plots under uniform and skewed loads; load-factor analysis
- Brief profiling to explain bottlenecks (contention, traversal, cache)
- Lessons learned: when fine/segment wins vs overheads; alignment impact; guidelines applicable to other shared-memory structures

## Out of Scope
- SIMD vectorization and full batch APIs
- Persistent memory or NUMA-aware resizing
- Fully lock-free production-grade design