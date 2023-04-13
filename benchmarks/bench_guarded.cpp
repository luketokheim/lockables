#include <benchmark/benchmark.h>
#include <lockables/guarded.hpp>

template <typename T, typename Mutex>
void BM_Guarded_Shared(benchmark::State& state) {
  lockables::Guarded<T, Mutex> value;
  for (auto _ : state) {
    T copy{};
    {
      const auto guard = value.with_shared();
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
    {
      auto guard = value.with_exclusive();
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
    T copy = lockables::with_exclusive(
        [](auto& x, auto& y) -> T { return x + y; }, value1, value2);

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK(BM_Guarded_Multiple<int, std::mutex>);
BENCHMARK(BM_Guarded_Multiple<int, std::shared_mutex>);

// Use a fixture to shared the Guarded<T> value across multiple threads that are
// spawned by the benchmark library. Benchmark also guarantees that all threads
// are running before any enter their state loop.
//
// https://github.com/google/benchmark/blob/main/docs/user_guide.md#fixtures
template <typename Mutex>
struct BM_Guarded_Fixture : benchmark::Fixture {
  lockables::Guarded<int64_t, Mutex> value{};

  void SetUp(const benchmark::State& state) override {
    auto guard = value.with_exclusive();
    *guard = 0;
  }

  void TearDown(const benchmark::State& state) override {
    auto guard = value.with_shared();
    assert(*guard == state.iterations() * state.range(0));
  }

  void BenchmarkCase(benchmark::State& state) override {
    // First argument controls how many threads are writers.
    // For example, 8 threads and 4 writers -> 4 readers
    const bool is_writer = state.thread_index() < state.range(0);

    if (is_writer) {
      RunWriter(state);
    } else {
      RunReader(state);
    }
  }

  void RunWriter(benchmark::State& state) {
    for (auto _ : state) {
      int64_t copy{};
      {
        auto guard = value.with_exclusive();
        *guard += 1;
        copy = *guard;
      }

      benchmark::DoNotOptimize(copy);
    }
  }

  void RunReader(benchmark::State& state) {
    for (auto _ : state) {
      int64_t copy{};
      {
        const auto guard = value.with_shared();
        copy = *guard;
      }

      benchmark::DoNotOptimize(copy);
    }
  }
};

BENCHMARK_TEMPLATE_DEFINE_F(BM_Guarded_Fixture, Scoped, std::mutex)
(benchmark::State& state) { BM_Guarded_Fixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, 8] writers and the rest readers.
BENCHMARK_REGISTER_F(BM_Guarded_Fixture, Scoped)
    ->ThreadRange(4, 16)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);

BENCHMARK_TEMPLATE_DEFINE_F(BM_Guarded_Fixture, Shared, std::shared_mutex)
(benchmark::State& state) { BM_Guarded_Fixture::BenchmarkCase(state); }

// Run 4-16 threads with [2, 4, 8] writers and the rest readers.
BENCHMARK_REGISTER_F(BM_Guarded_Fixture, Shared)
    ->ThreadRange(4, 16)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);
