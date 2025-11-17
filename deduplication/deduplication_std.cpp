#include <iostream>
#include <vector>
#include <unordered_set>
#include <omp.h>
#include <iomanip>
#include "deduplication_common.h"

using namespace std;

// Parallel data deduplication without library (using std::unordered_set + lock)
double deduplicateWithStdSet(const string& filename, int num_threads, size_t& total_count, size_t& unique_count) {
    unordered_set<int> seen;
    omp_lock_t set_lock;
    omp_init_lock(&set_lock);
    
    vector<int> data = readIntegersFromFile(filename);
    if (data.empty()) {
        cerr << "Error: Cannot read file or file is empty: " << filename << endl;
        omp_destroy_lock(&set_lock);
        return -1;
    }
    
    total_count = data.size();
    
    double start_time = omp_get_wtime();
    
    // Parallel deduplication (using global lock to protect std::unordered_set)
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp for
        for (size_t i = 0; i < data.size(); ++i) {
            omp_set_lock(&set_lock);
            seen.insert(data[i]);  // insert automatically deduplicates
            omp_unset_lock(&set_lock);
        }
    }
    
    double end_time = omp_get_wtime();
    unique_count = seen.size();
    
    omp_destroy_lock(&set_lock);
    
    return end_time - start_time;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <num_threads>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    int num_threads = stoi(argv[2]);
    
    size_t total_count = 0;
    size_t unique_count = 0;
    
    cout << "=====================================" << endl;
    cout << "  Deduplication (Using std::set + Lock)" << endl;
    cout << "=====================================" << endl;
    cout << "File: " << filename << endl;
    cout << "Threads: " << num_threads << endl;
    cout << endl;
    
    double time = deduplicateWithStdSet(filename, num_threads, total_count, unique_count);
    
    if (time < 0) {
        return 1;
    }
    
    cout << fixed << setprecision(4);
    cout << "Total items: " << total_count << endl;
    cout << "Unique items: " << unique_count << endl;
    cout << "Time: " << time << " seconds" << endl;
    cout << "Throughput: " << fixed << setprecision(2) 
         << (total_count / time / 1e6) << " M items/second" << endl;
    
    return 0;
}

