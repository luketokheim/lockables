#include <catch2/catch_test_macros.hpp>

#include <lockables/guarded.hpp>

TEST_CASE("Guarded example", "[lockables][examples][Guarded]") {
  using namespace lockables;

  Guarded<int> value{9};
  if (auto guard = value.with_exclusive()) {
    // Writer access. The mutex is locked until guard goes out of scope.
    *guard += 10;
  }

  int copy = 0;
  if (auto guard = value.with_shared()) {
    // Reader access.
    copy = *guard;
    // *guard += 10;  // will not compile!
  }

  assert(copy == 19);
}

TEST_CASE("Guarded vector example", "[lockables][examples][Guarded]") {
  using namespace lockables;

  // Parameters are forwarded to the std::vector constructor.
  Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  if (const auto guard = value.with_shared()) {
    // Guard is a std::unique_ptr<std::vector<int>>
    if (!guard->empty()) {
      [[maybe_unused]] const int copy = guard->back();
    }
  }

  // Writer with exclusive lock.
  if (auto guard = value.with_exclusive()) {
    guard->push_back(100);
    (*guard).clear();
  }
}

TEST_CASE("Guarded::with_shared example", "[lockables][examples][Guarded]") {
  using namespace lockables;

  Guarded<int> value;
  if (const auto guard = value.with_shared()) {
    [[maybe_unused]] const int copy = *guard;
  }

  Guarded<std::vector<int>> list;
  if (const auto guard = list.with_shared()) {
    if (!guard->empty()) {
      [[maybe_unused]] const int copy = guard->back();
    }
  }
}

TEST_CASE("Guarded::with_exclusive example", "[lockables][examples][Guarded]") {
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