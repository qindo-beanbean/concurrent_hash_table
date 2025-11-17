# Cache Simulation Application Scenario Usage Guide

## Overview

This is the third application scenario: **Cache Simulation**, which is a **communication-intensive** program.

## Characteristics

- **Communication-intensive**: Multiple threads frequently access shared cache data structures
- **High frequency operations**: Large number of mixed read/write operations
- **Real scenario simulation**: Simulates actual cache system behavior
- **Mixed load**: Mix of read and write operations

## File Description

- `cache_sim_library.cpp` - Version using concurrent hash table library
- `cache_sim_std.cpp` - Comparison version using std::unordered_map + lock
- `cache_sim_benchmark.cpp` - Performance comparison test program
- `cache_sim_common.h` - Shared utility functions

## Building

```bash
# Build from project root
make cache_sim/cache_sim_library
make cache_sim/cache_sim_std
make cache_sim/cache_sim_benchmark

# Or build all
make all
```

## Usage Steps

### 1. Run Individual Program

**Version using library:**
```bash
./cache_sim_library 1000000 10000 0.8 4
# Parameters: <num_operations> <key_range> <read_ratio> <num_threads>
```

**Version using std::map:**
```bash
./cache_sim_std 1000000 10000 0.8 4
# Parameters: <num_operations> <key_range> <read_ratio> <num_threads>
```

### 2. Run Performance Comparison Tests

```bash
# Using default thread counts (1, 2, 4, 8, 16)
./cache_sim_benchmark 1000000 10000 0.8

# Specify thread counts
./cache_sim_benchmark 1000000 10000 0.8 1 2 4 8
```

## Parameter Description

- **num_operations**: Total number of cache operations to execute
- **key_range**: Range of cache key values (affects cache hit rate)
- **read_ratio**: Ratio of read operations (0.0-1.0), e.g., 0.8 means 80% read operations, 20% write operations
- **num_threads**: Number of threads for parallel execution

## Performance Metrics

The program outputs:
- **Total operations**: Total number of cache operations executed
- **Cache hits**: Number of operations that successfully found cache
- **Cache misses**: Number of operations that did not find cache
- **Hit ratio**: Cache hit rate percentage
- **Execution time**: Seconds
- **Throughput**: Million operations/second
- **Speedup**: Speedup relative to single thread

## Different Problem Scale Testing

According to assignment requirements, different scales need to be tested:

```bash
# Small scale (1M operations, 10K key range)
./cache_sim_benchmark 1000000 10000 0.8 1 2 4 8 16 > results/results_small.txt

# Medium scale (10M operations, 100K key range)
./cache_sim_benchmark 10000000 100000 0.8 1 2 4 8 16 > results/results_medium.txt

# Large scale (50M operations, 500K key range)
./cache_sim_benchmark 50000000 500000 0.8 1 2 4 8 16 > results/results_large.txt
```

## Expected Results

The version using concurrent hash table library should have better:
- **Concurrent performance**: Higher throughput under multi-threading
- **Scalability**: More obvious performance improvement as thread count increases
- **Communication efficiency**: Fine-grained locks reduce inter-thread communication overhead

## Notes

1. Read ratio affects performance: More read operations may lead to more severe lock contention
2. Key range affects hit rate: Smaller key range leads to higher hit rate
3. It is recommended to run multiple times and take the average for more accurate results
4. Communication-intensive scenarios are sensitive to lock contention, results may be affected by hardware
