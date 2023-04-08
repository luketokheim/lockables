#include <catch2/catch_template_test_macros.hpp>
#include <lockables/guarded.hpp>

#include <string>
#include <vector>

TEMPLATE_PRODUCT_TEST_CASE("read and write PODs", "[lockables][Guarded]",
                           (lockables::Guarded),
                           ((int, std::mutex), (int, std::shared_mutex),
                            (double, std::mutex),
                            (double, std::shared_mutex))) {
  constexpr typename TestType::shared_scope::element_type kExpected = 100;

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
  float field2{};
  std::string field3{};

  bool operator==(const Fields&) const = default;
};

TEMPLATE_TEST_CASE("read and write struct", "[lockables][Guarded]",
                   (lockables::Guarded<Fields, std::mutex>),
                   (lockables::Guarded<Fields, std::shared_mutex>)) {
  const Fields kExpected{100, 3.14f, "Hello World!"};

  TestType value{kExpected};

  if (auto guard = value.with_shared()) {
    const auto copy = *guard;
    CHECK(copy == kExpected);
  }

  if (auto guard = value.with_exclusive()) {
    CHECK(*guard == kExpected);
    (*guard).field1 += 1;
    guard->field2 += 0.00159f;
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

TEST_CASE("examples", "[lockables][Guarded]") {
  // Parameters are forwarded to the std::vector constructor.
  lockables::Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  if (const auto guard = value.with_shared()) {
    // Guard is a std::unique_ptr<std::vector<int>>
    if (!guard->empty()) {
      int copy = guard->back();
    }
  }

  // Writer with exclusive lock.
  if (auto guard = value.with_exclusive()) {
    guard->push_back(100);
    (*guard).clear();
  }
}

TEST_CASE("with_shared examples", "[lockables][Guarded]") {
  using namespace lockables;

  Guarded<int> value;
  if (const auto guard = value.with_shared()) {
    const int copy = *guard;
  }

  Guarded<std::vector<int>> list;
  if (const auto guard = list.with_shared()) {
    if (!guard->empty()) {
      const int copy = guard->back();
    }
  }
}

TEST_CASE("with_exclusive examples", "[lockables][Guarded]") {
  using namespace lockables;

  Guarded<int> value;
  if (auto guard = value.with_exclusive()) {
    *guard = 10;
  }

  Guarded<std::vector<int>> list;
  if (auto guard = list.with_exclusive()) {
    guard->push_back(100);
  }
}