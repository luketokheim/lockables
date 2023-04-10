#include <catch2/catch_test_macros.hpp>

#include <lockables/guarded.hpp>

#include <future>
#include <thread>

TEST_CASE("Anti-pattern: Call with_exclusive with no values",
          "[lockables][antipatterns][Guarded]") {
  lockables::with_exclusive([]() {
    // No! We own a scoped_lock on 0 mutexes. Nothing is locked!
  });

  // Solution: Just FYI.
}

TEST_CASE("Anti-pattern: Stealing an unguarded pointer",
          "[lockables][antipatterns][Guarded]") {
  lockables::Guarded<int> value;

  // Could just be std::thread instead, same effect.
  auto future = std::async(std::launch::async, [&value]() {
    for (int i = 0; i < 1000; ++i) {
      if (auto guard = value.with_exclusive()) {
        *guard += 1;
      }
    }
  });

  [[maybe_unused]] int* unguarded_pointer{};
  if (auto guard = value.with_exclusive()) {
    // No! User must not keep a pointer or reference outside the guarded
    // scope.
    unguarded_pointer = &(*guard);

    // This may fail under normal operation! But then this test case is not
    // useful, so fail it.
    REQUIRE(*guard < 1000);
  }

  // No! Data race. 2 threads writing at the same time.
  // *unguarded_pointer = -10;

  // No! Data race. 1 thread writing, 1 reading at the same time.
  // int oops = *unguarded_pointer;

  future.wait();

  if (auto guard = value.with_exclusive()) {
    REQUIRE(*guard == 1000);
  }

  // Solution: The user must not keep a pointer or reference to the guarded
  // value after the guard object goes out of scope.
}

TEST_CASE("Anti-pattern: Deadlock with recursive guards",
          "[lockables][antipatterns][Guarded]") {
  lockables::Guarded<int> value;

  if (auto guard = value.with_exclusive()) {
    // No! Deadlock since this thread already owns a lock on value.
    // auto recursive_reader = value.with_shared();

    // No! Deadlock again.
    // auto recursive_writer = value.with_exclusive();

    // No! Deadlock again.
    // lockables::with_exclusive([](int& x) {}, value);
  }

  // Solution: A calling thread must not own the mutex prior to calling any of
  // the locking functions. To lock multiple values, use the with_exclusive
  // function which always locks in the same order.
}

TEST_CASE("Anti-pattern: Deadlock with multiple guards",
          "[lockables][antipatterns][Guarded]") {
  lockables::Guarded<int> value1;
  lockables::Guarded<int> value2;

  // Could just be std::thread instead, same effect.
  auto future = std::async(std::launch::async, [&]() {
    for (int i = 0; i < 1000; ++i) {
      auto guard1 = value1.with_exclusive();
      auto guard2 = value2.with_exclusive();
      if (guard1 && guard2) {
      }
    }
  });

  // No! Deadlock since another thread is locking value1 and value2 in a
  // different order.
  // {
  //   auto guard2 = value2.with_exclusive();
  //   auto guard1 = value1.with_exclusive();
  // }

  future.wait();

  // Solution: Use the with_exclusive function to lock multiple values. It uses
  // deadlock avoidance from std::scoped_lock.
}