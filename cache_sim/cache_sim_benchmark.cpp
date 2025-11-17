#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <omp.h>
#include "../fine_grained.h"
#include "cache_sim_common.h"

using namespace std;

// Cache simulation using concurrent hash table library
double cacheSimWithLibrary(const vector<CacheOperation>& operations, int num_threads, 
                           size_t& total_ops, size_t& cache_hits, size_t& cache_misses) {
    FineGrainedHashTable<int, int> cache(8192);
    
    total_ops = operations.size();
    cache_hits = 0;
    cache_misses = 0;
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads) reduction(+:cache_hits, cache_misses)
    {
        #pragma omp for
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];
            
            if (op.op == 'R') {
                int value;
                if (cache.search(op.key, value)) {
                    cache_hits++;
                } else {
                    cache_misses++;
                }
            } else {
                int existing_value;
                if (cache.search(op.key, existing_value)) {
                    cache.insert(op.key, op.value);
                } else {
                    cache.insert(op.key, op.value);
                    cache_misses++;
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    
    return end_time - start_time;
}

// Cache simulation without library (using std::unordered_map + lock)
double cacheSimWithStdMap(const vector<CacheOperation>& operations, int num_threads,
                          size_t& total_ops, size_t& cache_hits, size_t& cache_misses) {
    unordered_map<int, int> cache;
    omp_lock_t map_lock;
    omp_init_lock(&map_lock);
    
    total_ops = operations.size();
    cache_hits = 0;
    cache_misses = 0;
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads) reduction(+:cache_hits, cache_misses)
    {
        #pragma omp for
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];
            
            if (op.op == 'R') {
                omp_set_lock(&map_lock);
                bool found = (cache.find(op.key) != cache.end());
                omp_unset_lock(&map_lock);
                
                if (found) {
                    cache_hits++;
                } else {
                    cache_misses++;
                }
            } else {
                omp_set_lock(&map_lock);
                bool existed = (cache.find(op.key) != cache.end());
                cache[op.key] = op.value;
                omp_unset_lock(&map_lock);
                
                if (!existed) {
                    cache_misses++;
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    
    omp_destroy_lock(&map_lock);
    
    return end_time - start_time;
}

void runBenchmark(size_t num_ops, size_t key_range, double read_ratio, const vector<int>& thread_counts) {
    cout << "=====================================" << endl;
    cout << "  Cache Simulation Performance Benchmark" << endl;
    cout << "=====================================" << endl;
    cout << "Operations: " << num_ops << endl;
    cout << "Key range: " << key_range << endl;
    cout << "Read ratio: " << read_ratio << endl;
    cout << endl;
    
    // Generate operation sequence (all tests use the same sequence for fair comparison)
    vector<CacheOperation> operations = generateCacheOperations(num_ops, key_range, read_ratio);
    
    // Table header
    cout << setw(15) << "Implementation" 
         << setw(10) << "Threads"
         << setw(15) << "Time (s)"
         << setw(20) << "Throughput (M/s)"
         << setw(15) << "Speedup" << endl;
    cout << string(75, '-') << endl;
    
    double baseline_library = 0;
    double baseline_std = 0;
    
    // Test library version
    cout << "\n--- Using Concurrent Hash Table Library ---" << endl;
    for (int threads : thread_counts) {
        size_t total_ops = 0, cache_hits = 0, cache_misses = 0;
        double time = cacheSimWithLibrary(operations, threads, total_ops, cache_hits, cache_misses);
        
        double throughput = (total_ops / time) / 1e6;
        if (threads == 1) baseline_library = time;
        double speedup = baseline_library / time;
        
        cout << setw(15) << "Library"
             << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    // Test std::map version
    cout << "\n--- Using std::unordered_map + Lock ---" << endl;
    for (int threads : thread_counts) {
        size_t total_ops = 0, cache_hits = 0, cache_misses = 0;
        double time = cacheSimWithStdMap(operations, threads, total_ops, cache_hits, cache_misses);
        
        double throughput = (total_ops / time) / 1e6;
        if (threads == 1) baseline_std = time;
        double speedup = baseline_std / time;
        
        cout << setw(15) << "std::map+Lock"
             << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    // Comparison summary
    cout << "\n--- Summary ---" << endl;
    size_t total_ops = 0, cache_hits = 0, cache_misses = 0;
    double time_lib = cacheSimWithLibrary(operations, 8, total_ops, cache_hits, cache_misses);
    double time_std = cacheSimWithStdMap(operations, 8, total_ops, cache_hits, cache_misses);
    
    if (time_lib > 0 && time_std > 0) {
        double speedup = time_std / time_lib;
        cout << "Library vs std::map speedup (8 threads): " << fixed << setprecision(2) << speedup << "x" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <num_operations> <key_range> <read_ratio> [thread_counts...]" << endl;
        cerr << "Example: " << argv[0] << " 1000000 10000 0.8 1 2 4 8 16" << endl;
        return 1;
    }
    
    size_t num_ops = stoull(argv[1]);
    size_t key_range = stoull(argv[2]);
    double read_ratio = stod(argv[3]);
    vector<int> thread_counts;
    
    if (argc > 4) {
        for (int i = 4; i < argc; i++) {
            thread_counts.push_back(stoi(argv[i]));
        }
    } else {
        // Default thread count
        thread_counts = {1, 2, 4, 8, 16};
    }
    
    runBenchmark(num_ops, key_range, read_ratio, thread_counts);
    
    return 0;
}

