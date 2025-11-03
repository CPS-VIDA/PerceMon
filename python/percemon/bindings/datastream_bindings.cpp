#include "percemon/datastream.hpp"

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
using namespace percemon::datastream; // NOLINT(google-build-using-namespace)

using ObjectMap = std::map<std::string, Object>;

// NOLINTBEGIN(misc-redundant-expression)
// NOLINTNEXTLINE(misc-use-internal-linkage)
void init_datastream_bindings(nb::module_& m) {
  // Binding types
  nb::bind_map<ObjectMap>(m, "ObjectMap");

  // =====================================================================
  // RefPointType Enum
  // =====================================================================

  nb::enum_<RefPointType>(m, "RefPointType")
      .value("LeftMargin", RefPointType::LeftMargin)
      .value("RightMargin", RefPointType::RightMargin)
      .value("TopMargin", RefPointType::TopMargin)
      .value("BottomMargin", RefPointType::BottomMargin)
      .value("Center", RefPointType::Center);

  // =====================================================================
  // BoundingBox Struct
  // =====================================================================

  nb::class_<BoundingBox>(m, "BoundingBox")
      .def(nb::init<>())
      .def(nb::init<double, double, double, double>())
      .def_rw("xmin", &BoundingBox::xmin)
      .def_rw("xmax", &BoundingBox::xmax)
      .def_rw("ymin", &BoundingBox::ymin)
      .def_rw("ymax", &BoundingBox::ymax)
      .def("area", &BoundingBox::area, "Compute area of bounding box")
      .def("width", &BoundingBox::width, "Compute width of bounding box")
      .def("height", &BoundingBox::height, "Compute height of bounding box")
      .def("center", &BoundingBox::center, "Get center point (x, y) of bounding box")
      .def(nb::self == nb::self)
      .def(nb::self < nb::self);

  // =====================================================================
  // Object Struct
  // =====================================================================

  nb::class_<Object>(m, "Object")
      .def(nb::init<>())
      .def_rw("object_class", &Object::object_class)
      .def_rw("probability", &Object::probability)
      .def_rw("bbox", &Object::bbox)
      .def(nb::self == nb::self)
      .def(nb::self < nb::self);

  // =====================================================================
  // Frame Struct
  // =====================================================================

  nb::class_<Frame>(m, "Frame")
      .def(nb::init<>())
      .def_rw("timestamp", &Frame::timestamp)
      .def_rw("frame_num", &Frame::frame_num)
      .def_rw("size_x", &Frame::size_x)
      .def_rw("size_y", &Frame::size_y)
      .def_rw("objects", &Frame::objects, "Map from object ID (string) to Object")
      .def("universe_bbox", &Frame::universe_bbox, "Get universe bounding box (entire frame)");

  // =====================================================================
  // Trace Type Alias
  // =====================================================================

  // Trace is std::vector<Frame>, so it's automatically supported
  // We expose it as Python list[Frame] through the STL bindings

  // =====================================================================
  // Utility Functions
  // =====================================================================

  m.def(
      "get_reference_point",
      &get_reference_point,
      nb::arg("bbox"),
      nb::arg("ref_type"),
      "Get coordinates of a reference point on a bounding box");

  m.def(
      "euclidean_distance",
      &euclidean_distance,
      nb::arg("bbox1"),
      nb::arg("ref1"),
      nb::arg("bbox2"),
      nb::arg("ref2"),
      "Compute Euclidean distance between two reference points");
}
// NOLINTEND(misc-redundant-expression)
