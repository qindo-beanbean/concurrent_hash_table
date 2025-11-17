#include <iostream>
#include <vector>
#include <unordered_map>
#include <omp.h>
#include <iomanip>
#include "cache_sim_common.h"

using namespace std;

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
    
    // Execute cache operations in parallel (using global lock for protection)
    #pragma omp parallel num_threads(num_threads) reduction(+:cache_hits, cache_misses)
    {
        #pragma omp for
        for (size_t i = 0; i < operations.size(); ++i) {
            const auto& op = operations[i];
            
            if (op.op == 'R') {
                // Read operation: lookup cache
                omp_set_lock(&map_lock);
                bool found = (cache.find(op.key) != cache.end());
                omp_unset_lock(&map_lock);
                
                if (found) {
                    cache_hits++;
                } else {
                    cache_misses++;
                }
            } else {
                // Write operation: insert or update cache
                omp_set_lock(&map_lock);
                bool existed = (cache.find(op.key) != cache.end());
                cache[op.key] = op.value;
                omp_unset_lock(&map_lock);
                
                if (!existed) {
                    cache_misses++;  // First write counts as miss
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    
    omp_destroy_lock(&map_lock);
    
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
    cout << "  Cache Simulation (Using std::map + Lock)" << endl;
    cout << "=====================================" << endl;
    cout << "Operations: " << num_ops << endl;
    cout << "Key range: " << key_range << endl;
    cout << "Read ratio: " << read_ratio << endl;
    cout << "Threads: " << num_threads << endl;
    cout << endl;
    
    // Generate operation sequence
    vector<CacheOperation> operations = generateCacheOperations(num_ops, key_range, read_ratio);
    
    size_t total_ops = 0, cache_hits = 0, cache_misses = 0;
    double time = cacheSimWithStdMap(operations, num_threads, total_ops, cache_hits, cache_misses);
    
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

