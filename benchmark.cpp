#include <iostream>
#include <iomanip>
#include <omp.h>
#include <vector>
#include <string>
#include <random> // Use C++ random for better, thread-safe random numbers
#include "sequential.h"
#include "coarse_grained.h"
#include "segment_based.h"
#include "fine_grained.h"
#include "lock_free.h"

#define OPERATIONS 20000000


using namespace std;

// This function will now be used for all hash tables, including sequential.
template<typename HashTable>
double benchmarkWorkload(int num_threads, int total_ops, double read_ratio) {
    HashTable ht(16384);
    int initial_inserts = total_ops / 2;
    int workload_ops = total_ops - initial_inserts;

    // --- Phase 1: Pre-fill the hash table ---
    // Note: For sequential, this loop runs with num_threads = 1.
    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < initial_inserts; ++i) {
        ht.insert(i, i * 2);
    }

    // --- Phase 2: Run the mixed workload ---
    double start_time = omp_get_wtime();

    #pragma omp parallel num_threads(num_threads)
    {
        // Each thread gets its own random number generator to avoid data races on rand()
        unsigned int seed = omp_get_thread_num();
        std::mt19937 generator(seed);
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        #pragma omp for
        for (int i = 0; i < workload_ops; ++i) {
            // Generate a key. Note: This simple key generation might lead to contention.
            int key = i; 
            if (distribution(generator) < read_ratio) {
                int value;
                ht.search(key % initial_inserts, value);
            } else {
                ht.insert(initial_inserts + i, i);
            }
        }
    }

    double end_time = omp_get_wtime();
    return end_time - start_time;
}

template<typename HashTable>
void runParallelBenchmark(const string& name, double baseline_time) {
    cout << "\n=== " << name << " ===" << endl;
    cout << setw(10) << "Threads" 
         << setw(15) << "Time (s)" 
         << setw(20) << "Throughput (M/s)" 
         << setw(15) << "Speedup" << endl;
    cout << string(60, '-') << endl;
    
    const double READ_RATIO = 0.8;
    
    for (int threads : {1, 4, 8, 16}) {
        double time = benchmarkWorkload<HashTable>(threads, OPERATIONS, READ_RATIO);
        double throughput = (OPERATIONS / time) / 1e6;
        double speedup = (baseline_time > 0 && time > 0) ? baseline_time / time : 0.0;
        
        cout << setw(10) << threads 
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
}

int main() {
    cout << "=====================================" << endl;
    cout << "  Concurrent Hash Table Benchmark" << endl;
    cout << "=====================================" << endl;

    const double READ_RATIO = 0.8;

    // --- Get the true baseline using the SequentialHashTable ---
    // We run it via benchmarkWorkload with 1 thread to ensure identical optimization.
    cout << "\n--- Measuring Sequential Baseline ---" << endl;
    double baseline_time = benchmarkWorkload<SequentialHashTable<int, int>>(1, OPERATIONS, READ_RATIO);
    cout << "Sequential Time: " << fixed << setprecision(4) << baseline_time << "s" << endl;
    
    // --- Run Parallel Benchmarks ---
    // runParallelBenchmark<CoarseGrainedHashTable<int, int>>("Coarse-Grained", baseline_time);
    runParallelBenchmark<FineGrainedHashTable<int, int>>("Fine-Grained", baseline_time);
    // runParallelBenchmark<SegmentBasedHashTable<int, int>>("Segment-Based", baseline_time);
    // runParallelBenchmark<LockFreeHashTable<int, int>>("Lock-Free", baseline_time);
    
    return 0;
}