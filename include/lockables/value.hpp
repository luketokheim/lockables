/*
  Value<T> is a class template that stores a mutex together with the value it
  guards.

  Value {
    T value
    std::mutex mutex
  }

  Users read or write the protected value by supplying a function object. Value
  acquires a read or write lock and calls the user function.

  Usage:

  Value<int> value{9};
  value.with_exclusive([](int& x) {
    // Writer access. The mutex is locked until this function returns.
    x += 10;
  });

  const int copy = value.with_shared([](const int& x) {
    // Reader access.
    // x += 10;  // will not compile!
    return x;
  });

  assert(copy == 19);
*/
#ifndef LOCKABLES_VALUE_H_
#define LOCKABLES_VALUE_H_

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

/*
  Value<T> is a class template that stores a mutex together with the value it
  guards. Allow multiple reader threads or one writer thread access to the
  lockable value at a time.

  The user supplies a function object for shared or exclusive access. The
  function is called with a reference to the lockable value that is safe to use
  until the user function returns.

  Usage:

  // Parameters are forwarded to the std::vector constructor.
  Value<std::vector<int>> value{1, 2, 3, 4, 5};

  // Reader with shared lock.
  value.with_shared([](const std::vector<int>& x) {
    if (!x.empty()) {
      int copy = x.back();
    }
  });

  // Writer with exclusive lock.
  value.with_exclusive([](std::vector<int>& x) {
    x.push_back(100);
    x.clear();
  });
 */
template <typename T, typename Mutex = std::mutex>
class Value {
 public:
  using value_type = T;

  /*
    Construct a lockable value of type T. All arguments in the parameter pack
    Args are forwarded to the constructor of T.
   */
  template <typename... Args>
  explicit Value(Args&&...);

  /*
    Reader thread access. Acquires a shared lock. Calls function object F with a
    const reference to the lockable value. Returns the result from the function
    object F.

    The user may safely access the lockable value for the duration of their
    function F. The user must not keep a reference or pointer to the lockable
    object after returning from their function F.

    Usage:

    Value<int> value{101};
    const int copy = value.with_shared([](const int& x) {
      return x;
    });

    assert(copy == 101);
  */
  template <typename F>
  std::invoke_result_t<F, const T&> with_shared(F&& f) const;

  /*
    Writer thread access. Acquires an exclusive lock. Calls function object F
    with a reference to the lockable value. Returns the result from the function
    object F.

    The user may safely access the lockable value for the duration of their
    function F. The user must not keep a reference or pointer to the lockable
    object after returning from their function F.

    Usage:

    Value<int> value;
    value.with_exclusive([](int& x) {
      x = 102;
    });
  */
  template <typename F>
  std::invoke_result_t<F, T&> with_exclusive(F&& f);

 private:
  T value_{};
  mutable Mutex mutex_{};
};

template <typename T, typename Mutex>
template <typename... Args>
Value<T, Mutex>::Value(Args&&... args)
    : value_{std::forward<Args>(args)...}, mutex_{} {}

template <typename T, typename Mutex>
template <typename F>
std::invoke_result_t<F, const T&> Value<T, Mutex>::with_shared(F&& f) const {
  static_assert(std::is_invocable_v<F, const T&>,
                "function object F does meet type requirements");

  // Use std::shared_lock for std::shared_mutex but std::scoped_lock for
  // everyone else.
  using shared_lock =
      std::conditional_t<std::is_same_v<Mutex, std::shared_mutex>,
                         std::shared_lock<Mutex>, std::scoped_lock<Mutex>>;

  shared_lock lock{mutex_};
  return std::invoke(std::forward<F>(f), std::forward<const T&>(value_));
}

template <typename T, typename Mutex>
template <typename F>
std::invoke_result_t<F, T&> Value<T, Mutex>::with_exclusive(F&& f) {
  static_assert(std::is_invocable_v<F, T&>,
                "function object F does meet type requirements");

  std::scoped_lock<Mutex> lock{mutex_};
  return std::invoke(std::forward<F>(f), std::forward<T&>(value_));
}

}  // namespace lockables

#endif  // LOCKABLES_VALUE_H_
