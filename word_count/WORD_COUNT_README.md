# Word Count Application Scenario Usage Guide

## Overview

This is the first application scenario: **Word Count**, which is a **compute-intensive** program.

## File Description

- `word_count_library.cpp` - Version using concurrent hash table library
- `word_count_std.cpp` - Comparison version using std::unordered_map + lock
- `word_count_benchmark.cpp` - Performance comparison test program
- `word_count_common.h` - Shared utility functions
- `generate_test_data.cpp` - Test data generator

## Building

```bash
# Build all programs
make all

# Or build individually
make word_count_library
make word_count_std
make word_count_benchmark
make generate_test_data
```

## Usage Steps

### 1. Generate Test Data

```bash
# Small scale test (100K words, 1K unique words)
./generate_test_data test_small.txt 100000 1000

# Medium scale test (1M words, 10K unique words)
./generate_test_data test_medium.txt 1000000 10000

# Large scale test (10M words, 50K unique words)
./generate_test_data test_large.txt 10000000 50000
```

### 2. Run Individual Program

**Version using library:**
```bash
./word_count_library test_small.txt 4
# Parameters: <input_file> <num_threads>
```

**Version using std::map:**
```bash
./word_count_std test_small.txt 4
# Parameters: <input_file> <num_threads>
```

### 3. Run Performance Comparison Tests

```bash
# Using default thread counts (1, 2, 4, 8, 16)
./word_count_benchmark test_small.txt

# Specify thread counts
./word_count_benchmark test_small.txt 1 2 4 8
```

## Performance Metrics

The program outputs:
- **Total words**: Total number of words processed
- **Unique words**: Number of different words
- **Execution time**: Seconds
- **Throughput**: Million words/second
- **Speedup**: Speedup relative to single thread

## Expected Results

The version using concurrent hash table library should have better:
- **Concurrent performance**: Higher throughput under multi-threading
- **Scalability**: More obvious performance improvement as thread count increases

## Different Problem Scale Testing

According to assignment requirements, different scales need to be tested:

```bash
# Small scale
./word_count_benchmark test_small.txt 1 2 4 8 16 > results_small.txt

# Medium scale
./word_count_benchmark test_medium.txt 1 2 4 8 16 > results_medium.txt

# Large scale
./word_count_benchmark test_large.txt 1 2 4 8 16 > results_large.txt
```

## Notes

1. Ensure sufficient memory to load test files
2. Large scale tests may take a long time
3. It is recommended to run multiple times and take the average for more accurate results
