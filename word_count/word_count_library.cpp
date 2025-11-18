#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <omp.h>
#include <iomanip>
#include "../fine_grained.h"  // Use our concurrent hash table library
#include "word_count_common.h"

using namespace std;

// Word count using concurrent hash table library
double wordCountWithLibrary(const string& filename, int num_threads, size_t& total_words, size_t& unique_words) {
    FineGrainedHashTable<string, int> wordCount(8192);  // Fine-grained locking implementation
    
    vector<string> words = readWordsFromFile(filename);
    if (words.empty()) {
        cerr << "Error: Cannot read file or file is empty: " << filename << endl;
        return -1;
    }
    
    total_words = words.size();
    
    double start_time = omp_get_wtime();
    
    // Parallel word frequency counting
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <num_threads> [output_file]" << endl;
        return 1;
    }
    
    string filename = argv[1];
    int num_threads = stoi(argv[2]);
    bool output_results = (argc >= 4);
    string output_file = output_results ? argv[3] : "";
    
    size_t total_words = 0;
    size_t unique_words = 0;
    
    cout << "=====================================" << endl;
    cout << "  Word Count (Using Library)" << endl;
    cout << "=====================================" << endl;
    cout << "File: " << filename << endl;
    cout << "Threads: " << num_threads << endl;
    cout << endl;
    
    double time = wordCountWithLibrary(filename, num_threads, total_words, unique_words);
    
    if (time < 0) {
        return 1;
    }
    
    cout << fixed << setprecision(4);
    cout << "Total words: " << total_words << endl;
    cout << "Unique words: " << unique_words << endl;
    cout << "Time: " << time << " seconds" << endl;
    cout << "Throughput: " << fixed << setprecision(2) 
         << (total_words / time / 1e6) << " M words/second" << endl;
    
    return 0;
}

