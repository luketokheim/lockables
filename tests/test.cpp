#include <catch2/catch_template_test_macros.hpp>
#include <lockables/value.hpp>

#include <array>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>

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
      std::bind_front(std::equal_to<T>{}, kExpected);

  REQUIRE(value.with_shared(fn));
  REQUIRE(value.with_exclusive(fn));

  std::function<bool(T)> fn2 = std::bind_front(std::equal_to<T>{}, kExpected);

  REQUIRE(value.with_shared(fn2));
  REQUIRE(value.with_exclusive(fn2));

  std::function<bool(T&)> fn3 = std::bind_front(std::equal_to<T>{}, kExpected);
  // REQUIRE(value.with_shared(fn3)); // Must not compile
  REQUIRE(value.with_exclusive(fn3));
}
