#ifndef PERCEMON_EVALUATION_HPP
#define PERCEMON_EVALUATION_HPP

#include "percemon/datastream.hpp"
#include "percemon/stql.hpp"

#include <deque>
#include <iterator>
#include <map>
#include <string>

namespace percemon::monitoring {

/**
 * @brief Evaluation context for STQL formula evaluation.
 *
 * Maintains the state needed to evaluate an STQL formula on a specific frame,
 * including buffers for temporal operators and bindings for quantified variables.
 *
 * The context is designed to be passed through the evaluation visitor, allowing
 * recursive evaluation of nested expressions while maintaining consistent state.
 *
 * @note The template parameters are used to store the begin and end iterators for the
 *  history and horizon in the current context, along with an iterator (as a pointer) to
 *  the current frame. Since these change at every temporal scope, the implementation of
 *  this is hidden from the user.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
  requires std::same_as<std::iter_value_t<Iter>, datastream::Frame>
struct EvaluationContext {
  /**
   * @brief Current frame being evaluated.
   *
   * This is the frame against which constraints and perception primitives
   * are evaluated. For temporal operators, this is typically the reference point,
   * with history/horizon providing adjacent frames.
   */
  Iter current_frame;

  /**
   * @brief Start of the history buffer: past frames in chronological order.
   *
   * `history_begin` points to the oldest frame, and `prev(history_end)`  is the most recent past
   * frame, i.e., the current frame for past-time operators.
   *
   * Used by past-time temporal operators (Previous, Holds, Sometimes, Since, BackTo).
   * Size is bounded by History requirement from compute_requirements().
   */
  Iter history_begin;
  Sentinel history_end;

  /**
   * @brief End of the horizon buffer: future frames in chronological order.
   *
   * `horizon_begin` points to the next future frame and `prev(horizon_end)` is the furthest
   * future frame.
   *
   * Used by future-time temporal operators (Next, Always, Eventually, Until).
   * Size is bounded by Horizon requirement from compute_requirements().
   */
  Iter horizon_begin;
  Sentinel horizon_end;

  /**
   * @brief Frozen time variable bindings.
   *
   * Maps variable names (e.g., "x") to captured timestamps.
   * Populated when entering a FreezeExpr with time variable.
   * Used by TimeBoundExpr to compare elapsed time.
   *
   * Example:
   * - Formula: {x}.(◇((C_TIME - x) ≤ 5.0))
   * - frozen_times["x"] = current_frame->timestamp (captured at freeze)
   */
  std::map<std::string, double> frozen_times;

  /**
   * @brief Frozen frame variable bindings.
   *
   * Maps variable names (e.g., "f") to captured frame numbers.
   * Populated when entering a FreezeExpr with frame variable.
   * Used by FrameBoundExpr to compare frame distance.
   */
  std::map<std::string, int64_t> frozen_frames;

  /**
   * @brief Object variable bindings from quantifiers.
   *
   * Maps quantified variable names (e.g., "car") to actual object IDs
   * from the current frame. Populated by ExistsExpr and ForallExpr
   * during quantifier iteration.
   *
   * Example:
   * - Formula: ∃{car}@(C(car) = vehicle)
   * - bound_objects["car"] = "obj_001" (object ID from current frame)
   */
  std::map<std::string, std::string> bound_objects;

  [[nodiscard]] constexpr auto has_history() const -> bool { return history_begin != history_end; }
  [[nodiscard]] constexpr auto has_horizon() const -> bool { return horizon_begin != horizon_end; }

  [[nodiscard]] constexpr auto num_horizon() const -> std::iter_difference_t<Iter> {
    return std::distance(horizon_begin, horizon_end);
  }
  [[nodiscard]] constexpr auto num_history() const -> std::iter_difference_t<Iter> {
    return std::distance(history_begin, history_end);
  }
};

/**
 * @brief Boolean evaluator for STQL formulas using boolean semantics.
 *
 * Evaluates STQL formulas on perception data frames with pure boolean semantics
 * (true = satisfied, false = violated). Unlike robustness-based approaches,
 * this evaluator returns a single boolean value indicating formula satisfaction
 * on the current frame.
 *
 * **Usage**:
 * @code
 * // Create evaluator
 * BooleanEvaluator evaluator;
 *
 * // Evaluate formula on a frame
 * auto requirements = compute_requirements(formula, fps);
 * std::deque<Frame> history, horizon;
 * // ... populate history and horizon buffers ...
 *
 * bool satisfied = evaluator.evaluate(formula, current_frame, history, horizon);
 * if (satisfied) {
 *     std::cout << "Formula satisfied at this frame\n";
 * } else {
 *     std::cout << "Formula violated at this frame\n";
 * }
 * @endcode
 *
 * **Operator Semantics**:
 *
 * - **Logical**: AND/OR/NOT use standard boolean logic
 * - **Temporal**: Always/Eventually/Until evaluate over buffered frames
 * - **Quantifiers**: Exists/Forall iterate over objects in current frame
 * - **Constraints**: Time/frame/distance compared using comparison operators
 * - **Spatial**: Regions composed using union/intersection/complement
 * - **Primitives**: Class/probability/position extracted from detected objects
 *
 * **Error Handling**:
 *
 * - Missing objects in frame → false (constraint violated)
 * - Insufficient buffered frames (next/previous unavailable) → false
 * - Unbound frozen variables in constraints → false
 * - Invalid bindings (variable bound multiple times) → throws exception
 *
 * **See Also**: percemon::monitoring::compute_requirements() for buffer sizing
 */
struct BooleanEvaluator {
  /**
   * @brief Evaluate an STQL formula on a frame with buffers.
   *
   * Performs recursive evaluation of the formula AST using std::visit
   * to dispatch on expression variant types. The formula is evaluated
   * in the context of the current frame, with history and horizon
   * buffers providing data for temporal operators.
   *
   * @param formula The STQL formula (AST root node)
   * @param current_frame The frame on which to evaluate
   * @param history Past frames (oldest first), up to history requirement size
   * @param horizon Future frames (next first), up to horizon requirement size
   *
   * @return true if formula is satisfied on current_frame, false if violated
   *
   * @throws std::invalid_argument If variable bindings are invalid
   * @throws std::logic_error If formula structure is malformed
   *
   * **Example**:
   * @code
   * // Evaluate "eventually car detected with high confidence"
   * auto formula = eventually(exists(
   *     prob_compare(prob("car"), CompareOp::GE, 0.9)
   * ));
   *
   * bool satisfied = evaluator.evaluate(formula, frame, history, horizon);
   * @endcode
   */
  [[nodiscard]] auto evaluate(
      const stql::Expr& formula,
      const datastream::Frame& current_frame,
      const std::deque<datastream::Frame>& history,
      const std::deque<datastream::Frame>& horizon) const -> bool;
};

} // namespace percemon::monitoring

#endif // PERCEMON_EVALUATION_HPP
