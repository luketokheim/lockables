#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace lockables {

/**
  lockables::Value<int> value;

  value.with_exclusive([](int& x) {
    x = 10;
  });

  const int y = value.with_shared([](int x) {
    return x;
  });
*/
template <typename T, typename Mutex = std::mutex>
class Value {
 public:
  template <typename F>
  auto with_shared(F&& f) const -> std::invoke_result_t<F, const T&> {
    using result = std::invoke_result_t<F, T&>;

    static_assert(!std::is_reference_v<result>,
                  "function object must not return reference");

    static_assert(!std::is_pointer_v<result>,
                  "function object must not return pointer");

    static_assert(std::is_invocable_r_v<void, F, const T&> ||
                      std::is_invocable_r_v<std::decay_t<result>, F, const T&>,
                  "function object must return void or non const rvalue");

    shared_lock lock{mutex_};
    return std::invoke(std::forward<F>(f), std::forward<const T&>(value_));
  }

  template <typename F>
  auto with_exclusive(F&& f) -> std::invoke_result_t<F, T&> {
    using result = std::invoke_result_t<F, T&>;

    static_assert(!std::is_reference_v<result>,
                  "function object must not return reference");

    static_assert(!std::is_pointer_v<result>,
                  "function object must not return pointer");

    static_assert(std::is_invocable_r_v<void, F, T&> ||
                      std::is_invocable_r_v<std::decay_t<result>, F, T&>,
                  "function object must return void or non const rvalue");

    exclusive_lock lock{mutex_};
    return std::invoke(std::forward<F>(f), std::forward<T&>(value_));
  }

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

}  // namespace lockables
