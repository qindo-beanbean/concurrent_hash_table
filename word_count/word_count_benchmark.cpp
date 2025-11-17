#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <omp.h>
#include "../fine_grained.h"
#include "word_count_common.h"

using namespace std;

// Word count using concurrent hash table library
double wordCountWithLibrary(const string& filename, int num_threads, size_t& total_words, size_t& unique_words) {
    FineGrainedHashTable<string, int> wordCount(8192);
    
    vector<string> words = readWordsFromFile(filename);
    if (words.empty()) {
        return -1;
    }
    
    total_words = words.size();
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp for
        for (size_t i = 0; i < words.size(); ++i) {
            int count;
            if (wordCount.search(words[i], count)) {
                wordCount.insert(words[i], count + 1);
            } else {
                wordCount.insert(words[i], 1);
            }
        }
    }
    
    double end_time = omp_get_wtime();
    unique_words = wordCount.size();
    
    return end_time - start_time;
}

// Parallel word count without library (using std::unordered_map + lock)
double wordCountWithStdMap(const string& filename, int num_threads, size_t& total_words, size_t& unique_words) {
    unordered_map<string, int> wordCount;
    omp_lock_t map_lock;
    omp_init_lock(&map_lock);
    
    vector<string> words = readWordsFromFile(filename);
    if (words.empty()) {
        omp_destroy_lock(&map_lock);
        return -1;
    }
    
    total_words = words.size();
    
    double start_time = omp_get_wtime();
    
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp for
        for (size_t i = 0; i < words.size(); ++i) {
            omp_set_lock(&map_lock);
            wordCount[words[i]]++;
            omp_unset_lock(&map_lock);
        }
    }
    
    double end_time = omp_get_wtime();
    unique_words = wordCount.size();
    
    omp_destroy_lock(&map_lock);
    
    return end_time - start_time;
}

void runBenchmark(const string& filename, const vector<int>& thread_counts) {
    cout << "=====================================" << endl;
    cout << "  Word Count Performance Benchmark" << endl;
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
        size_t total_words = 0, unique_words = 0;
        double time = wordCountWithLibrary(filename, threads, total_words, unique_words);
        
        if (time < 0) continue;
        
        double throughput = (total_words / time) / 1e6;
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
        size_t total_words = 0, unique_words = 0;
        double time = wordCountWithStdMap(filename, threads, total_words, unique_words);
        
        if (time < 0) continue;
        
        double throughput = (total_words / time) / 1e6;
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
    size_t total_words = 0, unique_words = 0;
    double time_lib = wordCountWithLibrary(filename, 8, total_words, unique_words);
    double time_std = wordCountWithStdMap(filename, 8, total_words, unique_words);
    
    if (time_lib > 0 && time_std > 0) {
        double speedup = time_std / time_lib;
        cout << "Library vs std::map speedup (8 threads): " << fixed << setprecision(2) << speedup << "x" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file> [thread_counts...]" << endl;
        cerr << "Example: " << argv[0] << " test.txt 1 2 4 8 16" << endl;
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
