# Lockables

Lockables are C++17 class templates for data synchronization between threads.

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

Use ``apply`` for multiple ``Guarded<T>`` values with deadlock avoidance.

```cpp
#include <lockables/guarded.hpp>

#include <vector>

int main()
{
  lockables::Guarded<int> value1{100};
  lockables::Guarded<std::vector<int>> value2{1, 2, 3, 4, 5};

  // Exclusive lock on both value1 and value2 with deadlock avoidance.
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
```

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines):
    * CP.20: Use RAII, never plain lock()/unlock()
    * CP.22: Never call unknown code while holding a lock (e.g., a callback)
    * CP.50: Define a mutex together with the data it guards. Use
    * synchronized_value<T> where possible

- [N4033](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4033.html):
    synchronized_value<T> for associating a mutex with a value

- [P0290R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0290r2.html):
    apply() for synchronized_value<T>

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
