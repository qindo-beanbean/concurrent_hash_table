# Deduplication Application Scenario Usage Guide

## Overview

This is the second application scenario: **Deduplication**, which is a **memory-intensive** program.

## Characteristics

- **Memory-intensive**: Requires frequent access to hash tables in memory
- **High concurrent access**: Multiple threads simultaneously access shared data structures
- **Fast lookup**: Uses hash tables to quickly determine if elements already exist

## File Description

- `deduplication_library.cpp` - Version using concurrent hash table library
- `deduplication_std.cpp` - Comparison version using std::unordered_set + lock
- `deduplication_benchmark.cpp` - Performance comparison test program
- `deduplication_common.h` - Shared utility functions
- `generate_dedup_data.cpp` - Test data generator

## Building

```bash
# Build from project root
make deduplication/deduplication_library
make deduplication/deduplication_std
make deduplication/deduplication_benchmark
make deduplication/generate_dedup_data

# Or build all
make all
```

## Usage Steps

### 1. Generate Test Data

```bash
# Small scale test (100K items, 1K unique values)
./generate_dedup_data data/data_small.txt 100000 1000

# Medium scale test (1M items, 10K unique values)
./generate_dedup_data data/data_medium.txt 1000000 10000

# Large scale test (10M items, 50K unique values)
./generate_dedup_data data/data_large.txt 10000000 50000
```

### 2. Run Individual Program

**Version using library:**
```bash
./deduplication_library data/data_small.txt 4
# Parameters: <input_file> <num_threads>
```

**Version using std::set:**
```bash
./deduplication_std data/data_small.txt 4
# Parameters: <input_file> <num_threads>
```

### 3. Run Performance Comparison Tests

```bash
# Using default thread counts (1, 2, 4, 8, 16)
./deduplication_benchmark data/data_small.txt

# Specify thread counts
./deduplication_benchmark data/data_small.txt 1 2 4 8
```

## Performance Metrics

The program outputs:
- **Total items**: Total number of items processed
- **Unique items**: Number of items after deduplication
- **Execution time**: Seconds
- **Throughput**: Million items/second
- **Speedup**: Speedup relative to single thread

## Expected Results

The version using concurrent hash table library should have better:
- **Concurrent performance**: Higher throughput under multi-threading
- **Scalability**: More obvious performance improvement as thread count increases
- **Memory access efficiency**: Fine-grained locks reduce lock contention

## Different Problem Scale Testing

```bash
# Small scale
./deduplication_benchmark data/data_small.txt 1 2 4 8 16 > results/results_small.txt

# Medium scale
./deduplication_benchmark data/data_medium.txt 1 2 4 8 16 > results/results_medium.txt

# Large scale
./deduplication_benchmark data/data_large.txt 1 2 4 8 16 > results/results_large.txt
```

## Notes

1. Ensure sufficient memory to load test files
2. Large scale tests may take a long time
3. It is recommended to run multiple times and take the average for more accurate results
4. Deduplication scenarios are sensitive to memory bandwidth, results may be affected by hardware
