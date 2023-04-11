# Lockables

Lockables are class templates for mutex based concurrency in C++17. Synchronize
data between multiple threads using locks.

## Quick start

[``Guarded<T>``](include/lockables/guarded.hpp) is a class template that stores
a mutex together with the value it guards.

```cpp
#include <lockables/guarded.hpp>

int main()
{
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
}
```

The [``Guarded<T>``](include/lockables/guarded.hpp) class methods return a
pointer like object that owns a lock on the guarded value.

```cpp
#include <lockables/guarded.hpp>

#include <numeric>
#include <vector>

int main()
{
  lockables::Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // The guard allows for multiple operations in the lock scope.
  if (auto guard = value.with_exclusive()) {
    // sum = value[0] + ... + value[n - 1]
    const int sum = std::reduce(guard->begin(), guard->end());

    // value = value + sum(value)
    std::transform(guard->begin(), guard->end(), guard->begin(),
                   [sum](int x) { return x + sum; });

    assert(sum == 15);
    assert((*guard == std::vector<int>{16, 17, 18, 19, 20}));
  }
}
```

Use the ``with_exclusive`` function for multiple [``Guarded<T>``](include/lockables/guarded.hpp)
values with deadlock avoidance.

```cpp
#include <lockables/guarded.hpp>

#include <vector>

int main()
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

  assert(result == 150);
}
```

## Anti-patterns, do not do this!

Solution: The user must not keep a pointer or reference to the guarded value
outside the locked scope.

```cpp
lockables::Guarded<int> value;

int* unguarded_pointer{};
if (auto guard = value.with_exclusive()) {
  // No! User must not keep a pointer or reference outside the guarded
  // scope.
  unguarded_pointer = &(*guard);
}

// No! Data race if another thread is accessing value.
// *unguarded_pointer = 1;
```

Solution: A calling thread must not own the mutex prior to calling any of the
locking functions. To lock multiple values, use the ``with_exclusive`` function
which voids deadlock.

```cpp
lockables::Guarded<int> value;

if (auto guard = value.with_exclusive()) {
  // No! Deadlock since this thread already owns a lock on value.
  // auto recursive_reader = value.with_shared();

  // No! Deadlock again.
  // auto recursive_writer = value.with_exclusive();

  // No! Deadlock again.
  // lockables::with_exclusive([](int& x) {}, value);
}
```

Solution: Use the ``with_exclusive`` function to lock multiple values. It uses
deadlock avoidance from ``std::scoped_lock``.

```cpp
lockables::Guarded<int> value1;
lockables::Guarded<int> value2;

// No! Deadlock possible if another thread locks value1 and value2 in different
// order.
// {
//   auto guard2 = value2.with_exclusive();
//   auto guard1 = value1.with_exclusive();
// }
```

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines):
    * CP.20: Use RAII, never plain lock()/unlock()
    * CP.22: Never call unknown code while holding a lock (e.g., a callback)
    * CP.50: Define a mutex together with the data it guards. Use
      synchronized_value<T> where possible

- [P2559R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2559r1.html):
  Plan for Concurrency Technical Specification Version 2
  
- [P0290R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0290r4.html):
  apply() for synchronized_value<T>

- [folly::Synchronized](https://github.com/facebook/folly/blob/main/folly/docs/Synchronized.md)

- [CsLibGuarded](https://github.com/copperspice/cs_libguarded)

- [MutexProtected](https://awesomekling.github.io/MutexProtected-A-C++-Pattern-for-Easier-Concurrency/)

## Package manager

This project uses the [Conan](https://conan.io/) C++ package manager for
Continuous Integration (CI) and to build Docker images.

## Build

The library is header only with no dependencies except the standard library. Use
conan to build unit tests.

```console
conan build . --build=missing -o developer_mode=True
```

Run tests.

```console
cd build/Release
ctest -C Release
```

See the [BUILDING](BUILDING.md) document for vanilla CMake usage and other
build options.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.
