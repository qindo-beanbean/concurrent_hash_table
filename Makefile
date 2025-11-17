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

# Compile with profiling support (gprof)
test_prof: test.cpp
	$(CXX) -pg -std=c++17 -fopenmp -O3 -Wall -Wextra test.cpp -o test_prof

benchmark_prof: benchmark.cpp
	$(CXX) -pg -std=c++17 -fopenmp -O3 -Wall -Wextra benchmark.cpp -o benchmark_prof

# Run test
run_test: test
	./test

# Run benchmark
run_benchmark: benchmark
	./benchmark

# Run TSan test
run_tsan: test_tsan
	./test_tsan

# Run profiling
run_prof: test_prof
	./test_prof
	gprof test_prof gmon.out > profiling_results.txt
	@echo "Profiling results saved to profiling_results.txt"

run_benchmark_prof: benchmark_prof
	./benchmark_prof
	gprof benchmark_prof gmon.out > benchmark_profiling_results.txt
	@echo "Benchmark profiling results saved to benchmark_profiling_results.txt"

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
	@mkdir -p word_count/data word_count/results
	@echo "Generating test data..."
	word_count/generate_test_data word_count/data/test_small.txt 50000 500
	word_count/generate_test_data word_count/data/test_medium.txt 200000 2000
	word_count/generate_test_data word_count/data/test_large.txt 1000000 10000
	@echo "Running benchmarks..."
	word_count/word_count_benchmark word_count/data/test_small.txt 1 2 4 8 > word_count/results/results_small.txt
	word_count/word_count_benchmark word_count/data/test_medium.txt 1 2 4 8 > word_count/results/results_medium.txt
	word_count/word_count_benchmark word_count/data/test_large.txt 1 2 4 8 > word_count/results/results_large.txt
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
	@mkdir -p deduplication/data deduplication/results
	@echo "Generating test data..."
	deduplication/generate_dedup_data deduplication/data/data_small.txt 50000 500
	deduplication/generate_dedup_data deduplication/data/data_medium.txt 200000 2000
	deduplication/generate_dedup_data deduplication/data/data_large.txt 1000000 10000
	@echo "Running benchmarks..."
	deduplication/deduplication_benchmark deduplication/data/data_small.txt 1 2 4 8 > deduplication/results/results_small.txt
	deduplication/deduplication_benchmark deduplication/data/data_medium.txt 1 2 4 8 > deduplication/results/results_medium.txt
	deduplication/deduplication_benchmark deduplication/data/data_large.txt 1 2 4 8 > deduplication/results/results_large.txt
	@echo "Done! Results saved to deduplication/results/*.txt"

# Run all cache simulation benchmarks
run_all_cache_benchmarks: $(CACHE_SIM_TARGETS)
	@mkdir -p cache_sim/results
	@echo "Running benchmarks..."
	cache_sim/cache_sim_benchmark 500000 5000 0.8 1 2 4 8 > cache_sim/results/results_small.txt
	cache_sim/cache_sim_benchmark 2000000 20000 0.8 1 2 4 8 > cache_sim/results/results_medium.txt
	cache_sim/cache_sim_benchmark 5000000 50000 0.8 1 2 4 8 > cache_sim/results/results_large.txt
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

# Create results summary
summary:
	@echo "====================================="
	@echo "  Results Summary" 
	@echo "====================================="
	@echo ""
	@if [ -f benchmark ]; then \
		echo "=== Basic Benchmark ==="; \
		./benchmark 2>&1 | tail -20; \
		echo ""; \
	fi
	@if [ -d word_count/results ]; then \
		echo "=== Word Count Results ==="; \
		for f in word_count/results/results_*.txt; do \
			if [ -f "$$f" ]; then \
				echo "File: $$f"; \
				tail -10 "$$f"; \
				echo ""; \
			fi; \
		done; \
	fi
	@if [ -d deduplication/results ]; then \
		echo "=== Deduplication Results ==="; \
		for f in deduplication/results/results_*.txt; do \
			if [ -f "$$f" ]; then \
				echo "File: $$f"; \
				tail -10 "$$f"; \
				echo ""; \
			fi; \
		done; \
	fi
	@if [ -d cache_sim/results ]; then \
		echo "=== Cache Simulation Results ==="; \
		for f in cache_sim/results/results_*.txt; do \
			if [ -f "$$f" ]; then \
				echo "File: $$f"; \
				tail -10 "$$f"; \
				echo ""; \
			fi; \
		done; \
	fi

.PHONY: all clean run_test run_benchmark run_tsan run_prof run_benchmark_prof run_all_benchmarks run_all_word_count_benchmarks run_all_dedup_benchmarks run_all_cache_benchmarks summary