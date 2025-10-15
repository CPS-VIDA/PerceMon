#pragma once

#ifndef PERCEMON_MONITORING_HPP
#define PERCEMON_MONITORING_HPP

#include "percemon/stql.hpp"
#include <cstdint>
#include <limits>
#include <string>

/**
 * @file monitoring.hpp
 * @brief Monitoring interface for STQL formulas
 *
 * This file provides the interface for analyzing STQL formulas to compute
 * required history (past frames) and horizon (future frames) needed for monitoring.
 */

namespace percemon::monitoring {

// Sentinel value for unbounded depth
constexpr int64_t UNBOUNDED = std::numeric_limits<int64_t>::max();

/**
 * @brief Required history depth for monitoring.
 *
 * Represents the number of past frames that must be retained in memory
 * to evaluate the given STQL formula.
 *
 * For past-time operators like Previous, Since, and Holds, this indicates
 * how far back in the trace we need to look.
 *
 * A value of UNBOUNDED indicates unbounded history requirement.
 *
 * Example:
 * @code
 * History hist{5};  // Need 5 past frames
 * if (hist.frames == UNBOUNDED) {
 *     // Handle unbounded history
 * }
 * @endcode
 */
struct History {
  int64_t frames; ///< Number of frames to look back (0 = no history needed)

  [[nodiscard]] auto to_string() const -> std::string {
    if (frames == UNBOUNDED) { return "History{unbounded}"; }
    return "History{" + std::to_string(frames) + " frames}";
  }

  /// Check if history is bounded
  [[nodiscard]] auto is_bounded() const -> bool { return frames != UNBOUNDED; }
};

/**
 * @brief Required horizon depth for online monitoring.
 *
 * Represents the number of future frames that must be buffered before
 * a verdict can be computed for the current frame.
 *
 * For future-time operators like Next, Until, and Always, this indicates
 * how far ahead in the trace we need to look. A horizon of 0 means
 * the formula can be evaluated purely from current and past data.
 *
 * A value of UNBOUNDED indicates unbounded horizon requirement (not online-monitorable).
 *
 * Example:
 * @code
 * Horizon horiz{10};  // Need 10 future frames
 * if (horiz.frames == UNBOUNDED) {
 *     // Not online-monitorable
 * }
 * @endcode
 */
struct Horizon {
  int64_t frames; ///< Number of frames to look ahead (0 = no horizon needed)

  [[nodiscard]] auto to_string() const -> std::string {
    if (frames == UNBOUNDED) { return "Horizon{unbounded}"; }
    return "Horizon{" + std::to_string(frames) + " frames}";
  }

  /// Check if horizon is bounded
  [[nodiscard]] auto is_bounded() const -> bool { return frames != UNBOUNDED; }
};

/**
 * @brief Result of analyzing monitoring requirements for a formula.
 *
 * Contains both the required history (past frames) and horizon (future frames)
 * needed to monitor a given STQL formula.
 *
 * Example:
 * @code
 * MonitoringRequirements reqs = compute_requirements(formula, 30.0);
 * std::cout << "History: " << reqs.history.frames << " frames\n";
 * std::cout << "Horizon: " << reqs.horizon.frames << " frames\n";
 * @endcode
 */
struct MonitoringRequirements {
  History history;
  Horizon horizon;

  [[nodiscard]] auto to_string() const -> std::string {
    return "MonitoringRequirements{\n  " + history.to_string() + ",\n  " + horizon.to_string() +
           "\n}";
  }
};

/**
 * @brief Compute monitoring requirements for a given STQL formula.
 *
 * Analyzes the formula structure to determine:
 * - How many past frames are needed (for past temporal operators)
 * - How many future frames are needed (for future temporal operators)
 *
 * Time-based constraints in the formula (e.g., {x}.((x - C_TIME) <= 5.0))
 * are converted to frame counts using the provided FPS.
 *
 * @param formula The STQL formula to analyze
 * @param fps Frames per second - used to convert time-based constraints to frame counts.
 *            For example, with fps=10, a 5-second constraint becomes 50 frames.
 *            Default is 1.0 (1 frame per second).
 * @return MonitoringRequirements indicating required history and horizon in frames
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto formula = next(phi, 5);  // Next 5 frames
 *
 * auto reqs = compute_requirements(formula);
 * assert(reqs.horizon.frames == 5);  // Need 5 future frames
 * assert(reqs.history.frames == 0);  // No history needed
 * @endcode
 *
 * Example with time-based constraints:
 * @code
 * auto x = TimeVar{"x"};
 * auto phi = make_true();
 * // Formula: {x}.(eventually((x - C_TIME) <= 5.0 && phi))
 * // With fps=10, 5 seconds = 50 frames
 * auto time_bound = TimeBoundExpr{TimeDiff{x, C_TIME}, CompareOp::LessEqual, 5.0};
 * auto body = eventually(time_bound && phi);
 * auto formula = freeze(x, body);
 *
 * auto reqs = compute_requirements(formula, 10.0);  // 10 FPS
 * assert(reqs.horizon.frames == 50);  // 5 seconds * 10 fps = 50 frames
 * @endcode
 *
 * Implementation Strategy:
 * 1. Traverse the AST using std::visit
 * 2. Track maximum history and horizon requirements
 * 3. Convert time bounds to frames using: frames = ceil(time_seconds * fps)
 * 4. Handle operators:
 *    - Next(phi, n): horizon += n
 *    - Previous(phi, n): history += n
 *    - Until(phi, psi): horizon = UNBOUNDED (unless bounded interval)
 *    - Since(phi, psi): history = UNBOUNDED (unless bounded interval)
 *    - Always: horizon = UNBOUNDED
 *    - Eventually: horizon = UNBOUNDED
 *    - Holds: history = UNBOUNDED
 *    - Sometimes: history = UNBOUNDED
 *    - Freeze with time bounds: convert time to frames using FPS
 *    - And/Or: max of children
 *    - Not: same as child
 *    - Exists/Forall: same as body
 *    - Spatial operators: depends on embedded temporal operators
 *
 * @note This function computes a syntactic bound - the actual runtime requirement
 *       may be less depending on the formula evaluation.
 */
auto compute_requirements(const stql::Expr& formula, double fps = 1.0) -> MonitoringRequirements;

/**
 * @brief Check if formula requires only past data (no future operators).
 *
 * A past-time formula uses only past temporal operators (Previous, Since, Holds)
 * and can be evaluated with zero horizon (no buffering needed).
 *
 * @param formula The STQL formula to check
 * @return true if the formula requires no future frames, false otherwise
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto past = previous(phi);    // Past operator
 * auto future = next(phi);      // Future operator
 *
 * assert(is_past_time_formula(past));
 * assert(!is_past_time_formula(future));
 * @endcode
 *
 * Implementation:
 * @code
 * auto reqs = compute_requirements(formula);
 * return reqs.horizon.frames == 0;
 * @endcode
 */
auto is_past_time_formula(const stql::Expr& formula) -> bool;

} // namespace percemon::monitoring

#endif // PERCEMON_MONITORING_HPP
