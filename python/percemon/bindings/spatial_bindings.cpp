#include "percemon/spatial.hpp"

#include <nanobind/make_iterator.h>
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
using namespace percemon::spatial; // NOLINT

// NOLINTNEXTLINE(misc-use-internal-linkage)
void init_spatial_bindings(nb::module_& m) {
  // =====================================================================
  // Spatial Region Types
  // =====================================================================

  // NOLINTBEGIN(misc-redundant-expression)
  nb::class_<Empty>(m, "Empty")
      .def(nb::init<>())
      .def("to_string", &Empty::to_string)
      .def("__str__", &Empty::to_string)
      .def("__repr__", &Empty::to_string)
      .def(nb::self == nb::self);

  nb::class_<Universe>(m, "Universe")
      .def(nb::init<>())
      .def("to_string", &Universe::to_string)
      .def("__str__", &Universe::to_string)
      .def("__repr__", &Universe::to_string)
      .def(nb::self == nb::self);

  nb::class_<BBox>(m, "BBox")
      .def(
          nb::init<double, double, double, double, bool, bool, bool, bool>(),
          nb::arg("xmin"),
          nb::arg("xmax"),
          nb::arg("ymin"),
          nb::arg("ymax"),
          nb::arg("lopen") = false,
          nb::arg("ropen") = false,
          nb::arg("topen") = false,
          nb::arg("bopen") = false)
      .def(nb::init<const percemon::datastream::BoundingBox&>())
      .def_ro("xmin", &BBox::xmin)
      .def_ro("xmax", &BBox::xmax)
      .def_ro("ymin", &BBox::ymin)
      .def_ro("ymax", &BBox::ymax)
      .def_ro("lopen", &BBox::lopen)
      .def_ro("ropen", &BBox::ropen)
      .def_ro("topen", &BBox::topen)
      .def_ro("bopen", &BBox::bopen)
      .def("area", &BBox::area, "Compute area")
      .def("is_closed", &BBox::is_closed, "Check if all boundaries are closed")
      .def("is_open", &BBox::is_open, "Check if any boundary is open")
      .def("to_string", &BBox::to_string)
      .def("__str__", &BBox::to_string)
      .def("__repr__", &BBox::to_string)
      .def(nb::self == nb::self)
      .def(nb::self < nb::self);

  nb::class_<Union>(m, "Union")
      .def(nb::init<>())
      .def(nb::init<std::initializer_list<BBox>>())
      .def("size", &Union::size, "Number of boxes in union")
      .def("empty", &Union::empty, "Check if union is empty")
      .def("to_string", &Union::to_string)
      .def("__str__", &Union::to_string)
      .def("__repr__", &Union::to_string);

  // =====================================================================
  // Topological Operations
  // =====================================================================

  m.def("is_closed", &is_closed, "Check if region is closed (all boundaries included)");

  m.def("is_open", &is_open, "Check if region is open (has at least one open boundary)");

  m.def("area", &area, "Compute area of spatial region");

  m.def(
      "interior", &interior, "Compute topological interior (largest open set contained in region)");

  m.def("closure", &closure, "Compute topological closure (smallest closed set containing region)");

  // =====================================================================
  // Set Operations
  // =====================================================================

  m.def(
      "complement",
      &complement,
      nb::arg("region"),
      nb::arg("universe"),
      "Compute spatial complement: region̅ = universe \\ region");

  m.def(
      "intersect",
      nb::overload_cast<const Region&, const Region&>(&intersect),
      "Spatial intersection: Ω₁ ⊓ Ω₂");

  m.def(
      "intersect",
      nb::overload_cast<const std::vector<Region>&>(&intersect),
      "Variadic spatial intersection: Ω₁ ⊓ Ω₂ ⊓ ... ⊓ Ωₙ");

  m.def(
      "union_of",
      nb::overload_cast<const Region&, const Region&>(&union_of),
      "Spatial union: Ω₁ ⊔ Ω₂");

  m.def(
      "union_of",
      nb::overload_cast<const std::vector<Region>&>(&union_of),
      "Variadic spatial union: Ω₁ ⊔ Ω₂ ⊔ ... ⊔ Ωₙ");

  // =====================================================================
  // Simplification
  // =====================================================================

  m.def("simplify", &simplify, "Simplify region to disjoint bounding boxes");

  // =====================================================================
  // Datastream Conversion Helpers
  // =====================================================================

  m.def("from_datastream", &from_datastream, "Convert datastream BoundingBox to spatial BBox");

  m.def("bbox_of_object", &bbox_of_object, "Create spatial region from an object's bounding box");

  m.def("frame_universe", &frame_universe, "Get universe region for a frame");
}
// NOLINTEND(misc-redundant-expression)
