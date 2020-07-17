#pragma once

#ifndef __PERCEMON_UTILS_HPP__
#define __PERCEMON_UTILS_HPP__

#include <iterator>
#include <tuple>

namespace percemon {
namespace utils {

/**
 * Visit helper for a set of visitor lambdas.
 *
 * @see https://en.cppreference.com/w/cpp/utility/variant/visit
 */
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

/**
 * Python-style enumerate function for range-based for loops.
 *
 * @see http://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
 */
template <
    typename T,
    typename TIter = decltype(std::begin(std::declval<T>())),
    typename       = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable) {
  struct iterator {
    size_t i;
    TIter iter;
    bool operator!=(const iterator& other) const {
      return iter != other.iter;
    }
    void operator++() {
      ++i;
      ++iter;
    }
    auto operator*() const {
      return std::tie(i, *iter);
    }
  };
  struct iterable_wrapper {
    T iterable;
    auto begin() {
      return iterator{0, std::begin(iterable)};
    }
    auto end() {
      return iterator{0, std::end(iterable)};
    }
  };
  return iterable_wrapper{std::forward<T>(iterable)};
}

/**
 * Python-style reversed iterator
 */
template <
    typename T,
    typename = decltype(std::rbegin(std::declval<T>())),
    typename = decltype(std::rend(std::declval<T>()))>
constexpr auto reversed(T&& iterable) {
  struct iterable_wrapper {
    T iterable;
    auto begin() {
      return std::rbegin(iterable);
    }
    auto end() {
      return std::rend(iterable);
    }
  };
  return iterable_wrapper{std::forward<T>(iterable)};
}

} // namespace utils
} // namespace percemon

#endif /* end of include guard: __PERCEMON_UTILS_HPP__ */
