# Concurrent Hash Table Project

## Project Structure

```
concurrent_hash_table/
├── common.h                 # Common definitions and utilities
├── sequential.h             # Sequential hash table implementation (for comparison)
├── coarse_grained.h         # Coarse-grained locking implementation
├── fine_grained.h           # Fine-grained locking implementation
├── segment_based.h          # Segment-based locking implementation
├── lock_free.h              # Lock-free implementation
├── test.cpp                 # Correctness tests
├── benchmark.cpp            # Basic performance tests
├── Makefile                 # Build file
│
├── word_count/              # Word count application scenario (compute-intensive)
│   ├── word_count_library.cpp      # Version using library
│   ├── word_count_std.cpp          # Comparison version using std::map
│   ├── word_count_benchmark.cpp    # Performance comparison tests
│   ├── word_count_common.h          # Shared utility functions
│   ├── generate_test_data.cpp      # Test data generator
│   ├── WORD_COUNT_README.md         # Word count usage instructions
│   ├── BENCHMARK_GUIDE.md           # Performance testing guide
│   ├── run_all_benchmarks.bat       # Windows batch script
│   ├── data/                        # Test data directory
│   └── results/                      # Test results directory
│
├── deduplication/           # Data deduplication application scenario (memory-intensive)
│   ├── deduplication_library.cpp    # Version using library
│   ├── deduplication_std.cpp        # Comparison version using std::set
│   ├── deduplication_benchmark.cpp  # Performance comparison tests
│   ├── deduplication_common.h        # Shared utility functions
│   ├── generate_dedup_data.cpp      # Test data generator
│   ├── README.md                     # Deduplication usage instructions
│   ├── run_all_benchmarks.bat       # Windows batch script
│   ├── data/                        # Test data directory
│   └── results/                      # Test results directory
│
└── cache_sim/               # Cache simulation application scenario (communication-intensive)
    ├── cache_sim_library.cpp        # Version using library
    ├── cache_sim_std.cpp            # Comparison version using std::map
    ├── cache_sim_benchmark.cpp      # Performance comparison tests
    ├── cache_sim_common.h            # Shared utility functions
    ├── README.md                     # Cache simulation usage instructions
    ├── run_all_benchmarks.bat       # Windows batch script
    └── results/                      # Test results directory
```

## Building

```bash
# Build all programs
make all

# Build only basic tests
make test benchmark

# Build only word count programs
make word_count/word_count_library word_count/word_count_std word_count/word_count_benchmark word_count/generate_test_data

# Build only deduplication programs
make deduplication/deduplication_library deduplication/deduplication_std deduplication/deduplication_benchmark deduplication/generate_dedup_data

# Build only cache simulation programs
make cache_sim/cache_sim_library cache_sim/cache_sim_std cache_sim/cache_sim_benchmark
```

## Running Tests

### Basic Tests

```bash
# Run correctness tests
make run_test

# Run basic performance tests
make run_benchmark
```

### Word Count Application Scenario

```bash
# Navigate to word count directory
cd word_count

# Generate test data
./generate_test_data data/test_small.txt 100000 1000

# Run single test
./word_count_library data/test_small.txt 4

# Run performance comparison tests
./word_count_benchmark data/test_small.txt 1 2 4 8 16

# Run all scale tests (Windows)
run_all_benchmarks.bat

# Run all scale tests (Linux/Mac)
chmod +x run_all_benchmarks.sh
./run_all_benchmarks.sh
```

Or from project root:

```bash
# Run all scale benchmarks for word count
make run_all_benchmarks

# Run all scale benchmarks for deduplication
make run_all_dedup_benchmarks

# Run all scale benchmarks for cache simulation
make run_all_cache_benchmarks
```

## Cleanup

```bash
# Clean all compiled files and test data
make clean
```

## Application Scenarios

### 1. Word Count - Compute-Intensive ✅
- **Location**: `word_count/`
- **Description**: Multi-threaded word frequency counting in text
- **Comparison**: Using concurrent hash table library vs std::unordered_map + global lock
- **Characteristics**: Compute-intensive, requires processing large amounts of text data

### 2. Deduplication - Memory-Intensive ✅
- **Location**: `deduplication/`
- **Description**: Multi-threaded processing of data stream to remove duplicates
- **Comparison**: Using concurrent hash table library vs std::unordered_set + global lock
- **Characteristics**: Memory-intensive, frequent hash table lookups

### 3. Cache Simulation - Communication-Intensive ✅
- **Location**: `cache_sim/`
- **Description**: Simulating multi-threaded frequent access to shared cache
- **Comparison**: Using concurrent hash table library vs std::unordered_map + global lock
- **Characteristics**: Communication-intensive, high-frequency mixed read/write operations

## Performance Testing

According to assignment requirements, each application scenario needs to test:
- **3 different scales**: Small, Medium, Large
- **Multiple thread counts**: 1, 2, 4, 8, 16
- **Two version comparison**: Using library vs not using library

For detailed instructions, please refer to the README files in each application scenario directory.
