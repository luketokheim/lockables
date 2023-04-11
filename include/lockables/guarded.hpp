//
// lockables/guarded.hpp
//
// Copyright 2023 Luke Tokheim
//
/**
  Guarded<T> is a class template that stores a mutex together with the value it
  guards.

  Guarded {
    T value
    std::mutex mutex
  }

  Users read or write the guarded value using the pointer like GuardedScope<T>
  object.

  GuardedScope {
    T* non_owning
    std::scoped_lock lock
  }

  Usage:

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
*/
#ifndef LOCKABLES_GUARDED_HPP_
#define LOCKABLES_GUARDED_HPP_

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

/**
  Forward declare the return type of Guarded<T> public methods. A pointer like
  object.

  Contains a non-owning pointer to a value of type T and a lock on the mutex
  that protects that value.

  GuardedScope {
    T* non_owning
    std::scoped_lock lock
  }
*/
template <typename T, typename Mutex>
class GuardedScope;

/**
  Guarded<T> is a class template that stores a mutex together with the value it
  guards. Allow multiple reader threads or one writer thread access to the
  guarded value at a time.

  Methods return the GuardedScope<T> pointer like object that also holds the
  lock. When the object goes out of scope the lock is released.

  The user must not keep a pointer or reference to the guarded value after the
  GuardedScope<T> goes out of scope.

  Usage:

  // Parameters are forwarded to the std::vector constructor.
  Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  if (const auto guard = value.with_shared()) {
    // Deference pointer like object GuardedScope<std::vector<int>>
    if (!guard->empty()) {
      int copy = guard->back();
    }
  }

  // Writer with exclusive lock.
  if (auto guard = value.with_exclusive()) {
    guard->push_back(100);
    (*guard).clear();
  }

  References:

  C++ Core Guidelines
  CP.20: Use RAII, never plain lock()/unlock()
  CP.22: Never call unknown code while holding a lock (e.g., a callback)
  CP.50: Define a mutex together with the data it guards. Use
  synchronized_value<T> where possible

  P2559R1: Plan for Concurrency Technical Specification Version 2
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2559r1.html

  P0290R4: apply() for synchronized_value<T>
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0290r4.html
*/
template <typename T, typename Mutex = std::mutex>
class Guarded {
 public:
  using shared_scope = GuardedScope<const T, Mutex>;
  using exclusive_scope = GuardedScope<T, Mutex>;

  /**
    Construct a guarded value of type T. All arguments in the parameter pack
    Args are forwarded to the constructor of T.
   */
  template <typename... Args>
  explicit Guarded(Args&&... args);

  /**
    Reader thread access. Acquires a shared lock. Return a pointer like object
    to the guarded value.

    The user holds the shared lock to until the returned pointer goes out of
    scope.

    Usage:

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
  */
  [[nodiscard]] shared_scope with_shared() const;

  /**
    Writer thread access. Acquires an exclusive lock. Return a pointer like
    object to the guarded value.

    The user holds the exclusive lock to until the returned pointer goes out of
    scope.

    Usage:

    Guarded<int> value;
    if (auto guard = value.with_exclusive()) {
        *guard = 10;
    }

    Guarded<std::vector<int>> list;
    if (auto guard = list.with_exclusive()) {
        guard->push_back(100);
    }
   */
  [[nodiscard]] exclusive_scope with_exclusive();

 private:
  T value_{};
  mutable Mutex mutex_{};

  // The with_exclusive function needs access to the internals to lock multiple
  // Guarded<T> values at once.
  template <typename F, typename... ValueTypes, typename... MutexTypes>
  friend std::invoke_result_t<F, ValueTypes&...> with_exclusive(
      F&& f, Guarded<ValueTypes, MutexTypes>&... values);
};

template <typename T, typename Mutex>
template <typename... Args>
Guarded<T, Mutex>::Guarded(Args&&... args)
    : value_{std::forward<Args>(args)...} {}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_shared() const -> shared_scope {
  return shared_scope{&value_, mutex_};
}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_exclusive() -> exclusive_scope {
  return exclusive_scope{&value_, mutex_};
}

/*
  The with_exclusive function provides access to one or more Guarded<T> objects
  from a user supplied callback.

  Basic usage:

  Guarded<int> value;
  with_exclusive([](int& x) {
    x += 10;
  }, value);

  The intent is to support locking of multiple Guarded<T> objects. The
  with_exclusive function relies on std::scoped_lock for deadlock avoidance.

  Usage:

  Guarded<int> value1{1};
  Guarded<int> value2{2};

  with_exclusive([](int& x, int& y) {
    x += y;
    y /= 2;
  }, value1, value2);

  References:

  P0290R4: apply() for synchronized_value<T>
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0290r4.html
*/
template <typename F, typename... ValueTypes, typename... MutexTypes>
std::invoke_result_t<F, ValueTypes&...> with_exclusive(
    F&& f, Guarded<ValueTypes, MutexTypes>&... values) {
  std::scoped_lock<MutexTypes...> lock{
      std::forward<MutexTypes&>(values.mutex_)...};
  return std::invoke(std::forward<F>(f),
                     std::forward<ValueTypes&>(values.value_)...);
}

/**
  Type trait to select which lock is used in GuardedScope<T>.

  Use these std lock types internally for shared access from readers.
   - std::scoped_lock<std::mutex> lock(...);
   - std::shared_lock<std::shared_mutex> lock(...);

  Always use std::scoped_lock<Mutex> for writers.

  This means that if the user chooses std::mutex the shared and exclusive locks
  are the same type.

  By convention GuardedScope<T>, automatically selects shared_lock if T is
  const.
*/
template <typename Mutex>
struct SharedLock {
  using type = std::scoped_lock<Mutex>;
};

template <typename Mutex>
using shared_lock_t = typename SharedLock<Mutex>::type;

template <>
struct SharedLock<std::shared_mutex> {
  using type = std::shared_lock<std::shared_mutex>;
};

template <>
struct SharedLock<std::shared_timed_mutex> {
  using type = std::shared_lock<std::shared_timed_mutex>;
};

/**
  GuardedScope<T> is a pointer like object that owns a lock and has a non-owning
  pointer to the guarded value of type T in Guarded<T>.
*/
template <typename T, typename Mutex>
class GuardedScope {
 public:
  // Model a restricted std::unique_ptr interface.
  using pointer = T*;
  using element_type = T;

  // Use these std lock types internally for shared access from readers.
  // - std::scoped_lock<std::mutex>
  // - std::shared_lock<std::shared_mutex>
  //
  // Always use std::scoped_lock<Mutex> for writers.
  //
  // This means that if the user chooses std::mutex the shared and exclusive
  // locks are the same type.
  //
  // By convention, automatically selects shared lock if T is const.
  using lock_type = std::conditional_t<std::is_const_v<T>, shared_lock_t<Mutex>,
                                       std::scoped_lock<Mutex>>;

  // RAII to support std::scoped_lock.
  GuardedScope(pointer ptr, Mutex& mutex) : non_owning_{ptr}, lock_{mutex} {}

  // Rule of 5. No copy or move.
  GuardedScope(const GuardedScope&) = delete;
  GuardedScope(GuardedScope&&) noexcept = delete;
  GuardedScope& operator=(const GuardedScope&) = delete;
  GuardedScope& operator=(GuardedScope&&) noexcept = delete;
  ~GuardedScope() = default;

  explicit operator bool() const noexcept { return non_owning_ != nullptr; }

  std::add_lvalue_reference_t<T> operator*() const noexcept {
    return *non_owning_;
  }

  pointer operator->() const noexcept { return non_owning_; }

 private:
  pointer non_owning_;
  lock_type lock_;
};

}  // namespace lockables

#endif  // LOCKABLES_GUARDED_HPP_
