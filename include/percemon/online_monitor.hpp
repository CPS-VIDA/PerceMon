#ifndef PERCEMON_ONLINE_MONITOR_HPP
#define PERCEMON_ONLINE_MONITOR_HPP

#include "percemon/datastream.hpp"
#include "percemon/evaluation.hpp"
#include "percemon/monitoring.hpp"
#include "percemon/stql.hpp"

#include <deque>
#include <memory>

namespace percemon::monitoring {

/**
 * @brief Online monitor for STQL formulas on streaming perception data.
 *
 * This class manages the evaluation of an STQL formula on a stream of perception data frames.
 * It maintains the necessary buffers (history and horizon) and applies the BooleanEvaluator
 * to determine formula satisfaction at each frame.
 *
 * **Usage Pattern**:
 * @code
 * // Create monitor for a formula
 * auto formula = eventually(exists(
 *     (C(car) == 1) & (prob(car) >= 0.8)
 * ));
 * OnlineMonitor monitor(formula, 10.0);  // 10 FPS
 *
 * // Feed frames and get verdicts
 * for (const auto& frame : data_stream) {
 *     bool satisfied = monitor.evaluate(frame);
 *     if (satisfied) {
 *         std::cout << "Formula satisfied at frame " << frame.frame_num << "\n";
 *     }
 * }
 * @endcode
 *
 * **Formula Requirements**:
 * - ~~Formula must have bounded horizon (checked via is_monitorable())~~
 * - UPDATE: Mixing past and future time operators is difficult. We enforce that
 *   formulas are strictly past-time using is_monitorable().
 * - Unbounded operators (Always/Eventually without constraints) cannot be monitored
 * - Use bounded temporal constraints to limit buffering: {x}.(◇((C_TIME - x) ≤ 5s))
 *
 * **Memory Usage**:
 * - History buffer: stores up to history.frames frames (FIFO queue)
 * - Horizon buffer: stores up to horizon.frames frames (FIFO queue)
 * - Total memory bounded by compute_requirements() result
 *
 * **Semantics**:
 * - Returns true if formula is SATISFIED on the current frame
 * - Returns false if formula is VIOLATED on the current frame
 * - Boolean semantics: no robustness values, pure satisfaction checking
 *
 * **Thread Safety**:
 * - NOT thread-safe; serialize calls to evaluate()
 * - Each instance has independent buffers; multiple monitors can run in parallel
 *
 * **See Also**:
 * - percemon::monitoring::compute_requirements() for formula analysis
 * - percemon::monitoring::BooleanEvaluator for evaluation semantics
 * - percemon::stql for formula construction
 */
class OnlineMonitor {
 public:
  /**
   * @brief Create an online monitor for an STQL formula.
   *
   * Analyzes the formula to determine history and horizon requirements,
   * then initializes buffers. Throws an exception if the formula has
   * unbounded horizon (not online-monitorable).
   *
   * @param formula The STQL formula to monitor
   * @param fps Frames per second (for time-to-frame conversion)
   *            Default: 1.0 (1 frame per second)
   *
   * @throws std::invalid_argument If formula has unbounded horizon
   *
   * **Example**:
   * @code
   * // Monitor "eventually car detected with high confidence"
   * auto formula = eventually(exists(prob("car") >= 0.9));
   * OnlineMonitor monitor(formula, 30.0);  // 30 FPS video
   * @endcode
   */
  explicit OnlineMonitor(const stql::Expr& formula, double fps = 1.0);

  /**
   * @brief Evaluate formula on a new frame and advance buffers.
   *
   * Adds the new frame to the buffer system, maintaining history and horizon.
   * Evaluates the formula using the current buffer state and returns the verdict.
   *
   * The operation:
   * 1. Shifts horizon frames into history as needed
   * 2. Adds the new frame to the horizon buffer
   * 3. Trims history if it exceeds the requirement
   * 4. Evaluates the formula
   * 5. Returns the satisfaction result
   *
   * @param frame The new frame to add and evaluate
   * @return true if formula is satisfied on this frame, false if violated
   *
   * @throws std::invalid_argument If frame is invalid
   * @throws std::logic_error If formula structure is corrupted
   *
   * **Example**:
   * @code
   * for (const auto& frame : data_stream) {
   *     bool verdict = monitor.evaluate(frame);
   *     if (verdict) {
   *         on_satisfaction(frame);
   *     } else {
   *         on_violation(frame);
   *     }
   * }
   * @endcode
   */
  [[nodiscard]] auto evaluate(const datastream::Frame& frame) -> bool;

  /**
   * @brief Check if formula is online-monitorable.
   *
   * A formula is online-monitorable if it is purely past-time.
   */
  [[nodiscard]] auto is_monitorable() const -> bool;

  /**
   * @brief Get the memory requirements for this formula.
   *
   * Returns the analysis result showing history frames to retain
   * and horizon frames to buffer.
   *
   * @return MonitoringRequirements with history and horizon bounds
   *
   * **Example**:
   * @code
   * auto reqs = monitor.requirements();
   * std::cout << "Need " << reqs.history.frames << " past frames\n";
   * std::cout << "Need " << reqs.horizon.frames << " future frames\n";
   * @endcode
   */
  [[nodiscard]] auto requirements() const -> const MonitoringRequirements&;

 private:
  // Formula and its requirements
  stql::Expr formula_;
  MonitoringRequirements requirements_;

  // Evaluator instance
  BooleanEvaluator evaluator_;

  // Buffers for temporal operators
  std::deque<datastream::Frame> history_;
  std::deque<datastream::Frame> horizon_;

  // Field to keep track of the current frame
  datastream::Frame current_frame_ = {};
};

} // namespace percemon::monitoring

#endif // PERCEMON_ONLINE_MONITOR_HPP
