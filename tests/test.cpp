#include <catch2/catch_test_macros.hpp>

#include <lockables/guarded.hpp>

#include <algorithm>
#include <numeric>
#include <vector>

TEST_CASE("README", "[lockables][examples]") {
  {
    lockables::Guarded<int> value{100};

    {
      // The guard is a pointer like object that owns a lock on value.
      auto guard = value.with_exclusive();

      // Writer lock until guard goes out of scope.
      *guard += 10;
    }

    int copy = 0;
    {
      auto guard = value.with_shared();

      // Reader lock.
      copy = *guard;
    }

    CHECK(copy == 110);
  }

  {
    lockables::Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

    // The guard allows for multiple operations in one locked scope.
    {
      auto guard = value.with_exclusive();

      // sum = value[0] + ... + value[n - 1]
      const int sum = std::reduce(guard->begin(), guard->end());

      // value[i] = value[i] + sum(value)
      std::transform(guard->begin(), guard->end(), guard->begin(),
                     [sum](int x) { return x + sum; });

      CHECK(sum == 15);
      CHECK((*guard == std::vector<int>{16, 17, 18, 19, 20}));
    }
  }

  {
    lockables::Guarded<int> value1{10};
    lockables::Guarded<std::vector<int>> value2{1, 2, 3, 4, 5};

    const int result = lockables::with_exclusive(
        [](int& x, std::vector<int>& y) {
          // sum = (y[0] + ... + y[n - 1]) * x
          const int sum = std::reduce(y.begin(), y.end()) * x;

          // y[i] += sum
          for (auto& item : y) {
            item += sum;
          }

          return sum;
        },
        value1, value2);

    CHECK(result == 150);
  }
}

TEST_CASE("Guarded example", "[lockables][examples][Guarded]") {
  lockables::Guarded<int> value{9};
  {
    // Writer access. The mutex is locked until guard goes out of scope.
    auto guard = value.with_exclusive();

    *guard += 10;
  }

  int copy = 0;
  {
    // Reader access.
    auto guard = value.with_shared();

    copy = *guard;
    // *guard += 10;  // will not compile!
  }

  assert(copy == 19);
}

TEST_CASE("Guarded vector example", "[lockables][examples][Guarded]") {
  // Parameters are forwarded to the std::vector constructor.
  lockables::Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  {
    const auto guard = value.with_shared();

    // Deference pointer like object guard
    if (!guard->empty()) {
      int copy = guard->back();
    }
  }

  // Writer with exclusive lock.
  {
    auto guard = value.with_exclusive();

    guard->push_back(100);
    (*guard).clear();
  }
}

TEST_CASE("Guarded::with_shared example", "[lockables][examples][Guarded]") {
  lockables::Guarded<int> value;
  {
    const auto guard = value.with_shared();

    [[maybe_unused]] const int copy = *guard;
  }

  lockables::Guarded<std::vector<int>> list;
  {
    const auto guard = list.with_shared();

    if (!guard->empty()) {
      [[maybe_unused]] const int copy = guard->back();
    }
  }
}

TEST_CASE("Guarded::with_exclusive example", "[lockables][examples][Guarded]") {
  lockables::Guarded<int> value;
  {
    auto guard = value.with_exclusive();

    *guard = 10;
  }

  lockables::Guarded<std::vector<int>> list;
  {
    auto guard = list.with_exclusive();

    guard->push_back(100);
    guard->push_back(10);
  }
}

TEST_CASE("Guarded with_exclusive single example",
          "[lockables][examples][Guarded][with_exclusive]") {
  lockables::Guarded<int> value;

  lockables::with_exclusive(
      [](int& x) {
        // Writer with exclusive lock on value.
        x += 10;
      },
      value);
}

TEST_CASE("Guarded with_exclusive multiple example",
          "[lockables][examples][Guarded][with_exclusive]") {
  lockables::Guarded<int> value1{1};
  lockables::Guarded<int> value2{2};

  lockables::with_exclusive(
      [](int& x, int& y) {
        // Writer with exclusive lock on value1 and value2.
        x += y;
        y /= 2;
      },
      value1, value2);
}
