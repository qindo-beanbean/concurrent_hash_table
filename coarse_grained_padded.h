#ifndef COARSE_GRAINED_PADDED_H
#define COARSE_GRAINED_PADDED_H

#include "common.h"

template<typename K, typename V>
class CoarseGrainedHashTablePadded {
private:
    std::vector<std::list<KeyValue<K,V>>> buckets;
    size_t bucket_count;
    alignas(64) mutable omp_lock_t global_lock; // aligned to reduce cache-line contention
    std::atomic<size_t> element_count;

    size_t hash(const K& key) const {
        return Hash<K>{}(key) % bucket_count;
    }

public:
    CoarseGrainedHashTablePadded(size_t bucket_count = 1024)
        : bucket_count(bucket_count), element_count(0) {
        buckets.resize(bucket_count);
        omp_init_lock(&global_lock);
    }

    ~CoarseGrainedHashTablePadded() {
        omp_destroy_lock(&global_lock);
    }

    bool insert(const K& key, const V& value) {
        omp_set_lock(&global_lock);
        size_t idx = hash(key);
        auto& bucket = buckets[idx];
        for (auto& kv : bucket) {
            if (kv.key == key) { kv.value = value; omp_unset_lock(&global_lock); return false; }
        }
        bucket.emplace_back(key, value);
        element_count++;
        omp_unset_lock(&global_lock);
        return true;
    }

    bool search(const K& key, V& value) const {
        omp_set_lock(&global_lock);
        size_t idx = hash(key);
        const auto& bucket = buckets[idx];
        for (const auto& kv : bucket) {
            if (kv.key == key) { value = kv.value; omp_unset_lock(&global_lock); return true; }
        }
        omp_unset_lock(&global_lock);
        return false;
    }

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

    size_t size() const { return element_count.load(); }
    std::string getName() const { return "Coarse-Grained-Padded"; }
};

#endif