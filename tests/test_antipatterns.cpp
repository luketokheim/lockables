#include <catch2/catch_test_macros.hpp>

#include <lockables/guarded.hpp>

#include <future>
#include <thread>

TEST_CASE("Antipattern: Call apply with no values",
          "[lockables][antipatterns][Guarded]") {
  lockables::apply([]() {
    // No! We own a scoped_lock on 0 mutexes. Nothing is locked!
  });
}

TEST_CASE("Antipattern: Stealing an unguarded pointer",
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

  int* unguarded_pointer{};
  if (auto guard = value.with_exclusive()) {
    // No! User must not keep a pointer or reference outside the guarded
    // scope.
    unguarded_pointer = &(*guard);

    // This may fail under normal operation! But then this test case is not
    // useful, so fail it.
    REQUIRE(*guard < 1000);
  }

  // No! Race condition. 2 threads writing at the same time.
  // *unguarded_pointer = -10;

  // No! Race condition. 1 thread writing, 1 reading at the same time.
  // int oops = *unguarded_pointer;

  future.wait();

  if (auto guard = value.with_exclusive()) {
    REQUIRE(*guard == 1000);
  }
}
