#ifndef SEGMENT_BASED_PADDED_H
#define SEGMENT_BASED_PADDED_H

#include "common.h"

template<typename K, typename V>
class SegmentBasedHashTablePadded {
private:
    static const size_t NUM_SEGMENTS = 16;

    struct alignas(64) Segment {
        std::vector<std::list<KeyValue<K,V>>> buckets;
        omp_lock_t lock;
        size_t buckets_per_segment;
        Segment(size_t bps) : buckets_per_segment(bps) {
            buckets.resize(buckets_per_segment);
            omp_init_lock(&lock);
        }
        ~Segment() { omp_destroy_lock(&lock); }
        Segment(const Segment&) = delete;
        Segment& operator=(const Segment&) = delete;
    };

    std::vector<Segment*> segments;
    size_t total_buckets;
    std::atomic<size_t> element_count;

    size_t getSegmentIndex(const K& key) const {
        return Hash<K>{}(key) % NUM_SEGMENTS;
    }
    size_t getBucketIndex(const K& key, size_t bps) const {
        return Hash<K>{}(key) % bps;
    }

public:
    SegmentBasedHashTablePadded(size_t bucket_count = 1024)
        : total_buckets(bucket_count), element_count(0) {
        size_t bps = bucket_count / NUM_SEGMENTS;
        if (bps == 0) bps = 1;
        for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
            segments.push_back(new Segment(bps));
        }
    }

    ~SegmentBasedHashTablePadded() {
        for (auto s : segments) delete s;
    }

    bool insert(const K& key, const V& value) {
        size_t seg_idx = getSegmentIndex(key);
        Segment* seg = segments[seg_idx];
        omp_set_lock(&seg->lock);
        size_t bidx = getBucketIndex(key, seg->buckets_per_segment);
        auto& bucket = seg->buckets[bidx];
        for (auto& kv : bucket) {
            if (kv.key == key) { kv.value = value; omp_unset_lock(&seg->lock); return false; }
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
        size_t bidx = getBucketIndex(key, seg->buckets_per_segment);
        const auto& bucket = seg->buckets[bidx];
        for (const auto& kv : bucket) {
            if (kv.key == key) { value = kv.value; omp_unset_lock(&seg->lock); return true; }
        }
        omp_unset_lock(&seg->lock);
        return false;
    }

    bool remove(const K& key) {
        size_t seg_idx = getSegmentIndex(key);
        Segment* seg = segments[seg_idx];
        omp_set_lock(&seg->lock);
        size_t bidx = getBucketIndex(key, seg->buckets_per_segment);
        auto& bucket = seg->buckets[bidx];
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

    size_t size() const { return element_count.load(); }
    std::string getName() const { return "Segment-Based-Padded"; }
};

#endif