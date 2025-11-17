#include <iostream>
#include <vector>
#include <unordered_set>
#include <iomanip>
#include <omp.h>
#include "../fine_grained.h"
#include "deduplication_common.h"

using namespace std;

// Data deduplication using concurrent hash table library
double deduplicateWithLibrary(const string& filename, int num_threads, size_t& total_count, size_t& unique_count) {
    FineGrainedHashTable<int, bool> seen(8192);
    
    vector<int> data = readIntegersFromFile(filename);
    if (data.empty()) {
        return -1;
    }
    
    total_count = data.size();
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp for
        for (size_t i = 0; i < data.size(); ++i) {
            bool exists;
            if (!seen.search(data[i], exists)) {
                seen.insert(data[i], true);
            }
        }
    }
    
    double end_time = omp_get_wtime();
    unique_count = seen.size();
    
    return end_time - start_time;
}

// Parallel data deduplication without library (using std::unordered_set + lock)
double deduplicateWithStdSet(const string& filename, int num_threads, size_t& total_count, size_t& unique_count) {
    unordered_set<int> seen;
    omp_lock_t set_lock;
    omp_init_lock(&set_lock);
    
    vector<int> data = readIntegersFromFile(filename);
    if (data.empty()) {
        omp_destroy_lock(&set_lock);
        return -1;
    }
    
    total_count = data.size();
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp for
        for (size_t i = 0; i < data.size(); ++i) {
            omp_set_lock(&set_lock);
            seen.insert(data[i]);
            omp_unset_lock(&set_lock);
        }
    }
    
    double end_time = omp_get_wtime();
    unique_count = seen.size();
    
    omp_destroy_lock(&set_lock);
    
    return end_time - start_time;
}

void runBenchmark(const string& filename, const vector<int>& thread_counts) {
    cout << "=====================================" << endl;
    cout << "  Deduplication Performance Benchmark" << endl;
    cout << "=====================================" << endl;
    cout << "Input file: " << filename << endl;
    cout << endl;
    
    // Get file size
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        return;
    }
    size_t file_size = file.tellg();
    file.close();
    
    cout << "File size: " << fixed << setprecision(2) 
         << (file_size / 1024.0 / 1024.0) << " MB" << endl;
    cout << endl;
    
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
        size_t total_count = 0, unique_count = 0;
        double time = deduplicateWithLibrary(filename, threads, total_count, unique_count);
        
        if (time < 0) continue;
        
        double throughput = (total_count / time) / 1e6;
        if (threads == 1) baseline_library = time;
        double speedup = baseline_library / time;
        
        cout << setw(15) << "Library"
             << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    // Test std::set version
    cout << "\n--- Using std::unordered_set + Lock ---" << endl;
    for (int threads : thread_counts) {
        size_t total_count = 0, unique_count = 0;
        double time = deduplicateWithStdSet(filename, threads, total_count, unique_count);
        
        if (time < 0) continue;
        
        double throughput = (total_count / time) / 1e6;
        if (threads == 1) baseline_std = time;
        double speedup = baseline_std / time;
        
        cout << setw(15) << "std::set+Lock"
             << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    // Comparison summary
    cout << "\n--- Summary ---" << endl;
    size_t total_count = 0, unique_count = 0;
    double time_lib = deduplicateWithLibrary(filename, 8, total_count, unique_count);
    double time_std = deduplicateWithStdSet(filename, 8, total_count, unique_count);
    
    if (time_lib > 0 && time_std > 0) {
        double speedup = time_std / time_lib;
        cout << "Library vs std::set speedup (8 threads): " << fixed << setprecision(2) << speedup << "x" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file> [thread_counts...]" << endl;
        cerr << "Example: " << argv[0] << " data.txt 1 2 4 8 16" << endl;
        return 1;
    }
    
    string filename = argv[1];
    vector<int> thread_counts;
    
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            thread_counts.push_back(stoi(argv[i]));
        }
    } else {
        // Default thread count
        thread_counts = {1, 2, 4, 8, 16};
    }
    
    runBenchmark(filename, thread_counts);
    
    return 0;
}

