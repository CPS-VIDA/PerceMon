#include "percemon/monitoring.hpp"
#include "percemon/stql.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "utils.hpp"

namespace percemon::monitoring {

// =============================================================================
// Internal Helper Types and Functions
// =============================================================================

namespace {

/**
 * @brief Direction of temporal scope (future or past)
 *
 * Determines which constraint forms are meaningful:
 * - Future: only (C_TIME - var ∼ bound) constraints matter
 * - Past: only (var - C_TIME ∼ bound) constraints matter
 */
enum class ScopeDirection : std::uint8_t { Future, Past };

/**
 * @brief Represents a bound extracted from a constraint
 *
 * Stores the number of frames and whether it's in the future or past direction.
 */
struct ConstraintBound {
  int64_t frames;           ///< Number of frames
  ScopeDirection direction; ///< Whether this bounds the future or past

  /// Check if bound is meaningful in the given scope
  [[nodiscard]] auto is_meaningful_in_scope(ScopeDirection scope) const -> bool {
    return direction == scope;
  }
};

/**
 * @brief Try to extract a constraint bound from a TimeBoundExpr
 *
 * Only extracts bounds when the constraint has the correct form for the scope:
 * - Future scope: only (C_TIME - var ∼ n) bounds matter
 * - Past scope: only (var - C_TIME ∼ n) bounds matter
 *
 * @param expr The TimeBoundExpr to analyze
 * @param fps Frames per second for time-to-frame conversion
 * @param scope Current temporal scope direction
 * @return Optional bound if constraint is meaningful in this scope
 */
auto extract_time_bound_constraint(
    const stql::TimeBoundExpr& expr,
    double fps,
    ScopeDirection scope) -> std::optional<ConstraintBound> {
  // using namespace stql;
  using stql::CompareOp;

  // Extract the structure of the time difference
  // TimeDiff contains: lhs_var (either TimeVar or C_TIME), rhs_var (either TimeVar or C_TIME)
  const auto& diff = expr.diff;
  // Convert time to frames
  auto frames = static_cast<int64_t>(std::ceil(expr.value * fps));

  // // Check the structure
  // if (std::holds_alternative<TimeVar>(diff.lhs) && std::holds_alternative<TimeVar>(diff.rhs)) {
  //   // Both are variables - not a meaningful constraint for our purposes
  //   return std::nullopt;
  // }

  // Determine the constraint form
  const bool lhs_is_c_time = diff.lhs.name == "C_TIME";
  const bool rhs_is_c_time = diff.rhs.name == "C_TIME";

  // Map constraint operator and scope to direction
  // C_TIME - var < n means C_TIME < var + n, so we can look up to n frames forward (future)
  // var - C_TIME < n means var < C_TIME + n, so we can look up to n frames back (past)
  // Determine which form this constraint has
  // We need to check if it's (C_TIME - var) or (var - C_TIME)

  if (lhs_is_c_time && !rhs_is_c_time) {
    // Constraint form: (C_TIME - var ∼ n)
    // This is meaningful in Future scope
    if (scope == ScopeDirection::Future) {
      // < and <= both contribute to horizon
      if (expr.op == CompareOp::LessThan || expr.op == CompareOp::LessEqual) {
        return ConstraintBound{.frames = frames, .direction = ScopeDirection::Future};
      }
    }
  } else if (!lhs_is_c_time && rhs_is_c_time) {
    // Constraint form: (var - C_TIME ∼ n)
    // This is meaningful in Past scope
    if (scope == ScopeDirection::Past) {
      // < and <= both contribute to history
      if (expr.op == CompareOp::LessThan || expr.op == CompareOp::LessEqual) {
        return ConstraintBound{.frames = frames, .direction = ScopeDirection::Past};
      }
    }
  }
  // Not a simple constraint form we can handle
  return std::nullopt;
}

/**
 * @brief Try to extract a constraint bound from a FrameBoundExpr
 *
 * Similar logic to time bounds, but operates on frame counts directly.
 */
auto extract_frame_bound_constraint(const stql::FrameBoundExpr& expr, ScopeDirection scope)
    -> std::optional<ConstraintBound> {
  // using namespace stql;
  using stql::CompareOp;

  // Similar structure checking as time bounds
  const auto& diff = expr.diff;

  bool is_c_frame_minus_var = false;
  bool is_var_minus_c_frame = false;

  const bool lhs_is_c_frame = diff.lhs.name == "C_FRAME";
  const bool rhs_is_c_frame = diff.rhs.name == "C_FRAME";

  if (lhs_is_c_frame && !rhs_is_c_frame) {
    is_c_frame_minus_var = true;
  } else if (!lhs_is_c_frame && rhs_is_c_frame) {
    is_var_minus_c_frame = true;
  } else {
    return std::nullopt;
  }

  if (is_c_frame_minus_var) {
    // Constraint form: (C_FRAME - var ∼ n)
    // Meaningful in Future scope
    if (scope == ScopeDirection::Future) {
      if (expr.op == CompareOp::LessThan || expr.op == CompareOp::LessEqual) {
        return ConstraintBound{.frames = expr.value, .direction = ScopeDirection::Future};
      }
    }
  } else if (is_var_minus_c_frame) {
    // Constraint form: (var - C_FRAME ∼ n)
    // Meaningful in Past scope
    if (scope == ScopeDirection::Past) {
      if (expr.op == CompareOp::LessThan || expr.op == CompareOp::LessEqual) {
        return ConstraintBound{.frames = expr.value, .direction = ScopeDirection::Past};
      }
    }
  }

  return std::nullopt;
}

/**
 * @brief Combine two bounds under AND (conjunction)
 *
 * AND takes the minimum (tighter) bound.
 */
// auto intersect_bounds(const ConstraintBound& a, const ConstraintBound& b) -> ConstraintBound {
//   if (a.direction != b.direction) {
//     // Conflicting directions - conservatively return unbounded
//     return ConstraintBound{.frames = UNBOUNDED, .direction = a.direction};
//   }
//   return ConstraintBound{.frames = std::min(a.frames, b.frames), .direction = a.direction};
// }

/**
 * @brief Combine two bounds under OR (disjunction)
 *
 * OR takes the maximum (looser) bound. If either side is unbounded, result is unbounded.
 */
// auto union_bounds(const ConstraintBound& a, const ConstraintBound& b) -> ConstraintBound {
//   if (a.direction != b.direction) {
//     // Conflicting directions - result is unbounded
//     return ConstraintBound{.frames = UNBOUNDED, .direction = a.direction};
//   }
//   if (a.frames == UNBOUNDED || b.frames == UNBOUNDED) {
//     return ConstraintBound{.frames = UNBOUNDED, .direction = a.direction};
//   }
//   return ConstraintBound{.frames = std::max(a.frames, b.frames), .direction = a.direction};
// }

/**
 * @brief Recursively compute requirements from an expression
 *
 * Returns {history, horizon} pair where UNBOUNDED means infinite requirement.
 * Assumes fps has already been validated.
 */
auto compute_requirements_impl(const stql::Expr& expr, double fps, ScopeDirection current_scope)
    -> std::pair<int64_t, int64_t>;

} // namespace

// =============================================================================
// Public API Implementation
// =============================================================================

auto compute_requirements(const stql::Expr& formula, double fps) -> MonitoringRequirements {
  auto [history, horizon] = compute_requirements_impl(formula, fps, ScopeDirection::Future);
  return MonitoringRequirements{.history = History{history}, .horizon = Horizon{horizon}};
}

auto is_past_time_formula(const stql::Expr& formula) -> bool {
  auto reqs = compute_requirements(formula);
  return reqs.horizon.frames == 0;
}

// =============================================================================
// Implementation Details
// =============================================================================

namespace {
auto compute_requirements_impl(
    const stql::SpatialExpr& /* expr */,
    double /* fps */,
    ScopeDirection /* current_scope */) -> std::pair<int64_t, int64_t> {
  return {0, 0};
}

auto compute_requirements_impl(const stql::Expr& expr, double fps, ScopeDirection current_scope)
    -> std::pair<int64_t, int64_t> {
  using namespace stql; // NOLINT(google-build-using-namespace)

  return std::visit(
      [&](const auto& e) -> std::pair<int64_t, int64_t> {
        using T = std::decay_t<decltype(e)>;

        // Constants and comparisons require no history or horizon
        if constexpr (utils::is_one_of_v<
                          T,
                          ConstExpr,
                          ObjectIdCompareExpr,
                          ClassCompareExpr,
                          ProbCompareExpr,
                          DistCompareExpr,
                          LatLonCompareExpr,
                          AreaCompareExpr>) {
          return {0, 0};
        }
        // Some operators don't change requirements
        else if constexpr (utils::is_one_of_v<T, NotExpr, SpatialExistsExpr, SpatialForallExpr>) {
          return compute_requirements_impl(*e.arg, fps, current_scope);
        } else if constexpr (utils::is_one_of_v<T, FreezeExpr, ExistsExpr, ForallExpr>) {
          return compute_requirements_impl(*e.body, fps, current_scope);
        } else if constexpr (std::is_same_v<T, AndExpr>) {
          // AND: take max of all children (worst case)
          int64_t max_hist = 0, max_horiz = 0;
          for (const auto& arg : e.args) {
            auto [hist, horiz] = compute_requirements_impl(arg, fps, current_scope);
            max_hist           = std::max(max_hist, hist);
            max_horiz          = std::max(max_horiz, horiz);
          }
          return {max_hist, max_horiz};
        } else if constexpr (std::is_same_v<T, OrExpr>) {
          // OR: also take max of all children (worst case)
          int64_t max_hist = 0, max_horiz = 0;
          for (const auto& arg : e.args) {
            auto [hist, horiz] = compute_requirements_impl(arg, fps, current_scope);
            max_hist           = std::max(max_hist, hist);
            max_horiz          = std::max(max_horiz, horiz);
          }
          return {max_hist, max_horiz};
        }

        // Future-time temporal operators
        else if constexpr (std::is_same_v<T, NextExpr>) {
          // next(φ, n) adds n frames to horizon
          auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Future);
          if (horiz == UNBOUNDED) {
            horiz = UNBOUNDED; // Stays unbounded
          } else {
            horiz += static_cast<int64_t>(e.steps);
          }
          return {hist, horiz};
        } else if constexpr (std::is_same_v<T, AlwaysExpr>) {
          // always(φ) requires infinite horizon (unless constrained by subexpression)
          // Extract requirements from subexpression
          auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Future);
          // If subexpression has bounded horizon from a constraint, use it
          // Otherwise, always requires unbounded horizon
          if (horiz != 0) { return {hist, horiz}; }
          return {hist, UNBOUNDED};
        } else if constexpr (std::is_same_v<T, EventuallyExpr>) {
          // eventually(φ) requires infinite horizon (unless constrained by subexpression)
          // Extract requirements from subexpression
          auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Future);
          // If subexpression has bounded horizon from a constraint, use it
          // Otherwise, eventually requires unbounded horizon
          if (horiz != 0) { return {hist, horiz}; }
          return {hist, UNBOUNDED};
        } else if constexpr (std::is_same_v<T, UntilExpr>) {
          // until(φ1, φ2) requires infinite horizon (unless constrained by subexpressions)
          auto [hist1, horiz1] = compute_requirements_impl(*e.lhs, fps, ScopeDirection::Future);
          auto [hist2, horiz2] = compute_requirements_impl(*e.rhs, fps, ScopeDirection::Future);
          // Take max of both horizons; if either has a constraint, use it
          int64_t combined_horiz = std::max(horiz1, horiz2);
          if (combined_horiz != 0) { return {std::max(hist1, hist2), combined_horiz}; }
          return {std::max(hist1, hist2), UNBOUNDED};
        } else if constexpr (std::is_same_v<T, ReleaseExpr>) {
          // release(φ1, φ2) dual of until, also requires infinite horizon (unless constrained)
          auto [hist1, horiz1] = compute_requirements_impl(*e.lhs, fps, ScopeDirection::Future);
          auto [hist2, horiz2] = compute_requirements_impl(*e.rhs, fps, ScopeDirection::Future);
          // Take max of both horizons; if either has a constraint, use it
          int64_t combined_horiz = std::max(horiz1, horiz2);
          if (combined_horiz != 0) { return {std::max(hist1, hist2), combined_horiz}; }
          return {std::max(hist1, hist2), UNBOUNDED};
        }

        // Past-time temporal operators
        else if constexpr (std::is_same_v<T, PreviousExpr>) {
          // previous(φ, n) adds n frames to history
          auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Past);
          if (hist == UNBOUNDED) {
            hist = UNBOUNDED; // Stays unbounded
          } else {
            hist += static_cast<int64_t>(e.steps);
          }
          return {hist, horiz};
        } else if constexpr (std::is_same_v<T, SinceExpr>) {
          // since(φ1, φ2) requires infinite history (unless constrained by subexpressions)
          // Extract requirements from both subexpressions
          auto [hist1, horiz1] = compute_requirements_impl(*e.lhs, fps, ScopeDirection::Past);
          auto [hist2, horiz2] = compute_requirements_impl(*e.rhs, fps, ScopeDirection::Past);
          // Take max of both histories
          int64_t combined_hist = std::max(hist1, hist2);
          // If either has bounded history, use it; otherwise unbounded
          if (combined_hist != 0) { return {combined_hist, std::max(horiz1, horiz2)}; }
          return {UNBOUNDED, std::max(horiz1, horiz2)};
        } else if constexpr (std::is_same_v<T, BackToExpr>) {
          // backto(φ1, φ2) dual of since, also requires infinite history (unless constrained)
          // Extract requirements from both subexpressions
          auto [hist1, horiz1] = compute_requirements_impl(*e.lhs, fps, ScopeDirection::Past);
          auto [hist2, horiz2] = compute_requirements_impl(*e.rhs, fps, ScopeDirection::Past);
          // Take max of both histories
          int64_t combined_hist = std::max(hist1, hist2);
          // If either has bounded history, use it; otherwise unbounded
          if (combined_hist != 0) { return {combined_hist, std::max(horiz1, horiz2)}; }
          return {UNBOUNDED, std::max(horiz1, horiz2)};
        }

        // Constraints
        else if constexpr (std::is_same_v<T, TimeBoundExpr>) {
          if (auto bound = extract_time_bound_constraint(e, fps, current_scope)) {
            if (bound->direction == ScopeDirection::Future) {
              return {0, bound->frames};
            } else {
              return {bound->frames, 0};
            }
          }
          return {0, 0}; // Trivially satisfied constraint contributes nothing
        } else if constexpr (std::is_same_v<T, FrameBoundExpr>) {
          if (auto bound = extract_frame_bound_constraint(e, current_scope)) {
            if (bound->direction == ScopeDirection::Future) {
              return {0, bound->frames};
            } else {
              return {bound->frames, 0};
            }
          }
          return {0, 0};
        }

        // Fallback (should not reach here with complete variant coverage)
        else {
          return {0, 0};
        }
      },
      expr);
}

} // namespace

} // namespace percemon::monitoring
