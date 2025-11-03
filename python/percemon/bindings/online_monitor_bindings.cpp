#include "percemon/online_monitor.hpp"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/bind_map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace percemon::monitoring; // NOLINT(google-build-using-namespace)

// NOLINTBEGIN(misc-redundant-expression)
// NOLINTNEXTLINE(misc-use-internal-linkage)
void init_online_monitor_bindings(nb::module_& m) {
  // =====================================================================
  // OnlineMonitor Class
  // =====================================================================

  nb::class_<OnlineMonitor>(m, "OnlineMonitor")
      .def(
          nb::init<const percemon::stql::Expr&, double>(),
          nb::arg("formula"),
          nb::arg("fps") = 1.0,
          "Create an online monitor for an STQL formula")
      .def(
          "evaluate",
          &OnlineMonitor::evaluate,
          nb::arg("frame"),
          "Evaluate formula on a new frame and return satisfaction verdict")
      .def(
          "is_monitorable",
          &OnlineMonitor::is_monitorable,
          "Check if formula is online-monitorable (purely past-time)")
      .def(
          "requirements",
          &OnlineMonitor::requirements,
          nb::rv_policy::reference,
          "Get memory requirements (history and horizon) for this formula");
}
// NOLINTEND(misc-redundant-expression)
