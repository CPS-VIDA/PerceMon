#include "percemon/monitoring.hpp"

#include "percemon/fmt.hpp"

#include "percemon/exception.hh"

#include <algorithm>
#include <map>
#include <numeric>

#include "percemon/utils.hpp"
#include <itertools.hpp>

using namespace percemon;
using namespace percemon::monitoring;
namespace ds = percemon::datastream;

namespace {

// TODO: Revisit if this can be configurable.
constexpr double TOP    = std::numeric_limits<double>::infinity();
constexpr double BOTTOM = -TOP;

constexpr double bool_to_robustness(const bool v) {
  return (v) ? TOP : BOTTOM;
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

  std::vector<double> eval(const ast::Expr e) {
    return std::visit(*this, e);
  }

  std::vector<double> eval(const ast::TemporalBoundExpr e) {
    return std::visit(*this, e);
  }

  std::vector<double> operator()(const ast::Const e);
  std::vector<double> operator()(const ast::TimeBound e);
  std::vector<double> operator()(const ast::FrameBound e);
  std::vector<double> operator()(const ast::CompareId e);
  std::vector<double> operator()(const ast::CompareClass e);
  std::vector<double> operator()(const ast::ExistsPtr e);
  std::vector<double> operator()(const ast::ForallPtr e);
  std::vector<double> operator()(const ast::PinPtr e);
  std::vector<double> operator()(const ast::NotPtr e);
  std::vector<double> operator()(const ast::AndPtr e);
  std::vector<double> operator()(const ast::OrPtr e);
  std::vector<double> operator()(const ast::PreviousPtr e);
  std::vector<double> operator()(const ast::AlwaysPtr e);
  std::vector<double> operator()(const ast::SometimesPtr e);
  std::vector<double> operator()(const ast::SincePtr e);
  std::vector<double> operator()(const ast::BackToPtr e);
};
} // namespace

OnlineMonitor::OnlineMonitor(const ast::Expr& phi_, const double fps_) :
    phi{phi_}, fps{fps_} {
  // Set up the horizon. This will throw if phi is unbounded.
  this->max_horizon = get_horizon(phi, fps);
}

void OnlineMonitor::add_frame(const datastream::Frame& frame) {
  this->buffer.push_back(frame);
  if (this->buffer.size() > this->max_horizon) {
    this->buffer.pop_front();
  }
}
void OnlineMonitor::add_frame(datastream::Frame&& frame) {
  this->buffer.push_back(std::move(frame));
  if (this->buffer.size() > this->max_horizon) {
    this->buffer.pop_front();
  }
}

double OnlineMonitor::eval() const {
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

std::vector<double> RobustnessOp::operator()(const ast::TimeBound e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());
  // TODO: Parallelize
  std::transform( // Iterate the trace in reverse order.
      std::rbegin(this->trace),
      std::rend(this->trace),
      std::back_inserter(ret),
      [&](const ds::Frame& f) {
        // TODO: Check if evaluating against timestamp is correct.
        // TODO: Timestamp should be elapsed time from beginning of monitoring.
        switch (e.op) {
          case ast::ComparisonOp::LT:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.x)) - f.timestamp < e.bound);
          case ast::ComparisonOp::LE:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.x)) - f.timestamp <= e.bound);
          case ast::ComparisonOp::GT:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.x)) - f.timestamp > e.bound);
          case ast::ComparisonOp::GE:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.x)) - f.timestamp >= e.bound);
          default: throw std::logic_error("unreachable as EQ and NE are not possible");
        }
      });
  // And reverse the vector to preserve order
  std::reverse(std::begin(ret), std::end(ret));
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::FrameBound e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());
  // TODO: Parallelize
  std::transform( // Iterate the trace in reverse order.
      std::rbegin(this->trace),
      std::rend(this->trace),
      std::back_inserter(ret),
      [&](const ds::Frame& f) {
        // TODO: Check if evaluating against frame_num is correct.
        switch (e.op) {
          case ast::ComparisonOp::LT:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.f)) - f.frame_num < e.bound);
          case ast::ComparisonOp::LE:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.f)) - f.frame_num <= e.bound);
          case ast::ComparisonOp::GT:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.f)) - f.frame_num > e.bound);
          case ast::ComparisonOp::GE:
            return bool_to_robustness(
                this->var_map.at(fmt::to_string(e.f)) - f.frame_num >= e.bound);
          default: throw std::logic_error("unreachable as EQ and NE are not possible");
        }
      });
  // And reverse the vector to preserve order
  std::reverse(std::begin(ret), std::end(ret));
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareId e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());
  // TODO: Parallelize
  std::transform( // Iterate the trace in reverse order.
      std::rbegin(this->trace),
      std::rend(this->trace),
      std::back_inserter(ret),
      [&](const ds::Frame&) {
        // Get the object ID associated with each ID ins CompareId.
        const auto obj_id1 = this->obj_map.at(fmt::to_string(e.lhs));
        const auto obj_id2 = this->obj_map.at(fmt::to_string(e.rhs));
        // TODO: Ask Mohammad if the ID constraints are correctly evaluated. Paper is
        // vague. Do I have to check bounding boxes and other things too?
        switch (e.op) {
          case ComparisonOp::EQ: return bool_to_robustness(obj_id1 == obj_id2);
          case ComparisonOp::NE: return bool_to_robustness(obj_id1 != obj_id2);
          default:
            throw std::logic_error("unreachable as constraint can only be EQ and NE");
        }
      });
  // And reverse the vector to preserve order
  std::reverse(std::begin(ret), std::end(ret));
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::CompareClass e) {
  auto ret = std::vector<double>{};
  ret.reserve(this->trace.size());
  // TODO: Parallelize
  std::transform( // Iterate the trace in reverse order.
      std::rbegin(this->trace),
      std::rend(this->trace),
      std::back_inserter(ret),
      [&](const ds::Frame& f) {
        // Get the object ID associated with each ID ins CompareId.

        const auto class1 =
            f.objects.at(this->obj_map.at(fmt::to_string(e.lhs.id))).object_class;

        const auto class2 = std::visit(
            utils::overloaded{[](int i) { return i; },
                              [&](ast::Class c) {
                                return f.objects
                                    .at(this->obj_map.at(fmt::to_string(c.id)))
                                    .object_class;
                              }},
            e.rhs);

        switch (e.op) {
          case ComparisonOp::EQ: return bool_to_robustness(class1 == class2);
          case ComparisonOp::NE: return bool_to_robustness(class1 != class2);
          default:
            throw std::logic_error("unreachable as constraint can only be EQ and NE");
        }
      });
  // And reverse the vector to preserve order
  std::reverse(std::begin(ret), std::end(ret));
  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::ExistsPtr e) {
  // This is hard...
  // Need to iterate over all k-sized, repeated permutations of IDs in the Frame, where
  // k = number of IDs in the Exists quantifier.  Moreover, I need to assign the Var_id
  // to each permutation in the map.  This cannot be parallelized yet as the current
  // class holds the context for sub-formulas. Look into Task Graph for parallelization.

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

  auto rob         = std::vector<double>{};
  auto running_max = std::vector<double>(n, BOTTOM);

  const auto& cur_frame = this->trace.back(); // Reference to the current frame.
  for (const auto permutation : utils::product(
           cur_frame.objects,
           k)) { // For every k-sized permutation (with repetition) of objects in frame
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
    std::transform(
        std::begin(running_max),
        std::end(running_max),
        std::begin(sub_rob),
        std::begin(running_max),
        [](const double a, const double b) { return std::max(a, b); });
  }

  // Remove the Var_id in obj_map as they are out of scope.
  for (auto&& id : ids) { obj_map.erase(fmt::to_string(id)); }
  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::ForallPtr e) {
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

  auto rob         = std::vector<double>{};
  auto running_min = std::vector<double>(n, TOP);

  const auto& cur_frame = this->trace.back(); // Reference to the current frame.
  for (const auto permutation : utils::product(
           cur_frame.objects,
           k)) { // For every k-sized permutation (with repetition) of objects in frame
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
    std::transform(
        std::begin(running_min),
        std::end(running_min),
        std::begin(sub_rob),
        std::begin(running_min),
        [](const double a, const double b) { return std::min(a, b); });
  }

  // Remove the Var_id in obj_map as they are out of scope.
  for (auto&& id : ids) { obj_map.erase(fmt::to_string(id)); }
  return rob;
};

std::vector<double> RobustnessOp::operator()(const ast::PinPtr e) {
  // Just update the Var_x and/or Var_f.
  if (e->x.has_value()) {
    this->var_map[fmt::to_string(*(e->x))] = this->trace.back().timestamp;
  }
  if (e->f.has_value()) {
    this->var_map[fmt::to_string(*(e->f))] = this->trace.back().frame_num;
  }
  return this->eval(e->phi);
}

std::vector<double> RobustnessOp::operator()(const ast::NotPtr e) {
  auto rob = this->eval(e->arg);
  std::transform(
      std::begin(rob), std::end(rob), std::begin(rob), [](double r) { return -1 * r; });
  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::AndPtr e) {
  // Populate robustness signals of sub formulas
  auto subformula_robs = std::vector<std::vector<double>>{};
  std::transform(
      std::begin(e->temporal_bound_args),
      std::end(e->temporal_bound_args),
      std::back_inserter(subformula_robs),
      [&](const ast::TemporalBoundExpr sub_expr) { return this->eval(sub_expr); });
  std::transform(
      std::begin(e->args),
      std::end(e->args),
      std::back_inserter(subformula_robs),
      [&](const ast::Expr sub_expr) { return this->eval(sub_expr); });

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

std::vector<double> RobustnessOp::operator()(const ast::OrPtr e) {
  // Populate robustness signals of sub formulas
  auto subformula_robs = std::vector<std::vector<double>>{};
  std::transform(
      std::begin(e->temporal_bound_args),
      std::end(e->temporal_bound_args),
      std::back_inserter(subformula_robs),
      [&](const ast::TemporalBoundExpr sub_expr) { return this->eval(sub_expr); });
  std::transform(
      std::begin(e->args),
      std::end(e->args),
      std::back_inserter(subformula_robs),
      [&](const ast::Expr sub_expr) { return this->eval(sub_expr); });

  // Compute max across the returned robustness values.
  auto ret       = std::vector<double>{};
  const size_t n = std::size(this->trace);
  for (const size_t i : iter::range(n)) {
    double rval_i = TOP;
    for (const auto& rvec : subformula_robs) { rval_i = std::max(rvec.at(i), rval_i); }
    ret.push_back(rval_i);
  }

  return ret;
}

std::vector<double> RobustnessOp::operator()(const ast::PreviousPtr e) {
  // Get subformula robustness for horizon
  auto rho = this->eval(e->arg);
  // Iterate from the back and keep updating
  for (auto i = std::rbegin(rho); i != std::rend(rho); i++) {
    if (std::next(i) != std::rend(rho)) {
      *i = *std::next(i);
    }
    // Else don't do anything. This will not mess up mins or maxs.
    // TODO: Is this correct? Any parent temporal operator should not
    // evaluate this as it should exceed bounds.
  }
  return rho;
}

std::vector<double> RobustnessOp::operator()(const ast::AlwaysPtr e) {
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

std::vector<double> RobustnessOp::operator()(const ast::SometimesPtr e) {
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

std::vector<double> RobustnessOp::operator()(const ast::SincePtr e) {
  // Ported from signal-temporal-logic:
  //  Unbounded Until but parse the signal in reverse order.
  //  Since the actual loop computes Until robustness DP-style in reverse, we have to do
  //  it from front.
  const auto x = this->eval(e->args.first);
  const auto y = this->eval(e->args.second);

  auto rob = std::vector<double>{};

  double prev      = TOP;
  double max_right = BOTTOM;

  for (auto&& [i, j] : iter::zip(x, y)) {
    max_right = std::max(max_right, j);
    prev      = std::max({j, std::min(i, prev), -max_right});
    rob.push_back(prev);
  }

  return rob;
}

std::vector<double> RobustnessOp::operator()(const ast::BackToPtr) {
  throw percemon::not_implemented_error(
      "Robustness for BackTo has not been implemented yet.");
}
