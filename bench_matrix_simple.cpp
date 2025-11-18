#include <bits/stdc++.h>
#include <omp.h>

#include "common.h"
#include "sequential.h"
#include "coarse_grained.h"
#include "fine_grained.h"
#include "segment_based.h"
#include "lock_free.h"

// Simple hot-set generator: p_hot = probability of choosing from [0, hotN),
// otherwise choose from [hotN, universe)
struct HotsetGen {
    int universe, hotN; double p_hot;
    std::mt19937 gen;
    std::uniform_int_distribution<int> hotD, coldD;
    std::uniform_real_distribution<double> coin;
    HotsetGen(int u, int h, double p, uint32_t seed)
        : universe(u), hotN(std::max(1,h)), p_hot(p), gen(seed),
          hotD(0, std::max(0,h-1)), coldD(std::min(h,u-1), std::max(0,u-1)), coin(0.0,1.0) {}
    int draw() { return (coin(gen) < p_hot) ? hotD(gen) : coldD(gen); }
};

struct Row {
    std::string impl, mode, mix, dist;
    int threads, ops, buckets;
    double read_ratio, p_hot;
    double time_s, thr_mops, speedup, seq_baseline_s;
};

template <class HT>
double run_workload(int threads, int total_ops, double read_ratio, bool skewed,
                    int bucket_count, double p_hot, double hot_frac) {
    HT ht(bucket_count);
    int initial = total_ops/2, mixed = total_ops - initial;

    #pragma omp parallel for num_threads(threads)
    for (int i=0;i<initial;++i) ht.insert(i, i*2);

    HotsetGen hot(initial, std::max(1, int(initial*hot_frac)), p_hot, 12345);

    double t0 = omp_get_wtime();
    #pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        std::mt19937 rng(0xC0FFEE + tid);
        std::uniform_real_distribution<double> coin(0.0,1.0);

        #pragma omp for
        for (int i=0;i<mixed;++i) {
            bool is_read = coin(rng) < read_ratio;
            int key = skewed ? hot.draw() : (i % initial);
            if (is_read) { int v; ht.search(key, v); }
            else { ht.insert(initial + i, i); }
        }
    }
    return omp_get_wtime() - t0;
}

// Per-configuration sequential baseline cache
struct BaselineKey {
    std::string mode;
    double read_ratio;
    std::string dist;
    int buckets;
    double p_hot;
    int ops;
    bool operator<(const BaselineKey& o) const {
        if (mode != o.mode) return mode < o.mode;
        if (read_ratio != o.read_ratio) return read_ratio < o.read_ratio;
        if (dist != o.dist) return dist < o.dist;
        if (buckets != o.buckets) return buckets < o.buckets;
        if (p_hot != o.p_hot) return p_hot < o.p_hot;
        return ops < o.ops;
    }
};

static double get_baseline(const BaselineKey& k, double hot_frac,
                           std::map<BaselineKey,double>& cache) {
    auto it = cache.find(k);
    if (it != cache.end()) return it->second;
    bool skewed = (k.dist == "skew");
    double t = run_workload<SequentialHashTable<int,int>>(1, k.ops, k.read_ratio,
                                                          skewed, k.buckets, k.p_hot, hot_frac);
    cache[k] = t;
    return t;
}

template <class HT>
void run_matrix_for_impl(const std::string& impl_name,
                         std::vector<Row>& out,
                         const std::vector<int>& threads_vec,
                         int strong_ops, int weak_ops_per_thread,
                         const std::vector<double>& mixes,
                         const std::vector<int>& buckets_vec,
                         const std::vector<double>& p_hots,
                         double hot_frac)
{
    std::map<BaselineKey,double> baseline_cache;

    auto sweep = [&](const std::string& mode) {
        for (double mix : mixes) {
            for (int buckets : buckets_vec) {
                // Uniform
                for (int T : threads_vec) {
                    int ops = (mode=="strong") ? strong_ops : weak_ops_per_thread * T;
                    BaselineKey bk{mode, mix, "uniform", buckets, 0.0, ops};
                    double base_t = get_baseline(bk, hot_frac, baseline_cache);

                    double t = run_workload<HT>(T, ops, mix, false, buckets, 0.0, hot_frac);
                    double thr = (double)ops / t / 1e6;
                    double spd = base_t / t;
                    out.push_back(Row{impl_name, mode, (mix==0.8?"80/20":"50/50"), "uniform",
                                      T, ops, buckets, mix, 0.0, t, thr, spd, base_t});
                    printf("%-14s %s %6s %7s  T=%2d ops=%8d buckets=%7d  time=%.4f  thr=%.2f Mops  speedup=%.2f\n",
                           impl_name.c_str(), mode.c_str(), (mix==0.8?"80/20":"50/50"), "uniform",
                           T, ops, buckets, t, thr, spd);
                }
                // Skew (p_hot sweep)
                for (double ph : p_hots) {
                    for (int T : threads_vec) {
                        int ops = (mode=="strong") ? strong_ops : weak_ops_per_thread * T;
                        BaselineKey bk{mode, mix, "skew", buckets, ph, ops};
                        double base_t = get_baseline(bk, hot_frac, baseline_cache);

                        double t = run_workload<HT>(T, ops, mix, true, buckets, ph, hot_frac);
                        double thr = (double)ops / t / 1e6;
                        double spd = base_t / t;
                        out.push_back(Row{impl_name, mode, (mix==0.8?"80/20":"50/50"), "skew",
                                          T, ops, buckets, mix, ph, t, thr, spd, base_t});
                        printf("%-14s %s %6s %7s  T=%2d ops=%8d buckets=%7d p_hot=%4.2f  time=%.4f  thr=%.2f Mops  speedup=%.2f\n",
                               impl_name.c_str(), mode.c_str(), (mix==0.8?"80/20":"50/50"), "skew",
                               T, ops, buckets, ph, t, thr, spd);
                    }
                }
            }
        }
    };

    sweep("strong");
    sweep("weak");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --impl=<coarse|fine|segment|lockfree>\n", argv[0]);
        return 1;
    }
    std::string impl_arg = argv[1];
    std::string impl;
    if (impl_arg.rfind("--impl=", 0)==0) impl = impl_arg.substr(7);
    else { fprintf(stderr, "Error: use --impl=<...>\n"); return 1; }

    const char* bind = std::getenv("OMP_PROC_BIND");
    const char* places = std::getenv("OMP_PLACES");
    fprintf(stderr, "OMP_PROC_BIND=%s  OMP_PLACES=%s\n", bind?bind:"(null)", places?places:"(null)");

    std::vector<int> threads_vec = {1,2,4,8,16};
    int strong_ops = 2'000'000;
    int weak_ops_per_thread = 250'000;
    std::vector<double> mixes = {0.8, 0.5};
    std::vector<int> buckets_vec = {16384, 65536, 262144, 1048576};
    std::vector<double> p_hots = {0.7, 0.9, 0.99};
    double hot_frac = 0.10;

    std::vector<Row> rows;

    if (impl=="coarse") {
        run_matrix_for_impl<CoarseGrainedHashTable<int,int>>("Coarse", rows, threads_vec, strong_ops, weak_ops_per_thread, mixes, buckets_vec, p_hots, hot_frac);
    } else if (impl=="fine") {
        run_matrix_for_impl<FineGrainedHashTable<int,int>>("Fine", rows, threads_vec, strong_ops, weak_ops_per_thread, mixes, buckets_vec, p_hots, hot_frac);
    } else if (impl=="segment") {
        run_matrix_for_impl<SegmentBasedHashTable<int,int>>("Segment", rows, threads_vec, strong_ops, weak_ops_per_thread, mixes, buckets_vec, p_hots, hot_frac);
    } else if (impl=="lockfree" || impl=="lock-free") {
        run_matrix_for_impl<LockFreeHashTable<int,int>>("Lock-Free", rows, threads_vec, strong_ops, weak_ops_per_thread, mixes, buckets_vec, p_hots, hot_frac);
    } else {
        fprintf(stderr, "Error: --impl must be one of coarse|fine|segment|segment-exact|lockfree\n");
        return 1;
    }

    std::cout << "CSV_RESULTS_BEGIN\n";
    std::cout << "impl,mode,mix,dist,threads,ops,bucket_count,read_ratio,p_hot,time_s,throughput_mops,speedup,seq_baseline_s\n";
    for (auto& r : rows) {
        std::cout << r.impl << "," << r.mode << "," << r.mix << "," << r.dist << ","
                  << r.threads << "," << r.ops << "," << r.buckets << ","
                  << std::fixed << std::setprecision(2) << r.read_ratio << ","
                  << std::fixed << std::setprecision(2) << r.p_hot << ","
                  << std::fixed << std::setprecision(6) << r.time_s << ","
                  << std::fixed << std::setprecision(3) << r.thr_mops << ","
                  << std::fixed << std::setprecision(3) << r.speedup << ","
                  << std::fixed << std::setprecision(6) << r.seq_baseline_s
                  << "\n";
    }
    std::cout << "CSV_RESULTS_END\n";
    return 0;
}