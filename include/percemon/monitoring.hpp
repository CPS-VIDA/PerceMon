#pragma once

#ifndef __PERCEMON_MONITORING_HPP__
#define __PERCEMON_MONITORING_HPP__

#include "percemon/ast.hpp"
#include "percemon/datastream.hpp"

// TODO: Consider unordered_map if memory and hashing isn't an issue.
#include <map>

// TODO: Consider performance/space efficiency of deque vs vector.
#include <deque>

#include <cstdint>
#include <optional>

namespace percemon::monitoring {

/**
 * Object representing the online monitor.
 */
struct OnlineMonitor {
 public:
  OnlineMonitor() = delete;
  OnlineMonitor(
      ast::Expr phi_,
      const std::int64_t fps_,
      double x_boundary,
      double y_boundary);

  /**
   * Add a new frame to the monitor buffer
   */
  void add_frame(const datastream::Frame& frame);
  void add_frame(datastream::Frame&& frame); // Efficient move semantics

  /**
   * Compute the robustness of the currently buffered frames.
   */
  double eval();

  [[nodiscard]] size_t buffer_capacity() const { return max_buffer_size; }
  [[nodiscard]] std::int64_t get_fps() const { return fps; };
  const ast::Expr& get_phi() { return phi; }

 private:
  /**
   * The formula being monitored
   */
  const ast::Expr phi;
  /**
   * Frames per second for the datastream
   */
  const std::int64_t fps;

  /**
   * A buffer containing the history of Frames required to compute robustness of phi
   * efficiently.
   */
  std::deque<datastream::Frame> buffer;

  /**
   * Maximum width of buffer.
   */
  size_t max_buffer_size;
  /**
   * Track the center (time 0) of the buffer
   */
  size_t _zero_idx;

  /**
   * Boundary for UNIVERSE
   */
  double universe_x, universe_y;
};

/**
 * Offline monitor
 */
struct OfflineMonitor {
 public:
  OfflineMonitor() = delete;
  OfflineMonitor(
      ast::Expr phi_,
      const std::int64_t fps_,
      double x_boundary,
      double y_boundary);

  /**
   * Add a new frame to the monitor buffer
   */
  std::vector<bool> eval_trace(const std::vector<datastream::Frame>& frame);

  [[nodiscard]] std::int64_t get_fps() const { return fps; };
  const ast::Expr& get_phi() { return phi; }

 private:
  /**
   * The formula being monitored
   */
  const ast::Expr phi;
  /**
   * Frames per second for the datastream
   */
  const std::int64_t fps;

  /**
   * Boundary for UNIVERSE
   */
  double universe_x, universe_y;
};

} // namespace percemon::monitoring

#endif /* end of include guard: __PERCEMON_MONITORING_HPP__ */
