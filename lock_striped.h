#pragma once
#include "common.h"
#include <vector>
#include <list>
#include <atomic>
#include <omp.h>

// LockStripedHashTable: M locks < bucket_count, offering mid-level granularity.
// Lock index = hash(key) % num_locks; Bucket index = hash(key) % bucket_count.
template <typename K, typename V>
class LockStripedHashTable {
public:
    explicit LockStripedHashTable(size_t bucket_count = 16384, size_t num_locks = 256)
        : bucket_count_(bucket_count),
          num_locks_(num_locks ? num_locks : 1),
          buckets_(bucket_count),
          element_count_(0)
    {
        locks_.resize(num_locks_);
        for (size_t i = 0; i < num_locks_; ++i) omp_init_lock(&locks_[i]);
    }

    ~LockStripedHashTable() {
        for (size_t i = 0; i < num_locks_; ++i) omp_destroy_lock(&locks_[i]);
    }

    bool insert(const K& key, const V& value) {
        size_t h = Hash<K>{}(key);
        size_t lidx = h % num_locks_;
        omp_set_lock(&locks_[lidx]);
        size_t bidx = h % bucket_count_;
        auto& bucket = buckets_[bidx];
        for (auto& kv : bucket) {
            if (kv.key == key) { kv.value = value; omp_unset_lock(&locks_[lidx]); return false; }
        }
        bucket.emplace_back(key, value);
        element_count_.fetch_add(1, std::memory_order_relaxed);
        omp_unset_lock(&locks_[lidx]);
        return true;
    }

    bool search(const K& key, V& value) const {
        size_t h = Hash<K>{}(key);
        size_t lidx = h % num_locks_;
        omp_set_lock(&locks_[lidx]);
        size_t bidx = h % bucket_count_;
        const auto& bucket = buckets_[bidx];
        for (const auto& kv : bucket) {
            if (kv.key == key) { value = kv.value; omp_unset_lock(&locks_[lidx]); return true; }
        }
        omp_unset_lock(&locks_[lidx]);
        return false;
    }

    bool remove(const K& key) {
        size_t h = Hash<K>{}(key);
        size_t lidx = h % num_locks_;
        omp_set_lock(&locks_[lidx]);
        size_t bidx = h % bucket_count_;
        auto& bucket = buckets_[bidx];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count_.fetch_sub(1, std::memory_order_relaxed);
                omp_unset_lock(&locks_[lidx]);
                return true;
            }
        }
        omp_unset_lock(&locks_[lidx]);
        return false;
    }

    size_t size() const { return element_count_.load(std::memory_order_relaxed); }
    std::string getName() const { return "Lock-Striped"; }

private:
    size_t bucket_count_;
    size_t num_locks_;
    std::vector<std::list<KeyValue<K,V>>> buckets_;
    mutable std::vector<omp_lock_t> locks_;
    std::atomic<size_t> element_count_;
};