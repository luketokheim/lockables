#include <catch2/catch_template_test_macros.hpp>
#include <lockables/value.hpp>

#include <array>
#include <functional>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>

TEMPLATE_TEST_CASE("lambda", "[lockables][Value]", std::mutex,
                   std::shared_mutex) {
  using T = int;
  using Mutex = TestType;

  constexpr T kExpected{10};

  lockables::Value<T, Mutex> value{kExpected};

  value.with_shared([](const T& x) { REQUIRE(x == kExpected); });
  value.with_shared([](T x) { REQUIRE(x == kExpected); });

  REQUIRE(value.with_shared([](const T& x) { return x; }) == kExpected);
  REQUIRE(value.with_shared([](T x) { return x; }) == kExpected);

  value.with_exclusive([](const T& x) { REQUIRE(x == kExpected); });
  value.with_exclusive([](T x) { REQUIRE(x == kExpected); });

  REQUIRE(value.with_exclusive([](const T& x) { return x; }) == kExpected);
  REQUIRE(value.with_exclusive([](T x) { return x; }) == kExpected);
}

TEMPLATE_TEST_CASE("std::function", "[lockables][Value]", std::mutex,
                   std::shared_mutex) {
  using T = int;
  using Mutex = TestType;

  constexpr T kExpected{100};

  lockables::Value<T, Mutex> value;
  value.with_exclusive([](T& x) { x = kExpected; });

  std::function<bool(const T&)> fn =
      std::bind(std::equal_to<T>{}, kExpected, std::placeholders::_1);

  REQUIRE(value.with_shared(fn));
  REQUIRE(value.with_exclusive(fn));

  std::function<bool(T)> fn2 = std::bind_front(std::equal_to<T>{}, kExpected);

  REQUIRE(value.with_shared(fn2));
  REQUIRE(value.with_exclusive(fn2));

  std::function<bool(T&)> fn3 = std::bind_front(std::equal_to<T>{}, kExpected);
  // REQUIRE(value.with_shared(fn3)); // Must not compile
  REQUIRE(value.with_exclusive(fn3));
}

TEMPLATE_PRODUCT_TEST_CASE("read and write PODs", "[lockables][Value]",
                           (lockables::Value),
                           ((int, std::mutex), (int, std::shared_mutex),
                            (std::size_t, std::mutex),
                            (std::size_t, std::shared_mutex))) {
  constexpr typename TestType::value_type kExpected{100};

  TestType value{kExpected};

  value.with_shared([](const auto& x) { CHECK(x == kExpected); });

  value.with_exclusive([](auto& x) {
    CHECK(x == kExpected);
    x += 1;
  });

  value.with_shared([](const auto& x) { CHECK(x == kExpected + 1); });
}

struct Fields {
  int field1{};
  int64_t field2{};
  std::string field3{};

  bool operator==(const Fields&) const = default;
};

TEMPLATE_TEST_CASE("read and write struct", "[lockables][Value]",
                   (lockables::Value<Fields, std::mutex>),
                   (lockables::Value<Fields, std::shared_mutex>)) {
  const Fields expected{100, 3140000, "Hello World!"};

  TestType value{expected};

  value.with_shared([expected](const auto& x) { CHECK(x == expected); });

  value.with_exclusive([expected](auto& x) {
    CHECK(x == expected);
    x.field1 += 1;
    x.field2 += 1592;
  });

  value.with_shared([expected](const auto& x) {
    CHECK(x.field1 == expected.field1 + 1);
    CHECK(x != expected);
  });
}

TEMPLATE_TEST_CASE("read and write container", "[lockables][Value]",
                   (lockables::Value<std::vector<int>, std::mutex>),
                   (lockables::Value<std::vector<int>, std::shared_mutex>)) {
  TestType value;

  for (int i = 0; i < 100; ++i) {
    value.with_exclusive([i](auto& x) { x.push_back(i); });

    value.with_shared([i](const auto& x) {
      CHECK(!x.empty());
      CHECK(x.back() == i);
    });
  }
}

TEMPLATE_TEST_CASE("M reader threads, N writer threads", "[lockables][Value]",
                   (lockables::Value<int, std::mutex>),
                   (lockables::Value<int, std::shared_mutex>)) {
  constexpr auto kTarget = 1000;
  const std::size_t kNumThread =
      std::min(std::thread::hardware_concurrency(), 8u);

  TestType value;

  const auto writer_func = [&value]() -> std::size_t {
    for (auto i = 1; i <= kTarget; ++i) {
      value.with_exclusive([i](auto& x) { x = i; });
    }

    return 0;
  };

  const auto reader_func = [&value]() -> std::size_t {
    for (std::size_t i = 1; i < std::numeric_limits<std::size_t>::max(); ++i) {
      const bool is_done =
          value.with_shared([](const auto& x) { return x >= kTarget; });
      if (is_done) {
        return i;
      }
    }

    CHECK(false);
    return 0;
  };

  // Test all combinations of M reader + N writer
  for (int num_writer = 1; num_writer < static_cast<int>(kNumThread);
       ++num_writer) {
    std::vector<std::future<std::size_t>> fut_list(kNumThread);

    auto mid = std::next(fut_list.begin(), num_writer);

    std::generate(fut_list.begin(), mid, [writer_func]() {
      return std::async(std::launch::async, writer_func);
    });

    std::generate(mid, fut_list.end(), [reader_func]() {
      return std::async(std::launch::async, reader_func);
    });

    for (auto& fut : fut_list) {
      fut.wait();
    }

    for (auto& fut : fut_list) {
      [[maybe_unused]] const auto count = fut.get();
    }
  }
}
