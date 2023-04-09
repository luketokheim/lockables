#include <benchmark/benchmark.h>
#include <lockables/guarded.hpp>

#include <iostream>

template <typename T, typename Mutex>
void BM_Guarded_CopyShared(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value;
  for (auto _ : state) {
    T copy{};
    if (auto guard = value.with_shared()) {
      copy = *guard;
    }

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_CopyShared<int, std::mutex>);
BENCHMARK(BM_Guarded_CopyShared<int, std::shared_mutex>);

template <typename T, typename Mutex>
void BM_Guarded_CopyExclusive(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value;
  for (auto _ : state) {
    T copy{};
    if (auto guard = value.with_exclusive()) {
      copy = *guard;
    }

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_CopyExclusive<int, std::mutex>);
BENCHMARK(BM_Guarded_CopyExclusive<int, std::shared_mutex>);

template <typename Mutex>
struct BM_GuardedFixture : benchmark::Fixture {
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

BENCHMARK_TEMPLATE_DEFINE_F(BM_GuardedFixture, MutexThreadTest, std::mutex)
(benchmark::State& state) { BM_GuardedFixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_GuardedFixture, MutexThreadTest)
    ->ThreadRange(4, 16)
    ->DenseRange(2, 16, 2);

BENCHMARK_TEMPLATE_DEFINE_F(BM_GuardedFixture, SharedMutexThreadTest,
                            std::shared_mutex)
(benchmark::State& state) { BM_GuardedFixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_GuardedFixture, SharedMutexThreadTest)
    ->ThreadRange(4, 16)
    ->DenseRange(2, 16, 2);
