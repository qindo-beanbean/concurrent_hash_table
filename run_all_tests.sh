#!/bin/bash
# Complete test execution script for the concurrent hash table project
# This script runs all required tests and benchmarks

set -e  # Exit on error

echo "====================================="
echo "  Concurrent Hash Table - Complete Test Suite"
echo "====================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to print section header
print_section() {
    echo ""
    echo -e "${GREEN}=====================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}=====================================${NC}"
    echo ""
}

# Function to check if command succeeded
check_result() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Success${NC}"
    else
        echo -e "${RED}✗ Failed${NC}"
        exit 1
    fi
}

# Step 1: Compile all programs
print_section "Step 1: Compiling all programs"
make all
check_result

# Step 2: Run correctness tests
print_section "Step 2: Running correctness tests"
make run_test
check_result

# Step 3: Run basic benchmark
print_section "Step 3: Running basic performance benchmark"
make run_benchmark | tee benchmark_results.txt
check_result

# Step 4: Run all application scenario benchmarks
print_section "Step 4: Running all application scenario benchmarks"
echo "This may take a while..."
make run_all_benchmarks
check_result

# Step 5: Performance profiling
print_section "Step 5: Running performance profiling"
echo "Profiling test program..."
make test_prof
./test_prof > /dev/null 2>&1
gprof test_prof gmon.out > profiling_results.txt 2>/dev/null || echo "gprof not available, skipping profiling"
check_result

echo "Profiling benchmark program..."
make benchmark_prof
./benchmark_prof > /dev/null 2>&1
gprof benchmark_prof gmon.out > benchmark_profiling_results.txt 2>/dev/null || echo "gprof not available, skipping profiling"
check_result

# Step 6: Generate summary
print_section "Step 6: Generating results summary"
make summary | tee results_summary.txt
check_result

# Final summary
print_section "All tests completed!"
echo ""
echo "Results saved to:"
echo "  - benchmark_results.txt"
echo "  - profiling_results.txt"
echo "  - benchmark_profiling_results.txt"
echo "  - results_summary.txt"
echo "  - word_count/results/*.txt"
echo "  - deduplication/results/*.txt"
echo "  - cache_sim/results/*.txt"
echo ""
echo -e "${GREEN}✓ All tasks completed successfully!${NC}"

