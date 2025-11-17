#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <omp.h>
#include <iomanip>
#include "word_count_common.h"

using namespace std;

// Parallel word count without library (using std::unordered_map + lock)
double wordCountWithStdMap(const string& filename, int num_threads, size_t& total_words, size_t& unique_words) {
    unordered_map<string, int> wordCount;
    omp_lock_t map_lock;
    omp_init_lock(&map_lock);
    
    vector<string> words = readWordsFromFile(filename);
    if (words.empty()) {
        cerr << "Error: Cannot read file or file is empty: " << filename << endl;
        omp_destroy_lock(&map_lock);
        return -1;
    }
    
    total_words = words.size();
    
    double start_time = omp_get_wtime();
    
    // Parallel word frequency counting (using global lock to protect std::unordered_map)
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
    cout << "  Word Count (Using std::map + Lock)" << endl;
    cout << "=====================================" << endl;
    cout << "File: " << filename << endl;
    cout << "Threads: " << num_threads << endl;
    cout << endl;
    
    double time = wordCountWithStdMap(filename, num_threads, total_words, unique_words);
    
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

