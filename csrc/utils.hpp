#pragma once
#ifndef PERCEMON_CSRC_UTILS_HPP
#define PERCEMON_CSRC_UTILS_HPP

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace utils {

/**
 * Helper for SNIFAE: checks if given variable is any one of the Args
 */
template <typename T, typename... Args>
struct is_one_of : std::disjunction<std::is_same<std::decay_t<T>, Args>...> {};
template <typename T, typename... Args>
inline constexpr bool is_one_of_v = is_one_of<T, Args...>::value;

/**
 * SFINAE base checking if type `T` is derived from one of `Args...`
 */
template <typename T, typename... Args>
struct base_is_one_of : std::disjunction<std::is_base_of<Args, std::decay_t<T>>...> {};
template <typename T, typename... Args>
inline constexpr bool base_is_one_of_v = is_one_of<T, Args...>::value;

/**
 * Visit helper for a set of visitor lambdas.
 *
 * @see https://en.cppreference.com/w/cpp/utility/variant/visit
 */
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/**
 * Visit helper for virtual visitor pattern
 */
template <typename Visitor, typename Base, typename MaybeDerived>
constexpr auto try_visit(Visitor op, const std::shared_ptr<Base>& base) -> bool {
  if (const auto& derived = std::dynamic_pointer_cast<MaybeDerived>(base)) {
    op(derived);
    return true;
  }
  return false;
}

template <typename Visitor, typename Base, typename... Derived>
constexpr void visit_one_of(Visitor op, const std::shared_ptr<Base>& base) {
  (try_visit<Visitor, Base, Derived>(op, base) || ...);
}

} // namespace utils
namespace iter_helpers {
/**
 * Product iterator for a single vector repeated k times.
 */
template <typename Iter>
struct product_iterator {
 public:
  using ElementType = typename std::iterator_traits<Iter>::value_type;

  using iterator_category = std::forward_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = std::vector<ElementType>;
  using pointer           = std::vector<ElementType>*;
  using reference         = std::vector<ElementType>&;

  product_iterator(Iter first, Iter last, size_t k) :
      _begin{first}, _end{last}, _k{k}, _curr{std::vector<Iter>(k, first)} {};

  auto operator!=(const product_iterator<Iter>& other) const -> bool {
    for (size_t i = 0; i < _curr.size(); i++) {
      if (_curr.at(i) != other._curr.at(i)) { return true; }
    }
    return false;
  }
  auto operator==(const product_iterator<Iter>& other) const -> bool { return !(*this != other); }

  auto operator*() const -> value_type {
    auto _curr_val = std::vector<ElementType>{};
    std::transform(
        std::begin(_curr), std::end(_curr), std::back_inserter(_curr_val), [](const auto i) {
          return *i;
        });
    return _curr_val;
  }

  auto operator++() -> product_iterator& {
    increment(_k - 1);
    return *this;
  }

  static auto get_last(Iter first, Iter last, size_t k) -> product_iterator {
    auto ret = product_iterator{first, last, k, last};
    return ret;
  }

 private:
  product_iterator(Iter first, Iter last, size_t k, Iter fill) :
      _begin{first}, _end{last}, _k{k}, _curr{std::vector<Iter>{k, fill}} {};

  /**
   * Holds the end of the iter
   */
  Iter _begin;
  Iter _end;
  /**
   * Number of repetitions in the self-product.
   */
  size_t _k;

  /**
   * Holds the current k-sized product.
   */
  std::vector<Iter> _curr;

  auto increment(int i) -> bool {
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
};

template <
    typename Container,
    typename Iter = decltype(std::begin(std::declval<Container>())),
    typename      = decltype(std::end(std::declval<Container>()))>
struct product_range {
  Container iterable;
  size_t k;

  auto begin() { return product_iterator<Iter>{std::begin(iterable), std::end(iterable), k}; }
  auto end() {
    return product_iterator<Iter>::get_last(std::begin(iterable), std::end(iterable), k);
  }
};

template <
    typename Container,
    typename Iter = decltype(std::begin(std::declval<Container>())),
    typename      = decltype(std::end(std::declval<Container>()))>
constexpr auto product(Container&& iterable, size_t k = 1) { // NOLINT
  return product_range<Container, Iter>{iterable, k};
}

// template <typename T, typename Func>
// auto map(std::optional<T> opt, Func func) -> std::optional<decltype(func(*opt))> {
//   return opt ? std::optional(func(*opt)) : std::nullopt;
// }

template <typename Func, typename... Optionals>
auto map_optionals(Func func, const Optionals&... opts) -> std::optional<decltype(func(*opts...))> {
  if ((... && opts)) { return func(*opts...); }
  return std::nullopt;
}
} // namespace iter_helpers

#endif // PERCEMON_CSRC_UTILS_HPP
