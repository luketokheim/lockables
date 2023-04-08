#pragma once

#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

namespace detail {

template <typename T, typename Mutex>
class Scope;

}  // namespace detail

/**
  Class template that stores a value alongside the mutex to protect it. Allow
  multiple reader threads or one writer thread access to the lockable object at
  a time.

  Methods return a pointer like object that also holds the lock. When the object
  goes out of scope the lock is released.

  The user must not keep a pointer or reference to the lockable object after
  the unique_ptr goes out of scope.

  Usage:

  // Parameters are forwarded to the std::vector constructor.
  lockables::Guarded<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  if (const auto guard = value.with_shared()) {
    // Guard is a std::unique_ptr<std::vector<int>>
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
*/
template <typename T, typename Mutex = std::shared_mutex>
class Guarded {
 public:
  using shared_scope = detail::Scope<const T, Mutex>;
  using exclusive_scope = detail::Scope<T, Mutex>;

  /**
    Construct a lockable object of type T. All arguments in the parameter pack
    Args are forwarded to the constructor of T.
   */
  template <typename... Args>
  Guarded(Args&&...);

  /**
    Reader thread access. Acquires a shared lock. Return a pointer like object
    to the lockable object.

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
    object to the lockable object.

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
  T value_;
  mutable Mutex mutex_;
};

template <typename T, typename Mutex>
template <typename... Args>
Guarded<T, Mutex>::Guarded(Args&&... args)
    : value_{std::forward<Args>(args)...}, mutex_{} {}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_shared() const -> shared_scope {
  return shared_scope{&value_, mutex_};
}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_exclusive() -> exclusive_scope {
  return exclusive_scope{&value_, mutex_};
}

namespace detail {

/*
  A pointer like object that owns a lock and has a non-owning pointer the
  lockable object of type T in Guarded<T>.
*/
template <typename T, typename Mutex>
class Scope {
 public:
  using pointer = T*;
  using element_type = T;

  // RAII to support std::scoped_lock.
  Scope(pointer ptr, Mutex& mutex) : ptr_{ptr}, lock_{mutex} {}

  ~Scope() = default;
  Scope(const Scope& other) = delete;
  Scope(Scope&& other) noexcept = delete;
  Scope& operator=(const Scope& other) = delete;
  Scope& operator=(Scope&& other) noexcept = delete;

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  typename std::add_lvalue_reference_t<T> operator*() const
      noexcept(noexcept(*std::declval<pointer>())) {
    return *ptr_;
  }

  pointer operator->() const noexcept { return ptr_; }

 private:
  // Use these std lock types internally for shared access from reader threads.
  // - std::shared_lock<std::shared_mutex> lock(...);
  // - std::scoped_lock<std::mutex> lock(...);
  //
  // Always use std::scoped_lock<Mutex> for writer threads.
  //
  // This means that if the user chooses std::mutex the shared and exclusive
  // locks are the same type.
  //
  // By convention, automatically selects shared access if T is const.
  using lock_type =
      std::conditional_t<std::is_const_v<T> &&
                             std::is_same_v<Mutex, std::shared_mutex>,
                         std::shared_lock<Mutex>, std::scoped_lock<Mutex>>;

  pointer ptr_;
  lock_type lock_;
};

}  // namespace detail

}  // namespace lockables
