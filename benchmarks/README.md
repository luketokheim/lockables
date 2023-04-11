# Benchmarks

This project uses the excellent [Benchmark](benchmark) library from Google. Most
of the benchmarks compare ``std::mutex`` and ``std::shared_metux`` performance
over various numbers of reader and writer threads.

For example, the test named:

```console
BM_Guarded_Threads<std::mutex>/MutexTest/4/threads:8
```

Runs 8 total threads, 4 writer threads + 4 reader threads, all accessing one
``Guarded<int>`` value protected by a ``std::mutex``.

## Build

Use conan to build the benchmarks.

[benchmark]: (https://github.com/google/benchmark)

```console
conan build . --build=missing -o developer_mode=True -o enable_benchmarks=True
```

## Run

```console
./build/Release/benchmarks/lockables-bench
```
