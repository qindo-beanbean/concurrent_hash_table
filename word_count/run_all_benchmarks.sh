#!/bin/bash
# Script to run all scale performance tests

echo "====================================="
echo "  Running All Scale Benchmarks"
echo "====================================="
echo ""

# Define test scales (reduced for faster testing)
# Small scale: 50K words, 500 unique words
# Medium scale: 200K words, 2K unique words  
# Large scale: 1M words, 10K unique words

echo "Step 1: Generating test data..."
echo ""

# Generate test data
echo "Generating small scale data (50K words, 500 unique)..."
./generate_test_data data/test_small.txt 50000 500

echo "Generating medium scale data (200K words, 2K unique)..."
./generate_test_data data/test_medium.txt 200000 2000

echo "Generating large scale data (1M words, 10K unique)..."
./generate_test_data data/test_large.txt 1000000 10000

echo ""
echo "Step 2: Running benchmarks..."
echo ""

# Run benchmarks (reduced thread counts: 1 2 4 8 instead of 1 2 4 8 16)
echo "=== Small Scale Benchmark ===" > results/results_small.txt
./word_count_benchmark data/test_small.txt 1 2 4 8 >> results/results_small.txt

echo "=== Medium Scale Benchmark ===" > results/results_medium.txt
./word_count_benchmark data/test_medium.txt 1 2 4 8 >> results/results_medium.txt

echo "=== Large Scale Benchmark ===" > results/results_large.txt
./word_count_benchmark data/test_large.txt 1 2 4 8 >> results/results_large.txt

echo ""
echo "====================================="
echo "  All benchmarks completed!"
echo "====================================="
echo ""
echo "Results saved to:"
echo "  - results/results_small.txt"
echo "  - results/results_medium.txt"
echo "  - results/results_large.txt"
echo ""

