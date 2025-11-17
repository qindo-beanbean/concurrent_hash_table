# CIMS System Running Guide

## 1. Check Environment

First check if compiler and OpenMP are available:

```bash
# Check compiler
which g++ g++-11

# Check OpenMP support
g++ --version
echo | g++ -fopenmp -E -dM - | grep -i openmp
```

## 2. Compile Project

### Compile Basic Tests
```bash
make test
make benchmark
```

### Compile All Programs
```bash
make all
```

### If Compilation Fails, May Need to Load Modules
```bash
# Check available modules
module avail

# May need to load gcc or openmp modules (depending on system configuration)
module load gcc
# or
module load gcc/11.2.0
```

## 3. Run Tests

### Run Correctness Tests
```bash
make run_test
# or run directly
./test
```

### Run Performance Benchmark Tests
```bash
make run_benchmark
# or run directly
./benchmark
```

## 4. Run Application Scenario Tests

### Word Count Scenario
```bash
cd word_count

# Generate test data
./generate_test_data data/test_small.txt 100000 1000

# Run single test
./word_count_library data/test_small.txt 4

# Run comparison tests
./word_count_benchmark data/test_small.txt 1 2 4 8 16

# Run all scale tests (if script has execute permission)
chmod +x run_all_benchmarks.sh
./run_all_benchmarks.sh
```

### Run All Tests from Project Root
```bash
# In project root directory
make run_all_word_count_benchmarks
make run_all_dedup_benchmarks
make run_all_cache_benchmarks

# or run all
make run_all_benchmarks
```

## 5. Common Issue Troubleshooting

### Issue 1: OpenMP Not Found
If you see `fatal error: omp.h: No such file or directory`

Solution:
```bash
# Check OpenMP library
g++ -fopenmp -v 2>&1 | grep -i openmp

# If system uses modules, load appropriate module
module load gcc
# or
module load openmp
```

### Issue 2: Linking Errors
If you encounter linking errors, may need to specify library path:
```bash
# Check library path
echo $LD_LIBRARY_PATH

# May need to set
export LD_LIBRARY_PATH=/usr/lib64:$LD_LIBRARY_PATH
```

### Issue 3: Permission Problems
If unable to execute, check permissions:
```bash
chmod +x test benchmark
chmod +x word_count/*.sh
```

## 6. Running on Compute Nodes (If More Resources Needed)

If you need to run on compute nodes (instead of access nodes):

```bash
# Request interactive node
srun --pty bash

# or submit job (depending on system job scheduler)
# SLURM example:
sbatch --job-name=hash_test --time=01:00:00 --cpus-per-task=16 <<EOF
#!/bin/bash
cd $PWD
make run_all_benchmarks
EOF
```

## 7. Check Results

Test results will be saved to:
- `word_count/results/` - Word count test results
- `deduplication/results/` - Deduplication test results  
- `cache_sim/results/` - Cache simulation test results

View results:
```bash
cat word_count/results/results_small.txt
cat word_count/results/results_medium.txt
cat word_count/results/results_large.txt
```
