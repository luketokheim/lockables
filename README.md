# Lockables

Lockables are class templates for C++17 that store a mutex together with the
value it guards.

## Quick start

```cpp
#include <lockables/guarded.hpp>

int main()
{
  lockables::Guarded<int> value{100};

  if (auto guard = value.with_exclusive()) {
    // Writer lock
    *guard += 10;
  }

  int copy = 0;
  if (auto guard = value.with_shared()) {
    // Reader lock
    copy = *guard;
  }

  assert(copy == 110);
}
```

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines):
    CP.20: Use RAII, never plain lock()/unlock()
    CP.22: Never call unknown code while holding a lock (e.g., a callback)
    CP.50: Define a mutex together with the data it guards. Use
    synchronized_value<T> where possible

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
