#include <catch2/catch_template_test_macros.hpp>
#include <lockables/guarded.hpp>

#include <future>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

TEMPLATE_PRODUCT_TEST_CASE("read and write PODs", "[lockables][Guarded]",
                           (lockables::Guarded),
                           ((int, std::mutex), (int, std::shared_mutex),
                            (std::size_t, std::mutex),
                            (std::size_t, std::shared_mutex))) {
  constexpr typename TestType::shared_scope::element_type kExpected{100};

  TestType value{kExpected};

  {
    const auto guard = value.with_shared();
    const auto x = *guard;
    CHECK(x == kExpected);
  }

  {
    auto guard = value.with_exclusive();
    CHECK(*guard == kExpected);
    *guard += 1;
  }

  {
    const auto guard = value.with_shared();
    const auto x = *guard;
    CHECK(x == kExpected + 1);
  }
}

struct Fields {
  int field1{};
  int64_t field2{};
  std::string field3{};

  bool operator==(const Fields& other) const {
    return field1 == other.field1 && field2 == other.field2 &&
           field3 == other.field3;
  }

  bool operator!=(const Fields& other) const { return !operator==(other); }
};

TEMPLATE_TEST_CASE("read and write struct", "[lockables][Guarded]",
                   (lockables::Guarded<Fields, std::mutex>),
                   (lockables::Guarded<Fields, std::shared_mutex>)) {
  const Fields expected{100, 3140000, "Hello World!"};

  TestType value{expected};

  {
    auto guard = value.with_shared();
    CHECK(*guard == expected);
  }

  {
    auto guard = value.with_exclusive();
    CHECK(*guard == expected);
    (*guard).field1 += 1;
    guard->field2 += 1592;
  }

  {
    auto guard = value.with_shared();
    CHECK(guard->field1 == expected.field1 + 1);
    CHECK(*guard != expected);
  }
}

TEMPLATE_TEST_CASE("read and write container", "[lockables][Guarded]",
                   (lockables::Guarded<std::vector<int>, std::mutex>),
                   (lockables::Guarded<std::vector<int>, std::shared_mutex>)) {
  TestType value;

  for (int i = 0; i < 100; ++i) {
    {
      auto guard = value.with_exclusive();
      guard->push_back(i);
    }

    {
      const auto guard = value.with_shared();
      CHECK(!guard->empty());
      CHECK(guard->back() == i);
    }
  }
}

TEMPLATE_TEST_CASE("M reader threads, N writer threads", "[lockables][Guarded]",
                   (lockables::Guarded<int, std::mutex>),
                   (lockables::Guarded<int, std::shared_mutex>)) {
  constexpr auto kTarget = 1000;
  const std::size_t num_thread =
      std::min(std::thread::hardware_concurrency(), 8U);

  TestType value;

  const auto writer_func = [&value]() -> std::size_t {
    for (auto i = 1; i <= kTarget; ++i) {
      auto guard = value.with_exclusive();
      *guard = i;
    }

    return 0;
  };

  const auto reader_func = [&value]() -> std::size_t {
    std::size_t i = 1;
    for (; i < std::numeric_limits<std::size_t>::max(); ++i) {
      auto guard = value.with_shared();
      if (*guard >= kTarget) {
        break;
      }
    }

    return i;
  };

  // Test all combinations of M reader + N writer
  for (int num_writer = 1; num_writer < static_cast<int>(num_thread);
       ++num_writer) {
    std::vector<std::future<std::size_t>> fut_list(num_thread);

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
    CHECK(static_cast<bool>(scope));
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }

  {
    const Scope scope{&value, m};
    CHECK(scope);
    CHECK(static_cast<bool>(scope));
    CHECK(&(*scope) == &value);
    CHECK(*scope == value);
  }

  {
    Scope scope{nullptr, m};
    CHECK(!scope);
    CHECK(!static_cast<bool>(scope));
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

TEST_CASE("lockables::with_exclusive", "[lockables][examples][Guarded]") {
  lockables::Guarded<int> v1{1};
  lockables::Guarded<int> v2{2};
  lockables::Guarded<int> v3{3};
  lockables::Guarded<std::string> v4{"Hello with_exclusive"};
  lockables::Guarded<std::vector<int>> v5;

  const int sum = lockables::with_exclusive(
      [](int& x, int& y, int& z, std::string& /*str*/, std::vector<int>& list) {
        x += 10;
        y -= 20;
        z += 30;

        list.push_back(x);
        list.push_back(y);
        list.push_back(z);

        return x + y + z;
      },
      v1, v2, v3, v4, v5);

  CHECK(sum == 26);
}

TEMPLATE_TEST_CASE("all the mutex types", "[lockables][Guarded]", std::mutex,
                   std::timed_mutex, std::recursive_mutex,
                   std::recursive_timed_mutex, std::shared_mutex,
                   std::shared_timed_mutex) {
  using Mutex = TestType;

  lockables::Guarded<int, Mutex> value{10};

  int copy = 0;
  {
    const auto guard = value.with_shared();
    copy = *guard;

    using lock_type = typename decltype(guard)::lock_type;
    if constexpr (std::is_same_v<Mutex, std::shared_mutex> ||
                  std::is_same_v<Mutex, std::shared_timed_mutex>) {
      static_assert(std::is_same_v<lock_type, std::shared_lock<Mutex>>,
                    "shared_lock trait not found");
    } else {
      static_assert(std::is_same_v<lock_type, std::scoped_lock<Mutex>>,
                    "scoped_lock trait not found");
    }
  }

  CHECK(copy == 10);

  {
    auto guard = value.with_exclusive();
    *guard = copy * 2;
  }

  copy = lockables::with_exclusive([](int& x) { return x; }, value);

  CHECK(copy == 20);
}

TEST_CASE("constructor", "[lockables][Guarded]") {
  {
    lockables::Guarded<int> value{-1};
    {
      auto guard = value.with_shared();
      CHECK(*guard == -1);
    }
  }

  {
    lockables::Guarded<int> value{10};
    {
      auto guard = value.with_shared();
      CHECK(*guard == 10);
    }
  }

  {
    auto value = std::make_unique<lockables::Guarded<int>>(101);
    {
      auto guard = value->with_shared();
      CHECK(*guard == 101);
    }
  }

  {
    auto value = std::make_shared<lockables::Guarded<int>>(101);
    {
      auto guard = value->with_shared();
      CHECK(*guard == 101);
    }
  }

  {
    const std::vector<int> x(100, 1);

    lockables::Guarded<std::vector<int>> value{x};
    {
      auto guard = value.with_shared();
      CHECK(*guard == std::vector<int>(100, 1));
    }
  }

  {
    lockables::Guarded<std::vector<int>> value{std::vector<int>{1, 2, 3}};
    {
      auto guard = value.with_shared();
      CHECK(*guard == std::vector<int>{1, 2, 3});
    }
  }

  {
    lockables::Guarded<std::vector<int>> value{4, 5, 6};
    {
      auto guard = value.with_shared();
      CHECK(*guard == std::vector<int>{4, 5, 6});
    }
  }

  {
    using hash_map = std::unordered_map<std::string, int>;

    lockables::Guarded<hash_map> value{hash_map{{"Hello", 15}, {"World", 10}}};
    {
      auto guard = value.with_shared();
      CHECK(guard->at("Hello") == 15);
    }

    {
      auto guard = value.with_exclusive();
      CHECK((*guard)["Hello"] == 15);
      CHECK((*guard)["Nope"] == 0);
    }
  }

  {
    using hash_map = std::unordered_map<std::string, int>;
    hash_map map{{"Hello", 15}, {"World", 10}};

    lockables::Guarded<hash_map> value{map};
    {
      auto guard = value.with_shared();
      CHECK(guard->at("Hello") == 15);
    }
  }

  {
    using hash_map = std::unordered_map<std::string, int>;
    hash_map map{{"Hello", 15}, {"World", 10}};

    lockables::Guarded<hash_map> value{std::move(map)};
    {
      auto guard = value.with_shared();
      CHECK(guard->at("Hello") == 15);
    }
  }

  {
    using hash_map = std::unordered_map<std::string, int>;
    auto map =
        std::make_unique<hash_map>(hash_map{{"Hello", 15}, {"World", 10}});

    lockables::Guarded<std::unique_ptr<hash_map>> value{std::move(map)};
    {
      auto guard = value.with_shared();
      CHECK((*guard)->at("Hello") == 15);
    }
  }

  {
    using hash_map = std::unordered_map<std::string, std::unique_ptr<int>>;

    lockables::Guarded<hash_map> value;
    {
      auto guard = value.with_shared();
      CHECK(guard->empty());
    }
  }
}
