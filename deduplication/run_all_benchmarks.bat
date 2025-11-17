@echo off
REM Windows batch script: Run all scale deduplication performance tests

echo =====================================
echo   Running All Scale Benchmarks
echo   Deduplication Application
echo =====================================
echo.

REM Define test scales
REM Small scale: 100K items, 1K unique values
REM Medium scale: 1M items, 10K unique values  
REM Large scale: 10M items, 50K unique values

echo Step 1: Generating test data...
echo.

REM Generate test data
echo Generating small scale data (100K items, 1K unique)...
generate_dedup_data.exe data\data_small.txt 100000 1000

echo Generating medium scale data (1M items, 10K unique)...
generate_dedup_data.exe data\data_medium.txt 1000000 10000

echo Generating large scale data (10M items, 50K unique)...
generate_dedup_data.exe data\data_large.txt 10000000 50000

echo.
echo Step 2: Running benchmarks...
echo.

REM Run benchmarks
echo === Small Scale Benchmark === > results\results_small.txt
deduplication_benchmark.exe data\data_small.txt 1 2 4 8 16 >> results\results_small.txt

echo === Medium Scale Benchmark === > results\results_medium.txt
deduplication_benchmark.exe data\data_medium.txt 1 2 4 8 16 >> results\results_medium.txt

echo === Large Scale Benchmark === > results\results_large.txt
deduplication_benchmark.exe data\data_large.txt 1 2 4 8 16 >> results\results_large.txt

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

