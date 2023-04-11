#include <benchmark/benchmark.h>
#include <lockables/guarded.hpp>

#include <iostream>

template <typename T, typename Mutex>
void BM_Guarded_Shared(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value;
  for (auto _ : state) {
    T copy{};
    if (auto guard = value.with_shared()) {
      copy = *guard;
    }

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_Shared<int, std::mutex>);
BENCHMARK(BM_Guarded_Shared<int, std::shared_mutex>);

template <typename T, typename Mutex>
void BM_Guarded_Exclusive(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value;
  for (auto _ : state) {
    T copy{};
    if (auto guard = value.with_exclusive()) {
      copy = *guard;
    }

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_Exclusive<int, std::mutex>);
BENCHMARK(BM_Guarded_Exclusive<int, std::shared_mutex>);

template <typename T, typename Mutex>
void BM_Guarded_Multiple(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value1;
  lockables::Guarded<T, Mutex> value2;
  for (auto _ : state) {
    auto copy = lockables::with_exclusive(
        [](const auto& x, const auto& y) { return x + y; }, value1, value2);

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_Multiple<int, std::mutex>);
BENCHMARK(BM_Guarded_Multiple<int, std::shared_mutex>);

template <typename Mutex>
struct BM_Guarded_Threads : benchmark::Fixture {
  lockables::Guarded<int64_t, Mutex> value{};

  void BenchmarkCase(benchmark::State& state) override {
    // First argument controls how many threads are writers.
    // For example, 8 threads and 4 writers -> 4 readers
    const bool is_writer = state.thread_index() < state.range(0);

    for (auto _ : state) {
      int64_t copy{};
      if (is_writer) {
        if (auto guard = value.with_exclusive()) {
          *guard = 100;
          copy = 1;
        }
      } else {
        if (auto guard = value.with_shared()) {
          copy = *guard;
        }
      }

      benchmark::DoNotOptimize(copy);
    }
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(BM_Guarded_Threads, MutexTest, std::mutex)
(benchmark::State& state) { BM_Guarded_Threads::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_Guarded_Threads, MutexTest)
    ->ThreadRange(4, 16)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);

BENCHMARK_TEMPLATE_DEFINE_F(BM_Guarded_Threads, SharedMutexTest,
                            std::shared_mutex)
(benchmark::State& state) { BM_Guarded_Threads::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_Guarded_Threads, SharedMutexTest)
    ->ThreadRange(4, 16)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);
