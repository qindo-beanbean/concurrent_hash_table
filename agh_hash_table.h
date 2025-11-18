#pragma once
#include "common.h"
#include <vector>
#include <list>
#include <atomic>
#include <omp.h>

// Adaptive Granularity Hashing (AGH-lite): Segment-Exact + striped locks per segment.
// Goal: increase intra-segment concurrency with a small, static number of stripes K.
//
// Compile-time overrides:
//   -DAGH_DEFAULT_SEGMENTS=128 (used only if bench doesn't set SB_DEFAULT_SEGMENTS)
//   -DAGH_MAX_STRIPES=32
//   -DAGH_STRIPE_FACTOR=2   // K ~ next_pow2(threads / STRIPE_FACTOR)
//
// Notes:
// - Exact bucket distribution across segments (no over-allocation).
// - Stripe count K is decided at construction time (based on expected threads).
// - Each bucket maps to exactly one stripe (bucket_index & (K-1)), so locking is correct.
// - No mid-run changes to stripe mapping (keeps it simple and safe).

#ifndef AGH_DEFAULT_SEGMENTS
#  ifdef SB_DEFAULT_SEGMENTS
#    define AGH_DEFAULT_SEGMENTS SB_DEFAULT_SEGMENTS
#  else
#    define AGH_DEFAULT_SEGMENTS 128
#  endif
#endif

#ifndef AGH_MAX_STRIPES
#define AGH_MAX_STRIPES 32
#endif

#ifndef AGH_STRIPE_FACTOR
#define AGH_STRIPE_FACTOR 2
#endif

template<typename K, typename V>
class AGHHashTable {
private:
    static const size_t NUM_SEGMENTS = AGH_DEFAULT_SEGMENTS;

    struct PaddedLock {
        alignas(64) omp_lock_t l;
        PaddedLock()  { omp_init_lock(&l); }
        ~PaddedLock() { omp_destroy_lock(&l); }
        PaddedLock(const PaddedLock&) = delete;
        PaddedLock& operator=(const PaddedLock&) = delete;
    };

    struct alignas(64) Segment {
        std::vector<std::list<KeyValue<K,V>>> buckets;
        std::vector<PaddedLock*> stripes;
        size_t buckets_per_segment;
        size_t stripe_count;      // power of two
        size_t stripe_mask;       // stripe_count - 1

        Segment(size_t bps, size_t stripes_pow2)
            : buckets_per_segment(bps), stripe_count(stripes_pow2), stripe_mask(stripes_pow2 ? (stripes_pow2 - 1) : 0) {
            buckets.resize(buckets_per_segment);
            stripes.reserve(stripe_count);
            for (size_t i = 0; i < stripe_count; ++i) stripes.push_back(new PaddedLock());
        }
        ~Segment() {
            for (auto* p : stripes) delete p;
        }
        Segment(const Segment&) = delete;
        Segment& operator=(const Segment&) = delete;
        Segment(Segment&&) = delete;
        Segment& operator=(Segment&&) = delete;
    };

    std::vector<Segment*> segments;
    std::atomic<size_t> element_count;
    size_t requested_bucket_count;

    inline size_t seg_index(size_t h) const { return h % NUM_SEGMENTS; }
    inline size_t bucket_index(size_t h, size_t bps) const { return (h / NUM_SEGMENTS) % bps; }

    static size_t next_pow2(size_t x) {
        if (x <= 1) return 1;
        --x;
        x |= x >> 1; x |= x >> 2; x |= x >> 4;
        x |= x >> 8; x |= x >> 16;
#if INTPTR_MAX == INT64_MAX
        x |= x >> 32;
#endif
        return x + 1;
    }

    static size_t choose_stripes(size_t buckets_per_segment, size_t expected_threads) {
        size_t target = expected_threads / AGH_STRIPE_FACTOR;
        size_t k = next_pow2(target);
        if (k > AGH_MAX_STRIPES) k = AGH_MAX_STRIPES;
        if (k < 1) k = 1;
        // Cannot exceed number of buckets in the segment; keep power of two.
        while (k > buckets_per_segment && k > 1) k >>= 1;
        return k ? k : 1;
    }

public:
    // expected_threads: if 0, we use omp_get_max_threads()
    explicit AGHHashTable(size_t bucket_count = 1024, size_t expected_threads = 0)
        : element_count(0), requested_bucket_count(bucket_count) {

        if (expected_threads == 0) {
            #ifdef _OPENMP
            expected_threads = omp_get_max_threads();
            #else
            expected_threads = 1;
            #endif
        }

        size_t base = bucket_count / NUM_SEGMENTS;
        size_t rem  = bucket_count % NUM_SEGMENTS;

        segments.reserve(NUM_SEGMENTS);
        for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
            size_t bps = base + (i < rem ? 1 : 0);
            size_t stripes = choose_stripes(bps, expected_threads);
            segments.push_back(new Segment(bps, stripes));
        }
    }

    ~AGHHashTable() {
        for (auto s : segments) delete s;
    }

    AGHHashTable(const AGHHashTable&) = delete;
    AGHHashTable& operator=(const AGHHashTable&) = delete;
    AGHHashTable(AGHHashTable&&) = delete;
    AGHHashTable& operator=(AGHHashTable&&) = delete;

    bool insert(const K& key, const V& value) {
        size_t h = Hash<K>{}(key);
        size_t si = seg_index(h);
        Segment* s = segments[si];
        size_t bi = bucket_index(h, s->buckets_per_segment);
        size_t stripe = (s->stripe_count > 1) ? (bi & s->stripe_mask) : 0;

        omp_set_lock(&s->stripes[stripe]->l);
        auto& bucket = s->buckets[bi];
        for (auto& kv : bucket) {
            if (kv.key == key) { kv.value = value; omp_unset_lock(&s->stripes[stripe]->l); return false; }
        }
        bucket.emplace_back(key, value);
        element_count.fetch_add(1, std::memory_order_relaxed);
        omp_unset_lock(&s->stripes[stripe]->l);
        return true;
    }

    bool search(const K& key, V& value) const {
        size_t h = Hash<K>{}(key);
        size_t si = seg_index(h);
        Segment* s = segments[si];
        size_t bi = bucket_index(h, s->buckets_per_segment);
        size_t stripe = (s->stripe_count > 1) ? (bi & s->stripe_mask) : 0;

        omp_set_lock(&s->stripes[stripe]->l);
        const auto& bucket = s->buckets[bi];
        for (const auto& kv : bucket) {
            if (kv.key == key) { value = kv.value; omp_unset_lock(&s->stripes[stripe]->l); return true; }
        }
        omp_unset_lock(&s->stripes[stripe]->l);
        return false;
    }

    bool remove(const K& key) {
        size_t h = Hash<K>{}(key);
        size_t si = seg_index(h);
        Segment* s = segments[si];
        size_t bi = bucket_index(h, s->buckets_per_segment);
        size_t stripe = (s->stripe_count > 1) ? (bi & s->stripe_mask) : 0;

        omp_set_lock(&s->stripes[stripe]->l);
        auto& bucket = s->buckets[bi];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                element_count.fetch_sub(1, std::memory_order_relaxed);
                omp_unset_lock(&s->stripes[stripe]->l);
                return true;
            }
        }
        omp_unset_lock(&s->stripes[stripe]->l);
        return false;
    }

    size_t size() const { return element_count.load(std::memory_order_relaxed); }
    size_t effective_bucket_count() const { return requested_bucket_count; }
    std::string getName() const { return "AGH-Striped"; }
};