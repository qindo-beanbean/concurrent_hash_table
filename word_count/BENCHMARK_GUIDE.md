# Performance Testing Guide

## Assignment Requirements

According to the assignment requirement: "Measure the performance with different problem sizes", **at least 3 different scales** of data need to be tested.

## Recommended Test Scales

### 1. Small Scale
- **Word count**: 100,000
- **Unique words**: 1,000
- **File size**: Approximately 0.7 MB
- **Purpose**: Quickly verify program correctness, test basic performance

### 2. Medium Scale
- **Word count**: 1,000,000
- **Unique words**: 10,000
- **File size**: Approximately 7 MB
- **Purpose**: Demonstrate performance under medium load

### 3. Large Scale
- **Word count**: 10,000,000
- **Unique words**: 50,000
- **File size**: Approximately 70 MB
- **Purpose**: Demonstrate scalability and performance under high load

## Test Thread Counts

It is recommended to test the following thread counts:
- 1 thread (baseline)
- 2 threads
- 4 threads
- 8 threads
- 16 threads (if CPU supports)

## Quick Testing Method

### Windows Users

```powershell
# Run all tests
.\run_all_benchmarks.bat
```

### Linux/Mac Users

```bash
chmod +x run_all_benchmarks.sh
./run_all_benchmarks.sh
```

### Manual Testing

```bash
# 1. Generate test data
./generate_test_data test_small.txt 100000 1000
./generate_test_data test_medium.txt 1000000 10000
./generate_test_data test_large.txt 10000000 50000

# 2. Run benchmarks
./word_count_benchmark test_small.txt 1 2 4 8 16 > results_small.txt
./word_count_benchmark test_medium.txt 1 2 4 8 16 > results_medium.txt
./word_count_benchmark test_large.txt 1 2 4 8 16 > results_large.txt
```

## Data to Collect

For each scale and each thread count, record:

1. **Execution time** (seconds)
2. **Throughput** (million operations/second)
3. **Speedup** (relative to single thread)
4. **Memory usage** (optional, but helpful for analysis)

## Data Analysis Points

### 1. Scalability Analysis
- Performance improvement as thread count increases under different scales
- Whether performance bottlenecks exist (e.g., performance no longer improves after thread count exceeds a certain value)

### 2. Scale Impact
- Performance differences between small, medium, and large scales
- Whether concurrent advantages are more obvious as scale increases

### 3. Implementation Comparison
- Performance differences between using library vs not using library
- Whether differences are consistent across different scales

## Presentation in Report

It is recommended to include in the report:

1. **Tables**: Performance data under different scales and thread counts
2. **Charts**:
   - Throughput vs thread count (different scales)
   - Speedup vs thread count (different scales)
   - Execution time vs problem scale (different thread counts)
3. **Analysis**:
   - Why performance is better/worse under certain scales
   - Factors limiting scalability
   - Advantages and disadvantages of different implementations

## Notes

1. **Multiple runs**: Run each test at least 3 times and take the average
2. **System load**: Ensure low system load during testing
3. **Memory limits**: Large scale tests require sufficient memory
4. **Time cost**: Large scale tests may take a long time

## Expected Results

- **Small scale**: May not show obvious differences, mainly for correctness verification
- **Medium scale**: Start to see concurrent advantages
- **Large scale**: Concurrent advantages are most obvious, performance differences are largest
