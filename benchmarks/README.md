# Benchmarks

This project uses the excellent [Benchmark](https://github.com/google/benchmark)
library from Google. Most of the benchmarks compare ``std::mutex`` and
``std::shared_mutex`` performance over various numbers of reader and writer
threads.

For example, the test case named:

```console
BM_Guarded_Fixture<std::mutex>/Scoped/2/threads:8 
```

Runs 8 total threads all accessing one ``Guarded<int>`` value protected by a
``std::mutex``. In this run there are 2 writer threads and 6 reader threads.


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
Run on (8 X 4900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 256 KiB (x8)
  L3 Unified 12288 KiB (x1)
Load Average: 0.09, 0.13, 0.20
----------------------------------------------------------------------------------------------------
Benchmark                                                          Time             CPU   Iterations
----------------------------------------------------------------------------------------------------
BM_Guarded_Shared<int, std::mutex>                              4.53 ns         4.53 ns    157135470
BM_Guarded_Shared<int, std::shared_mutex>                       12.8 ns         12.8 ns     54742381
BM_Guarded_Exclusive<int, std::mutex>                           4.53 ns         4.53 ns    154471766
BM_Guarded_Exclusive<int, std::shared_mutex>                    18.9 ns         18.9 ns     36433855
BM_Guarded_Multiple<int, std::mutex>                            14.3 ns         14.3 ns     48662736
BM_Guarded_Multiple<int, std::shared_mutex>                     37.2 ns         37.2 ns     18799451
BM_Guarded_Fixture<std::mutex>/Scoped/2/threads:4               51.1 ns          183 ns      4020044
BM_Guarded_Fixture<std::mutex>/Scoped/2/threads:8               53.4 ns          361 ns      1772704
BM_Guarded_Fixture<std::mutex>/Scoped/2/threads:16              52.7 ns          379 ns      1957728
BM_Guarded_Fixture<std::mutex>/Scoped/4/threads:4               50.7 ns          181 ns      3919188
BM_Guarded_Fixture<std::mutex>/Scoped/4/threads:8               54.5 ns          398 ns      1781784
BM_Guarded_Fixture<std::mutex>/Scoped/4/threads:16              50.1 ns          396 ns      1734576
BM_Guarded_Fixture<std::mutex>/Scoped/8/threads:4               50.0 ns          179 ns      3807104
BM_Guarded_Fixture<std::mutex>/Scoped/8/threads:8               56.5 ns          411 ns      1787320
BM_Guarded_Fixture<std::mutex>/Scoped/8/threads:16              52.5 ns          418 ns      1805072
BM_Guarded_Fixture<std::shared_mutex>/Shared/2/threads:4         105 ns          244 ns      2942380
BM_Guarded_Fixture<std::shared_mutex>/Shared/2/threads:8        79.2 ns          439 ns      1665912
BM_Guarded_Fixture<std::shared_mutex>/Shared/2/threads:16       76.1 ns          713 ns      1153584
BM_Guarded_Fixture<std::shared_mutex>/Shared/4/threads:4         166 ns          653 ns      1255744
BM_Guarded_Fixture<std::shared_mutex>/Shared/4/threads:8         100 ns          493 ns      1364160
BM_Guarded_Fixture<std::shared_mutex>/Shared/4/threads:16       74.9 ns          660 ns      1159248
BM_Guarded_Fixture<std::shared_mutex>/Shared/8/threads:4         159 ns          627 ns      1233704
BM_Guarded_Fixture<std::shared_mutex>/Shared/8/threads:8         158 ns         1200 ns       606584
BM_Guarded_Fixture<std::shared_mutex>/Shared/8/threads:16        111 ns         1126 ns       709856
```
