#include "percemon/online_monitor.hpp"

#include <stdexcept>
#include <utility>

namespace percemon::monitoring {

// ============================================================================
// OnlineMonitor Implementation
// ============================================================================

OnlineMonitor::OnlineMonitor(const stql::Expr& formula, double fps) :
    formula_(formula), requirements_(compute_requirements(formula, fps)), evaluator_() {
  // Check if formula is online-monitorable
  if (!is_monitorable()) {
    throw std::invalid_argument(
        "Given STQL expression doesn't have a bounded horizon. "
        "Cannot perform online monitoring for this formula.");
  }
}

auto OnlineMonitor::evaluate(const datastream::Frame& frame) -> bool {
  // Update buffers with the new frame
  //
  // Move the current frame into the history
  datastream::Frame last_frame = std::exchange(current_frame_, frame);
  history_.push_back(last_frame);
  // Trim history to requirement size
  // Note: We compare with size_t, so cast is safe
  while (static_cast<int64_t>(history_.size()) > requirements_.history.frames) {
    history_.pop_front();
  }
  // In an ideal world, where we process formulas with bounded horizon, we will also
  // have to trim the horizon... Do you see the complexity now?

  // Determine the current frame for evaluation
  // If horizon is empty, evaluate on current frame parameter
  // Otherwise, evaluate on first frame in horizon (most recent)
  // Realistically, this shouldnt happen because we add the frame to the horizon.
  const datastream::Frame* eval_frame = &current_frame_;

  // Evaluate the formula using the standard offline boolean evaluator.
  return evaluator_.evaluate(formula_, *eval_frame, history_, horizon_);
}

auto OnlineMonitor::is_monitorable() const -> bool { return requirements_.horizon.frames == 0; }

auto OnlineMonitor::requirements() const -> const MonitoringRequirements& { return requirements_; }

} // namespace percemon::monitoring
