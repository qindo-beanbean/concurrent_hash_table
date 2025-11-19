# AGH Tuning Summary (threads ≤ 16)
## Recommended Configuration
- S=512, MAX_STRIPES=16, STRIPE_FACTOR=3
- Skew (T=16, B=262,144): 7.59 Mops/s
- Uniform (T=16, B=1,048,576): 44.54 Mops/s

## Top by Skew
| Rank | Config | Throughput (Mops/s) |
|------|--------|---------------------|
| 1 | S512-M16-F3 | 7.59 |
| 2 | S512-M16-F2 | 7.46 |
| 3 | S512-M32-F2 | 7.42 |
| 4 | S512-M32-F1 | 7.38 |
| 5 | S512-M16-F1 | 7.38 |
| 6 | S512-M8-F2 | 7.37 |
| 7 | S512-M32-F3 | 7.37 |
| 8 | S512-M8-F3 | 7.37 |