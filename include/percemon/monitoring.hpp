#ifndef __PERCEMON_MONITORING_HPP__
#define __PERCEMON_MONITORING_HPP__

#include "percemon/ast/ast.hpp"
#include "percemon/ast/functions.hpp"
#include "percemon/ast/primitives.hpp"
#include "percemon/ast/s4u.hpp"
#include "percemon/ast/tqtl.hpp"
#include "percemon/datastream.hpp"

// TODO: Consider unordered_map if memory and hashing isn't an issue.
#include <map>

// TODO: Consider performance/space efficiency of deque vs vector.
#include <deque>

#include <optional>

namespace percemon {
namespace monitoring {

/**
 * Returns if the formula is purely past-time. Only place where this can be true is in
 * the TimeBound and FrameBound constraints.
 *
 * @todo This can be enforced in the TimeBound and FrameBound constructors.
 */
// bool is_past_time(const percemon::ast::Expr& expr);

/**
 * Get the horizon for a formula in number of frames.  This function will throw an
 * exception if there are TimeBound constraints in the formula.
 */
std::optional<size_t> get_horizon(const percemon::ast::Expr& expr);

/**
 * Get the horizon for a formula in number of frames. This uses the frames per second of
 * the stream to convert TimeBound constraints to FrameBound constraints.
 */
std::optional<size_t> get_horizon(const percemon::ast::Expr& expr, double fps);

/**
 * Object representing the online monitor.
 */
struct OnlineMonitor {
 public:
  /**
   * The formula being monitored
   */
  const ast::Expr phi;
  /**
   * Frames per second for the datastream
   */
  const double fps;

  OnlineMonitor() = delete;
  OnlineMonitor(const ast::Expr& phi_, const double fps_);

  /**
   * Add a new frame to the monitor buffer
   */
  void add_frame(const datastream::Frame& frame);
  void add_frame(datastream::Frame&& frame); // Efficient move semantics

  /**
   * Compute the robustness of the currently buffered frames.
   */
  double eval() const;

  size_t getHorizon() const {
    return max_horizon;
  }

 private:
  // Primary data

  /**
   * A buffer containing the history of Frames required to compute robustness of phi
   * efficiently.
   */
  std::deque<datastream::Frame> buffer;

  /**
   * Maximum width of buffer.
   */
  size_t max_horizon;
};

} // namespace monitoring
// namespace monitoring
} // namespace percemon

#endif /* end of include guard: __PERCEMON_MONITORING_HPP__ */
