# Benchmarks

This project uses the excellent [Benchmark](https://github.com/google/benchmark)
library from Google. Most of the benchmarks compare ``std::mutex`` and
``std::shared_metux`` performance over various numbers of reader and writer
threads.

For example, the test named:

```console
BM_Guarded_Threads<std::mutex>/MutexTest/2/threads:8
```

Runs 8 total threads, 2 writer threads + 6 reader threads, all accessing one
``Guarded<int>`` value protected by a ``std::mutex``.

## Build

Use conan to build the benchmarks.

```console
conan build . --build=missing -o developer_mode=True -o enable_benchmarks=True
```

## Run

```console
./build/Release/benchmarks/lockables-bench
```

Will generate a report that looks something like this.

```console
Running ./build/Release/benchmarks/lockables-bench
Run on (8 X 24.1208 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x8)
Load Average: 1.45, 1.64, 1.73
-------------------------------------------------------------------------------------------------------------
Benchmark                                                                   Time             CPU   Iterations
-------------------------------------------------------------------------------------------------------------
BM_Guarded_Shared<int, std::mutex>                                       6.77 ns         6.77 ns     82091215
BM_Guarded_Shared<int, std::shared_mutex>                                15.5 ns         15.5 ns     45115883
BM_Guarded_Exclusive<int, std::mutex>                                    6.77 ns         6.77 ns    101840402
BM_Guarded_Exclusive<int, std::shared_mutex>                             16.9 ns         16.9 ns     42285852
BM_Guarded_Multiple<int, std::mutex>                                     13.9 ns         13.9 ns     50140752
BM_Guarded_Multiple<int, std::shared_mutex>                              30.8 ns         30.8 ns     22705378
BM_Guarded_Threads<std::mutex>/MutexTest/2/threads:4                     24.3 ns         80.3 ns     10367912
BM_Guarded_Threads<std::mutex>/MutexTest/2/threads:8                     24.6 ns          130 ns      4945120
BM_Guarded_Threads<std::mutex>/MutexTest/2/threads:16                    27.1 ns          221 ns      3233344
BM_Guarded_Threads<std::mutex>/MutexTest/4/threads:4                     16.3 ns         49.8 ns     13972752
BM_Guarded_Threads<std::mutex>/MutexTest/4/threads:8                     23.5 ns          128 ns      5604032
BM_Guarded_Threads<std::mutex>/MutexTest/4/threads:16                    23.9 ns          209 ns      3329280
BM_Guarded_Threads<std::mutex>/MutexTest/8/threads:4                     15.9 ns         48.6 ns     14068160
BM_Guarded_Threads<std::mutex>/MutexTest/8/threads:8                     21.7 ns         94.3 ns      5667616
BM_Guarded_Threads<std::mutex>/MutexTest/8/threads:16                    25.3 ns          175 ns      3429536
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/2/threads:4        81.7 ns          180 ns      3780668
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/2/threads:8         216 ns          742 ns       930712
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/2/threads:16        241 ns         1008 ns       681584
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/4/threads:4         122 ns          259 ns      2866504
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/4/threads:8         327 ns         1143 ns       753288
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/4/threads:16        341 ns         1279 ns       529440
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/8/threads:4         123 ns          262 ns      2809836
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/8/threads:8         405 ns         1455 ns       473328
BM_Guarded_Threads<std::shared_mutex>/SharedMutexTest/8/threads:16        534 ns         1958 ns       376960
```