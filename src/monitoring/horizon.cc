
#include "percemon/monitoring.hpp"

#include <optional>
#include <variant>
#include <algorithm>
#include <numeric>

#include "percemon/utils.hpp"

namespace percemon::monitoring {

namespace {
using namespace ast;
struct HorizonCompute {
  std::optional<double> fps = {};

  HorizonCompute() = default;
  HorizonCompute(double fps_) : fps{fps_} {};

  template <typename T>
  size_t compute(const T&) {
    return 0;
  };

  template <>
  size_t compute<TimeBound>(const TimeBound& e) {
    if (!fps.has_value()) {
      throw std::invalid_argument(
          "Trying to get the horizon (in number of frames) with a TimeBound in the formula. No FPS provided.");
    }
    const double fps_val        = *fps;
    const double frame_duration = 1 / fps_val;
    double bound                = e.bound;
    if (e.op == ComparisonOp::GT) { // Remove 1 frame
      bound += frame_duration;
    }
    // TODO(anand): Verify this correctness.
    return static_cast<size_t>(bound * fps_val);
  };

  template <>
  size_t compute<FrameBound>(const FrameBound& e) {
    if (e.op == ComparisonOp::GT) {
      return e.bound - 1;
    }
    return e.bound;
  };

  template <typename T, std::enable_if_t<utils::is_one_of_v<T, AndPtr, OrPtr>>>
      size_t compute(const T& e){
        std::vector<size_t> sub_horizons;
        std::transform<
        return std::max(
      }

      size_t eval(const Expr& expr) {
    return std::visit(
        [&](const auto e) { return this->compute<std::decay_t<decltype(e)>>(e); },
        expr);
  };
};
} // namespace

size_t get_horizon(const Expr& expr) {
  auto hrz = HorizonCompute{};
  return hrz.eval(expr);
}

size_t get_horizon(const Expr& expr, double fps);

} // namespace percemon::monitoring
