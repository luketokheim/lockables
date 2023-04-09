#include <benchmark/benchmark.h>
#include <lockables/value.hpp>

#include <thread>
#include <vector>

template <typename T, typename Mutex>
void BM_Value_CopyShared(benchmark::State& state) {
  lockables::Value<T, Mutex> value;
  for (auto _ : state) {
    const T copy = value.with_shared([](const T& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Value_CopyShared<int, std::mutex>);
BENCHMARK(BM_Value_CopyShared<int, std::shared_mutex>);

template <typename T, typename Mutex>
void BM_Value_CopyExclusive(benchmark::State& state) {
  lockables::Value<T, Mutex> value;
  for (auto _ : state) {
    const T copy = value.with_exclusive([](T& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Value_CopyExclusive<int, std::mutex>);
BENCHMARK(BM_Value_CopyExclusive<int, std::shared_mutex>);

template <typename Mutex>
struct BM_ValueFixture : benchmark::Fixture {
  lockables::Value<int64_t, Mutex> value{};

  void BenchmarkCase(benchmark::State& state) override {
    // First argument controls how many threads are writers.
    // For example, 8 threads and 4 writers -> 4 readers
    const bool is_writer = state.thread_index() < state.range(0);

    for (auto _ : state) {
      int64_t copy{};
      if (is_writer) {
        copy = value.with_exclusive([](auto& x) {
          x = 100;
          return 1;
        });
      } else {
        copy = value.with_shared([](const auto& x) { return x; });
      }

      benchmark::DoNotOptimize(copy);
    }
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(BM_ValueFixture, MutexThreadTest, std::mutex)
(benchmark::State& state) { BM_ValueFixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_ValueFixture, MutexThreadTest)
    ->ThreadRange(4, 16)
    ->DenseRange(2, 16, 2);

BENCHMARK_TEMPLATE_DEFINE_F(BM_ValueFixture, SharedMutexThreadTest,
                            std::shared_mutex)
(benchmark::State& state) { BM_ValueFixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, ..., 16) writers and the rest readers.
BENCHMARK_REGISTER_F(BM_ValueFixture, SharedMutexThreadTest)
    ->ThreadRange(4, 16)
    ->DenseRange(2, 16, 2);