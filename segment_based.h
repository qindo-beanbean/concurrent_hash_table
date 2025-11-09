#ifndef SEGMENT_BASED_H
#define SEGMENT_BASED_H

#include "common.h"

template<typename K, typename V>
class SegmentBasedHashTable {
private:
    static const size_t NUM_SEGMENTS = 16;  // Number of segments
    
    struct Segment {
        std::vector<std::list<KeyValue<K, V>>> buckets;
        omp_lock_t lock;
        size_t buckets_per_segment;
        
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
    std::atomic<size_t> element_count;
    
    size_t getSegmentIndex(const K& key) const {
        return Hash<K>{}(key) % NUM_SEGMENTS;
    }
    
    size_t getBucketIndex(const K& key, size_t buckets_per_segment) const {
        return Hash<K>{}(key) % buckets_per_segment;
    }

public:
    SegmentBasedHashTable(size_t bucket_count = 1024) 
        : total_buckets(bucket_count), element_count(0) {
        
        size_t buckets_per_segment = bucket_count / NUM_SEGMENTS;
        if (buckets_per_segment == 0) buckets_per_segment = 1;
        
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
        size_t seg_idx = getSegmentIndex(key);
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);  // Lock only the corresponding segment
        
        size_t bucket_idx = getBucketIndex(key, seg->buckets_per_segment);
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
        size_t seg_idx = getSegmentIndex(key);
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);
        
        size_t bucket_idx = getBucketIndex(key, seg->buckets_per_segment);
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
        size_t seg_idx = getSegmentIndex(key);
        Segment* seg = segments[seg_idx];
        
        omp_set_lock(&seg->lock);
        
        size_t bucket_idx = getBucketIndex(key, seg->buckets_per_segment);
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