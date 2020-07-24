
#include "percemon/monitoring.hpp"

#include <optional>
#include <variant>

namespace percemon::monitoring {

namespace {
using namespace ast;
struct HorizonCompute {
  std::optional<double> fps = {};

  HorizonCompute() = default;
  HorizonCompute(double fps_) : fps{fps_} {};

  size_t operator()(Const) {
    return 0;
  };
  size_t operator(const TimeBound& e) {
  };

  size_t eval(const Expr& expr) {
    return std::visit(*this, expr);
  };
};
} // namespace

size_t get_horizon(const Expr& expr) {
  auto hrz = HorizonCompute{};
  return hrz.eval(expr);
}

size_t get_horizon(const Expr& expr, double fps);

} // namespace percemon::monitoring
