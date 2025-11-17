#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>
#include <iomanip>

using namespace std;

// Generate test data with duplicates
void generateDedupData(const string& filename, size_t total_count, size_t unique_count) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create file " << filename << endl;
        return;
    }
    
    // Generate list of unique values
    vector<int> unique_values;
    for (size_t i = 0; i < unique_count; i++) {
        unique_values.push_back(static_cast<int>(i));
    }
    
    // Random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, unique_values.size() - 1);
    
    // Generate data with duplicates
    const size_t values_per_line = 20;
    for (size_t i = 0; i < total_count; i++) {
        if (i > 0 && i % values_per_line == 0) {
            file << "\n";
        }
        file << unique_values[dis(gen)] << " ";
    }
    
    file.close();
    cout << "Generated test file: " << filename << endl;
    cout << "Total items: " << total_count << endl;
    cout << "Unique items: " << unique_count << endl;
    cout << "Duplication ratio: " << fixed << setprecision(2) 
         << (1.0 - (double)unique_count / total_count) * 100 << "%" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <output_file> <total_count> <unique_count>" << endl;
        cerr << "Example: " << argv[0] << " data_small.txt 100000 1000" << endl;
        cerr << "Example: " << argv[0] << " data_medium.txt 1000000 10000" << endl;
        cerr << "Example: " << argv[0] << " data_large.txt 10000000 50000" << endl;
        return 1;
    }
    
    string filename = argv[1];
    size_t total_count = stoull(argv[2]);
    size_t unique_count = stoull(argv[3]);
    
    if (unique_count > total_count) {
        cerr << "Error: unique_count cannot be greater than total_count" << endl;
        return 1;
    }
    
    generateDedupData(filename, total_count, unique_count);
    
    return 0;
}

