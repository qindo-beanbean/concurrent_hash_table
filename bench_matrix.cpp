#include <bits/stdc++.h>
#include <omp.h>
#include "common.h"
#include "sequential.h"
#include "coarse_grained.h"
#include "fine_grained.h"
#include "segment_based.h"
#include "lock_striped.h"

// Self-contained hot-set generator (avoids extra header dependency)
struct HotsetGen {
    int universe, hotN;
    double p_hot;
    std::mt19937 gen;
    std::uniform_int_distribution<int> hotD, coldD;
    std::uniform_real_distribution<double> coin;
    HotsetGen(int u, int h, double p, uint32_t seed)
        : universe(u), hotN(std::max(1,h)), p_hot(p), gen(seed),
          hotD(0, std::max(0,h-1)), coldD(std::min(h,u-1), std::max(0,u-1)), coin(0.0,1.0) {}
    int draw() { return (coin(gen) < p_hot) ? hotD(gen) : coldD(gen); }
};

template <class HT>
double run_workload(int threads, int total_ops, double read_ratio, bool skewed, int bucket_count, int hot_frac_pct=10, double p_hot=0.9) {
    HT ht(bucket_count);
    int initial = total_ops/2, mixed = total_ops - initial;

    #pragma omp parallel for num_threads(threads)
    for (int i=0;i<initial;++i) ht.insert(i, i*2);

    HotsetGen hot(initial, std::max(1, initial*hot_frac_pct/100), p_hot, 12345);

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

struct Row {
    std::string impl, mode, mix, dist;
    int threads, ops, buckets;
    double read_ratio, p_hot;
    double time_s, thr_mops, speedup;
};

template <class HT>
void sweep_impl(const std::string& name,
                double seq_baseline,
                bool strong_scaling,
                const std::vector<int>& threads_vec,
                const std::vector<int>& ops_vec, // if strong: same value; if weak: base ops per thread
                const std::vector<double>& mixes,
                const std::vector<int>& buckets_vec,
                const std::vector<double>& p_hots,
                std::vector<Row>& out)
{
    for (double mix : mixes) {
        for (int buckets : buckets_vec) {
            for (double p_hot : p_hots) {
                for (size_t i=0;i<threads_vec.size();++i) {
                    int T = threads_vec[i];
                    int ops = strong_scaling ? ops_vec[0] : (ops_vec[0]*T);
                    // uniform then skewed
                    for (int skew=0; skew<2; ++skew) {
                        bool is_skew = (skew==1);
                        double t = run_workload<HT>(T, ops, mix, is_skew, buckets, 10, p_hot);
                        double thr = (double)ops / t / 1e6;
                        double spd = seq_baseline / t;
                        out.push_back(Row{
                            name, strong_scaling ? "strong" : "weak",
                            (mix==0.8?"80/20":(mix==0.5?"50/50":(mix==0.95?"95/5":"mix"))),
                            is_skew?"skew":"uniform", T, ops, buckets, is_skew? p_hot:0.0,
                            t, thr, spd
                        });
                        printf("%-16s %s %6s %7s  T=%2d  ops=%9d  buckets=%6d  p_hot=%.2f  time=%.4f  thr=%.2f Mops  speedup=%.2f\n",
                               name.c_str(),
                               strong_scaling ? "STRONG" : "WEAK",
                               (mix==0.8?"80/20":(mix==0.5?"50/50":(mix==0.95?"95/5":"mix"))),
                               is_skew?"skew":"uniform",
                               T, ops, buckets, is_skew? p_hot:0.0, t, thr, spd);
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    // Print binding info (helpful for reproducibility in labs)
    const char* bind = std::getenv("OMP_PROC_BIND");
    const char* places = std::getenv("OMP_PLACES");
    fprintf(stderr, "OMP_PROC_BIND=%s  OMP_PLACES=%s\n", bind?bind:"(null)", places?places:"(null)");

    // Config
    std::vector<int> threads_vec = {1,2,4,8,16};
    int strong_ops = 2'000'000;
    int weak_ops_per_thread = 250'000;
    std::vector<int> ops_strong = {strong_ops};
    std::vector<int> ops_weak   = {weak_ops_per_thread};
    std::vector<double> mixes = {0.8, 0.5};         // 80/20, 50/50
    std::vector<int> buckets_vec = {8192, 16384, 65536};
    std::vector<double> p_hots = {0.7, 0.9, 0.99};  // skew sweep

    // Baseline (sequential, uniform 80/20, strong_ops)
    double baseline = run_workload<SequentialHashTable<int,int>>(1, strong_ops, 0.8, false, 16384);

    std::vector<Row> rows;
    // STRONG scaling
    sweep_impl<CoarseGrainedHashTable<int,int>>("Coarse", baseline, true, threads_vec, ops_strong, mixes, buckets_vec, p_hots, rows);
    sweep_impl<FineGrainedHashTable<int,int>>  ("Fine",   baseline, true, threads_vec, ops_strong, mixes, buckets_vec, p_hots, rows);
    sweep_impl<SegmentBasedHashTable<int,int>> ("Segment",baseline, true, threads_vec, ops_strong, mixes, buckets_vec, p_hots, rows);
    sweep_impl<LockStripedHashTable<int,int>>  ("Striped",baseline, true, threads_vec, ops_strong, mixes, buckets_vec, p_hots, rows);

    // WEAK scaling
    sweep_impl<CoarseGrainedHashTable<int,int>>("Coarse", baseline, false, threads_vec, ops_weak, mixes, buckets_vec, p_hots, rows);
    sweep_impl<FineGrainedHashTable<int,int>>  ("Fine",   baseline, false, threads_vec, ops_weak, mixes, buckets_vec, p_hots, rows);
    sweep_impl<SegmentBasedHashTable<int,int>> ("Segment",baseline, false, threads_vec, ops_weak, mixes, buckets_vec, p_hots, rows);
    sweep_impl<LockStripedHashTable<int,int>>  ("Striped",baseline, false, threads_vec, ops_weak, mixes, buckets_vec, p_hots, rows);

    // CSV
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
                  << std::fixed << std::setprecision(6) << baseline
                  << "\n";
    }
    std::cout << "CSV_RESULTS_END\n";
    return 0;
}