#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
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
            // Use increment method to atomically increment count
            // This avoids race condition between search and insert
            wordCount.increment(words[i], 1);
        }
    }
    
    double end_time = omp_get_wtime();
    unique_words = wordCount.size();
    
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
    
    double baseline_time = 0;
    
    // Test library version with different thread counts
    cout << "\n--- Concurrent Hash Table Library Performance ---" << endl;
    for (int threads : thread_counts) {
        size_t total_words = 0, unique_words = 0;
        double time = wordCountWithLibrary(filename, threads, total_words, unique_words);
        
        if (time < 0) continue;
        
        double throughput = (total_words / time) / 1e6;
        if (threads == 1) baseline_time = time;
        double speedup = (baseline_time > 0 && time > 0) ? baseline_time / time : 0.0;
        
        cout << setw(15) << "Library"
             << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    // Summary
    cout << "\n--- Summary ---" << endl;
    if (baseline_time > 0) {
        cout << "Baseline (1 thread): " << fixed << setprecision(4) << baseline_time << "s" << endl;
        if (thread_counts.size() > 1) {
            size_t total_words = 0, unique_words = 0;
            int max_threads = thread_counts.back();
            double max_time = wordCountWithLibrary(filename, max_threads, total_words, unique_words);
            if (max_time > 0) {
                double max_speedup = baseline_time / max_time;
                cout << "Best speedup (" << max_threads << " threads): " 
                     << fixed << setprecision(2) << max_speedup << "x" << endl;
            }
        }
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
