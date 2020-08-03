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

  /**
   * Maintain a map from Var_x and Var_f to size_t (double gets converted using fps)
   */
  std::map<std::string, double> var_map;

  /**
   * Maintain a map from Var_id to an actual object ID in frame
   */
  std::map<std::string, std::string> obj_map;

  RobustnessOp(const std::deque<ds::Frame>& buffer, const double fps_) :
      trace{buffer}, fps{fps_} {};

  std::vector<double> eval(const ast::Expr& e) { return std::visit(*this, e); }

  std::vector<double> eval(const ast::TemporalBoundExpr& e) {
    return std::visit(*this, e);
  }

  std::vector<double> operator()(const ast::Const e);
  std::vector<double> operator()(const ast::TimeBound& e);
  std::vector<double> operator()(const ast::FrameBound& e);
  std::vector<double> operator()(const ast::CompareId& e);
  std::vector<double> operator()(const ast::CompareClass& e);
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

  std::vector<double> operator()(const ast::CompareED& e);
  std::vector<double> operator()(const ast::CompareLat& e);
  std::vector<double> operator()(const ast::CompareLon& e);
  std::vector<double> operator()(const ast::CompareArea& e);
};
} // namespace

OnlineMonitor::OnlineMonitor(ast::Expr phi_, const double fps_) :
    phi{std::move(phi_)}, fps{fps_} {
  // Set up the horizon. This will throw if phi is unbounded.
  if (auto opt_hrz = get_horizon(phi, fps)) {
    this->max_horizon = (*opt_hrz == 0) ? 1 : *opt_hrz;
  } else {
    throw std::invalid_argument(fmt::format(
        "Given STQL expression doesn't have a bounded horizon. Cannot perform online monitoring for this formula.\n\tphi := {}",
        phi));
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

  auto rho_op = RobustnessOp{this->buffer, this->fps};
  auto rho    = rho_op.eval(this->phi);
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
      utils::overloaded{[](const int i) -> variant_id { return i; },
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

std::vector<double> RobustnessOp::operator()(const ast::CompareArea& e) {
  using variant_id = std::variant<double, std::string>;

  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());

  // Get tentative IDs to lookup for each frame.
  const std::string& id1      = this->obj_map.at(fmt::to_string(e.lhs.id));
  const variant_id id2_holder = std::visit(
      utils::overloaded{[](double i) -> variant_id { return i; },
                        [&](const ast::Area& a) -> variant_id {
                          return this->obj_map.at(fmt::to_string(a.id));
                        }},
      e.rhs);
  std::function<bool(double, double)> op;
  switch (e.op) {
    case ast::ComparisonOp::GE: op = std::greater_equal<double>{}; break;
    case ast::ComparisonOp::GT: op = std::greater<double>{}; break;
    case ast::ComparisonOp::LE: op = std::less_equal<double>{}; break;
    case ast::ComparisonOp::LT: op = std::less<double>{}; break;
    default:
      throw std::invalid_argument(
          "Malformed expression. Cannot Compare Area with == and !=.");
  }

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
    if (auto area2_ptr = std::get_if<Area>(&(e.rhs))) {
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
      utils::overloaded{[](double i) -> variant_id { return i; },
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
      utils::overloaded{[](double i) -> variant_id { return i; },
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
  // where k = number of IDs in the Exists quantifier.  Moreover, I need to assign the
  // Var_id to each permutation in the map.  This cannot be parallelized yet as the
  // current class holds the context for sub-formulas. Look into Task Graph for
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
    if (std::next(i) != std::rend(rho)) { *i = *std::next(i); }
    // Else don't do anything. This will not mess up mins or maxs.
    // TODO: Is this correct? Any parent temporal operator should not
    // evaluate this as it should exceed bounds.
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
  //  Since the actual loop computes Until robustness DP-style in reverse, we have to
  //  do it from front.
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
