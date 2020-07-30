#include "percemon/utils.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <itertools.hpp>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

namespace utils = percemon::utils;

int main() {
  std::map<std::string, int> m = {{"a", 2}, {"b", 4}, {"c", 10}};

  fmt::print("Original Map: {}\n", m);

  std::vector<std::string> ids = {"Var_1", "Var_2"};
  std::map<std::string, std::string> var_map;

  for (auto p : utils::product(m, std::size(ids))) {
    for (auto&& [i, kv] : iter::enumerate(p)) { var_map[ids.at(i)] = kv.first; }
    fmt::print("-- var_map = {}\n", var_map);
  }

  return 0;
}
