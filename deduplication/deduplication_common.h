#ifndef DEDUPLICATION_COMMON_H
#define DEDUPLICATION_COMMON_H

#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <random>

// Read integer data from file
inline std::vector<int> readIntegersFromFile(const std::string& filename) {
    std::vector<int> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return data;
    }
    
    int value;
    while (file >> value) {
        data.push_back(value);
    }
    
    file.close();
    return data;
}

// Generate test data with duplicates and save to file
inline void generateDedupData(const std::string& filename, size_t total_count, size_t unique_count) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    // Generate list of unique values
    std::vector<int> unique_values;
    for (size_t i = 0; i < unique_count; i++) {
        unique_values.push_back(static_cast<int>(i));
    }
    
    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, unique_values.size() - 1);
    
    // Generate data with duplicates
    const size_t values_per_line = 20;
    for (size_t i = 0; i < total_count; i++) {
        if (i > 0 && i % values_per_line == 0) {
            file << "\n";
        }
        file << unique_values[dis(gen)] << " ";
    }
    
    file.close();
}

#endif // DEDUPLICATION_COMMON_H

