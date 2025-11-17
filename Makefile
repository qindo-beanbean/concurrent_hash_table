# Use real GCC (prefer g++-11, fallback to g++ if not available)
CXX := $(shell which g++-11 2>/dev/null || which g++ 2>/dev/null || echo "g++")

# GCC supports standard -fopenmp
CXXFLAGS = -std=c++17 -fopenmp -O3 -Wall -Wextra
SANITIZE = -fsanitize=thread -g

# Target files
TARGETS = test benchmark
WORD_COUNT_TARGETS = word_count/word_count_library word_count/word_count_std word_count/word_count_benchmark word_count/generate_test_data
DEDUP_TARGETS = deduplication/deduplication_library deduplication/deduplication_std deduplication/deduplication_benchmark deduplication/generate_dedup_data
CACHE_SIM_TARGETS = cache_sim/cache_sim_library cache_sim/cache_sim_std cache_sim/cache_sim_benchmark

all: $(TARGETS) $(WORD_COUNT_TARGETS) $(DEDUP_TARGETS) $(CACHE_SIM_TARGETS)

# Compile test program
test: test.cpp
	$(CXX) $(CXXFLAGS) test.cpp -o test

# Compile benchmark
benchmark: benchmark.cpp
	$(CXX) $(CXXFLAGS) benchmark.cpp -o benchmark

# Compile test with Thread Sanitizer
test_tsan: test.cpp
	$(CXX) $(CXXFLAGS) $(SANITIZE) test.cpp -o test_tsan

# Run test
run_test: test
	./test

# Run benchmark
run_benchmark: benchmark
	./benchmark

# Run TSan test
run_tsan: test_tsan
	./test_tsan

# Compile word count programs
word_count/word_count_library: word_count/word_count_library.cpp
	$(CXX) $(CXXFLAGS) -I. word_count/word_count_library.cpp -o word_count/word_count_library

word_count/word_count_std: word_count/word_count_std.cpp
	$(CXX) $(CXXFLAGS) -I. word_count/word_count_std.cpp -o word_count/word_count_std

word_count/word_count_benchmark: word_count/word_count_benchmark.cpp
	$(CXX) $(CXXFLAGS) -I. word_count/word_count_benchmark.cpp -o word_count/word_count_benchmark

word_count/generate_test_data: word_count/generate_test_data.cpp
	$(CXX) $(CXXFLAGS) word_count/generate_test_data.cpp -o word_count/generate_test_data

# Compile deduplication programs
deduplication/deduplication_library: deduplication/deduplication_library.cpp
	$(CXX) $(CXXFLAGS) -I. deduplication/deduplication_library.cpp -o deduplication/deduplication_library

deduplication/deduplication_std: deduplication/deduplication_std.cpp
	$(CXX) $(CXXFLAGS) -I. deduplication/deduplication_std.cpp -o deduplication/deduplication_std

deduplication/deduplication_benchmark: deduplication/deduplication_benchmark.cpp
	$(CXX) $(CXXFLAGS) -I. deduplication/deduplication_benchmark.cpp -o deduplication/deduplication_benchmark

deduplication/generate_dedup_data: deduplication/generate_dedup_data.cpp
	$(CXX) $(CXXFLAGS) deduplication/generate_dedup_data.cpp -o deduplication/generate_dedup_data

# Compile cache simulation programs
cache_sim/cache_sim_library: cache_sim/cache_sim_library.cpp
	$(CXX) $(CXXFLAGS) -I. cache_sim/cache_sim_library.cpp -o cache_sim/cache_sim_library

cache_sim/cache_sim_std: cache_sim/cache_sim_std.cpp
	$(CXX) $(CXXFLAGS) -I. cache_sim/cache_sim_std.cpp -o cache_sim/cache_sim_std

cache_sim/cache_sim_benchmark: cache_sim/cache_sim_benchmark.cpp
	$(CXX) $(CXXFLAGS) -I. cache_sim/cache_sim_benchmark.cpp -o cache_sim/cache_sim_benchmark

# Run all word count benchmarks
run_all_word_count_benchmarks: $(WORD_COUNT_TARGETS)
	@echo "Generating test data..."
	word_count/generate_test_data word_count/data/test_small.txt 100000 1000
	word_count/generate_test_data word_count/data/test_medium.txt 1000000 10000
	word_count/generate_test_data word_count/data/test_large.txt 10000000 50000
	@echo "Running benchmarks..."
	word_count/word_count_benchmark word_count/data/test_small.txt 1 2 4 8 16 > word_count/results/results_small.txt
	word_count/word_count_benchmark word_count/data/test_medium.txt 1 2 4 8 16 > word_count/results/results_medium.txt
	word_count/word_count_benchmark word_count/data/test_large.txt 1 2 4 8 16 > word_count/results/results_large.txt
	@echo "Done! Results saved to word_count/results/*.txt"

# Run all benchmarks (all three scenarios)
run_all_benchmarks: run_all_word_count_benchmarks run_all_dedup_benchmarks run_all_cache_benchmarks
	@echo ""
	@echo "====================================="
	@echo "  All scenarios completed!"
	@echo "====================================="
	@echo "Results saved to:"
	@echo "  - word_count/results/*.txt"
	@echo "  - deduplication/results/*.txt"
	@echo "  - cache_sim/results/*.txt"

# Run all deduplication benchmarks
run_all_dedup_benchmarks: $(DEDUP_TARGETS)
	@echo "Generating test data..."
	deduplication/generate_dedup_data deduplication/data/data_small.txt 100000 1000
	deduplication/generate_dedup_data deduplication/data/data_medium.txt 1000000 10000
	deduplication/generate_dedup_data deduplication/data/data_large.txt 10000000 50000
	@echo "Running benchmarks..."
	deduplication/deduplication_benchmark deduplication/data/data_small.txt 1 2 4 8 16 > deduplication/results/results_small.txt
	deduplication/deduplication_benchmark deduplication/data/data_medium.txt 1 2 4 8 16 > deduplication/results/results_medium.txt
	deduplication/deduplication_benchmark deduplication/data/data_large.txt 1 2 4 8 16 > deduplication/results/results_large.txt
	@echo "Done! Results saved to deduplication/results/*.txt"

# Run all cache simulation benchmarks
run_all_cache_benchmarks: $(CACHE_SIM_TARGETS)
	@echo "Running benchmarks..."
	cache_sim/cache_sim_benchmark 1000000 10000 0.8 1 2 4 8 16 > cache_sim/results/results_small.txt
	cache_sim/cache_sim_benchmark 10000000 100000 0.8 1 2 4 8 16 > cache_sim/results/results_medium.txt
	cache_sim/cache_sim_benchmark 50000000 500000 0.8 1 2 4 8 16 > cache_sim/results/results_large.txt
	@echo "Done! Results saved to cache_sim/results/*.txt"

# Clean
clean:
	rm -f $(TARGETS) *.o *.exe
	rm -f word_count/*.exe word_count/*.o
	rm -f word_count/data/*.txt word_count/results/*.txt
	rm -f deduplication/*.exe deduplication/*.o
	rm -f deduplication/data/*.txt deduplication/results/*.txt
	rm -f cache_sim/*.exe cache_sim/*.o
	rm -f cache_sim/results/*.txt

.PHONY: all clean run_test run_benchmark run_tsan run_all_benchmarks run_all_word_count_benchmarks run_all_dedup_benchmarks run_all_cache_benchmarks