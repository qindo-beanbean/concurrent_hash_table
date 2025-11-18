#ifndef SEGMENT_BASED_H
#define SEGMENT_BASED_H

#include "common.h"
#include <algorithm>  // For std::max and std::min

template<typename K, typename V>
class SegmentBasedHashTable {
private:
    // Fixed segment count: 16 segments
    // Note: Increasing segments reduces lock contention but may increase
    // memory overhead and cache misses. 16 is a reasonable balance.
    static const size_t NUM_SEGMENTS = 16;  // Number of segments
    
    // Align to cache line size (64 bytes) to avoid false sharing
    // This prevents different segments from sharing the same cache line,
    // which would cause false sharing and severe performance degradation
    struct alignas(64) Segment {
        std::vector<std::list<KeyValue<K, V>>> buckets;
        mutable omp_lock_t lock;  // mutable allows locking in const methods
        size_t buckets_per_segment;
        // Padding to ensure each Segment occupies at least one cache line (64 bytes)
        // This prevents false sharing when multiple threads access different segments
        char padding[64];
        
        Segment(size_t bps) : buckets_per_segment(bps) {
            buckets.resize(buckets_per_segment);
            omp_init_lock(&lock);
        }
        
        ~Segment() {
            omp_destroy_lock(&lock);
        }
        
        // Disable copy
        Segment(const Segment&) = delete;
        Segment& operator=(const Segment&) = delete;
    };
    
    std::vector<Segment*> segments;
    size_t total_buckets;
    size_t buckets_per_segment;  // Cache this to avoid accessing seg->buckets_per_segment every time
    std::atomic<size_t> element_count;
    
    // Use static hasher to avoid recreating Hash object on every call
    static size_t hashKey(const K& key) {
        // Hash<K> is just a wrapper around std::hash, so this is efficient
        return std::hash<K>{}(key);
    }

public:
    SegmentBasedHashTable(size_t bucket_count = 1024) 
        : total_buckets(bucket_count), 
          buckets_per_segment(bucket_count / NUM_SEGMENTS == 0 ? 1 : bucket_count / NUM_SEGMENTS),
          element_count(0) {
        
        for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
            segments.push_back(new Segment(buckets_per_segment));
        }
    }
    
    ~SegmentBasedHashTable() {
        for (auto seg : segments) {
            delete seg;
        }
    }
    
    bool insert(const K& key, const V& value) {
        // Compute hash once and reuse (using efficient hash function)
        size_t hash_val = hashKey(key);
        size_t seg_idx = hash_val % NUM_SEGMENTS;
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);  // Lock only the corresponding segment
        
        size_t bucket_idx = (hash_val >> 4) % buckets_per_segment; // Use cached value
        auto& bucket = seg->buckets[bucket_idx];
        
        // Check if key already exists
        for (auto& kv : bucket) {
            if (kv.key == key) {
                kv.value = value;
                omp_unset_lock(&seg->lock);
                return false;
            }
        }
        
        bucket.emplace_back(key, value);
        element_count++;
        
        omp_unset_lock(&seg->lock);
        return true;
    }
    
    bool search(const K& key, V& value) const {
        // Compute hash once and reuse (using efficient hash function)
        size_t hash_val = hashKey(key);
        size_t seg_idx = hash_val % NUM_SEGMENTS;
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);  // mutable lock allows this in const method
        
        size_t bucket_idx = (hash_val>> 4) % buckets_per_segment;  // Use cached value
        const auto& bucket = seg->buckets[bucket_idx];
        
        for (const auto& kv : bucket) {
            if (kv.key == key) {
                value = kv.value;
                omp_unset_lock(&seg->lock);
                return true;
            }
        }
        
        omp_unset_lock(&seg->lock);
        return false;
    }
    
    bool remove(const K& key) {
        // Compute hash once and reuse (using efficient hash function)
        size_t hash_val = hashKey(key);
        size_t seg_idx = hash_val % NUM_SEGMENTS;
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);
        
        size_t bucket_idx = (hash_val>> 4) % buckets_per_segment;  // Use cached value
        auto& bucket = seg->buckets[bucket_idx];
        
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count--;
                omp_unset_lock(&seg->lock);
                return true;
            }
        }
        
        omp_unset_lock(&seg->lock);
        return false;
    }
    
    size_t size() const {
        return element_count.load();
    }
    
    std::string getName() const {
        return "Segment-Based";
    }
};

#endif // SEGMENT_BASED_H