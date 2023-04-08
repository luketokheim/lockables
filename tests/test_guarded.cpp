#include <catch2/catch_template_test_macros.hpp>
#include <lockables/guarded.hpp>

#include <future>
#include <string>
#include <thread>
#include <vector>

TEMPLATE_PRODUCT_TEST_CASE("read and write PODs", "[lockables][Guarded]",
                           (lockables::Guarded),
                           ((int, std::mutex), (int, std::shared_mutex),
                            (std::size_t, std::mutex),
                            (std::size_t, std::shared_mutex))) {
  constexpr typename TestType::shared_scope::element_type kExpected{100};

  TestType value{kExpected};

  if (const auto guard = value.with_shared()) {
    const auto x = *guard;
    CHECK(x == kExpected);
  }

  if (auto guard = value.with_exclusive()) {
    CHECK(*guard == kExpected);
    *guard += 1;
  }

  if (const auto guard = value.with_shared()) {
    const auto x = *guard;
    CHECK(x == kExpected + 1);
  }
}

struct Fields {
  int field1{};
  int64_t field2{};
  std::string field3{};

  bool operator==(const Fields&) const = default;
};

TEMPLATE_TEST_CASE("read and write struct", "[lockables][Guarded]",
                   (lockables::Guarded<Fields, std::mutex>),
                   (lockables::Guarded<Fields, std::shared_mutex>)) {
  const Fields kExpected{100, 3140000, "Hello World!"};

  TestType value{kExpected};

  if (auto guard = value.with_shared()) {
    const auto copy = *guard;
    CHECK(copy == kExpected);
  }

  if (auto guard = value.with_exclusive()) {
    CHECK(*guard == kExpected);
    (*guard).field1 += 1;
    guard->field2 += 1592;
  }

  if (auto guard = value.with_shared()) {
    CHECK(guard->field1 == kExpected.field1 + 1);
    CHECK(*guard != kExpected);
  }
}

TEMPLATE_TEST_CASE("read and write container", "[lockables][Guarded]",
                   (lockables::Guarded<std::vector<int>, std::mutex>),
                   (lockables::Guarded<std::vector<int>, std::shared_mutex>)) {
  TestType value;

  for (int i = 0; i < 100; ++i) {
    if (auto guard = value.with_exclusive()) {
      guard->push_back(i);
    }

    if (const auto guard = value.with_shared()) {
      CHECK(!guard->empty());
      CHECK(guard->back() == i);
    }
  }
}

TEMPLATE_TEST_CASE("M reader threads, N writer threads", "[lockables][Guarded]",
                   (lockables::Guarded<int, std::mutex>),
                   (lockables::Guarded<int, std::shared_mutex>)) {
  constexpr auto kTarget = 1000;
  const std::size_t kNumThread =
      std::min(std::thread::hardware_concurrency(), 8u);

  TestType value;

  const auto writer_func = [&value]() -> std::size_t {
    for (auto i = 1; i <= kTarget; ++i) {
      if (auto guard = value.with_exclusive()) {
        *guard = i;
      } else {
        CHECK(false);
      }
    }

    return 0;
  };

  const auto reader_func = [&value]() -> std::size_t {
    for (std::size_t i = 1; i < std::numeric_limits<std::size_t>::max(); ++i) {
      if (auto guard = value.with_shared()) {
        if (*guard >= kTarget) {
          return i;
        }
      } else {
        CHECK(false);
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

TEMPLATE_TEST_CASE("operators", "[lockables][GuardedScope]",
                   (lockables::GuardedScope<int, std::mutex>),
                   (lockables::GuardedScope<const int, std::mutex>),
                   (lockables::GuardedScope<int, std::shared_mutex>),
                   (lockables::GuardedScope<const int, std::shared_mutex>)) {
  using Scope = TestType;
  using T = typename TestType::element_type;
  using Mutex = typename TestType::lock_type::mutex_type;

  T value = 10;
  Mutex m;

  {
    Scope scope{&value, m};
    CHECK(scope);
    if (!scope) {
      CHECK(false);
    }
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }

  {
    const Scope scope{&value, m};
    CHECK(scope);
    if (!scope) {
      CHECK(false);
    }
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }

  {
    Scope scope{nullptr, m};
    CHECK(!scope);
    if (scope) {
      CHECK(false);
    }
  }

  if (auto scope = Scope{&value, m}) {
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }

  if (const auto scope = Scope{&value, m}) {
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }
}
