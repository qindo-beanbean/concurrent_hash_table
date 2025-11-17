#ifndef SEQUENTIAL_H
#define SEQUENTIAL_H

#include "common.h" // Use common.h for Hash and KeyValue structs

template<typename K, typename V>
class SequentialHashTable {
private:
    std::vector<std::list<KeyValue<K, V>>> buckets;
    size_t bucket_count;
    size_t element_count;
    
    size_t hash(const K& key) const {
        return Hash<K>{}(key) % bucket_count;
    }

public:
    SequentialHashTable(size_t b_count = 1024) 
        : bucket_count(b_count), element_count(0) {
        buckets.resize(bucket_count);
    }
    
    ~SequentialHashTable() = default;
    
    bool insert(const K& key, const V& value) {
        size_t idx = hash(key);
        auto& bucket = buckets[idx];
        
        for (auto& kv : bucket) {
            if (kv.key == key) {
                kv.value = value;
                return false; 
            }
        }
        
        bucket.emplace_back(key, value);
        element_count++;
        return true;
    }
    
    bool search(const K& key, V& value) const {
        size_t idx = hash(key);
        const auto& bucket = buckets[idx];
        
        for (const auto& kv : bucket) {
            if (kv.key == key) {
                value = kv.value;
                return true;
            }
        }
        
        return false;
    }
    
    bool remove(const K& key) {
        size_t idx = hash(key);
        auto& bucket = buckets[idx];
        
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count--;
                return true;
            }
        }
        
        return false;
    }
    
    size_t size() const {
        return element_count;
    }
    
    std::string getName() const {
        return "Sequential";
    }
};

#endif // SEQUENTIAL_H