#include <catch2/catch_test_macros.hpp>

#include <lockables/guarded.hpp>
#include <lockables/value.hpp>

TEST_CASE("README") {
  lockables::Guarded<int> value{100};

  // The guard is a pointer like object that owns a lock on value.
  if (auto guard = value.with_exclusive()) {
    // Writer lock until guard goes out of scope.
    *guard += 10;
  }

  int copy = 0;
  if (auto guard = value.with_shared()) {
    // Reader lock.
    copy = *guard;
  }

  assert(copy == 110);

  lockables::Guarded<int> value1{100};
  lockables::Guarded<std::vector<int>> value2{1, 2, 3, 4, 5};

  const int sum = lockables::apply(
      [](int& x, std::vector<int>& y) {
        int sum = 0;
        for (auto& item : y) {
          item *= x;
          sum += item;
        }

        return sum;
      },
      value1, value2);

  assert(sum == 1500);
}

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

TEST_CASE("Guarded apply single example",
          "[lockables][examples][Guarded][apply]") {
  using namespace lockables;

  Guarded<int> value;
  lockables::apply([](int& x) { x += 10; }, value);
}

TEST_CASE("Guarded apply multiple example",
          "[lockables][examples][Guarded][apply]") {
  using namespace lockables;

  Guarded<int> value1{1};
  Guarded<int> value2{2};

  lockables::apply(
      [](int& x, int& y) {
        x += y;
        y /= 2;
      },
      value1, value2);
}

TEST_CASE("Value example", "[lockables][examples][Value]") {
  using namespace lockables;

  Value<int> value{9};
  value.with_exclusive([](int& x) {
    // Writer access. The mutex is locked until this function returns.
    x += 10;
  });

  const int copy = value.with_shared([](const int& x) {
    // Reader access.
    // x += 10;  // will not compile!
    return x;
  });

  assert(copy == 19);
}

TEST_CASE("Value vector example", "[lockables][examples][Value]") {
  using namespace lockables;

  // Parameters are forwarded to the std::vector constructor.
  Value<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  value.with_shared([](const std::vector<int>& x) {
    if (!x.empty()) {
      [[maybe_unused]] int copy = x.back();
    }
  });

  // Writer with exclusive lock.
  value.with_exclusive([](std::vector<int>& x) {
    x.push_back(100);
    x.clear();
  });
}

TEST_CASE("Value::with_shared example", "[lockables][examples][Value]") {
  using namespace lockables;

  Value<int> value{101};
  const int copy = value.with_shared([](const int& x) { return x; });

  assert(copy == 101);
}

TEST_CASE("Value::with_exclusive example", "[lockables][examples][Value]") {
  using namespace lockables;

  Value<int> value;
  value.with_exclusive([](int& x) { x = 102; });
}