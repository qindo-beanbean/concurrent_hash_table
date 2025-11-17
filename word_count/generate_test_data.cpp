#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>

using namespace std;

// Generate test text data
void generateTestData(const string& filename, size_t num_words, size_t unique_words) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create file " << filename << endl;
        return;
    }
    
    // Generate base words
    vector<string> words;
    for (size_t i = 0; i < unique_words; i++) {
        string word = "word";
        size_t num = i;
        while (num > 0) {
            word += char('a' + (num % 26));
            num /= 26;
        }
        words.push_back(word);
    }
    
    // Random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, words.size() - 1);
    
    // Generate text
    const size_t words_per_line = 20;
    for (size_t i = 0; i < num_words; i++) {
        if (i > 0 && i % words_per_line == 0) {
            file << "\n";
        }
        file << words[dis(gen)] << " ";
    }
    
    file.close();
    cout << "Generated test file: " << filename << endl;
    cout << "Total words: " << num_words << endl;
    cout << "Unique words: " << unique_words << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <output_file> <num_words> <unique_words>" << endl;
        cerr << "Example: " << argv[0] << " test_small.txt 100000 1000" << endl;
        cerr << "Example: " << argv[0] << " test_medium.txt 1000000 10000" << endl;
        cerr << "Example: " << argv[0] << " test_large.txt 10000000 50000" << endl;
        return 1;
    }
    
    string filename = argv[1];
    size_t num_words = stoull(argv[2]);
    size_t unique_words = stoull(argv[3]);
    
    generateTestData(filename, num_words, unique_words);
    
    return 0;
}

