#pragma once

#include <concepts>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

namespace concepts {

/**
  Not a reference or a pointer. Concept for return values from function object
  F in the Value interface. Make it harder to users to capture an unsafe
  reference from Value::with_shared and Value::with_exclusive.
*/
template <typename T>
concept NotRef = (!std::is_reference_v<T> && !std::is_pointer_v<T> &&
                  std::is_same_v<std::decay_t<T>, T>);

/** Function object concept for Value::with_shared. */
template <typename F, typename T>
concept ConstCallable = std::is_invocable_v<F, const T&> &&
                        NotRef<std::invoke_result_t<F, const T&>>;

/** Function object concept for Value::with_exclusive. */
template <typename F, typename T>
concept Callable =
    std::is_invocable_v<F, T&> && NotRef<std::invoke_result_t<F, T&>>;

}  // namespace concepts

/*
  Class template that stores a value alongside the mutex to protect it. Allow
  multiple reader threads or one writer thread access to the lockable object at
  a time.

  The user supplies a function object for shared or exclusive access. The
  function is called with a reference to the lockable object that is safe to use
  until the function returns.

  The user function object may return any type except a pointer or reference.

  Usage:

  lockables::Value<int> value{10};

  // Reader with shared lock.
  int copy = value.with_shared([](const int& x) {
    return x;
  });

  // Writer with exclusive lock.
  value.with_exclusive([](int& x) {
    x = 10;
  });
 */
template <typename T, typename Mutex = std::shared_mutex>
class Value {
 public:
  /*
    Construct a lockable object of type T. All arguments in the parameter pack
    Args are forwarded to the constructor of T.
   */
  template <typename... Args>
  Value(Args&&...);

  /*
    Reader thread access. Acquires a shared lock. Calls function object F with a
    const reference to the lockable object. Returns the result from the function
    object F.

    The user may safely access the lockable object for the duration of their
    function F. The user must not keep a reference or pointer to the lockable
    object after returning from their function F.

    Usage:

    Value<int> value{101};
    const int copy = value.with_shared([](const int& x) {
      return x;
    });
  */
  template <typename F>
    requires concepts::ConstCallable<F, T>
  std::invoke_result_t<F, const T&> with_shared(F&& f) const;

  /*
    Writer thread access. Acquires an exclusive lock. Calls functio object F
    with a reference to the lockable object. Returns the result from the
    function object F.

    The user may safely access the lockable object for the duration of their
    function F. The user must not keep a reference or pointer to the lockable
    object after returning from their function F.

    Usage:

    Value<int> value;
    value.with_exclusive([](int& x) {
      x = 102;
    });
  */
  template <typename F>
    requires concepts::Callable<F, T>
  std::invoke_result_t<F, T&> with_exclusive(F&& f);

 private:
  // Use std::shared_lock for std::shared_mutex but std::scoped_lock for
  // everyone else.
  using shared_lock =
      std::conditional_t<std::is_same_v<Mutex, std::shared_mutex>,
                         std::shared_lock<Mutex>, std::scoped_lock<Mutex>>;

  // Use std::unique_lock for std::shared_mutex but std::scoped_lock for
  // everyone else.
  using exclusive_lock =
      std::conditional_t<std::is_same_v<Mutex, std::shared_mutex>,
                         std::unique_lock<Mutex>, std::scoped_lock<Mutex>>;

  T value_{};
  mutable Mutex mutex_{};
};

template <typename T, typename Mutex>
template <typename... Args>
Value<T, Mutex>::Value(Args&&... args)
    : value_{std::forward<Args>(args)...}, mutex_{} {}

template <typename T, typename Mutex>
template <typename F>
  requires concepts::ConstCallable<F, T>
std::invoke_result_t<F, const T&> Value<T, Mutex>::with_shared(F&& f) const {
  shared_lock lock{mutex_};
  return std::invoke(std::forward<F>(f), std::forward<const T&>(value_));
}

template <typename T, typename Mutex>
template <typename F>
  requires concepts::Callable<F, T>
std::invoke_result_t<F, T&> Value<T, Mutex>::with_exclusive(F&& f) {
  exclusive_lock lock{mutex_};
  return std::invoke(std::forward<F>(f), std::forward<T&>(value_));
}

}  // namespace lockables
