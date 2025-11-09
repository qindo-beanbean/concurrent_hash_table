#ifndef COARSE_GRAINED_H
#define COARSE_GRAINED_H

#include "common.h"

template<typename K, typename V>
class CoarseGrainedHashTable {
private:
    std::vector<std::list<KeyValue<K, V>>> buckets;
    size_t bucket_count;
    mutable omp_lock_t global_lock;  // Global lock (mutable allows use in const functions)
    std::atomic<size_t> element_count;
    
    size_t hash(const K& key) const {
        return Hash<K>{}(key) % bucket_count;
    }

public:
    CoarseGrainedHashTable(size_t bucket_count = 1024) 
        : bucket_count(bucket_count), element_count(0) {
        buckets.resize(bucket_count);
        omp_init_lock(&global_lock);
    }
    
    ~CoarseGrainedHashTable() {
        omp_destroy_lock(&global_lock);
    }
    
    // Insert operation
    bool insert(const K& key, const V& value) {
        omp_set_lock(&global_lock);  // Lock the entire table
        
        size_t idx = hash(key);
        auto& bucket = buckets[idx];
        
        // Check if key already exists
        for (auto& kv : bucket) {
            if (kv.key == key) {
                kv.value = value;  // Update value
                omp_unset_lock(&global_lock);
                return false;  // Key already exists
            }
        }
        
        // Insert new key-value pair
        bucket.emplace_back(key, value);
        element_count++;
        
        omp_unset_lock(&global_lock);
        return true;
    }
    
    // search operation
    bool search(const K& key, V& value) const {
        omp_set_lock(&global_lock);
        
        size_t idx = hash(key);
        const auto& bucket = buckets[idx];
        
        for (const auto& kv : bucket) {
            if (kv.key == key) {
                value = kv.value;
                omp_unset_lock(&global_lock);
                return true;
            }
        }
        
        omp_unset_lock(&global_lock);
        return false;
    }
    
    // delete operation
    bool remove(const K& key) {
        omp_set_lock(&global_lock);
        
        size_t idx = hash(key);
        auto& bucket = buckets[idx];
        
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count--;
                omp_unset_lock(&global_lock);
                return true;
            }
        }
        
        omp_unset_lock(&global_lock);
        return false;
    }
    
    size_t size() const {
        return element_count.load();
    }
    
    std::string getName() const {
        return "Coarse-Grained";
    }
};

#endif // COARSE_GRAINED_H