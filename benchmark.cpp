#include <iostream>
#include <iomanip>
#include <omp.h>
#include <random>
#include <string>
#include <vector>
#include "sequential.h"
#include "coarse_grained.h"
#include "fine_grained.h"
#include "segment_based.h"
#include "coarse_grained_padded.h"
#include "fine_grained_padded.h"
#include "segment_based_padded.h"
#include "hotset.h"

using namespace std;

struct RunConfig {
    string table_name;
    string distribution; // uniform | skew
    string mix;          // e.g. "80/20"
    int threads;
    int ops;
    double read_ratio;
    double time_sec;
    double throughput_mops;
    double speedup;
};

template<typename HT>
double run_workload(int threads, int total_ops, double read_ratio, bool skewed) {
    HT ht(16384);
    int initial = total_ops / 2;
    int mixed = total_ops - initial;

    // Pre-fill phase (parallel)
    #pragma omp parallel for num_threads(threads)
    for (int i = 0; i < initial; ++i) {
        ht.insert(i, i * 2);
    }

    // --- Phase 2: Run the mixed workload ---
    // Use thread-local fast linear congruential RNG to avoid:
    // 1. Pre-generation overhead (serial generation of millions of random numbers)
    // 2. Memory overhead (large vector)
    // 3. Cache contention and false sharing (all threads reading same vector)
    double start_time = omp_get_wtime();

    // Convert read_ratio to integer threshold (0-10000 for precision)
    int read_threshold = static_cast<int>(read_ratio * 10000);

    #pragma omp parallel num_threads(num_threads)
    {
        // Each thread gets its own fast linear congruential RNG
        // Using simple LCG: x = (a * x + c) mod m
        // Parameters from Numerical Recipes (a=1664525, c=1013904223)
        unsigned int rng_state = omp_get_thread_num() + 1;  // Different seed per thread
        
        #pragma omp for
        for (int i = 0; i < workload_ops; ++i) {
            // Fast LCG: only a few CPU cycles
            rng_state = rng_state * 1664525u + 1013904223u;
            // Extract lower 16 bits and scale to 0-10000 range
            int rand_val = (rng_state & 0xFFFF) * 10000 / 65536;
            
            int key = i;
            if (rand_val < read_threshold) {
                int value;
                ht.search(key % initial_inserts, value);
            } else {
                ht.insert(initial + i, i);
            }
        }
    }
    double t1 = omp_get_wtime();
    return t1 - t0;
}

template<typename HashTable>
void runParallelBenchmark(const string& name, double baseline_time) {
    cout << "\n=== " << name << " ===" << endl;
    cout << setw(10) << "Threads" 
         << setw(15) << "Time (s)" 
         << setw(20) << "Throughput (M/s)" 
         << setw(15) << "Speedup" << endl;
    cout << string(60, '-') << endl;
    cout << "  (Speedup is relative to Sequential baseline: " << fixed << setprecision(4) << baseline_time << "s)" << endl;
    cout << string(60, '-') << endl;
    
    const double READ_RATIO = 0.8;
    
    for (int threads : {1, 2, 4, 8}) {  // Reduced from {1,4,8,16} to {1,2,4,8}
        double time = benchmarkWorkload<HashTable>(threads, OPERATIONS, READ_RATIO);
        double throughput = (OPERATIONS / time) / 1e6;
        double speedup = (baseline_time > 0 && time > 0) ? baseline_time / time : 0.0;
        
        cout << setw(10) << threads 
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup;
        if (speedup > 1.0) {
            cout << " (faster)";
        } else if (speedup < 1.0 && speedup > 0) {
            cout << " (slower)";
        }
        cout << endl;
    }
}

int main() {
    ios::sync_with_stdio(false);
    const int OPS = 2'000'000;

    cout << "====================================================\n";
    cout << "Concurrent Hash Table Benchmark (Option A Core)\n";
    cout << "====================================================\n";

    cout << "\nBaseline (Sequential, uniform 80/20)...\n";
    double baseline_seq = run_workload<SequentialHashTable<int,int>>(1, OPS, 0.8, false);
    cout << "Sequential Time: " << baseline_seq << " s\n";

    vector<RunConfig> results;

    auto header = [] (const string& title) {
        cout << "\n=== " << title << " ===\n";
        cout << setw(10) << "Threads"
             << setw(15) << "Time(s)"
             << setw(18) << "Throughput(Mops/s)"
             << setw(12) << "Speedup"
             << "\n" << string(60,'-') << "\n";
    };

    // Uniform 80/20
    header("Coarse-Grained uniform 80/20");
    run_suite<CoarseGrainedHashTable<int,int>>("Coarse", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Coarse-Grained-Padded uniform 80/20");
    run_suite<CoarseGrainedHashTablePadded<int,int>>("Coarse-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Fine-Grained uniform 80/20");
    run_suite<FineGrainedHashTable<int,int>>("Fine", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Fine-Grained-Padded uniform 80/20");
    run_suite<FineGrainedHashTablePadded<int,int>>("Fine-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Segment-Based uniform 80/20");
    run_suite<SegmentBasedHashTable<int,int>>("Segment", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Segment-Based-Padded uniform 80/20");
    run_suite<SegmentBasedHashTablePadded<int,int>>("Segment-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    // Skewed 80/20
    header("Coarse-Grained skew 80/20");
    run_suite<CoarseGrainedHashTable<int,int>>("Coarse", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Coarse-Grained-Padded skew 80/20");
    run_suite<CoarseGrainedHashTablePadded<int,int>>("Coarse-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Fine-Grained skew 80/20");
    run_suite<FineGrainedHashTable<int,int>>("Fine", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Fine-Grained-Padded skew 80/20");
    run_suite<FineGrainedHashTablePadded<int,int>>("Fine-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Segment-Based skew 80/20");
    run_suite<SegmentBasedHashTable<int,int>>("Segment", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Segment-Based-Padded skew 80/20");
    run_suite<SegmentBasedHashTablePadded<int,int>>("Segment-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    // Simple CSV dump (stdout)
    cout << "\nCSV_RESULTS_BEGIN\n";
    cout << "table,distribution,mix,threads,ops,read_ratio,time_sec,throughput_mops,speedup\n";
    for (auto& r : results) {
        cout << r.table_name << "," << r.distribution << "," << r.mix << ","
             << r.threads << "," << r.ops << "," << r.read_ratio << ","
             << fixed << setprecision(6) << r.time_sec << ","
             << fixed << setprecision(6) << r.throughput_mops << ","
             << fixed << setprecision(4) << r.speedup << "\n";
    }
    cout << "CSV_RESULTS_END\n";

    return 0;
}