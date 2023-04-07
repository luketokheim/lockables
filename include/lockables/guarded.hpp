#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

/**
  Class template that stores a value alongside the mutex to protect it. Allow
  multiple reader threads or one writer thread access to the lockable object at
  a time.

  Methods return a pointer like object (std::unique_ptr) that also holds the
  lock. When the object goes out of scope the lock is released.

  The user must not keep a pointer or reference to the lockable object after
  the unique_ptr goes out of scope.

  Usage:

  // Parameters are forwarded to the std::vector constructor.
  lockables::Guarded<std::vector<int>> value{100, 1};

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
*/
template <typename T, typename Mutex = std::shared_mutex>
class Guarded {
 public:
  // Use these std lock types internally for shared access from reader threads.
  // - std::shared_lock<std::shared_mutex> lock(...);
  // - std::unique_lock<std::mutex> lock(...);
  //
  // This means that if the user chooses std::mutex the shared and exclusive
  // locks are the same type.
  using shared_lock =
      std::conditional_t<std::is_same_v<Mutex, std::shared_mutex>,
                         std::shared_lock<Mutex>, std::unique_lock<Mutex>>;

  // Always use std::unique_lock because we need a movable lock type.
  using exclusive_lock = std::unique_lock<Mutex>;

  // Custom deleter.
  template <typename Lock>
  class Deleter;

  using shared_scope = std::unique_ptr<const T, Deleter<shared_lock>>;
  using exclusive_scope = std::unique_ptr<T, Deleter<exclusive_lock>>;

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
template <typename Lock>
class Guarded<T, Mutex>::Deleter {
 public:
  explicit Deleter(Lock&& lock) : lock_{std::move(lock)} {
    assert(lock_.owns_lock());
  }

  void operator()(const T* ptr) const {
    //
    assert(lock_.owns_lock());
  }

  void operator()(T* ptr) {
    //
    assert(lock_.owns_lock());
  }

 private:
  Lock lock_;
};

template <typename T, typename Mutex>
template <typename... Args>
Guarded<T, Mutex>::Guarded(Args&&... args)
    : value_{std::forward<Args>(args)...}, mutex_{} {}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_shared() const -> shared_scope {
  shared_lock lock{mutex_};
  return shared_scope{&value_, Deleter<shared_lock>{std::move(lock)}};
}

template <typename T, typename Mutex>
auto Guarded<T, Mutex>::with_exclusive() -> exclusive_scope {
  exclusive_lock lock{mutex_};
  return exclusive_scope{&value_, Deleter<exclusive_lock>{std::move(lock)}};
}

}  // namespace lockables
