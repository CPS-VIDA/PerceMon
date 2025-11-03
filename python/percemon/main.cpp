#include <nanobind/nanobind.h>

namespace nb = nanobind;

// Forward declarations of binding initialization functions
void init_stql_bindings(nb::module_& m);
void init_monitoring_bindings(nb::module_& m);
void init_datastream_bindings(nb::module_& m);
void init_evaluation_bindings(nb::module_& m);
void init_online_monitor_bindings(nb::module_& m);
void init_spatial_bindings(nb::module_& m);

/**
 * @brief Main entry point for the PerceMon Python module.
 *
 * This module combines all PerceMon Python bindings:
 * - STQL formula construction (stql_bindings.cpp)
 * - Monitoring requirements analysis (monitoring_bindings.cpp)
 * - Runtime data structures (datastream_bindings.cpp)
 * - Formula evaluation (evaluation_bindings.cpp)
 * - Online monitoring (online_monitor_bindings.cpp)
 * - Spatial operations (spatial_bindings.cpp)
 *
 * Example usage:
 * @code
 * import percemon
 *
 * // Create formula
 * car = percemon.ObjectVar("car")
 * formula = percemon.exists([car], percemon.is_class(car, 1))
 *
 * // Analyze requirements
 * reqs = percemon.compute_requirements(formula, fps=30.0)
 *
 * // Create monitor
 * monitor = percemon.OnlineMonitor(formula, fps=30.0)
 *
 * // Evaluate on frames
 * for frame in frames:
 *     result = monitor.evaluate(frame)
 * @endcode
 */
NB_MODULE(percemon, m) { // NOLINT(*)
  m.doc() = "PerceMon - Spatio-Temporal Quality Logic (STQL) monitoring library";

  // Initialize all sub-module bindings
  init_stql_bindings(m);
  init_monitoring_bindings(m);
  init_datastream_bindings(m);
  init_evaluation_bindings(m);
  init_online_monitor_bindings(m);
  // init_spatial_bindings(m);
}
