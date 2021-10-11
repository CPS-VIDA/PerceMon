#include "percemon/ast.hpp"
#include "percemon/exception.hh"
#include "percemon/fmt.hpp"
#include "percemon/iter.hpp"
#include "percemon/monitoring.hpp"
#include "percemon/topo.hpp"
#include "percemon/utils.hpp"

#include <algorithm>
#include <deque>
#include <functional>
#include <itertools.hpp>
#include <map>
#include <numeric>

using namespace percemon;
using namespace percemon::monitoring;
namespace ds     = percemon::datastream;
namespace utiter = percemon::iter_helpers;

namespace {

// TODO: Revisit if this can be configurable.
constexpr double TOP    = std::numeric_limits<double>::infinity();
constexpr double BOTTOM = -TOP;

constexpr double bool_to_robustness(const bool v) { return (v) ? TOP : BOTTOM; }

inline std::function<bool(double, double)> get_relational_fn(ast::ComparisonOp op) {
  switch (op) {
    case ast::ComparisonOp::GE: return std::greater_equal<double>{}; break;
    case ast::ComparisonOp::GT: return std::greater<double>{}; break;
    case ast::ComparisonOp::LE: return std::less_equal<double>{}; break;
    case ast::ComparisonOp::LT: return std::less<double>{}; break;
    default:
      throw std::invalid_argument(
          "Malformed expression. `==` and `!=` are not relational operations.");
  }
}

inline std::function<bool(double, double)> get_equality_fn(ast::ComparisonOp op) {
  switch (op) {
    case ast::ComparisonOp::EQ: return std::equal_to<double>{}; break;
    case ast::ComparisonOp::NE: return std::not_equal_to<double>{}; break;
    default:
      throw std::invalid_argument(
          "Malformed expression. `<`,`<=`,`>`,`>=` are not equality operations.");
  }
}

constexpr double get_lateral_distance(const topo::BoundingBox& bbox, const CRT crt) {
  switch (crt) {
    // Based on partial ordering defined in paper.
    case ast::CRT::CT: return (bbox.xmax + bbox.xmin) / 2; break;
    case ast::CRT::RM: return bbox.xmax; break; // Bottom Right
    case ast::CRT::LM: return bbox.xmin; break; // Top Left
    case ast::CRT::TM: return bbox.xmax; break; // Top Right
    case ast::CRT::BM: return bbox.xmin; break; // Bottom left
    default: return 0;
  }
}

constexpr double
get_longidutnal_distance(const topo::BoundingBox& bbox, const CRT crt) {
  switch (crt) {
    // Based on partial ordering defined in paper.
    case ast::CRT::CT: return (bbox.ymax + bbox.ymin) / 2; break;
    case ast::CRT::TM: return bbox.ymin; break; // Top Right
    case ast::CRT::BM: return bbox.ymax; break; // Bottom left
    case ast::CRT::LM: return bbox.ymin; break; // Top Left
    case ast::CRT::RM: return bbox.ymax; break; // Bottom Right
    default: return 0;
  }
}

struct RobustnessOp {
  // This version will be incredibly inefficient runtime-wise but will probably be very
  // space efficient, as there are minimal new structures being created, and RAII will
  // handle destruction of various intermediate vectors.
  //
  // TODO: This can be greatly improved by compromising a little bit on space
  // efficiency.
  //
  // How to improve this?
  // ====================
  //
  // At every call of eval, only one frame has been added. Thus, if we store a table
  // with a row for each subformula and a column for each frame in the bounded horizon,
  // we can use a super efficient DP formulation (especially if we encode the DP
  // recursion as some task graph formulation, thereby allowing easy parallelization).
  //
  // Why can't we do it now?
  // =======================
  //
  // Expr is of type std::variant, in which, some variants are not pointers. I tried to
  // do a std::shared_ptr<const Expr>, but for some reason, I kept getting compiler
  // errors. I think a workaround for this will be to define all variants as some form
  // of std::shared_ptr, while exposing functional interfaces to directly create
  // pointers (instead of using std::make_shared).
  //
  const std::deque<ds::Frame>& trace;
  const double fps;
  const topo::BoundingBox universe;

  /**
   * Maintain a map from Var_x and Var_f to size_t (double gets converted using fps)
   */
  std::map<std::string, double> var_map;

  /**
   * Maintain a map from Var_id to an actual object ID in frame
   */
  std::map<std::string, std::string> obj_map;

  RobustnessOp(
      const std::deque<ds::Frame>& buffer,
      const double fps_,
      topo::BoundingBox universe_) :
      trace{buffer}, fps{fps_}, universe{universe_} {};

  std::vector<double> eval(const ast::Expr& e) { return std::visit(*this, e); }

  std::vector<double> eval(const ast::TemporalBoundExpr& e) {
    return std::visit(*this, e);
  }

  std::vector<topo::Region> eval(const ast::SpatialExpr& expr) {
    return std::visit(*this, expr);
  }

  std::vector<double> operator()(const ast::Const e);
  std::vector<double> operator()(const ast::TimeBound& e);
  std::vector<double> operator()(const ast::FrameBound& e);
  std::vector<double> operator()(const ast::CompareId& e);
  std::vector<double> operator()(const ast::CompareProb& e);
  std::vector<double> operator()(const ast::CompareClass& e);
  std::vector<double> operator()(const ast::CompareED& e);
  std::vector<double> operator()(const ast::CompareLat& e);
  std::vector<double> operator()(const ast::CompareLon& e);
  std::vector<double> operator()(const ast::CompareArea& e);

  std::vector<double> operator()(const ast::ExistsPtr& e);
  std::vector<double> operator()(const ast::ForallPtr& e);
  std::vector<double> operator()(const ast::PinPtr& e);
  std::vector<double> operator()(const ast::NotPtr& e);
  std::vector<double> operator()(const ast::AndPtr& e);
  std::vector<double> operator()(const ast::OrPtr& e);
  std::vector<double> operator()(const ast::PreviousPtr& e);
  std::vector<double> operator()(const ast::AlwaysPtr& e);
  std::vector<double> operator()(const ast::SometimesPtr& e);
  std::vector<double> operator()(const ast::SincePtr& e);
  std::vector<double> operator()(const ast::BackToPtr& e);

  std::vector<double> operator()(const ast::CompareSpAreaPtr&);
  std::vector<double> operator()(const ast::SpExistsPtr&);
  std::vector<double> operator()(const ast::SpForallPtr&);

  std::vector<double> operator()(const ast::SpArea&);

  std::vector<topo::Region> operator()(const ast::EmptySet&);
  std::vector<topo::Region> operator()(const ast::UniverseSet&);
  std::vector<topo::Region> operator()(const ast::BBox&);
  std::vector<topo::Region> operator()(const ast::ComplementPtr&);
  std::vector<topo::Region> operator()(const ast::IntersectPtr&);
  std::vector<topo::Region> operator()(const ast::UnionPtr&);
  std::vector<topo::Region> operator()(const ast::InteriorPtr&);
  std::vector<topo::Region> operator()(const ast::ClosurePtr&);
  std::vector<topo::Region> operator()(const ast::SpPreviousPtr& e);
  std::vector<topo::Region> operator()(const ast::SpAlwaysPtr& e);
  std::vector<topo::Region> operator()(const ast::SpSometimesPtr& e);
  std::vector<topo::Region> operator()(const ast::SpSincePtr& e);
  std::vector<topo::Region> operator()(const ast::SpBackToPtr& e);
};
} // namespace

OnlineMonitor::OnlineMonitor(
    ast::Expr phi_,
    const double fps_,
    double x_boundary,
    double y_boundary) :
    phi{std::move(phi_)}, fps{fps_}, universe_x{x_boundary}, universe_y{y_boundary} {
  // Set up the horizon. This will throw if phi is unbounded.
  if (auto opt_hrz = get_horizon(phi, fps)) {
    this->max_horizon = (*opt_hrz == 0) ? 1 : *opt_hrz;
  } else {
    throw std::invalid_argument(fmt::format(
        "Given STQL expression doesn't have a bounded horizon. Cannot perform online monitoring for this formula."));
  }
}

void OnlineMonitor::add_frame(const datastream::Frame& frame) {
  this->buffer.push_back(frame);
  if (this->buffer.size() > this->max_horizon) { this->buffer.pop_front(); }
}
void OnlineMonitor::add_frame(datastream::Frame&& frame) {
  this->buffer.push_back(std::move(frame));
  if (this->buffer.size() > this->max_horizon) { this->buffer.pop_front(); }
}

double OnlineMonitor::eval() {
  // Trace will be traversed in reverse, so the semantics can remain the same as the
  // future semantics. Plus, the back of the returned vector should have the robustness
  // at the current time.

  auto rho_op = RobustnessOp{
      this->buffer,
      this->fps,
      topo::BoundingBox{0, 0, this->universe_x, this->universe_y}};
  auto rho = rho_op.eval(this->phi);
  return rho.back();
}

std::vector<double> RobustnessOp::operator()(const ast::Const e) {
  return std::vector(this->trace.size(), bool_to_robustness(e.value));
}

std::vector<double> RobustnessOp::operator()(const ast::TimeBound& e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());
  for (const ds::Frame& f : this->trace) {
    // TODO: Check if evaluating against timestamp is correct.
    // TODO: Timestamp should be elapsed time from beginning of monitoring.
    switch (e.op) {
      case ast::ComparisonOp::LT:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.x)) - f.timestamp < e.bound));
        break;
      case ast::ComparisonOp::LE:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.x)) - f.timestamp <= e.bound));
        break;
      case ast::ComparisonOp::GT:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.x)) - f.timestamp > e.bound));
        break;
      case ast::ComparisonOp::GE:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.x)) - f.timestamp >= e.bound));
        break;
      default: throw std::logic_error("unreachable as EQ and NE are not possible");
    }
  }
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::FrameBound& e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  for (const ds::Frame& f : this->trace) {
    // TODO: Check if evaluating against frame_num is correct.
    switch (e.op) {
      case ast::ComparisonOp::LT:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.f)) - f.frame_num < e.bound));
        break;
      case ast::ComparisonOp::LE:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.f)) - f.frame_num <= e.bound));
        break;
      case ast::ComparisonOp::GT:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.f)) - f.frame_num > e.bound));
        break;
      case ast::ComparisonOp::GE:
        ret.push_back(bool_to_robustness(
            this->var_map.at(fmt::to_string(e.f)) - f.frame_num >= e.bound));
        break;
      default: throw std::logic_error("unreachable as EQ and NE are not possible");
    }
  }
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareId& e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  for ([[maybe_unused]] const auto& f : this->trace) {
    // Get the object ID associated with each ID ins CompareId.
    const auto obj_id1 = this->obj_map.at(fmt::to_string(e.lhs));
    const auto obj_id2 = this->obj_map.at(fmt::to_string(e.rhs));
    // TODO: Ask Mohammad if the ID constraints are correctly evaluated. Paper is
    // vague. Do I have to check bounding boxes and other things too?
    switch (e.op) {
      case ast::ComparisonOp::EQ:
        ret.push_back(bool_to_robustness(obj_id1 == obj_id2));
        break;
      case ast::ComparisonOp::NE:
        ret.push_back(bool_to_robustness(obj_id1 != obj_id2));
        break;
      default:
        throw std::logic_error("unreachable as constraint can only be EQ and NE");
    }
  }

  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareClass& e) {
  using variant_id = std::variant<int, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1 = this->obj_map.at(fmt::to_string(e.lhs.id));
  const variant_id id2   = std::visit(
      utils::overloaded{
          [](const int i) -> variant_id { return i; },
          [&](const ast::Class& c) -> variant_id {
            return this->obj_map.at(fmt::to_string(c.id));
          }},
      e.rhs);

  for (const auto& f : this->trace) { // For each frame in trace,

    // Check if ID1 is there in the frame.
    const auto class1_it = f.objects.find(id1);
    if (class1_it == f.objects.end()) { // If ID isn't in the frame: -inf
      ret.push_back(BOTTOM);
      continue;
    }
    // Get the class of the ID1 if it is there in frame.
    int class1 = class1_it->second.object_class;

    // Check if ID2 is there in the frame.
    int class2 = -1;
    if (auto p_int = std::get_if<int>(&id2)) {
      // If we are comparing against a class literal, just assign it.
      class2 = *p_int;
    } else if (auto p_str = std::get_if<std::string>(&id2)) {
      // If we are comparing against another class function, check if ID2 is there in
      // the frame.
      const auto class2_it = f.objects.find(*p_str);
      if (class2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      } else {
        class2 = class2_it->second.object_class;
      }
    }

    switch (e.op) {
      case ast::ComparisonOp::EQ:
        ret.push_back(bool_to_robustness(class1 == class2));
        break;
      case ast::ComparisonOp::NE:
        ret.push_back(bool_to_robustness(class1 != class2));
        break;
      default:
        throw std::logic_error("unreachable as constraint can only be EQ and NE");
    }
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareProb& e) {
  using variant_id = std::variant<double, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1      = this->obj_map.at(fmt::to_string(e.lhs.id));
  const variant_id id2_holder = std::visit(
      utils::overloaded{
          [](double i) -> variant_id { return i; },
          [&](const ast::Prob& a) -> variant_id {
            return this->obj_map.at(fmt::to_string(a.id));
          }},
      e.rhs);
  std::function<bool(double, double)> op = get_relational_fn(e.op);

  for (const auto& f : this->trace) { // For each frame in trace,

    // Check if ID1 is there in the frame.
    const auto id1_it = f.objects.find(id1);
    if (id1_it == f.objects.end()) { // If ID isn't in the frame: -inf
      ret.push_back(BOTTOM);
      continue;
    }
    auto prob1 = id1_it->second.probability * e.lhs.scale;

    // Check if ID2 is needed and exists and get the comparing prob.
    double prob2 = 0;
    if (auto prob2_ptr = std::get_if<Prob>(&(e.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      // Just need to refer to the scale.
      prob2 = id2_it->second.probability * prob2_ptr->scale;
    } else {
      prob2 = std::get<double>(e.rhs);
    }
    ret.push_back(bool_to_robustness(op(prob1, prob2)));
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareArea& e) {
  using variant_id = std::variant<double, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1      = this->obj_map.at(fmt::to_string(e.lhs.id));
  const variant_id id2_holder = std::visit(
      utils::overloaded{
          [](double i) -> variant_id { return i; },
          [&](const ast::AreaOf& a) -> variant_id {
            return this->obj_map.at(fmt::to_string(a.id));
          }},
      e.rhs);
  std::function<bool(double, double)> op = get_relational_fn(e.op);

  for (const auto& f : this->trace) { // For each frame in trace,

    // Check if ID1 is there in the frame.
    const auto id1_it = f.objects.find(id1);
    if (id1_it == f.objects.end()) { // If ID isn't in the frame: -inf
      ret.push_back(BOTTOM);
      continue;
    }
    auto bbox1 = topo::BoundingBox{id1_it->second.bbox};
    auto area1 = topo::area(bbox1) * e.lhs.scale;
    // Check if ID2 is needed and exists and get the comparing area.
    double area2 = 0;
    if (auto area2_ptr = std::get_if<AreaOf>(&(e.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      auto bbox2 = topo::BoundingBox{id2_it->second.bbox};
      // Just need to refer to the scale.
      area2 = topo::area(bbox2) * area2_ptr->scale;
    } else {
      area2 = std::get<double>(e.rhs);
    }
    ret.push_back(bool_to_robustness(op(area1, area2)));
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareED& e) {
  // TODO!!!
  throw not_implemented_error(
      "Semantics for comparing Euclidean distances between objects hasn't been implemented due to ambiguities in the formalization.");
}

std::vector<double> RobustnessOp::operator()(const ast::CompareLat& expr) {
  using variant_id = std::variant<double, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1      = this->obj_map.at(fmt::to_string(expr.lhs.id));
  const variant_id id2_holder = std::visit(
      utils::overloaded{
          [](double i) -> variant_id { return i; },
          [&](const auto& a) -> variant_id {
            return this->obj_map.at(fmt::to_string(a.id));
          }},
      expr.rhs);
  std::function<bool(double, double)> op = get_relational_fn(expr.op);

  for (const auto& f : this->trace) { // For each frame in trace,

    // Check if ID1 is there in the frame.
    const auto id1_it = f.objects.find(id1);
    if (id1_it == f.objects.end()) { // If ID isn't in the frame: -inf
      ret.push_back(BOTTOM);
      continue;
    }
    auto bbox1         = topo::BoundingBox{id1_it->second.bbox};
    auto lat_distance1 = get_lateral_distance(bbox1, expr.lhs.crt) * expr.lhs.scale;

    // Check if ID2 is needed and exists and get the comparing area.
    double distance2 = 0;
    if (auto rhs_lon_ptr = std::get_if<Lon>(&(expr.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      auto bbox2 = topo::BoundingBox{id2_it->second.bbox};
      // Just need to refer to the scale.
      distance2 =
          get_longidutnal_distance(bbox2, rhs_lon_ptr->crt) * rhs_lon_ptr->scale;
    } else if (auto rhs_lat_ptr = std::get_if<Lat>(&(expr.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      auto bbox2 = topo::BoundingBox{id2_it->second.bbox};
      // Just need to refer to the scale.
      distance2 = get_lateral_distance(bbox2, rhs_lon_ptr->crt) * rhs_lat_ptr->scale;
    } else {
      distance2 = std::get<double>(expr.rhs);
    }
    ret.push_back(bool_to_robustness(op(lat_distance1, distance2)));
  }

  assert(ret.size() == this->trace.size());
  return ret;
}
std::vector<double> RobustnessOp::operator()(const ast::CompareLon& expr) {
  using variant_id = std::variant<double, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1      = this->obj_map.at(fmt::to_string(expr.lhs.id));
  const variant_id id2_holder = std::visit(
      utils::overloaded{
          [](double i) -> variant_id { return i; },
          [&](const auto& a) -> variant_id {
            return this->obj_map.at(fmt::to_string(a.id));
          }},
      expr.rhs);
  std::function<bool(double, double)> op = get_relational_fn(expr.op);

  for (const auto& f : this->trace) { // For each frame in trace,

    // Check if ID1 is there in the frame.
    const auto id1_it = f.objects.find(id1);
    if (id1_it == f.objects.end()) { // If ID isn't in the frame: -inf
      ret.push_back(BOTTOM);
      continue;
    }
    auto bbox1         = topo::BoundingBox{id1_it->second.bbox};
    auto lon_distance1 = get_longidutnal_distance(bbox1, expr.lhs.crt) * expr.lhs.scale;

    // Check if ID2 is needed and exists and get the comparing area.
    double distance2 = 0;
    if (auto rhs_lon_ptr = std::get_if<Lon>(&(expr.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      auto bbox2 = topo::BoundingBox{id2_it->second.bbox};
      // Just need to refer to the scale.
      distance2 =
          get_longidutnal_distance(bbox2, rhs_lon_ptr->crt) * rhs_lon_ptr->scale;
    } else if (auto rhs_lat_ptr = std::get_if<Lat>(&(expr.rhs))) {
      // We already have ID. Get it
      const std::string& id2 =
          std::get<std::string>(id2_holder); // The holder must hold strin.
      // Check if it exists in the frame.
      auto id2_it = f.objects.find(id2);
      if (id2_it == f.objects.end()) { // ID2 not in frame: -inf
        ret.push_back(BOTTOM);
        continue;
      }
      auto bbox2 = topo::BoundingBox{id2_it->second.bbox};
      // Just need to refer to the scale.
      distance2 = get_lateral_distance(bbox2, rhs_lon_ptr->crt) * rhs_lat_ptr->scale;
    } else {
      distance2 = std::get<double>(expr.rhs);
    }
    ret.push_back(bool_to_robustness(op(lon_distance1, distance2)));
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::ExistsPtr& e) {
  // This is hard...
  // Need to iterate over all k-sized, repeated permutations of IDs in the Frame,
  // where k = number of IDs in the Exists quantifier.  Moreover, I need to assign
  // the Var_id to each permutation in the map.  This cannot be parallelized yet as
  // the current class holds the context for sub-formulas. Look into Task Graph for
  // parallelization.

  // Overview:
  //
  // For each permutation of id values:
  //    Update the obj_var map
  //    Compute robustness vector for sub-formula.
  //    Maintain a running element-wise max

  // Get the list of variable names
  const auto& ids = e->ids;
  // Check if they are already in the map. This will imply that someone is using the
  // same variable name again. This is an error.
  for (auto&& id : ids) {
    if (this->obj_map.find(fmt::to_string(id)) != std::end(this->obj_map)) {
      throw std::invalid_argument(fmt::format(
          "{} seems to be created multiple times in the formula. This is not valid.",
          id));
    }
  }

  const size_t n = std::size(trace);
  const size_t k = std::size(ids); // Number of Var_id declared in this scope.

  auto running_max = std::vector<double>(n, BOTTOM);

  const auto& cur_frame = this->trace.back(); // Reference to the current frame.
  for (const auto& permutation : utiter::product(
           cur_frame.objects,
           k)) { // For every k-sized permutation (with repetition) of objects in
                 // frame
    // Populate the obj_map
    for (auto&& [i, entry] : iter::enumerate(permutation)) {
      this->obj_map[fmt::to_string(ids.at(i))] = entry.first;
    }
    // Compute robustness of subformula.
    auto sub_rob =
        (e->pinned_at.has_value())
            ? this->eval(ast::Expr{std::make_shared<ast::Pin>(*(e->pinned_at))})
            : this->eval(*(e->phi));
    // Do elementwise max with running_max
    for (auto&& [a, b] : iter::zip(running_max, sub_rob)) { a = std::max(a, b); }
  }

  // Remove the Var_id in obj_map as they are out of scope.
  for (auto&& id : ids) { obj_map.erase(fmt::to_string(id)); }
  return running_max;
}

std::vector<double> RobustnessOp::operator()(const ast::ForallPtr& e) {
  // See notes in Exists, and replace any reference to "max" with "min".
  // Get the list of variable names
  const auto& ids = e->ids;
  // Check if they are already in the map. This will imply that someone is using the
  // same variable name again. This is an error.
  for (auto&& id : ids) {
    if (this->obj_map.find(fmt::to_string(id)) != std::end(this->obj_map)) {
      throw std::invalid_argument(fmt::format(
          "{} seems to be created multiple times in the formula. This is not valid.",
          id));
    }
  }

  const size_t n = std::size(trace);
  const size_t k = std::size(ids); // Number of Var_id declared in this scope.

  auto running_min = std::vector<double>(n, TOP);

  const auto& cur_frame = this->trace.back(); // Reference to the current frame.
  for (const auto& permutation : utiter::product(
           cur_frame.objects,
           k)) { // For every k-sized permutation (with repetition) of objects in
                 // frame
    // Populate the obj_map
    for (auto&& [i, entry] : iter::enumerate(permutation)) {
      this->obj_map[fmt::to_string(ids.at(i))] = entry.first;
    }
    // Compute robustness of subformula.
    auto sub_rob =
        (e->pinned_at.has_value())
            ? this->eval(ast::Expr{std::make_shared<ast::Pin>(*(e->pinned_at))})
            : this->eval(*(e->phi));
    // Do elementwise min with running_min
    for (auto&& [a, b] : iter::zip(running_min, sub_rob)) { a = std::min(a, b); }
  }

  // Remove the Var_id in obj_map as they are out of scope.
  for (auto&& id : ids) { obj_map.erase(fmt::to_string(id)); }
  return running_min;
}

std::vector<double> RobustnessOp::operator()(const ast::PinPtr& e) {
  // Just update the Var_x and/or Var_f.
  if (e->x.has_value()) {
    this->var_map[fmt::to_string(*(e->x))] = this->trace.back().timestamp;
  }
  if (e->f.has_value()) {
    this->var_map[fmt::to_string(*(e->f))] = this->trace.back().frame_num;
  }
  return this->eval(e->phi);
}

std::vector<double> RobustnessOp::operator()(const ast::NotPtr& e) {
  auto rob = this->eval(e->arg);
  for (auto&& r : rob) { r = -1 * r; }
  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::AndPtr& e) {
  // Populate robustness signals of sub formulas
  auto subformula_robs = std::vector<std::vector<double>>{};
  for (const auto& sub_expr : e->temporal_bound_args) {
    auto sub_rho = this->eval(sub_expr);
    subformula_robs.push_back(sub_rho);
  }
  for (const auto& sub_expr : e->args) {
    auto sub_rho = this->eval(sub_expr);
    subformula_robs.push_back(sub_rho);
  }

  // Compute min across the returned robustness values.
  auto ret       = std::vector<double>{};
  const size_t n = std::size(this->trace);
  for (const size_t i : iter::range(n)) {
    double rval_i = TOP;
    for (const auto& rvec : subformula_robs) { rval_i = std::min(rvec.at(i), rval_i); }
    ret.push_back(rval_i);
  }

  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::OrPtr& e) {
  // Populate robustness signals of sub formulas
  auto subformula_robs = std::vector<std::vector<double>>{};
  for (const auto& sub_expr : e->temporal_bound_args) {
    auto sub_rho = this->eval(sub_expr);
    subformula_robs.push_back(sub_rho);
  }
  for (const auto& sub_expr : e->args) {
    auto sub_rho = this->eval(sub_expr);
    subformula_robs.push_back(sub_rho);
  }

  // Compute max across the returned robustness values.
  auto ret       = std::vector<double>{};
  const size_t n = std::size(this->trace);
  for (const size_t i : iter::range(n)) {
    double rval_i = BOTTOM;
    for (const auto& rvec : subformula_robs) { rval_i = std::max(rvec.at(i), rval_i); }
    ret.push_back(rval_i);
  }

  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::PreviousPtr& e) {
  // Get subformula robustness for horizon
  auto rho = this->eval(e->arg);
  // Iterate from the back and keep updating
  for (auto i = std::rbegin(rho); i != std::rend(rho); i++) {
    if (std::next(i) != std::rend(rho)) {
      *i = *std::next(i);
    } else {
      *i = BOTTOM;
    }
  }
  return rho;
}

std::vector<double> RobustnessOp::operator()(const ast::AlwaysPtr& e) {
  // Just do min across the entire horizon, and hope that the bounds hold.
  // TODO: Verify this.
  auto rho = this->eval(e->arg);
  // DP-style min, start from the front
  double running_min = TOP;
  for (auto&& i : rho) {
    running_min = std::min(i, running_min);
    i           = running_min;
  }
  return rho;
}

std::vector<double> RobustnessOp::operator()(const ast::SometimesPtr& e) {
  // Just do max across the entire horizon, and hope that the bounds hold.
  // TODO: Verify this.
  auto rho = this->eval(e->arg);
  // DP-style max, start from the front
  double running_max = BOTTOM;
  for (auto&& i : rho) {
    running_max = std::max(i, running_max);
    i           = running_max;
  }
  return rho;
}

std::vector<double> RobustnessOp::operator()(const ast::SincePtr& e) {
  // Ported from signal-temporal-logic:
  //  Unbounded Until but parse the signal in reverse order.
  //  Since the actual loop computes Until robustness DP-style in reverse, we have
  //  to do it from front.
  const auto x = this->eval(e->args.first);
  const auto y = this->eval(e->args.second);

  auto rob = std::vector<double>{};

  double prev      = TOP;
  double max_right = BOTTOM;

  for (const auto&& [i, j] : iter::zip(x, y)) {
    max_right = std::max(max_right, j);
    prev      = std::max({j, std::min(i, prev), -max_right});
    rob.push_back(prev);
  }

  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::BackToPtr& e) {
  // phi1 BackTo phi2 is the dual for Since.
  // phi1 B phi1 === ~(~phi1 S ~phi2)
  // TODO: VERIFY!!!
  auto x = this->eval(e->args.first);      // Get rho(phi1)
  for (auto&& rob : x) { rob = -1 * rob; } // Get rho(~phi1)

  auto y = this->eval(e->args.second);     // Get rho(phi2)
  for (auto&& rob : y) { rob = -1 * rob; } // Get rho(~phi2)

  // Get since.
  auto rob = std::vector<double>{};

  double prev      = TOP;
  double max_right = BOTTOM;

  for (const auto&& [i, j] : iter::zip(x, y)) {
    max_right = std::max(max_right, j);
    prev      = std::max({j, std::min(i, prev), -max_right});
    rob.push_back(-prev); // Negate on the fly
  }

  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareSpAreaPtr& expr) {
  // Get subformula robustness
  auto rob     = this->operator()(expr->lhs);
  auto rhs_rob = std::visit(
      utils::overloaded{
          [n = this->trace.size()](const double a) {
            return std::vector<double>(n, a);
          },
          [&](const ast::SpArea& a) {
            return this->operator()(a);
          }},
      expr->rhs);

  assert(rob.size() == rhs_rob.size());

  const auto op = get_relational_fn(expr->op);

  for (auto&& [left, right] : iter::zip(rob, rhs_rob)) {
    bool r = op(left, right);
    left   = bool_to_robustness(r);
  }

  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::SpExistsPtr& expr) {
  auto rob = std::vector<double>{};
  rob.reserve(this->trace.size());

  auto sub_regions = this->eval(expr->arg);

  for (auto&& region : sub_regions) {
    auto simplified = topo::simplify_region(region);
    rob.push_back(bool_to_robustness(!std::holds_alternative<topo::Empty>(simplified)));
  }

  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::SpForallPtr&) {
  throw not_implemented_error("SpForall semantics");
}

std::vector<double> RobustnessOp::operator()(const ast::SpArea& expr) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  auto sub_region_vec = this->eval(expr.arg);
  for (auto&& reg : sub_region_vec) { ret.push_back(topo::area(reg)); }
  return ret;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::EmptySet&) {
  auto vec = std::vector<topo::Region>(this->trace.size(), topo::Empty{});
  assert(vec.size() == this->trace.size());
  return vec;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::UniverseSet&) {
  auto vec = std::vector<topo::Region>(this->trace.size(), topo::Universe{});
  assert(vec.size() == this->trace.size());
  return vec;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::BBox& expr) {
  auto vec = std::vector<topo::Region>{};
  vec.reserve(this->trace.size());

  auto id = this->obj_map.at(fmt::to_string(expr.id));

  for (auto&& frame : this->trace) {
    // Check if ID is in frame.
    auto iter = frame.objects.find(id);
    if (iter != frame.objects.end()) {
      vec.emplace_back(topo::BoundingBox{iter->second.bbox});
    } else {
      vec.emplace_back(topo::Empty{});
    }
  }

  assert(vec.size() == this->trace.size());
  return vec;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::ComplementPtr& expr) {
  auto rob = this->eval(expr->arg);

  for (auto&& region : rob) {
    region = topo::spatial_complement(region, this->universe);
  }

  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::IntersectPtr& expr) {
  std::vector<std::vector<topo::Region>> sub_regions;

  // Initialize region vec to each subformula.
  for (auto&& sub_expr : expr->args) { sub_regions.push_back(this->eval(sub_expr)); }

  // Compute intersection across the
  auto rob       = std::vector<topo::Region>{};
  const size_t n = this->trace.size();

  for (const size_t i : iter::range(n)) {
    topo::Region reg = topo::Universe{};
    for (const auto& region_vec : sub_regions) {
      reg = topo::spatial_intersect(reg, region_vec.at(i));
    }
    rob.push_back(reg);
  }

  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::UnionPtr& expr) {
  std::vector<std::vector<topo::Region>> sub_regions;

  // Initialize region vec to each subformula.
  for (auto&& sub_expr : expr->args) { sub_regions.push_back(this->eval(sub_expr)); }

  // Compute unionion across the
  auto rob       = std::vector<topo::Region>{};
  const size_t n = this->trace.size();

  for (const size_t i : iter::range(n)) {
    topo::Region reg = topo::Empty{};
    for (const auto& region_vec : sub_regions) {
      reg = topo::spatial_union(reg, region_vec.at(i));
    }
    rob.push_back(reg);
  }

  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::InteriorPtr& expr) {
  auto rob = this->eval(expr->arg);
  for (auto&& region : rob) { region = topo::interior(region); }
  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::ClosurePtr& expr) {
  auto rob = this->eval(expr->arg);
  for (auto&& region : rob) { region = topo::closure(region); }
  assert(rob.size() == this->trace.size());
  return rob;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::SpPreviousPtr& e) {
  auto sub = this->eval(e->arg);
  // Iterate from the back and keep updating
  for (auto i = std::rbegin(sub); i != std::rend(sub); i++) {
    if (std::next(i) != std::rend(sub)) {
      *i = *std::next(i);
    } else {
      *i = topo::Empty{};
    }
  }
  return sub;
}

namespace {
constexpr size_t interval_size(const FrameInterval& expr) {
  switch (expr.bound) {
    case FrameInterval::OPEN: return expr.high - 1;
    case FrameInterval::LOPEN:
    case FrameInterval::ROPEN: return expr.high;
    case FrameInterval::CLOSED: return expr.high + 1;
  }
}

} // namespace

std::vector<topo::Region> RobustnessOp::operator()(const ast::SpAlwaysPtr& e) {
  auto sub   = this->eval(e->arg);
  auto width = this->trace.size();
  if (e->interval.has_value()) {
    width = std::min(interval_size(*(e->interval)), width);
  }

  auto ret = std::vector<topo::Region>{};
  ret.reserve(this->trace.size());
  {
    // Fill up the first "width" -1  amount with running intersection
    topo::Region running_intersection = topo::Universe{};
    for (auto i : iter::range(width - 1)) {
      running_intersection = topo::spatial_intersect(running_intersection, sub.at(i));
      ret.push_back(running_intersection);
    }
  }

  // Now we need to maintain a sliding window of stuff.
  for (auto&& window : iter::sliding_window(sub, width)) {
    // Compute intersection in each window and push it to the back of ret.
    // TODO: Optimize
    topo::Region running_intersection = topo::Universe{};
    for (auto&& r : window) {
      running_intersection = topo::spatial_intersect(running_intersection, r);
    }
    ret.push_back(running_intersection);
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::SpSometimesPtr& e) {
  auto sub   = this->eval(e->arg);
  auto width = this->trace.size();
  if (e->interval.has_value()) {
    width = std::min(interval_size(*(e->interval)), width);
  }

  auto ret = std::vector<topo::Region>{};
  ret.reserve(this->trace.size());
  {
    // Fill up the first "width" -1  amount with running union
    topo::Region running_union = topo::Empty{};
    for (auto i : iter::range(width - 1)) {
      running_union = topo::spatial_union(running_union, sub.at(i));
      ret.push_back(running_union);
    }
  }

  // Now we need to maintain a sliding window of stuff.
  for (auto&& window : iter::sliding_window(sub, width)) {
    // Compute union in each window and push it to the back of ret.
    // TODO: Optimize
    topo::Region running_union = topo::Universe{};
    for (auto&& r : window) { running_union = topo::spatial_union(running_union, r); }
    ret.push_back(running_union);
  }

  assert(ret.size() == this->trace.size());
  return ret;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::SpSincePtr& e) {
  auto lhs = this->eval(e->args.first);
  auto rhs = this->eval(e->args.second);

  // TODO: Deal with bounded.
  auto vec = std::vector<topo::Region>{};

  topo::Region prev = topo::Universe{};
  for (const auto&& [i, j] : iter::zip(lhs, rhs)) {
    auto tmp = topo::spatial_intersect(i, prev);
    prev     = topo::spatial_union(tmp, j);
    vec.push_back(prev);
  }
  assert(vec.size() == this->trace.size());
  return vec;
}

std::vector<topo::Region> RobustnessOp::operator()(const ast::SpBackToPtr& e) {
  auto lhs = this->eval(e->args.first);
  auto rhs = this->eval(e->args.second);

  // TODO: Deal with bounded.
  auto vec = std::vector<topo::Region>{};

  topo::Region prev = topo::Empty{};
  for (const auto&& [i, j] : iter::zip(lhs, rhs)) {
    auto tmp = topo::spatial_union(i, prev);
    prev     = topo::spatial_intersect(tmp, j);
    vec.push_back(prev);
  }
  assert(vec.size() == this->trace.size());
  return vec;
}
