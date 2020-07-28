#include "percemon/monitoring.hpp"

#include <algorithm>
#include <numeric>

#include <map>

#include <itertools.hpp>

using namespace percemon;
using namespace percemon::monitoring;
namespace ds = percemon::datastream;

namespace {
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
   * Maintain a map from Var_id to object in frame
   */
  std::map<std::string, std::map<std::string, ds::Object>::const_iterator> obj_map;

  RobustnessOp(const std::deque<ds::Frame>& buffer, const double fps_) :
      trace{buffer}, fps{fps_} {};

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
  auto rho    = std::visit(rho_op, this->phi);
  return rho.back();
}
