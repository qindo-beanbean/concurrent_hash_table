#ifndef SEGMENT_BASED_H
#define SEGMENT_BASED_H

#include "common.h"
#include <vector>
#include <list>
#include <atomic>
#include <omp.h>

// Compile-time override:
//   g++ ... -DSB_DEFAULT_SEGMENTS=256
#ifndef SB_DEFAULT_SEGMENTS
#define SB_DEFAULT_SEGMENTS 128
#endif

template<typename K, typename V>
class SegmentBasedHashTable {
private:
    static const size_t NUM_SEGMENTS = SB_DEFAULT_SEGMENTS;

    struct alignas(64) Segment {
        std::vector<std::list<KeyValue<K,V>>> buckets;
        size_t buckets_per_segment;
        omp_lock_t lock;
        explicit Segment(size_t bps) : buckets_per_segment(bps) {
            buckets.resize(buckets_per_segment);
            omp_init_lock(&lock);
        }
        ~Segment() { omp_destroy_lock(&lock); }
        Segment(const Segment&) = delete;
        Segment& operator=(const Segment&) = delete;
        Segment(Segment&&) = delete;
        Segment& operator=(Segment&&) = delete;
    };

    std::vector<Segment*> segments;
    std::atomic<size_t> element_count;
    size_t requested_bucket_count;

    inline size_t segment_index(size_t h) const { return h % NUM_SEGMENTS; }

    inline size_t bucket_in_segment(size_t h, size_t seg) const {
        // Use higher hash bits to avoid reusing low bits for both segment and bucket
        size_t bps = segments[seg]->buckets_per_segment;
        return (h / NUM_SEGMENTS) % bps;
    }

public:
    explicit SegmentBasedHashTable(size_t bucket_count = 1024)
        : element_count(0), requested_bucket_count(bucket_count) {

        size_t base = bucket_count / NUM_SEGMENTS;
        size_t rem  = bucket_count % NUM_SEGMENTS;

        segments.reserve(NUM_SEGMENTS);
        for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
            size_t bps = base + (i < rem ? 1 : 0);
            segments.push_back(new Segment(bps));
        }
    }

    SegmentBasedHashTable(const SegmentBasedHashTable&) = delete;
    SegmentBasedHashTable& operator=(const SegmentBasedHashTable&) = delete;
    SegmentBasedHashTable(SegmentBasedHashTable&&) = delete;
    SegmentBasedHashTable& operator=(SegmentBasedHashTable&&) = delete;

    ~SegmentBasedHashTable() {
        for (auto s : segments) delete s;
    }

    bool insert(const K& key, const V& value) {
        size_t h = Hash<K>{}(key);
        size_t seg = segment_index(h);
        Segment* s = segments[seg];
        omp_set_lock(&s->lock);
        auto& bucket = s->buckets[bucket_in_segment(h, seg)];
        for (auto& kv : bucket) {
            if (kv.key == key) { kv.value = value; omp_unset_lock(&s->lock); return false; }
        }
        bucket.emplace_back(key, value);
        element_count.fetch_add(1, std::memory_order_relaxed);
        omp_unset_lock(&s->lock);
        return true;
    }

    bool search(const K& key, V& value) const {
        size_t h = Hash<K>{}(key);
        size_t seg = segment_index(h);
        Segment* s = segments[seg];
        omp_set_lock(&s->lock);
        const auto& bucket = s->buckets[bucket_in_segment(h, seg)];
        for (const auto& kv : bucket) {
            if (kv.key == key) { value = kv.value; omp_unset_lock(&s->lock); return true; }
        }
        omp_unset_lock(&s->lock);
        return false;
    }

    bool remove(const K& key) {
        size_t h = Hash<K>{}(key);
        size_t seg = segment_index(h);
        Segment* s = segments[seg];
        omp_set_lock(&s->lock);
        auto& bucket = s->buckets[bucket_in_segment(h, seg)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count.fetch_sub(1, std::memory_order_relaxed);
                omp_unset_lock(&s->lock);
                return true;
            }
        }
        omp_unset_lock(&s->lock);
        return false;
    }

    size_t size() const { return element_count.load(std::memory_order_relaxed); }
    size_t effective_bucket_count() const { return requested_bucket_count; } // exact now
    std::string getName() const { return "Segment-Based-Exact"; }
};

#endif // SEGMENT_BASED_H