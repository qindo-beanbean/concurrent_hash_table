#include <iostream>
#include <iomanip>
#include <omp.h>
#include <random>
#include <string>
#include <vector>
#include "sequential.h"
#include "coarse_grained.h"
#include "fine_grained.h"
#include "segment_based.h"
#include "coarse_grained_padded.h"
#include "fine_grained_padded.h"
#include "segment_based_padded.h"
#include "hotset.h"

using namespace std;

struct RunConfig {
    string table_name;
    string distribution; // uniform | skew
    string mix;          // e.g. "80/20"
    int threads;
    int ops;
    double read_ratio;
    double time_sec;
    double throughput_mops;
    double speedup;
};

template<typename HT>
double run_workload(int threads, int total_ops, double read_ratio, bool skewed) {
    HT ht(16384);
    int initial = total_ops / 2;
    int mixed = total_ops - initial;

    // Pre-fill phase (parallel)
    #pragma omp parallel for num_threads(threads)
    for (int i = 0; i < initial; ++i) {
        ht.insert(i, i * 2);
    }

    HotsetGen hot(initial, max(1, initial/10), 0.9, 777); // hot 10%, 90% accesses

    double t0 = omp_get_wtime();
    #pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        std::mt19937 rng(1234 + tid);
        std::uniform_real_distribution<double> coin(0.0, 1.0);

        #pragma omp for
        for (int i = 0; i < mixed; ++i) {
            bool is_read = coin(rng) < read_ratio;
            int key = skewed ? hot.draw() : (i % initial);
            if (is_read) {
                int v;
                ht.search(key, v);
            } else {
                ht.insert(initial + i, i);
            }
        }
    }
    double t1 = omp_get_wtime();
    return t1 - t0;
}

template<typename HT>
void run_suite(const string& name,
               double baseline_seq,
               int total_ops,
               double read_ratio,
               const string& mix_label,
               bool skewed,
               vector<RunConfig>& out) {
    for (int th : {1,2,4,8,16}) {
        double t = run_workload<HT>(th, total_ops, read_ratio, skewed);
        double thr = (double)total_ops / t / 1e6;
        double speed = baseline_seq / t;
        out.push_back(RunConfig{name, skewed ? "skew" : "uniform", mix_label, th, total_ops, read_ratio, t, thr, speed});
        cout << setw(10) << th
             << setw(15) << fixed << setprecision(4) << t
             << setw(18) << fixed << setprecision(2) << thr
             << setw(12) << fixed << setprecision(2) << speed
             << endl;
    }
}

int main() {
    ios::sync_with_stdio(false);
    const int OPS = 2'000'000;

    cout << "====================================================\n";
    cout << "Concurrent Hash Table Benchmark (Option A Core)\n";
    cout << "====================================================\n";

    cout << "\nBaseline (Sequential, uniform 80/20)...\n";
    double baseline_seq = run_workload<SequentialHashTable<int,int>>(1, OPS, 0.8, false);
    cout << "Sequential Time: " << baseline_seq << " s\n";

    vector<RunConfig> results;

    auto header = [] (const string& title) {
        cout << "\n=== " << title << " ===\n";
        cout << setw(10) << "Threads"
             << setw(15) << "Time(s)"
             << setw(18) << "Throughput(Mops/s)"
             << setw(12) << "Speedup"
             << "\n" << string(60,'-') << "\n";
    };

    // Uniform 80/20
    header("Coarse-Grained uniform 80/20");
    run_suite<CoarseGrainedHashTable<int,int>>("Coarse", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Coarse-Grained-Padded uniform 80/20");
    run_suite<CoarseGrainedHashTablePadded<int,int>>("Coarse-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Fine-Grained uniform 80/20");
    run_suite<FineGrainedHashTable<int,int>>("Fine", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Fine-Grained-Padded uniform 80/20");
    run_suite<FineGrainedHashTablePadded<int,int>>("Fine-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Segment-Based uniform 80/20");
    run_suite<SegmentBasedHashTable<int,int>>("Segment", baseline_seq, OPS, 0.8, "80/20", false, results);

    header("Segment-Based-Padded uniform 80/20");
    run_suite<SegmentBasedHashTablePadded<int,int>>("Segment-Padded", baseline_seq, OPS, 0.8, "80/20", false, results);

    // Skewed 80/20
    header("Coarse-Grained skew 80/20");
    run_suite<CoarseGrainedHashTable<int,int>>("Coarse", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Coarse-Grained-Padded skew 80/20");
    run_suite<CoarseGrainedHashTablePadded<int,int>>("Coarse-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Fine-Grained skew 80/20");
    run_suite<FineGrainedHashTable<int,int>>("Fine", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Fine-Grained-Padded skew 80/20");
    run_suite<FineGrainedHashTablePadded<int,int>>("Fine-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Segment-Based skew 80/20");
    run_suite<SegmentBasedHashTable<int,int>>("Segment", baseline_seq, OPS, 0.8, "80/20", true, results);

    header("Segment-Based-Padded skew 80/20");
    run_suite<SegmentBasedHashTablePadded<int,int>>("Segment-Padded", baseline_seq, OPS, 0.8, "80/20", true, results);

    // Simple CSV dump (stdout)
    cout << "\nCSV_RESULTS_BEGIN\n";
    cout << "table,distribution,mix,threads,ops,read_ratio,time_sec,throughput_mops,speedup\n";
    for (auto& r : results) {
        cout << r.table_name << "," << r.distribution << "," << r.mix << ","
             << r.threads << "," << r.ops << "," << r.read_ratio << ","
             << fixed << setprecision(6) << r.time_sec << ","
             << fixed << setprecision(6) << r.throughput_mops << ","
             << fixed << setprecision(4) << r.speedup << "\n";
    }
    cout << "CSV_RESULTS_END\n";

    return 0;
}