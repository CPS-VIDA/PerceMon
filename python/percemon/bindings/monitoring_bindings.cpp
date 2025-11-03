#include "percemon/monitoring.hpp"
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
void init_monitoring_bindings(nb::module_& m) {
  // =====================================================================
  // Constants
  // =====================================================================

  m.attr("UNBOUNDED") = UNBOUNDED;

  // =====================================================================
  // History Struct
  // =====================================================================

  nb::class_<History>(m, "History")
      .def(nb::init<int64_t>())
      .def_ro("frames", &History::frames)
      .def("to_string", &History::to_string)
      .def("__str__", &History::to_string)
      .def("__repr__", &History::to_string)
      .def("is_bounded", &History::is_bounded, "Check if history has finite frame requirement");

  // =====================================================================
  // Horizon Struct
  // =====================================================================

  nb::class_<Horizon>(m, "Horizon")
      .def(nb::init<int64_t>())
      .def_ro("frames", &Horizon::frames)
      .def("to_string", &Horizon::to_string)
      .def("__str__", &Horizon::to_string)
      .def("__repr__", &Horizon::to_string)
      .def("is_bounded", &Horizon::is_bounded, "Check if horizon has finite frame requirement");

  // =====================================================================
  // MonitoringRequirements Struct
  // =====================================================================

  nb::class_<MonitoringRequirements>(m, "MonitoringRequirements")
      .def(nb::init<>())
      .def_ro("history", &MonitoringRequirements::history)
      .def_ro("horizon", &MonitoringRequirements::horizon)
      .def("to_string", &MonitoringRequirements::to_string)
      .def("__str__", &MonitoringRequirements::to_string)
      .def("__repr__", &MonitoringRequirements::to_string);

  // =====================================================================
  // Functions
  // =====================================================================

  m.def(
      "compute_requirements",
      nb::overload_cast<const percemon::stql::Expr&, double>(&compute_requirements),
      nb::arg("formula"),
      nb::arg("fps") = 1.0,
      "Analyze STQL formula to compute history and horizon requirements");

  m.def(
      "is_past_time_formula",
      &is_past_time_formula,
      "Check if formula only uses past-time operators (no future operators)");
}
// NOLINTEND(misc-redundant-expression)
