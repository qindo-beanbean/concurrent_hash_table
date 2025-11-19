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

// Forward declaration for run_suite so calls parse correctly
template<typename HT>
void run_suite(const std::string& name,
               double baseline_seq,
               int total_ops,
               double read_ratio,
               const std::string& mix_label,
               bool skewed,
               std::vector<RunConfig>& out_results);

// Minimal, corrected workload runner
template<typename HT>
double run_workload(int threads, int total_ops, double read_ratio, bool skewed) {
    // Fixed-size table for this run; adjust if you want to parameterize capacity
    HT ht(16384);

    // Split into build + mixed
    const int initial = total_ops / 2;
    const int mixed_ops = total_ops - initial;

    // Build (parallel bulk insert of unique keys)
    #pragma omp parallel for num_threads(threads)
    for (int i = 0; i < initial; ++i) {
        ht.insert(i, i * 2);
    }

    // Mixed phase
    const int read_threshold = static_cast<int>(read_ratio * 10000.0);
    const int hotN = std::max(1, initial / 10); // small hot set for skewed reads

    double t0 = omp_get_wtime();
    #pragma omp parallel num_threads(threads)
    {
        // Thread-local simple LCG
        unsigned int rng_state = 0x9e3779b9u + 1013904223u * (unsigned)omp_get_thread_num();

        // Optional hot-set generator (only used when skewed)
        HotsetGen hotgen(initial, hotN, /*p_hot=*/0.90, /*seed=*/rng_state);

        #pragma omp for
        for (int i = 0; i < mixed_ops; ++i) {
            // Fast LCG update and 0..9999 threshold scale
            rng_state = rng_state * 1664525u + 1013904223u;
            int rand_val = (int)((rng_state & 0xFFFFu) * 10000u / 65536u);

            if (rand_val < read_threshold) {
                // READ: pick an existing key
                int key = skewed ? hotgen.draw() : (int)(rng_state % (unsigned)initial);
                int value;
                ht.search(key, value);
            } else {
                // WRITE: insert a unique new key
                ht.insert(initial + i, i);
            }
        }
    }
    double t1 = omp_get_wtime();
    return t1 - t0;
}

// Simple implementation of run_suite used by main()
template<typename HT>
void run_suite(const std::string& name,
               double baseline_seq,
               int total_ops,
               double read_ratio,
               const std::string& mix_label,
               bool skewed,
               std::vector<RunConfig>& out_results)
{
    for (int threads : {1, 2, 4, 8}) {
        double time_s = run_workload<HT>(threads, total_ops, read_ratio, skewed);
        double thr_mops = (time_s > 0.0) ? (total_ops / time_s) / 1e6 : 0.0;
        double sp = (baseline_seq > 0.0 && time_s > 0.0) ? (baseline_seq / time_s) : 0.0;

        cout << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time_s
             << setw(18) << fixed << setprecision(2) << thr_mops
             << setw(12) << fixed << setprecision(2) << sp << "\n";

        out_results.push_back(RunConfig{
            name,
            skewed ? "skew" : "uniform",
            mix_label,
            threads,
            total_ops,
            read_ratio,
            time_s,
            thr_mops,
            sp
        });
    }
}

// This helper referenced undefined names; keep it out of the build to avoid errors.
#if 0
template<typename HashTable>
void runParallelBenchmark(const string& name, double baseline_time) {
    cout << "\n=== " << name << " ===" << endl;
    cout << setw(10) << "Threads"
         << setw(15) << "Time (s)"
         << setw(20) << "Throughput (M/s)"
         << setw(15) << "Speedup" << endl;
    cout << string(60, '-') << endl;
    cout << "  (Speedup is relative to Sequential baseline: " << fixed << setprecision(4) << baseline_time << "s)" << endl;
    cout << string(60, '-') << endl;

    const double READ_RATIO = 0.8;

    for (int threads : {1, 2, 4, 8}) {
        double time = benchmarkWorkload<HashTable>(threads, OPERATIONS, READ_RATIO);
        double throughput = (OPERATIONS / time) / 1e6;
        double speedup = (baseline_time > 0 && time > 0) ? baseline_time / time : 0.0;

        cout << setw(10) << threads
             << setw(15) << fixed << setprecision(4) << time
             << setw(20) << fixed << setprecision(2) << throughput
             << setw(15) << fixed << setprecision(2) << speedup;
        if (speedup > 1.0) {
            cout << " (faster)";
        } else if (speedup < 1.0 && speedup > 0) {
            cout << " (slower)";
        }
        cout << endl;
    }
}
#endif

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