#ifndef FINE_GRAINED_H
#define FINE_GRAINED_H

#include "common.h"

template<typename K, typename V>
class FineGrainedHashTable {
private:
    struct Bucket {
        std::list<KeyValue<K, V>> data;
        omp_lock_t lock;
        
        Bucket() {
            omp_init_lock(&lock);
        }
        
        ~Bucket() {
            omp_destroy_lock(&lock);
        }
        
        // Disable copy
        Bucket(const Bucket&) = delete;
        Bucket& operator=(const Bucket&) = delete;
    };
    
    std::vector<Bucket*> buckets;
    size_t bucket_count;
    std::atomic<size_t> element_count;
    
    size_t hash(const K& key) const {
        return Hash<K>{}(key) % bucket_count;
    }

public:
    FineGrainedHashTable(size_t bucket_count = 1024) 
        : bucket_count(bucket_count), element_count(0) {
        
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets.push_back(new Bucket());
        }
    }
    
    ~FineGrainedHashTable() {
        for (auto bucket : buckets) {
            delete bucket;
        }
    }
    
    bool insert(const K& key, const V& value) {
        size_t idx = hash(key);
        Bucket* bucket = buckets[idx];
        
        omp_set_lock(&bucket->lock);  // Lock only this bucket
        
        // Check if key already exists
        for (auto& kv : bucket->data) {
            if (kv.key == key) {
                kv.value = value;
                omp_unset_lock(&bucket->lock);
                return false;
            }
        }
        
        bucket->data.emplace_back(key, value);
        element_count++;
        
        omp_unset_lock(&bucket->lock);
        return true;
    }

    bool increment(const K& key, const V& delta) {
        size_t idx = hash(key);
        Bucket* bucket = buckets[idx];

        omp_set_lock(&bucket->lock);
        for (auto& kv : bucket->data) {
            if (kv.key == key) {
                kv.value += delta;      // requires V to support operator+=
                omp_unset_lock(&bucket->lock);
                return false;           // updated existing
            }
        }
        // not found: insert with initial value = delta
        bucket->data.emplace_back(key, delta);
        element_count++;
        omp_unset_lock(&bucket->lock);
        return true;                    // inserted new
    }
    
    
    bool search(const K& key, V& value) const {
        size_t idx = hash(key);
        Bucket* bucket = buckets[idx];
        
        omp_set_lock(&bucket->lock);
        
        for (const auto& kv : bucket->data) {
            if (kv.key == key) {
                value = kv.value;
                omp_unset_lock(&bucket->lock);
                return true;
            }
        }
        
        omp_unset_lock(&bucket->lock);
        return false;
    }
    
    bool remove(const K& key) {
        size_t idx = hash(key);
        Bucket* bucket = buckets[idx];
        
        omp_set_lock(&bucket->lock);
        
        for (auto it = bucket->data.begin(); it != bucket->data.end(); ++it) {
            if (it->key == key) {
                bucket->data.erase(it);
                element_count--;
                omp_unset_lock(&bucket->lock);
                return true;
            }
        }
        
        omp_unset_lock(&bucket->lock);
        return false;
    }
    
    size_t size() const {
        return element_count.load();
    }
    
    std::string getName() const {
        return "Fine-Grained";
    }
};

#endif // FINE_GRAINED_H