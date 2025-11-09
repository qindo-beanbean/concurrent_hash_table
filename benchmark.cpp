#include <iostream>
#include <iomanip>
#include <omp.h>
#include "coarse_grained.h"
#include "segment_based.h"
#include "fine_grained.h"
#include "lock_free.h"

using namespace std;

template<typename HashTable>
double benchmarkInsert(int num_threads, int operations) {
    HashTable ht(1024);
    
    double start = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < ops_per_thread; i++) {
            int key = tid * ops_per_thread + i;
            ht.insert(key, key);
        }
    }
    
    double end = omp_get_wtime();
    return end - start;
}

template<typename HashTable>
void runBenchmark(const string& name) {
    cout << "\n=== " << name << " ===" << endl;
    cout << setw(10) << "Threads" 
         << setw(15) << "Time (s)" 
         << setw(20) << "Throughput (M/s)" 
         << setw(15) << "Speedup" << endl;
    cout << string(60, '-') << endl;
    
    const int OPERATIONS = 1000000;
    double baseline_time = 0;
    
    for (int threads : {1, 2, 4, 8, 16}) {
        double time = benchmarkInsert<HashTable>(threads, OPERATIONS);
        double throughput = (OPERATIONS / time) / 1e6;
        
        if (threads == 1) baseline_time = time;
        double speedup = baseline_time / time;
        
        cout << setw(10) << threads 
             << setw(15) << fixed << setprecision(3) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
}

int main() {
    cout << "=====================================" << endl;
    cout << "  Concurrent Hash Table Benchmark" << endl;
    cout << "=====================================" << endl;
    
    runBenchmark<CoarseGrainedHashTable<int, int>>("Coarse-Grained");
    runBenchmark<SegmentBasedHashTable<int, int>>("Segment-Based");
    runBenchmark<FineGrainedHashTable<int, int>>("Fine-Grained");
    runBenchmark<LockFreeHashTable<int, int>>("Lock-Free");
    
    return 0;
}