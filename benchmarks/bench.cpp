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
struct ValueFixture : benchmark::Fixture {
  lockables::Value<int64_t, Mutex> value{};
};

BENCHMARK_TEMPLATE_DEFINE_F(ValueFixture, MutexManyReaderTest, std::mutex)
(benchmark::State& state) {
  const bool is_writer = (state.thread_index() == 0);

  for (auto _ : state) {
    if (is_writer) {
      value.with_exclusive([](auto& x) { x = 100; });
    }

    const auto copy = value.with_shared([](const auto& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK_REGISTER_F(ValueFixture, MutexManyReaderTest)->ThreadRange(2, 64);

BENCHMARK_TEMPLATE_DEFINE_F(ValueFixture, SharedMutexManyReaderTest,
                            std::shared_mutex)
(benchmark::State& state) {
  const bool is_writer = (state.thread_index() == 0);

  for (auto _ : state) {
    if (is_writer) {
      value.with_exclusive([](auto& x) { x = 100; });
    }

    const auto copy = value.with_shared([](const auto& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK_REGISTER_F(ValueFixture, SharedMutexManyReaderTest)
    ->ThreadRange(2, 64);

BENCHMARK_TEMPLATE_DEFINE_F(ValueFixture, MutexManyWriterTest, std::mutex)
(benchmark::State& state) {
  const bool is_writer = (state.thread_index() != 0);

  for (auto _ : state) {
    if (is_writer) {
      value.with_exclusive([](auto& x) { x = 100; });
    }

    const auto copy = value.with_shared([](const auto& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK_REGISTER_F(ValueFixture, MutexManyWriterTest)->ThreadRange(2, 64);

BENCHMARK_TEMPLATE_DEFINE_F(ValueFixture, SharedMutexManyWriterTest,
                            std::shared_mutex)
(benchmark::State& state) {
  const bool is_writer = (state.thread_index() != 0);

  for (auto _ : state) {
    if (is_writer) {
      value.with_exclusive([](auto& x) { x = 100; });
    }

    const auto copy = value.with_shared([](const auto& x) { return x; });

    benchmark::DoNotOptimize(copy);
  }
}

BENCHMARK_REGISTER_F(ValueFixture, SharedMutexManyWriterTest)
    ->ThreadRange(2, 64);

BENCHMARK_MAIN();
