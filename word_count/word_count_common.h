#ifndef WORD_COUNT_COMMON_H
#define WORD_COUNT_COMMON_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

// Clean word: convert to lowercase, remove punctuation
inline std::string cleanWord(const std::string& word) {
    std::string cleaned;
    for (char c : word) {
        if (std::isalnum(c)) {
            cleaned += std::tolower(c);
        }
    }
    return cleaned;
}

// Read word list from file
inline std::vector<std::string> readWordsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::vector<std::string>();
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                        std::istreambuf_iterator<char>());
    file.close();
    
    std::vector<std::string> words;
    std::istringstream iss(content);
    std::string word;
    while (iss >> word) {
        std::string cleaned = cleanWord(word);
        if (!cleaned.empty()) {
            words.push_back(cleaned);
        }
    }
    
    return words;
}

#endif // WORD_COUNT_COMMON_H

