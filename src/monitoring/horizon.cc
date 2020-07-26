
#include "percemon/monitoring.hpp"

#include <algorithm>
#include <numeric>
#include <optional>
#include <variant>

#include "percemon/utils.hpp"

using namespace percemon::ast;
namespace utils = percemon::utils;

namespace {

struct HorizonCompute {
  std::optional<double> fps = {};

  HorizonCompute() = default;
  HorizonCompute(double fps_) : fps{fps_} {};

  std::optional<size_t> eval(const Expr& expr) {
    return std::visit([&](const auto e) { return (*this)(e); }, expr);
  }

  std::optional<size_t> operator()(const Const&) {
    return 0;
  }
  std::optional<size_t> operator()(const TimeBound&) {
    return 0;
  }
  std::optional<size_t> operator()(const FrameBound&) {
    return 0;
  }
  std::optional<size_t> operator()(const CompareId&) {
    return 0;
  }
  std::optional<size_t> operator()(const CompareClass&) {
    return 0;
  }
  std::optional<size_t> operator()(const ExistsPtr&) {
    return 0;
  }
  std::optional<size_t> operator()(const ForallPtr&) {
    return 0;
  }
  std::optional<size_t> operator()(const PinPtr&) {
    return 0;
  }
  std::optional<size_t> operator()(const NotPtr& e) {
    return this->eval(e->arg);
  }
  std::optional<size_t> operator()(const AndPtr& expr) {
    return std::accumulate(
        std::begin(expr->args),
        std::end(expr->args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc, const Expr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          if (acc.has_value() && rhrz.has_value()) {
            return std::make_optional(std::max(*acc, *rhrz));
          } else {
            return {};
          }
        });
  }
  std::optional<size_t> operator()(const OrPtr& expr) {
    return std::accumulate(
        std::begin(expr->args),
        std::end(expr->args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc, const Expr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          if (acc.has_value() && rhrz.has_value()) {
            return std::make_optional(std::max(*acc, *rhrz));
          } else {
            return {};
          }
        });
  }
  std::optional<size_t> operator()(const PreviousPtr& e) {
    const auto sub_hrz = this->eval(e->arg);
    return (sub_hrz.has_value()) ? std::make_optional(1 + *sub_hrz) : std::nullopt;
  }
  std::optional<size_t> operator()(const AlwaysPtr& expr) {
    return this->eval(expr->arg);
  }
  std::optional<size_t> operator()(const SometimesPtr& expr) {
    return this->eval(expr->arg);
  }
  std::optional<size_t> operator()(const SincePtr& expr) {
    const auto [a, b] = expr->args;
    // TODO: Verify this.
    const auto lhrz = this->eval(a);
    const auto rhrz = this->eval(b);
    if (lhrz.has_value() && rhrz.has_value()) {
      return std::make_optional(std::max(*lhrz, *rhrz));
    } else {
      return {};
    }
  }

  std::optional<size_t> operator()(const BackToPtr& expr) {
    const auto [a, b] = expr->args;
    // TODO: Verify this.
    const auto lhrz = this->eval(a);
    const auto rhrz = this->eval(b);
    if (lhrz.has_value() && rhrz.has_value()) {
      return std::make_optional(std::max(*lhrz, *rhrz));
    } else {
      return {};
    }
  }

  std::optional<size_t> operator()(const BoundingImpliesPtr& expr) {
    // NOTE: Should this be 0 and passed onto the parent operation?
    // TODO: Need to optimize if note is correct.
    size_t hrz = 0;
    if (auto opt_sub_hrz = this->eval(expr->phi)) {
      // If subformula has a bound, we add that to the horizon.
      hrz += *opt_sub_hrz;
    } // Else, the formula gets bounded by the current expression.

    if (auto timebound_ptr = std::get_if<TimeBound>(&expr->condition)) {
      if (!fps.has_value()) {
        throw std::invalid_argument(
            "Cannot compute frame horizon for TimeBounded expression if no FPS is given");
      }
      hrz += static_cast<size_t>(timebound_ptr->bound * (*fps)) +
             ((timebound_ptr->op == ComparisonOp::GE) ? 1 : 0);

    } else if (auto framebound_ptr = std::get_if<FrameBound>(&expr->condition)) {
      hrz += framebound_ptr->bound + ((framebound_ptr->op == ComparisonOp::GE) ? 1 : 0);
    }
    return std::make_optional(hrz);
  }
};
} // namespace

size_t percemon::monitoring::get_horizon(const Expr& expr) {
  auto hrz_op = HorizonCompute{};
  if (auto opt_hrz = hrz_op.eval(expr)) {
    return *opt_hrz;
  } else {
    throw std::invalid_argument(
        "Given STQL expression doesn't have a bounded horizon. Cannot perform online monitoring for this formula.");
  }
}

size_t percemon::monitoring::get_horizon(const Expr& expr, double fps) {
  auto hrz_op = HorizonCompute{fps};
  if (auto opt_hrz = hrz_op.eval(expr)) {
    return *opt_hrz;
  } else {
    throw std::invalid_argument(
        "Given STQL expression doesn't have a bounded horizon. Cannot perform online monitoring for this formula.");
  }
}
