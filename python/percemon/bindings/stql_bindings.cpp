#include "percemon/stql.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace percemon::stql; // NOLINT

namespace {
template <typename T>
  requires ASTNode<T>
constexpr auto fill_expr_ops(nb::class_<T>& expr_cls) {
  expr_cls.def("to_string", &T::to_string)
      .def("__str__", &T::to_string)
      .def("__repr__", &T::to_string)
      .def(
          "__invert__", [](const Expr& e) { return ~e; }, nb::is_operator())
      .def(
          "__and__", [](const Expr& lhs, const Expr& rhs) { return lhs & rhs; }, nb::is_operator())
      .def("__or__", [](const Expr& lhs, const Expr& rhs) { return lhs | rhs; }, nb::is_operator());
}

template <typename Lhs, typename... Rhs>
constexpr auto fill_relational_op(nb::class_<Lhs>& expr) {
  ((expr.def("__lt__", [](const Lhs& a, const Rhs& b) { return a < b; }, nb::is_operator())), ...);
  ((expr.def("__le__", [](const Lhs& a, const Rhs& b) { return a <= b; }, nb::is_operator())), ...);
  ((expr.def("__gt__", [](const Lhs& a, const Rhs& b) { return a > b; }, nb::is_operator())), ...);
  ((expr.def("__ge__", [](const Lhs& a, const Rhs& b) { return a >= b; }, nb::is_operator())), ...);
}

} // namespace

// NOLINTNEXTLINE(misc-use-internal-linkage)
void init_stql_bindings(nb::module_& m) {
  // =====================================================================
  // Enums
  // =====================================================================

  nb::enum_<CoordRefPoint>(m, "CoordRefPoint")
      .value("LeftMargin", CoordRefPoint::LeftMargin)
      .value("RightMargin", CoordRefPoint::RightMargin)
      .value("TopMargin", CoordRefPoint::TopMargin)
      .value("BottomMargin", CoordRefPoint::BottomMargin)
      .value("Center", CoordRefPoint::Center);

  nb::enum_<CompareOp>(m, "CompareOp")
      .value("LessThan", CompareOp::LessThan)
      .value("LessEqual", CompareOp::LessEqual)
      .value("GreaterThan", CompareOp::GreaterThan)
      .value("GreaterEqual", CompareOp::GreaterEqual)
      .value("Equal", CompareOp::Equal)
      .value("NotEqual", CompareOp::NotEqual);

  // =====================================================================
  // Variables
  // =====================================================================

  // NOLINTBEGIN(misc-redundant-expression)
  nb::class_<TimeVar>(m, "TimeVar")
      .def(nb::init<std::string>())
      .def_ro("name", &TimeVar::name)
      .def("to_string", &TimeVar::to_string)
      .def("__str__", &TimeVar::to_string)
      .def("__repr__", &TimeVar::to_string)
      .def(nb::self - nb::self);

  nb::class_<FrameVar>(m, "FrameVar")
      .def(nb::init<std::string>())
      .def_ro("name", &FrameVar::name)
      .def("to_string", &FrameVar::to_string)
      .def("__str__", &FrameVar::to_string)
      .def("__repr__", &FrameVar::to_string)
      .def(nb::self - nb::self);
  // NOLINTEND(misc-redundant-expression)

  nb::class_<ObjectVar>(m, "ObjectVar")
      .def(nb::init<std::string>())
      .def_ro("name", &ObjectVar::name)
      .def("to_string", &ObjectVar::to_string)
      .def("__str__", &ObjectVar::to_string)
      .def("__repr__", &ObjectVar::to_string);

  // Export constants
  m.def("C_TIME", []() { return C_TIME; });
  m.def("C_FRAME", []() { return C_FRAME; });

  // =====================================================================
  // Reference Points and Differences
  // =====================================================================

  nb::class_<RefPoint>(m, "RefPoint")
      .def(nb::init<ObjectVar, CoordRefPoint>())
      .def_ro("object", &RefPoint::object)
      .def_ro("crt", &RefPoint::crt)
      .def("to_string", &RefPoint::to_string)
      .def("__str__", &RefPoint::to_string)
      .def("__repr__", &RefPoint::to_string);

  {
    auto cls = nb::class_<TimeDiff>(m, "TimeDiff")
                   .def(nb::init<TimeVar, TimeVar>())
                   .def_ro("lhs", &TimeDiff::lhs)
                   .def_ro("rhs", &TimeDiff::rhs)
                   .def("to_string", &TimeDiff::to_string)
                   .def("__str__", &TimeDiff::to_string)
                   .def("__repr__", &TimeDiff::to_string);
    fill_relational_op<TimeDiff, double>(cls);
  }

  {
    auto cls = nb::class_<FrameDiff>(m, "FrameDiff")
                   .def(nb::init<FrameVar, FrameVar>())
                   .def_ro("lhs", &FrameDiff::lhs)
                   .def_ro("rhs", &FrameDiff::rhs)
                   .def("to_string", &FrameDiff::to_string)
                   .def("__str__", &FrameDiff::to_string)
                   .def("__repr__", &FrameDiff::to_string);
    fill_relational_op<FrameDiff, int64_t>(cls);
  }

  // =====================================================================
  // Expression Types - All 26 variants
  // =====================================================================

  fill_expr_ops(
      nb::class_<ConstExpr>(m, "ConstExpr")
          .def(nb::init<bool>())
          .def_ro("value", &ConstExpr::value));

  fill_expr_ops(nb::class_<NotExpr>(m, "NotExpr").def(nb::init<Expr>()));

  fill_expr_ops(nb::class_<AndExpr>(m, "AndExpr").def(nb::init<std::vector<Expr>>()));

  fill_expr_ops(nb::class_<OrExpr>(m, "OrExpr").def(nb::init<std::vector<Expr>>()));

  fill_expr_ops(
      nb::class_<NextExpr>(m, "NextExpr")
          .def(nb::init<Expr, size_t>(), nb::arg("arg"), nb::arg("steps") = 1)
          .def_ro("steps", &NextExpr::steps));

  fill_expr_ops(
      nb::class_<PreviousExpr>(m, "PreviousExpr")
          .def(nb::init<Expr, size_t>(), nb::arg("arg"), nb::arg("steps") = 1)
          .def_ro("steps", &PreviousExpr::steps));

  fill_expr_ops(nb::class_<AlwaysExpr>(m, "AlwaysExpr").def(nb::init<Expr>()));

  fill_expr_ops(nb::class_<HoldsExpr>(m, "HoldsExpr").def(nb::init<Expr>()));

  fill_expr_ops(nb::class_<EventuallyExpr>(m, "EventuallyExpr").def(nb::init<Expr>()));

  fill_expr_ops(nb::class_<SometimesExpr>(m, "SometimesExpr").def(nb::init<Expr>()));

  fill_expr_ops(nb::class_<UntilExpr>(m, "UntilExpr").def(nb::init<Expr, Expr>()));

  fill_expr_ops(nb::class_<SinceExpr>(m, "SinceExpr").def(nb::init<Expr, Expr>()));

  fill_expr_ops(nb::class_<ReleaseExpr>(m, "ReleaseExpr").def(nb::init<Expr, Expr>()));

  fill_expr_ops(nb::class_<BackToExpr>(m, "BackToExpr").def(nb::init<Expr, Expr>()));

  fill_expr_ops(
      nb::class_<ExistsExpr>(m, "ExistsExpr").def(nb::init<std::vector<ObjectVar>, Expr>()));

  fill_expr_ops(
      nb::class_<ForallExpr>(m, "ForallExpr").def(nb::init<std::vector<ObjectVar>, Expr>()));

  fill_expr_ops(
      nb::class_<FreezeExpr>(m, "FreezeExpr")
          .def(nb::init<const std::optional<TimeVar>&, const std::optional<FrameVar>&, Expr>())
          .def(nb::init<const std::optional<FrameVar>&, Expr>())
          .def(nb::init<const std::optional<TimeVar>&, Expr>()));

  fill_expr_ops(
      nb::class_<TimeBoundExpr>(m, "TimeBoundExpr")
          .def(nb::init<TimeDiff, CompareOp, double>())
          .def_ro("op", &TimeBoundExpr::op)
          .def_ro("value", &TimeBoundExpr::value));

  fill_expr_ops(
      nb::class_<FrameBoundExpr>(m, "FrameBoundExpr")
          .def(nb::init<FrameDiff, CompareOp, int64_t>())
          .def_ro("op", &FrameBoundExpr::op)
          .def_ro("value", &FrameBoundExpr::value));

  fill_expr_ops(
      nb::class_<ObjectIdCompareExpr>(m, "ObjectIdCompareExpr")
          .def(nb::init<ObjectVar, CompareOp, ObjectVar>())
          .def_ro("op", &ObjectIdCompareExpr::op));

  nb::class_<ClassFunc>(m, "ClassFunc")
      .def(nb::init<ObjectVar>())
      .def("to_string", &ClassFunc::to_string)
      .def("__str__", &ClassFunc::to_string)
      .def("__repr__", &ClassFunc::to_string);

  fill_expr_ops(
      nb::class_<ClassCompareExpr>(m, "ClassCompareExpr")
          .def(nb::init<ClassFunc, CompareOp, std::variant<int, ClassFunc>>())
          .def_ro("op", &ClassCompareExpr::op));

  {
    auto cls = nb::class_<ProbFunc>(m, "ProbFunc")
                   .def(nb::init<ObjectVar>())
                   .def("to_string", &ProbFunc::to_string)
                   .def("__str__", &ProbFunc::to_string)
                   .def("__repr__", &ProbFunc::to_string);
    fill_relational_op<ProbFunc, double, ProbFunc>(cls);
  }

  fill_expr_ops(
      nb::class_<ProbCompareExpr>(m, "ProbCompareExpr")
          .def(nb::init<ProbFunc, CompareOp, std::variant<double, ProbFunc>>())
          .def_ro("op", &ProbCompareExpr::op));

  {
    auto cls = nb::class_<EuclideanDistFunc>(m, "EuclideanDistFunc")
                   .def(nb::init<RefPoint, RefPoint>())
                   .def("to_string", &EuclideanDistFunc::to_string)
                   .def("__str__", &EuclideanDistFunc::to_string)
                   .def("__repr__", &EuclideanDistFunc::to_string);
    fill_relational_op<EuclideanDistFunc, double>(cls);
  }

  fill_expr_ops(
      nb::class_<DistCompareExpr>(m, "DistCompareExpr")
          .def(nb::init<EuclideanDistFunc, CompareOp, double>())
          .def_ro("op", &DistCompareExpr::op)
          .def_ro("rhs", &DistCompareExpr::rhs));

  nb::class_<LatFunc>(m, "LatFunc")
      .def(nb::init<RefPoint>())
      .def("to_string", &LatFunc::to_string)
      .def("__str__", &LatFunc::to_string)
      .def("__repr__", &LatFunc::to_string);

  nb::class_<LonFunc>(m, "LonFunc")
      .def(nb::init<RefPoint>())
      .def("to_string", &LonFunc::to_string)
      .def("__str__", &LonFunc::to_string)
      .def("__repr__", &LonFunc::to_string);

  fill_expr_ops(
      nb::class_<LatLonCompareExpr>(m, "LatLonCompareExpr")
          .def(
              nb::init<
                  std::variant<LatFunc, LonFunc>,
                  CompareOp,
                  std::variant<double, LatFunc, LonFunc>>())
          .def_ro("op", &LatLonCompareExpr::op));

  // =====================================================================
  // Spatial Expression Types
  // =====================================================================

  fill_expr_ops(nb::class_<EmptySetExpr>(m, "EmptySetExpr").def(nb::init<>()));

  fill_expr_ops(nb::class_<UniverseSetExpr>(m, "UniverseSetExpr").def(nb::init<>()));

  fill_expr_ops(nb::class_<BBoxExpr>(m, "BBoxExpr").def(nb::init<ObjectVar>()));

  fill_expr_ops(
      nb::class_<SpatialComplementExpr>(m, "SpatialComplementExpr").def(nb::init<SpatialExpr>()));

  fill_expr_ops(
      nb::class_<SpatialUnionExpr>(m, "SpatialUnionExpr")
          .def(nb::init<std::vector<SpatialExpr>>()));

  fill_expr_ops(
      nb::class_<SpatialIntersectExpr>(m, "SpatialIntersectExpr")
          .def(nb::init<std::vector<SpatialExpr>>()));

  {
    auto cls = nb::class_<AreaFunc>(m, "AreaFunc")
                   .def(nb::init<SpatialExpr>())
                   .def("to_string", &AreaFunc::to_string)
                   .def("__str__", &AreaFunc::to_string)
                   .def("__repr__", &AreaFunc::to_string);
    fill_relational_op<AreaFunc, double, AreaFunc>(cls);
  }

  fill_expr_ops(
      nb::class_<AreaCompareExpr>(m, "AreaCompareExpr")
          .def(nb::init<AreaFunc, CompareOp, std::variant<double, AreaFunc>>())
          .def_ro("op", &AreaCompareExpr::op));

  fill_expr_ops(nb::class_<SpatialExistsExpr>(m, "SpatialExistsExpr").def(nb::init<SpatialExpr>()));

  fill_expr_ops(nb::class_<SpatialForallExpr>(m, "SpatialForallExpr").def(nb::init<SpatialExpr>()));

  // =====================================================================
  // Factory Functions
  // =====================================================================

  m.def("make_true", &make_true, "Create boolean constant true");
  m.def("make_false", &make_false, "Create boolean constant false");

  m.def(
      "next",
      nb::overload_cast<Expr, size_t>(&next),
      nb::arg("arg"),
      nb::arg("steps") = 1,
      "Create next (○) temporal operator");
  m.def(
      "previous",
      nb::overload_cast<Expr, size_t>(&previous),
      nb::arg("arg"),
      nb::arg("steps") = 1,
      "Create previous (◦) temporal operator");
  m.def("always", &always, "Create always (□) temporal operator");
  m.def("eventually", &eventually, "Create eventually (◇) temporal operator");
  m.def("holds", &holds, "Create holds (■) temporal operator");
  m.def("sometimes", &sometimes, "Create sometimes (⧇) temporal operator");
  m.def("until", &until, "Create until (U) temporal operator");
  m.def("since", &since, "Create since (S) temporal operator");
  m.def("release", &release, "Create release (R) temporal operator");
  m.def("backto", &backto, "Create back-to (B) temporal operator");

  m.def(
      "exists",
      nb::overload_cast<std::vector<ObjectVar>, Expr>(&exists),
      "Create existential quantifier (∃)");
  m.def(
      "forall",
      nb::overload_cast<std::vector<ObjectVar>, Expr>(&forall),
      "Create universal quantifier (∀)");

  m.def("freeze", nb::overload_cast<TimeVar, Expr>(&freeze), "Freeze time variable");
  m.def("freeze", nb::overload_cast<FrameVar, Expr>(&freeze), "Freeze frame variable");
  m.def(
      "freeze",
      nb::overload_cast<TimeVar, FrameVar, Expr>(&freeze),
      "Freeze both time and frame variables");

  m.def("bbox", &bbox, "Create bounding box spatial expression");
  m.def("empty_set", &empty_set, "Create empty spatial set");
  m.def("universe", &universe, "Create universal spatial set");
  m.def("spatial_complement", &spatial_complement, "Create spatial complement");
  m.def("spatial_union", &spatial_union, "Create spatial union (⊔)");
  m.def("spatial_intersect", &spatial_intersect, "Create spatial intersection (⊓)");
  m.def("spatial_exists", &spatial_exists, "Create spatial existence quantifier (∃)");
  m.def("spatial_forall", &spatial_forall, "Create spatial universal quantifier (∀)");

  // =====================================================================
  // Perception Helper Functions
  // =====================================================================

  m.def("is_class", &is_class, "Check if object is of specific class");
  m.def("is_not_class", &is_not_class, "Check if object is NOT of specific class");
  m.def(
      "high_confidence",
      nb::overload_cast<const ObjectVar&, double>(&high_confidence),
      nb::arg("obj"),
      nb::arg("threshold") = 0.8,
      "Check if object has high confidence");
  m.def(
      "low_confidence",
      nb::overload_cast<const ObjectVar&, double>(&low_confidence),
      nb::arg("obj"),
      nb::arg("threshold") = 0.5,
      "Check if object has low confidence");

  // =====================================================================
  // Operator Overloads (via Python dunder methods)
  // =====================================================================

  // We expose operators via factory functions or lambdas as needed
  // For Expr, we'd need to add __invert__, __and__, __or__ at the module level
  // This is handled implicitly through the constructors above
}
