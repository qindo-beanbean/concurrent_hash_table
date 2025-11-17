#include <iostream>
#include <vector>
#include <omp.h>
#include <iomanip>
#include "../fine_grained.h"
#include "cache_sim_common.h"

using namespace std;

// Cache simulation using concurrent hash table library
double cacheSimWithLibrary(const vector<CacheOperation>& operations, int num_threads, 
                           size_t& total_ops, size_t& cache_hits, size_t& cache_misses) {
    FineGrainedHashTable<int, int> cache(8192);  // Fine-grained locking implementation
    
    total_ops = operations.size();
    cache_hits = 0;
    cache_misses = 0;
    
    double start_time = omp_get_wtime();
    
    // Execute cache operations in parallel
    #pragma omp parallel num_threads(num_threads) reduction(+:cache_hits, cache_misses)
    {
        #pragma omp for
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];
            
            if (op.op == 'R') {
                // Read operation: lookup cache
                int value;
                if (cache.search(op.key, value)) {
                    cache_hits++;
                } else {
                    cache_misses++;
                }
            } else {
                // Write operation: insert or update cache
                int existing_value;
                if (cache.search(op.key, existing_value)) {
                    // Update
                    cache.insert(op.key, op.value);
                } else {
                    // Insert
                    cache.insert(op.key, op.value);
                    cache_misses++;  // First write counts as miss
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    
    return end_time - start_time;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <num_operations> <key_range> <read_ratio> <num_threads>" << endl;
        cerr << "Example: " << argv[0] << " 1000000 10000 0.8 4" << endl;
        return 1;
    }
    
    size_t num_ops = stoull(argv[1]);
    size_t key_range = stoull(argv[2]);
    double read_ratio = stod(argv[3]);
    int num_threads = stoi(argv[4]);
    
    cout << "=====================================" << endl;
    cout << "  Cache Simulation (Using Library)" << endl;
    cout << "=====================================" << endl;
    cout << "Operations: " << num_ops << endl;
    cout << "Key range: " << key_range << endl;
    cout << "Read ratio: " << read_ratio << endl;
    cout << "Threads: " << num_threads << endl;
    cout << endl;
    
    // Generate operation sequence
    vector<CacheOperation> operations = generateCacheOperations(num_ops, key_range, read_ratio);
    
    size_t total_ops = 0, cache_hits = 0, cache_misses = 0;
    double time = cacheSimWithLibrary(operations, num_threads, total_ops, cache_hits, cache_misses);
    
    cout << fixed << setprecision(4);
    cout << "Total operations: " << total_ops << endl;
    cout << "Cache hits: " << cache_hits << endl;
    cout << "Cache misses: " << cache_misses << endl;
    cout << "Hit ratio: " << fixed << setprecision(2) 
         << (100.0 * cache_hits / (cache_hits + cache_misses)) << "%" << endl;
    cout << "Time: " << fixed << setprecision(4) << time << " seconds" << endl;
    cout << "Throughput: " << fixed << setprecision(2) 
         << (total_ops / time / 1e6) << " M ops/second" << endl;
    
    return 0;
}

