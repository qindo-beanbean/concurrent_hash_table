@echo off
REM Windows batch script: Run all scale cache simulation performance tests

echo =====================================
echo   Running All Scale Benchmarks
echo   Cache Simulation Application
echo =====================================
echo.

REM Define test scales
REM Small scale: 1M operations, 10K key range
REM Medium scale: 10M operations, 100K key range  
REM Large scale: 50M operations, 500K key range

echo Step 1: Running benchmarks...
echo.

REM Run benchmarks
echo === Small Scale Benchmark === > results\results_small.txt
cache_sim_benchmark.exe 1000000 10000 0.8 1 2 4 8 16 >> results\results_small.txt

echo === Medium Scale Benchmark === > results\results_medium.txt
cache_sim_benchmark.exe 10000000 100000 0.8 1 2 4 8 16 >> results\results_medium.txt

echo === Large Scale Benchmark === > results\results_large.txt
cache_sim_benchmark.exe 50000000 500000 0.8 1 2 4 8 16 >> results\results_large.txt

echo.
echo =====================================
echo   All benchmarks completed!
echo =====================================
echo.
echo Results saved to:
echo   - results\results_small.txt
echo   - results\results_medium.txt
echo   - results\results_large.txt
echo.

pause

