#pragma once

#ifndef __PERCEMON_UTILS_HPP__
#define __PERCEMON_UTILS_HPP__

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <vector>

namespace percemon {
namespace utils {

/**
 * Helper for SNIFAE: checks if given variable is any one of the Args
 */
template <typename T, typename... Args>
struct is_one_of : std::disjunction<std::is_same<std::decay_t<T>, Args>...> {};
template <typename T, typename... Args>
inline constexpr bool is_one_of_v = is_one_of<T, Args...>::value;

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

namespace details {
/**
 * Product iterator for a single vector repeated k times.
 */
template <typename TIter>
struct product_iterator {
 public:
  using ElementType = typename std::iterator_traits<TIter>::value_type;

  using difference_type = std::ptrdiff_t;
  using value_type      = std::vector<ElementType>;

  product_iterator(TIter first, TIter last, size_t k) :
      _begin{first}, _end{last}, _k{k}, _curr{std::vector{k, first}} {};

  bool operator!=(const product_iterator<TIter>& other) const {
    for (size_t i = 0; i < std::size(_curr); i++) {
      if (_curr.at(i) != other._curr.at(i)) {
        return true;
      }
    }
    return false;
  }
  bool operator==(const product_iterator<TIter>& other) const {
    return !(*this != other);
  }

  value_type operator*() const {
    auto _curr_val = std::vector<ElementType>{};
    std::transform(
        std::begin(_curr),
        std::end(_curr),
        std::back_inserter(_curr_val),
        [](const auto i) { return *i; });
    return _curr_val;
  }

  product_iterator& operator++() {
    increment(_k - 1);
    return *this;
  }

  static product_iterator get_last(TIter first, TIter last, size_t k) {
    auto ret = product_iterator{first, last, k, last};
    return ret;
  }

  // private:
  product_iterator(TIter first, TIter last, size_t k, TIter fill) :
      _begin{first}, _end{last}, _k{k}, _curr{std::vector{k, fill}} {};

  /**
   * Holds the end of the iter
   */
  const TIter _begin;
  const TIter _end;
  /**
   * Number of repetitions in the self-product.
   */
  size_t _k;

  /**
   * Holds the current k-sized product.
   */
  std::vector<TIter> _curr;

  bool increment(int i) {
    if (i >= 0) {
      auto& it = _curr.at(i);
      it++; // Increment the current place.
      if (it == _end) {
        bool to_reset = increment(i - 1);
        if (to_reset) {
          // Increment the next value and then reset the current.
          // Order matters, or else, all the iterators will never be std::end together.
          it = _begin;
        }
        return to_reset;
      }
      return true;
    } else {
      // This prevents the thing from being reset in the last iteration.
      return false;
    }
  }
}; // namespace details
} // namespace details

template <
    typename T,
    typename TIter = decltype(std::begin(std::declval<T>())),
    typename       = decltype(std::end(std::declval<T>()))>
constexpr auto product(T&& iterable, size_t k = 1) {
  struct product_wrapper {
    T iterable;
    size_t k;

    auto begin() {
      return details::product_iterator{std::begin(iterable), std::end(iterable), k};
    }
    auto end() {
      return details::product_iterator<TIter>::get_last(
          std::begin(iterable), std::end(iterable), k);
    }
  };
  return product_wrapper{iterable, k};
}

} // namespace utils
} // namespace percemon

#endif /* end of include guard: __PERCEMON_UTILS_HPP__ */
