#include <cppitertools/itertools.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <vector>

int main() {
  auto vec = std::vector<int>{0, 1, 2, 3, 4, 5, 6};
  for (auto e : iter::sliding_window(vec, 2)) { fmt::print("{}\n", e); }

  return 0;
}
