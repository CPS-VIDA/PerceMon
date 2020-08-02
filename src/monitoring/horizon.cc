
#include "percemon/monitoring.hpp"

#include <algorithm>
#include <numeric>
#include <optional>
#include <variant>

#include "percemon/utils.hpp"

using namespace percemon::ast;
namespace utils = percemon::utils;

namespace {

constexpr std::optional<size_t> max(std::optional<size_t> x, std::optional<size_t> y) {
  if (x.has_value() && y.has_value()) {
    return std::make_optional(std::max(*x, *y));
  } else {
    return {};
  }
}

constexpr std::optional<size_t>
add_horizons(std::optional<size_t> x, std::optional<size_t> y) {
  // Adds horizon lengths
  // TODO: verify...
  if (x.has_value() && y.has_value()) {
    // if both are bounded, just add them
    return std::make_optional(*x + *y);
  } else if (x.has_value()) {
    // Example of x => y or x & y, should bound the horizon with x
    return x;
  } else if (y.has_value()) {
    // Example of y => x or y & x, should bound the horizon with y
    return y;
  }
  // Both are unbounded.
  return {};
}

constexpr std::optional<size_t>
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

constexpr std::optional<size_t>
interval_union(std::optional<size_t> x, std::optional<size_t> y) {
  // Same as max.
  return max(x, y);
}

// TODO: Revisit this to see if this sound.
// Monitorable formula is, implicitly, G(phi)
//
// {x,f} . G( c1 < x - CTIME < c2 => phi )
// --> Pin(x,f) -> Always -> OR(phi, ~(x-CTIME < c2 & x - CTIME > c1))
// --> Horizon = c2
//
// {x,f} . ( x - CTIME < c3 => phi1 )
// --> Horizon should be 0
// --> Outputted horizon should be c2

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
      default: // c <= x - CTIME or c < x - CTIME ===> Unbounded
        return {};
    }
  }
  std::optional<size_t> operator()(const FrameBound& expr) {
    // bound is going to be positive. Switch on op
    switch (expr.op) {
      case ComparisonOp::LT: // f - CFRAME < c ===> c
        return std::make_optional(expr.bound);
      case ComparisonOp::LE: // f - CFRAME <= c ===> c + 1
        return std::make_optional(1 + expr.bound);
      default: // c <= f - CFRANE or c < f - CFRAME ===> Unbounded
        return {};
    }
  }
  std::optional<size_t> operator()(const ExistsPtr& expr) {
    if (auto& pin = expr->pinned_at) {
      return this->eval(pin->phi);
    } else {
      return this->eval(*(expr->phi));
    }
  }
  std::optional<size_t> operator()(const ForallPtr& expr) {
    if (auto& pin = expr->pinned_at) {
      return this->eval(pin->phi);
    } else {
      return this->eval(*(expr->phi));
    }
  }
  std::optional<size_t> operator()(const PinPtr& expr) { return this->eval(expr->phi); }
  std::optional<size_t> operator()(const NotPtr& e) { return this->eval(e->arg); }
  std::optional<size_t> operator()(const AndPtr& expr) {
    auto hrz1 = std::optional<size_t>{};
    for (const auto& e : expr->args) {
      const auto rhrz = this->eval(e);
      hrz1            = max(hrz1, rhrz);
    }

    // Does an intersection of the intervals from each temporal bound
    auto hrz2 = std::optional<size_t>{};
    for (const auto& e : expr->temporal_bound_args) {
      const auto rhrz = this->eval(e);
      hrz2            = interval_intersection(hrz2, rhrz);
    }

    // TODO: verify...
    return add_horizons(hrz1, hrz2);
  }

  std::optional<size_t> operator()(const OrPtr& expr) {
    auto hrz1 = std::optional<size_t>{};
    for (const auto& e : expr->args) {
      const auto rhrz = this->eval(e);
      hrz1            = add_horizons(hrz1, rhrz);
    }

    auto hrz2 = std::optional<size_t>{};
    for (const auto& e : expr->temporal_bound_args) {
      const auto rhrz = this->eval(e);
      hrz2            = interval_union(hrz2, rhrz);
    }

    return add_horizons(hrz1, hrz2);
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

  // Set default horizon to 0
  template <typename T>
  std::optional<size_t> operator()(const T& expr) {
    return 0;
  }
};
} // namespace

std::optional<size_t> percemon::monitoring::get_horizon(const Expr& expr) {
  auto hrz_op = HorizonCompute{};
  return hrz_op.eval(expr);
}

std::optional<size_t> percemon::monitoring::get_horizon(const Expr& expr, double fps) {
  auto hrz_op = HorizonCompute{fps};
  return hrz_op.eval(expr);
}
