#include <iostream>
#include <cassert>
#include "coarse_grained.h"
#include "segment_based.h"
#include "fine_grained.h"
#include "lock_free.h"

using namespace std;

// Test a single implementation
template<typename HashTable>
void testHashTable(const string& name) {
    cout << "\n=== Testing " << name << " ===" << endl;
    
    HashTable ht(128);
    
    // Test insert
    cout << "Testing insert..." << endl;
    assert(ht.insert(1, 100) == true);
    assert(ht.insert(2, 200) == true);
    assert(ht.insert(1, 150) == false);  // Duplicate key
    assert(ht.size() == 2);
    
    // Test search
    cout << "Testing search..." << endl;
    int value;
    assert(ht.search(1, value) == true && value == 150);
    assert(ht.search(2, value) == true && value == 200);
    assert(ht.search(99, value) == false);
    
    // Test remove
    cout << "Testing remove..." << endl;
    assert(ht.remove(1) == true);
    assert(ht.search(1, value) == false);
    assert(ht.size() == 1);
    assert(ht.remove(99) == false);
    
    cout << "✓ All tests passed for " << name << endl;
}

// Concurrent test
template<typename HashTable>
void testConcurrent(const string& name, int num_threads) {
    cout << "\n=== Concurrent Test: " << name 
         << " (" << num_threads << " threads) ===" << endl;
    
    HashTable ht(1024);
    const size_t ITEMS_PER_THREAD = 1000;
    
    // Concurrent insert
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        for (size_t i = 0; i < ITEMS_PER_THREAD; i++) {
            int key = tid * ITEMS_PER_THREAD + i;
            ht.insert(key, key * 10);
        }
    }
    
    cout << "Inserted " << ht.size() << " elements" << endl;
    assert(ht.size() == num_threads * ITEMS_PER_THREAD);
    
    // Concurrent search
    int failed_searches = 0;
    #pragma omp parallel num_threads(num_threads) reduction(+:failed_searches)
    {
        int tid = omp_get_thread_num();
        int value;
        for (size_t i = 0; i < ITEMS_PER_THREAD; i++) {
            int key = tid * ITEMS_PER_THREAD + i;
            if (!ht.search(key, value) || value != key * 10) {
                failed_searches++;
            }
        }
    }
    
    assert(failed_searches == 0);
    cout << "✓ Concurrent test passed for " << name << endl;
}

int main() {
    cout << "==================================" << endl;
    cout << "  Hash Table Correctness Tests" << endl;
    cout << "==================================" << endl;
    
    // Single-threaded correctness tests
    testHashTable<CoarseGrainedHashTable<int, int>>("Coarse-Grained");
    testHashTable<SegmentBasedHashTable<int, int>>("Segment-Based");
    testHashTable<FineGrainedHashTable<int, int>>("Fine-Grained");
    testHashTable<LockFreeHashTable<int, int>>("Lock-Free");
    
    // Concurrent correctness tests
    testConcurrent<CoarseGrainedHashTable<int, int>>("Coarse-Grained", 4);
    testConcurrent<SegmentBasedHashTable<int, int>>("Segment-Based", 4);
    testConcurrent<FineGrainedHashTable<int, int>>("Fine-Grained", 4);
    testConcurrent<LockFreeHashTable<int, int>>("Lock-Free", 4);
    
    cout << "\n✓✓✓ All tests passed! ✓✓✓" << endl;
    
    return 0;
}