# Use real GCC (prefer g++-11, fallback to g++ if not available)
CXX := $(shell which g++-11 2>/dev/null || which g++ 2>/dev/null || echo "g++")

# GCC supports standard -fopenmp
CXXFLAGS = -std=c++17 -fopenmp -O3 -Wall -Wextra
SANITIZE = -fsanitize=thread -g

# Target files
TARGETS = test benchmark 

all: $(TARGETS)

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

# Clean
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean run_test run_benchmark run_tsan