#ifndef CACHE_SIM_COMMON_H
#define CACHE_SIM_COMMON_H

#include <vector>
#include <random>
#include <string>

// Cache access sequence (key-value pair)
struct CacheOperation {
    int key;
    int value;
    char op;  // 'R' = read, 'W' = write
};

// Generate random cache operation sequence
inline std::vector<CacheOperation> generateCacheOperations(size_t num_ops, size_t key_range, double read_ratio) {
    std::vector<CacheOperation> operations;
    operations.reserve(num_ops);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> op_dist(0.0, 1.0);
    std::uniform_int_distribution<int> key_dist(0, key_range - 1);
    std::uniform_int_distribution<int> value_dist(1, 1000);
    
    for (size_t i = 0; i < num_ops; i++) {
        CacheOperation op;
        op.key = key_dist(gen);
        op.value = value_dist(gen);
        op.op = (op_dist(gen) < read_ratio) ? 'R' : 'W';
        operations.push_back(op);
    }
    
    return operations;
}

#endif // CACHE_SIM_COMMON_H

