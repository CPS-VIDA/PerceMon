
#include "percemon/monitoring.hpp"

#include <algorithm>
#include <numeric>
#include <optional>
#include <variant>

#include "percemon/utils.hpp"

using namespace percemon::ast;
namespace utils = percemon::utils;

namespace {

std::optional<size_t> max(std::optional<size_t> x, std::optional<size_t> y) {
  if (x.has_value() && y.has_value()) {
    return std::make_optional(std::max(*x, *y));
  } else {
    return {};
  }
}

std::optional<size_t>
interval_intersection(std::optional<size_t> x, std::optional<size_t> y) {
  if (x.has_value() && y.has_value()) {
    // NOTE: max is a safe/conservative bet, instead of computing
    // the actual intercestion of [0, x] & [0, y] = [0, min(x,y)]...
    // TODO: Check...
    return std::make_optional(std::max(*x, *y));
  } else if (x.has_value()) {
    // Here, x is bounded but y isn't. Which implies the formula is bounded by x
    return x;
  } else if (y.has_value()) {
    // Likewise for a bounded y
    return y;
  }
  // Else none are bounded.
  return {};
}

std::optional<size_t> interval_union(std::optional<size_t> x, std::optional<size_t> y) {
  // Same as max.
  return max(x, y);
}

struct HorizonCompute {
  std::optional<double> fps = {};

  HorizonCompute() = default;
  HorizonCompute(double fps_) : fps{fps_} {};

  std::optional<size_t> eval(const TemporalBoundExpr& expr) {
    return std::visit([&](const auto e) { return (*this)(e); }, expr);
  }

  std::optional<size_t> eval(const Expr& expr) {
    return std::visit([&](const auto e) { return (*this)(e); }, expr);
  }

  std::optional<size_t> operator()(const Const&) {
    return 0;
  }
  std::optional<size_t> operator()(const TimeBound& expr) {
    if (!fps.has_value()) {
      throw std::invalid_argument(
          "Cannot compute Frame horizon for formula containing TimeBound without giving fps");
    }
    auto fps_val = *fps;
    // bound is going to be positive. Switch on op
    switch (expr.op) {
      case ComparisonOp::LT: // x - CTIME < c ===> c
        return std::make_optional(size_t(expr.bound * fps_val));
      case ComparisonOp::LE: // x - CTIME <= c ===> c + 1
        return std::make_optional(1 + size_t(expr.bound * fps_val));
      case ComparisonOp::GE:
      case ComparisonOp::GT: // c <= x - CTIME or c < x - CTIME ===> Unbounded
        return {};           // TODO: Check

      default: return {};
    }
  }
  std::optional<size_t> operator()(const FrameBound& expr) {
    // bound is going to be positive. Switch on op
    switch (expr.op) {
      case ComparisonOp::LT: // f - CFRAME < c ===> c
        return std::make_optional(expr.bound);
      case ComparisonOp::LE: // f - CFRAME <= c ===> c + 1
        return std::make_optional(1 + expr.bound);
      case ComparisonOp::GE:
      case ComparisonOp::GT: // c <= f - CFRANE or c < f - CFRAME ===> Unbounded
        return {};           // TODO: Check

      default: return {};
    }
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
    auto hrz1 = std::accumulate(
        std::begin(expr->args),
        std::end(expr->args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc, const Expr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          return max(acc, rhrz);
        });
    // Does an intersection of the intervals from each temporal bound
    auto hrz2 = std::accumulate(
        std::begin(expr->temporal_bound_args),
        std::end(expr->temporal_bound_args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc,
            const TemporalBoundExpr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          return interval_intersection(acc, rhrz);
        });
    return max(hrz1, hrz2); // TODO: Will it be intersection here? I think yes.
  }

  std::optional<size_t> operator()(const OrPtr& expr) {
    auto hrz1 = std::accumulate(
        std::begin(expr->args),
        std::end(expr->args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc, const Expr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          return max(acc, rhrz);
        });
    auto hrz2 = std::accumulate(
        std::begin(expr->temporal_bound_args),
        std::end(expr->temporal_bound_args),
        std::make_optional(0),
        [&](const std::optional<size_t> acc,
            const TemporalBoundExpr e) -> std::optional<size_t> {
          const auto rhrz = this->eval(e);
          return interval_union(acc, rhrz);
        });
    return max(hrz1, hrz2);
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
    return max(lhrz, rhrz);
  }

  std::optional<size_t> operator()(const BackToPtr& expr) {
    const auto [a, b] = expr->args;
    // TODO: Verify this.
    const auto lhrz = this->eval(a);
    const auto rhrz = this->eval(b);
    return max(lhrz, rhrz);
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
