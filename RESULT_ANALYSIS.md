# Benchmark Result Analysis

## Test Results Summary

### Sequential Baseline
- **Time**: 1.3880s
- **Operations**: 2,000,000
- **Throughput**: ~1.44 M ops/s

### Performance Comparison

#### 1. Coarse-Grained (Global Lock)
| Threads | Time (s) | Throughput (M/s) | Speedup |
|---------|----------|------------------|---------|
| 1       | 2.2027   | 0.91             | 0.63    |
| 2       | 3.5693   | 0.56             | 0.39    |
| 4       | 4.5326   | 0.44             | 0.31    |
| 8       | 5.6168   | 0.36             | 0.25    |

**Analysis**: 
- ❌ **Performance degrades with more threads**
- **Reason**: Global lock causes severe contention - all threads compete for the same lock
- **Conclusion**: Coarse-grained locking is not suitable for high-concurrency scenarios

#### 2. Fine-Grained (Per-Bucket Lock)
| Threads | Time (s) | Throughput (M/s) | Speedup |
|---------|----------|------------------|---------|
| 1       | 1.6111   | 1.24             | 0.86    |
| 2       | 2.0171   | 0.99             | 0.69    |
| 4       | 1.3486   | 1.48             | 1.03    |
| 8       | 0.8548   | 2.34             | **1.62** ✅ |

**Analysis**:
- ✅ **Best performance at 8 threads** (1.62x speedup)
- **Reason**: Fine-grained locking allows concurrent access to different buckets
- **Observation**: 1-2 threads slower than sequential due to lock overhead
- **Conclusion**: Fine-grained locking scales well with more threads

#### 3. Segment-Based (16 Segments)
| Threads | Time (s) | Throughput (M/s) | Speedup |
|---------|----------|------------------|---------|
| 1       | 44.2426  | 0.05             | 0.03    |
| 2       | 33.8211  | 0.06             | 0.04    |
| 4       | 29.8894  | 0.07             | 0.05    |
| 8       | 35.9814  | 0.06             | 0.04    |

**Analysis**:
- ❌ **Severely underperforming** (30-44x slower than sequential!)
- **Possible Issues**:
  1. Hash function called twice per operation (getSegmentIndex + getBucketIndex)
  2. May have implementation bugs
  3. Hash distribution may cause all keys to map to same segment
- **Conclusion**: Implementation needs investigation and optimization

#### 4. Lock-Free (Atomic Operations)
| Threads | Time (s) | Throughput (M/s) | Speedup |
|---------|----------|------------------|---------|
| 1       | 3.3716   | 0.59             | 0.41    |
| 2       | 2.7060   | 0.74             | 0.51    |
| 4       | 1.6322   | 1.23             | 0.85    |
| 8       | 1.6559   | 1.21             | 0.84    |

**Analysis**:
- ⚠️ **Performance plateaus at 4 threads**
- **Reason**: 
  - CAS (Compare-And-Swap) operations cause contention
  - Lock-free algorithms have overhead from retry loops
  - Memory ordering constraints
- **Conclusion**: Lock-free doesn't always outperform locking, especially with high contention

## Key Conclusions

### ✅ Correct Observations

1. **Fine-Grained is the best performer** at high thread counts
   - 1.62x speedup at 8 threads
   - Scales well with more threads

2. **Coarse-Grained degrades with more threads**
   - Expected behavior due to lock contention
   - Demonstrates the problem with global locks

3. **Lock-Free has overhead**
   - Single-threaded performance worse than sequential
   - CAS operations and retry loops add overhead

### ❌ Issues Found

1. **Segment-Based implementation has serious performance problems**
   - 30-44x slower than sequential
   - Needs investigation and fix

2. **All implementations slower than sequential at 1 thread**
   - Expected: Lock overhead in parallel implementations
   - Sequential has no synchronization overhead

## Recommendations

1. **Fix Segment-Based implementation**
   - Cache hash value to avoid double computation
   - Check hash distribution
   - Optimize segment selection

2. **For production use**: Fine-Grained is the best choice
   - Good scalability
   - Reasonable performance

3. **For analysis**: Results demonstrate important concepts
   - Lock contention effects
   - Trade-offs between different strategies
   - Overhead of synchronization primitives

## Expected vs Actual

| Implementation | Expected Behavior | Actual Behavior | Status |
|----------------|-------------------|-----------------|--------|
| Coarse-Grained | Poor scalability | ✅ As expected | Correct |
| Fine-Grained | Good scalability | ✅ Best performer | Correct |
| Segment-Based | Moderate performance | ❌ Very slow | **Needs fix** |
| Lock-Free | High performance | ⚠️ Moderate | Acceptable |

