# AGH vs Segment Summary
**Decision:** APPENDIX ONLY (exploratory)

Rationale:
- Skew mid bucket Δ=4.67% (≥10% target). Skew gain below threshold. Uniform high bucket Δ=29.86% (regression limit 5%).

## Slice Deltas
| dist | bucket | Segment (Mops/s) | AGH (Mops/s) | Δ% |
|------|--------|-----------------|--------------|----|
| skew | 262,144 | 7.03 | 7.36 | 4.67% |
| uniform | 1,048,576 | 14.71 | 19.11 | 29.86% |