# Complete Assignment Execution Guide

This guide contains complete steps to fulfill all assignment requirements.

## Assignment Requirements Checklist

### ✅ Completed
1. ✅ Implement concurrent data structure library (4 strategies)
2. ✅ Implement sequential version (Sequential)
3. ✅ Correctness tests
4. ✅ 3 application scenarios (Word Count, Deduplication, Cache Simulation)
5. ✅ Dual implementation comparison (Library vs std::map + Lock)

### 📋 Tasks to Execute

## Step 1: Compile All Programs

```bash
# On CIMS system
cd ~/multicore_lab/concurrent_hash_table

# Compile all programs
make all
```

## Step 2: Run Correctness Tests

```bash
# Run all correctness tests
make run_test
```

You should see all tests pass:
- Coarse-Grained ✓
- Segment-Based ✓
- Fine-Grained ✓
- Lock-Free ✓
- Concurrent tests ✓

## Step 3: Run Basic Performance Benchmark Tests

```bash
# Run benchmark tests (includes all parallel version comparisons)
make run_benchmark
```

This will test:
- Sequential version (baseline)
- Coarse-Grained (coarse-grained locking)
- Fine-Grained (fine-grained locking)
- Segment-Based (segment-based locking)
- Lock-Free (lock-free)

Test different thread counts: 1, 4, 8, 16
Calculate speedup and throughput

## Step 4: Run All Application Scenario Tests

### 4.1 Word Count Scenario (Compute-Intensive)
```bash
make run_all_word_count_benchmarks
```

This will:
- Generate data for 3 scales (Small, Medium, Large)
- Test different thread counts (1, 2, 4, 8, 16)
- Compare Library vs std::map + Lock
- Save results to `word_count/results/`

### 4.2 Deduplication Scenario (Memory-Intensive)
```bash
make run_all_dedup_benchmarks
```

### 4.3 Cache Simulation Scenario (Communication-Intensive)
```bash
make run_all_cache_benchmarks
```

### 4.4 Run All Scenarios
```bash
make run_all_benchmarks
```

This will run all tests for all 3 scenarios, which may take a long time.

## Step 5: Performance Analysis (Profiling)

### 5.1 Test Program Performance Analysis
```bash
# Compile version with profiling
make test_prof

# Run and generate analysis report
make run_prof

# View results
cat profiling_results.txt
```

### 5.2 Benchmark Performance Analysis
```bash
# Compile version with profiling
make benchmark_prof

# Run and generate analysis report
make run_benchmark_prof

# View results
cat benchmark_profiling_results.txt
```

## Step 6: View Result Summary

```bash
# Generate result summary
make summary
```

## Step 7: Analyze Results

### 7.1 View Detailed Results

```bash
# Word Count results
cat word_count/results/results_small.txt
cat word_count/results/results_medium.txt
cat word_count/results/results_large.txt

# Deduplication results
cat deduplication/results/results_small.txt
cat deduplication/results/results_medium.txt
cat deduplication/results/results_large.txt

# Cache Simulation results
cat cache_sim/results/results_small.txt
cat cache_sim/results/results_medium.txt
cat cache_sim/results/results_large.txt
```

### 7.2 Scalability Analysis Points

According to assignment requirements, you need to analyze:

1. **Problem size increases, thread count fixed**
   - Compare Small, Medium, Large results
   - Analysis: Does speedup increase or decrease? Why?

2. **Thread count increases, problem size fixed**
   - Compare 1, 2, 4, 8, 16 thread results
   - Analysis: Is speedup linear growth or diminishing returns? Why?

3. **Comparison of different parallel strategies**
   - Coarse-Grained vs Fine-Grained vs Segment-Based vs Lock-Free
   - Analysis: Which strategy performs best under what conditions?

4. **Library vs std::map + Lock**
   - Analysis: Where are the advantages of the custom library?

## Step 8: Generate Report

Based on collected results, write an analysis report including:

1. **Performance Comparison Tables**
   - Execution time for different implementations
   - Speedup
   - Throughput

2. **Scalability Analysis**
   - Impact of problem size on performance
   - Impact of thread count on performance
   - Performance bottleneck analysis

3. **Performance Analysis Results**
   - Profiling result analysis
   - Hot function identification
   - Optimization suggestions

## Quick Execution of All Tests

If you want to run all tests at once (may take a long time):

```bash
# 1. Compile all programs
make all

# 2. Run correctness tests
make run_test

# 3. Run basic benchmark tests
make run_benchmark > benchmark_results.txt

# 4. Run all application scenario tests (may take a long time)
make run_all_benchmarks

# 5. Performance analysis
make run_prof
make run_benchmark_prof

# 6. View summary
make summary
```

## Result File Locations

- `benchmark_results.txt` - Basic benchmark test results
- `profiling_results.txt` - Test program performance analysis
- `benchmark_profiling_results.txt` - Benchmark performance analysis
- `word_count/results/*.txt` - Word Count scenario results
- `deduplication/results/*.txt` - Deduplication scenario results
- `cache_sim/results/*.txt` - Cache Simulation scenario results

## Notes

1. **Execution Time**: Complete tests may take a long time, recommend running on compute nodes
2. **Resource Usage**: Ensure sufficient CPU cores and memory
3. **Result Saving**: All results are automatically saved to corresponding directories
4. **Error Handling**: If a test fails, check error messages and fix

## Troubleshooting

### If compilation fails
```bash
# Check compiler
which g++
g++ --version

# Check OpenMP support
echo | g++ -fopenmp -E -dM - | grep -i openmp
```

### If runtime errors occur
```bash
# Check file permissions
ls -la test benchmark

# Check dependencies
ldd test
ldd benchmark
```

### If result directories don't exist
```bash
# Manually create
mkdir -p word_count/{data,results}
mkdir -p deduplication/{data,results}
mkdir -p cache_sim/results
```
