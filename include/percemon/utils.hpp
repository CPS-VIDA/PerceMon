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

} // namespace utils
} // namespace percemon

#endif /* end of include guard: __PERCEMON_UTILS_HPP__ */
