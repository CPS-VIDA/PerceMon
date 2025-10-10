#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/variant.h>

#include "percemon/ast.hpp"
#include "percemon/percemon.hpp"

namespace nb = nanobind;

float square(float x) { return x * x; }

auto populate_ast_module(nb::module_& m) {
  using namespace nb::literals;
  using namespace percemon::ast;

  nb::enum_<ComparisonOp>(m, "ComparisonOp")
      .value("GT", ComparisonOp::GT)
      .value("GE", ComparisonOp::GE)
      .value("LT", ComparisonOp::LT)
      .value("LE", ComparisonOp::LE)
      .value("EQ", ComparisonOp::EQ)
      .value("NE", ComparisonOp::NE);

  nb::enum_<CRT>(m, "CRT")
      .value("LM", CRT::LM)
      .value("RM", CRT::RM)
      .value("TM", CRT::TM)
      .value("BM", CRT::BM)
      .value("CT", CRT::CT);
  nb::enum_<Bound>(m, "Bound")
      .value("OPEN", Bound::OPEN)
      .value("LOPEN", Bound::LOPEN)
      .value("ROPEN", Bound::ROPEN)
      .value("CLOSED", Bound::CLOSED);

  nb::class_<Var_x>(m, "Var_x") //
      .def(nb::init<std::string>())
      .def_ro("name", &Var_x::name)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self - C_TIME{});
  nb::class_<Var_f>(m, "Var_f")
      .def(nb::init<std::string>())
      .def_ro("name", &Var_f::name)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self - C_FRAME{});
  nb::class_<Var_id>(m, "Var_id")
      .def(nb::init<std::string>())
      .def_ro("name", &Var_id::name)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self);

  nb::class_<Class>(m, "ClassOf")
      .def(nb::init<Var_id>())
      .def(nb::self == int())
      .def(nb::self == nb::self)
      .def(nb::self != int())
      .def(nb::self != nb::self);

  nb::class_<Prob>(m, "Prob")
      .def(nb::init<Var_id, double>())
      .def(nb::self * double())
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double())
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self <= nb::self);

  nb::class_<ED>(m, "ED")
      .def(nb::init<Var_id, CRT, Var_id, CRT, double>())
      .def_ro("id1", &ED::id1)
      .def_ro("crt1", &ED::crt1)
      .def_ro("id2", &ED::id2)
      .def_ro("crt2", &ED::crt2)
      .def_ro("scale", &ED::scale)
      .def(nb::self *= double())
      .def(nb::self * double())
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double())
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self <= nb::self);
  nb::class_<Lat>(m, "LatDist")
      .def(nb::init<Var_id, CRT, double>())
      .def_ro("id", &Lat::id)
      .def_ro("crt", &Lat::crt)
      .def_ro("scale", &ED::scale)
      .def(nb::self *= double())
      .def(nb::self * double())
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double())
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self <= nb::self)
      .def(nb::self > Lon(Var_id{""}, CRT::LM))
      .def(nb::self >= Lon(Var_id{""}, CRT::LM))
      .def(nb::self < Lon(Var_id{""}, CRT::LM))
      .def(nb::self <= Lon(Var_id{""}, CRT::LM));

  nb::class_<Lon>(m, "LonDist")
      .def(nb::init<Var_id, CRT, double>())
      .def_ro("id", &Lon::id)
      .def_ro("crt", &Lon::crt)
      .def_ro("scale", &ED::scale)
      .def(nb::self *= double())
      .def(nb::self * double())
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double())
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self <= nb::self)
      .def(nb::self > Lat(Var_id{""}, CRT::LM))
      .def(nb::self >= Lat(Var_id{""}, CRT::LM))
      .def(nb::self < Lat(Var_id{""}, CRT::LM))
      .def(nb::self <= Lat(Var_id{""}, CRT::LM));

  nb::class_<AreaOf>(m, "AreaOf")
      .def_ro("id", &AreaOf::id)
      .def(nb::init<Var_id, double>())
      .def(nb::self *= double())
      .def(nb::self * double())
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double())
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self <= nb::self);

  // Expr
  nb::class_<BaseExpr>(m, "BaseExpr")
      .def("to_string", &BaseExpr::to_string)
      .def("__repr__", &BaseExpr::to_string)
      .def("__str__", &BaseExpr::to_string)
      .def(~nb::self)
      .def(nb::self & nb::self)
      .def(nb::self | nb::self)
      .def(nb::self >> nb::self);

  nb::class_<BaseSpatialExpr>(m, "BaseSpatialExpr")
      .def("to_string", &BaseSpatialExpr::to_string)
      .def("__repr__", &BaseSpatialExpr::to_string)
      .def("__str__", &BaseSpatialExpr::to_string)
      .def(~nb::self)
      .def(nb::self & nb::self)
      .def(nb::self | nb::self)
      .def(nb::self >> nb::self);

  nb::class_<EmptySet, BaseSpatialExpr>(m, "EmptySet").def(nb::init<>());
  nb::class_<UniverseSet, BaseSpatialExpr>(m, "UniverseSet").def(nb::init<>());

  nb::class_<Const, BaseExpr>(m, "Const")
      .def(nb::init<bool>())
      .def_ro("value", &Const::value)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self);

  nb::class_<TimeBound, BaseExpr>(m, "TimeBound")
      .def(nb::init<Var_x, ComparisonOp, double>())
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self > double())
      .def(nb::self >= double())
      .def(nb::self < double())
      .def(nb::self <= double());
  nb::class_<FrameBound, BaseExpr>(m, "FrameBound")
      .def(nb::init<Var_f, ComparisonOp, size_t>())
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self > size_t())
      .def(nb::self >= size_t())
      .def(nb::self < size_t())
      .def(nb::self <= size_t());

  nb::class_<TimeInterval>(m, "TimeInterval")
      .def_ro("low", &TimeInterval::low)
      .def_ro("high", &TimeInterval::high)
      .def_ro("bound", &TimeInterval::bound)
      .def_static("open", &TimeInterval::open, "low"_a, "high"_a)
      .def_static("lopen", &TimeInterval::lopen, "low"_a, "high"_a)
      .def_static("ropen", &TimeInterval::ropen, "low"_a, "high"_a)
      .def_static("closed", &TimeInterval::closed, "low"_a, "high"_a);
  nb::class_<FrameInterval>(m, "FrameInterval")
      .def_ro("low", &TimeInterval::low)
      .def_ro("high", &TimeInterval::high)
      .def_ro("bound", &TimeInterval::bound)
      .def_static("open", &FrameInterval::open, "low"_a, "high"_a)
      .def_static("lopen", &FrameInterval::lopen, "low"_a, "high"_a)
      .def_static("ropen", &FrameInterval::ropen, "low"_a, "high"_a)
      .def_static("closed", &FrameInterval::closed, "low"_a, "high"_a);

  nb::class_<CompareId, BaseExpr>(m, "CompareId")
      .def(nb::init<Var_id, ComparisonOp, Var_id>());
  nb::class_<CompareClass, BaseExpr>(m, "CompareClass")
      .def(nb::init<Class, ComparisonOp, std::variant<int, Class>>());
  nb::class_<CompareProb, BaseExpr>(m, "CompareProb")
      .def(nb::init<Prob, ComparisonOp, std::variant<double, Prob>>());

  nb::class_<CompareED, BaseExpr>(m, "CompareED")
      .def(nb::init<ED, ComparisonOp, double>());
  nb::class_<CompareLat, BaseExpr>(m, "CompareLat")
      .def(nb::init<Lat, ComparisonOp, std::variant<double, Lat, Lon>>());
  nb::class_<CompareLon, BaseExpr>(m, "CompareLon")
      .def(nb::init<Lon, ComparisonOp, std::variant<double, Lat, Lon>>());
  nb::class_<CompareArea, BaseExpr>(m, "CompareArea")
      .def(nb::init<AreaOf, ComparisonOp, std::variant<double, AreaOf>>());

  nb::class_<Exists, BaseExpr>(m, "Exists")
      .def(nb::init<std::vector<Var_id>, Expr>())
      .def_ro("ids", &Exists::ids)
      .def_ro("phi", &Exists::arg);
  nb::class_<Forall, BaseExpr>(m, "Forall")
      .def(nb::init<std::vector<Var_id>, Expr>())
      .def_ro("ids", &Forall::ids)
      .def_ro("phi", &Forall::arg);
  nb::class_<Pin, BaseExpr>(m, "Pin")
      .def_ro("time_var", &Pin::x)
      .def_ro("frame_var", &Pin::f)
      .def(nb::init<const std::optional<Var_x>&, const std::optional<Var_f>&, Expr>())
      .def(nb::init<const std::optional<Var_x>&, Expr>())
      .def(nb::init<const std::optional<Var_f>&, Expr>());

  nb::class_<Not, BaseExpr>(m, "Not").def(nb::init<Expr>()).def_ro("arg", &Not::arg);
  nb::class_<And, BaseExpr>(m, "And")
      .def(nb::init<const std::vector<Expr>&>())
      .def_ro("args", &And::args)
      .def_ro("temporal_bound_args", &And::temporal_bound_args);
  nb::class_<Or, BaseExpr>(m, "Or")
      .def(nb::init<const std::vector<Expr>&>())
      .def_ro("args", &Or::args)
      .def_ro("temporal_bound_args", &Or::temporal_bound_args);

  nb::class_<Previous, BaseExpr>(m, "Previous")

      ;
  nb::class_<Always, BaseExpr>(m, "Always");
  nb::class_<Sometimes, BaseExpr>(m, "Sometimes");
  nb::class_<Since, BaseExpr>(m, "Since");
  nb::class_<BackTo, BaseExpr>(m, "BackTo");
  nb::class_<CompareSpArea, BaseExpr>(m, "CompareSpArea");
  nb::class_<SpExists, BaseExpr>(m, "SpExists");
  nb::class_<SpForall, BaseExpr>(m, "SpForall");

  nb::class_<BBox>(m, "BBoxOf").def(nb::init<Var_id>()).def_ro("id", &BBox::id);
  nb::class_<Complement>(m, "Complement");
  nb::class_<Intersect>(m, "Intersect");
  nb::class_<Union>(m, "Union");
  nb::class_<Interior>(m, "Interior");
  nb::class_<Closure>(m, "Closure");
  nb::class_<SpPrevious>(m, "SpPrevious");
  nb::class_<SpAlways>(m, "SpAlways");
  nb::class_<SpSometimes>(m, "SpSometimes");
  nb::class_<SpSince>(m, "SpSince");
  nb::class_<SpBackTo>(m, "SpBackTo");
  nb::class_<SpArea>(m, "SpArea");
}

NB_MODULE(percemon, m) {
  auto ast_mod = m.def_submodule("ast");
  populate_ast_module(ast_mod);
}
