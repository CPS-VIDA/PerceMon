#include "percemon/evaluation.hpp"
#include "percemon/monitoring.hpp"
#include "percemon/spatial.hpp"
#include "percemon/stql.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace ranges = std::ranges;
namespace views  = std::ranges::views;

namespace percemon::monitoring {

namespace {

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Compare two values using the specified comparison operator.
 *
 * @param lhs Left-hand side value
 * @param op Comparison operator
 * @param rhs Right-hand side value
 * @return true if comparison succeeds, false otherwise
 */
template <typename T>
[[nodiscard]] auto compare_op(T lhs, stql::CompareOp op, T rhs) -> bool {
  switch (op) {
    case stql::CompareOp::LessThan: return lhs < rhs;
    case stql::CompareOp::LessEqual: return lhs <= rhs;
    case stql::CompareOp::GreaterThan: return lhs > rhs;
    case stql::CompareOp::GreaterEqual: return lhs >= rhs;
    case stql::CompareOp::Equal: return std::abs(lhs - rhs) < 1e-9; // Floating point tolerance
    case stql::CompareOp::NotEqual: return std::abs(lhs - rhs) >= 1e-9;
  }
  throw std::logic_error("Unknown comparison operator");
}

/**
 * @brief Extract the numeric value of a reference point from a bounding box.
 *
 * @param bbox The bounding box
 * @param crt Coordinate reference point (LM, RM, TM, BM, CT)
 * @param lateral If true, extract lateral (x) value; if false, longitudinal (y)
 * @return The coordinate value
 *
 * **Reference Frame**:
 * - Lateral (x): LM=left, RM=right, CT=center, TM=right, BM=left
 * - Longitudinal (y): TM=top, BM=bottom, CT=center, LM=top, RM=bottom
 *
 * **See Also**: STQL paper for coordinate reference point definitions
 */
[[nodiscard]] auto get_reference_point_value(
    const datastream::BoundingBox& bbox,
    stql::CoordRefPoint crt,
    bool lateral) -> double {
  using stql::CoordRefPoint;
  if (lateral) {
    // Lateral (x-coordinate)
    switch (crt) {
      case CoordRefPoint::Center: return (bbox.xmin + bbox.xmax) / 2.0; // Center

      case CoordRefPoint::LeftMargin: [[fallthrough]];    // Left Margin
      case CoordRefPoint::BottomMargin: return bbox.xmin; // Bottom-left corner uses left margin
      case CoordRefPoint::RightMargin: [[fallthrough]];   // Right Margin
      case CoordRefPoint::TopMargin: return bbox.xmax;    // Top-right corner uses right margin
    }
  } else {
    // Longitudinal (y-coordinate)
    switch (crt) {
      case CoordRefPoint::Center: return (bbox.ymin + bbox.ymax) / 2.0; // Center

      case CoordRefPoint::LeftMargin: [[fallthrough]];    // Top-left corner uses top margin
      case CoordRefPoint::TopMargin: return bbox.ymin;    // Top Margin
      case CoordRefPoint::RightMargin: [[fallthrough]];   // Bottom-right corner uses bottom margin
      case CoordRefPoint::BottomMargin: return bbox.ymax; // Bottom Margin
    }
  }
  throw std::logic_error("Unknown coordinate reference point");
}

/**
 * @brief Find an object by ID in a frame's object map.
 *
 * @param frame The frame to search
 * @param object_id The object ID to find
 * @return Optional reference to object, nullopt if not found
 */
[[nodiscard]] auto
find_object_in_frame(const datastream::Frame& frame, const std::string& object_id)
    -> std::optional<std::reference_wrapper<const datastream::Object>> {
  auto it = frame.objects.find(object_id);
  if (it != frame.objects.end()) { return std::cref(it->second); }
  return std::nullopt;
}

/**
 * @brief Find the ID of the object in the current context
 *
 * @param obj The object variable
 * @param ctx The evaluation context
 * @return the string ID of the object in the context
 * @throws std::logic_error If the object variable is not bound to the context
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] inline auto
get_object_binding(const stql::ObjectVar& obj, const EvaluationContext<Iter, Sentinel>& ctx)
    -> std::string {
  if (const auto it = ctx.bound_objects.find(obj.name); it != ctx.bound_objects.end()) {
    return it->second;
  }
  throw std::logic_error(std::format("Object variable `{}` not bound", obj.name));
}

/**
 * @brief Helper to shift the evaluation context to the future or past
 *
 * @note this function is wrong if the formula contains both past and future operators
 * as we store history and horizon separately. I am not sure how to fix that, but hope
 * that someone doesn't use those kind of functions. (should validate this...)
 *
 * @param ctx The context to shift
 * @param by The number of frames to shift by. If the number is negative, it shifts the history,
 * else, it shifts the horizon.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto shift_eval_context(const EvaluationContext<Iter, Sentinel>& ctx, int64_t by)
    -> EvaluationContext<Iter, Sentinel> {
  // Copy over the context first
  EvaluationContext<Iter, Sentinel> new_ctx = ctx;

  // NOTE: I could use ranges::advance to handle negative values, but I don't want to
  // deal with the current frame directions.
  if (by > 0) {
    // Shift horizon by `by`
    auto new_hrz_begin = ranges::next(ctx.horizon_begin, by, ctx.horizon_end);
    // Get the current frame using a sentinel value
    new_ctx.current_frame = ranges::prev(new_hrz_begin, 1, new_ctx.horizon_begin);
    new_ctx.horizon_begin = new_hrz_begin;
  } else if (by < 0) {
    // Shift history by `-by`
    by = -by;
    assert(by > 0);
    auto new_hist_end = ranges::prev(ctx.history_end, by, ctx.history_begin);
    // Get the current frame using a sentinel value
    new_ctx.current_frame = new_hist_end;
    new_ctx.history_end   = new_hist_end;
  } else {
    // Do nothing...
  }

  // Make sure C_TIME and C_FRAME points to the current frame
  new_ctx.frozen_times["C_TIME"]   = new_ctx.current_frame->timestamp;
  new_ctx.frozen_frames["C_FRAME"] = new_ctx.current_frame->frame_num;

  return new_ctx;
}

/**
 * @brief Main recursive evaluation function.
 *
 * Implements the visitor pattern using std::visit over the Expr variant.
 * Recursively evaluates all 40+ expression types with proper context handling.
 *
 * @param expr The expression to evaluate
 * @param ctx The evaluation context (bindings, buffers, current frame)
 * @return true if expression is satisfied, false if violated
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_impl(const stql::Expr& expr, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool;

/**
 * @brief Spatial recursive evaluation function.
 *
 * Implements the visitor pattern using std::visit over the SpatialExpr variant.
 *
 * @param expr The expression to evaluate
 * @param ctx The evaluation context (bindings, buffers, current frame)
 * @return true if expression is satisfied, false if violated
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_impl(const stql::SpatialExpr& expr, const EvaluationContext<Iter, Sentinel>& ctx)
    -> spatial::Region;

// ============================================================================
// Evaluation Implementation: Propositional Operators
// ============================================================================

/**
 * @brief Evaluate a boolean constant.
 *
 * **STQL**: `true`, `false`
 *
 * Trivial: return the constant value.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_const(const stql::ConstExpr& e, const EvaluationContext<Iter, Sentinel>& /*ctx*/) -> bool {
  return e.value;
}

/**
 * @brief Evaluate logical negation.
 *
 * **STQL**: `¬φ`
 *
 * Recursively evaluate the subexpression and negate the result.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_not(const stql::NotExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  return !eval_impl(*e.arg, ctx);
}

/**
 * @brief Evaluate logical conjunction.
 *
 * **STQL**: `φ ∧ ψ`
 *
 * Recursively evaluate both subexpressions and return logical AND.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_and(const stql::AndExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  return ranges::all_of(e.args, [&ctx](const auto& arg) { return eval_impl(arg, ctx); });
}

/**
 * @brief Evaluate logical disjunction.
 *
 * **STQL**: `φ ∨ ψ`
 *
 * Recursively evaluate both subexpressions and return logical OR.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_or(const stql::OrExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  return ranges::any_of(e.args, [&ctx](const auto& arg) { return eval_impl(arg, ctx); });
}

// ============================================================================
// Evaluation Implementation: Temporal Operators (Future-Time)
// ============================================================================

/**
 * @brief Evaluate "next" temporal operator.
 *
 * **STQL**: `○φ` (Next)
 *
 * Evaluates φ on the next frame (horizon[0]).
 * Returns false if horizon buffer is empty (cannot evaluate next).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_next(const stql::NextExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // Check if the frame `e.steps` away is available
  if (static_cast<size_t>(ctx.num_horizon()) < e.steps) { return false; }
  EvaluationContext<Iter, Sentinel> next_ctx = shift_eval_context(ctx, e.steps);
  return eval_impl(*e.arg, next_ctx);
}

/**
 * @brief Evaluate "always" temporal operator.
 *
 * **STQL**: `□φ` (Always)
 *
 * Returns true if φ is satisfied on ALL future frames (up to horizon).
 * Returns true if horizon is empty (vacuously true).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_always(const stql::AlwaysExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  for (auto i = ctx.num_horizon() + 1; i > 0; i--) {
    if (!eval_impl(*e.arg, frame_ctx)) { return false; }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, 1);
  }
  return true; // φ is true on all frames
}

/**
 * @brief Evaluate "eventually" temporal operator.
 *
 * **STQL**: `◇φ` (Eventually)
 *
 * Returns true if φ is satisfied on ANY future frame (up to horizon).
 * Returns false if horizon is empty (no opportunity to satisfy).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_eventually(const stql::EventuallyExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  for (auto i = ctx.num_horizon() + 1; i > 0; i--) {
    if (eval_impl(*e.arg, frame_ctx)) { return true; }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, 1);
  }
  return false; // φ is false on all frames
}

/**
 * @brief Evaluate "until" temporal operator.
 *
 * **STQL**: `φ U ψ` (Until)
 *
 * Returns true if:
 * - There exists a frame where ψ is true (the "until point")
 * - φ is true on ALL frames before that point
 *
 * Returns false if ψ never becomes true.
 *
 * **Semantics**: φ must hold until ψ eventually becomes true.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_until(const stql::UntilExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  // We will shift the frame context by 1 at the end of every iteration.
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  // Keep track of if lhs has been true until the previous frame
  bool lhs_until_prev = true;
  // Search for first frame where ψ is true, while keeping track of if LHS has been
  // true until now.
  // This is effectively the expansion of
  // φ U ψ = ψ | (φ & X(φ U ψ)
  for (auto i = ctx.num_horizon() + 1; i > 0; i--) {
    // Check if ψ is true at this frame

    // If rhs is true, just check if lhs was true until now. Return that value
    if (eval_impl(*e.rhs, frame_ctx)) {
      return lhs_until_prev;
    }
    // Otherwise, keep track of the validity of lhs
    else {
      lhs_until_prev = lhs_until_prev && eval_impl(*e.lhs, frame_ctx);
    }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, 1);
  }
  return false; // ψ never became true, i.e., strong until
}

/**
 * @brief Evaluate "release" temporal operator.
 *
 * **STQL**: `φ R ψ` (Release)
 *
 * Dual of Until: returns true if ψ holds on all frames,
 * OR ψ eventually becomes false and φ is true at that point.
 *
 * Returns true if ψ holds forever (ψ is never false).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_release(const stql::ReleaseExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;

  // Check if there's a frame where ψ becomes false
  for (auto i = ctx.num_horizon() + 1; i > 0; i--) {
    if (!eval_impl(*e.rhs, frame_ctx)) {
      // ψ is false at frame i; φ must be true
      // φ is true when ψ first becomes false
      // φ is also false; release violated
      return eval_impl(*e.lhs, frame_ctx);
    }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, 1);
  }
  return true; // ψ never became false; release is satisfied
}

// ============================================================================
// Evaluation Implementation: Temporal Operators (Past-Time)
// ============================================================================

/**
 * @brief Evaluate "previous" temporal operator.
 *
 * **STQL**: `◦φ` (Previous)
 *
 * Evaluates φ on the previous frame (history.back()).
 * Returns false if history buffer is empty (no previous frame).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_previous(const stql::PreviousExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  // Check if the frame `e.steps` away is available
  if (static_cast<size_t>(ctx.num_history()) < e.steps) { return false; }
  const auto neg_shift = -1 * static_cast<int64_t>(e.steps);
  // Create context for previous frame (most recent in history)
  EvaluationContext<Iter, Sentinel> prev_ctx = shift_eval_context(ctx, neg_shift);
  return eval_impl(*e.arg, prev_ctx);
}

/**
 * @brief Evaluate "holds" (always past) temporal operator.
 *
 * **STQL**: `■φ` (Holds, past always)
 *
 * Returns true if φ is satisfied on ALL past frames (up to history).
 * Returns true if history is empty (vacuously true).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_holds(const stql::HoldsExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  for (auto i = ctx.num_history() + 1; i > 0; i--) {
    if (!eval_impl(*e.arg, frame_ctx)) {
      return false; // Found a past frame where φ is false
    }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, -1);
  }
  return true;
}

/**
 * @brief Evaluate "sometimes" (eventually past) temporal operator.
 *
 * **STQL**: `♦φ` (Sometimes, past eventually)
 *
 * Returns true if φ is satisfied on ANY past frame.
 * Returns false if history is empty (no opportunity).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_sometimes(const stql::SometimesExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  for (auto i = ctx.num_history() + 1; i > 0; i--) {
    if (eval_impl(*e.arg, frame_ctx)) {
      return true; // Found a past frame where φ is true
    }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, -1);
  }
  return false;
}

/**
 * @brief Evaluate "since" temporal operator.
 *
 * **STQL**: `φ S ψ` (Since)
 *
 * Returns true if:
 * - There exists a past frame where ψ is true (the "since point")
 * - φ is true on ALL frames from that point to now
 *
 * Returns false if ψ never was true in the past.
 *
 * **Semantics**: φ has been true since ψ was last true.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_since(const stql::SinceExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  // NOTE: see eval_until for implementation details as this is just the past time
  // equivalent of until, where we iterate in the history in reverse effectively.

  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;
  bool lhs_holds                              = true;
  for (auto i = ctx.num_history() + 1; i > 0; i--) {
    if (eval_impl(*e.rhs, frame_ctx)) {
      return lhs_holds;
    } else {
      lhs_holds = lhs_holds && eval_impl(*e.lhs, frame_ctx);
    }
    // move frame one to the past after computing to prevent off-by-1
    frame_ctx = shift_eval_context(frame_ctx, -1);
  }
  return false; // ψ never became true, i.e., strong until
}

/**
 * @brief Evaluate "back-to" temporal operator.
 *
 * **STQL**: `φ B ψ` (BackTo)
 *
 * Dual of Since: returns true if ψ holds at all past frames,
 * OR ψ eventually becomes false going backwards and φ is true at that point.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_backto(const stql::BackToExpr& e, const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  EvaluationContext<Iter, Sentinel> frame_ctx = ctx;

  // Check if there's a frame where ψ becomes false
  for (auto i = ctx.num_history() + 1; i > 0; --i) {
    if (!eval_impl(*e.rhs, frame_ctx)) {
      // ψ is false at frame i; φ must be true
      // φ is also false; backto violated
      // φ is true when ψ first becomes false; backto holds
      return eval_impl(*e.lhs, frame_ctx);
    }
    // Shift at end so we don't do an off-by-1 error
    frame_ctx = shift_eval_context(frame_ctx, -1);
  }
  return true; // ψ never became false; backto is satisfied
}

// ============================================================================
// Evaluation Implementation: Quantifiers
// ============================================================================

/**
 * @brief Evaluate existential quantifier over objects.
 *
 * **STQL**: `∃{id₁, id₂, ...}@φ`
 *
 * Iterates over all permutations of k objects from the current frame,
 * binds each to the quantified variables, and evaluates φ.
 * Returns true if φ is satisfied for ANY permutation.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
  requires std::same_as<std::iter_value_t<Iter>, datastream::Frame>
[[nodiscard]] auto eval_exists(const stql::ExistsExpr& e, EvaluationContext<Iter, Sentinel> ctx)
    -> bool {
  if (ctx.current_frame->objects.empty()) {
    return false; // No objects to quantify over
  }

  const auto& ids     = e.variables;
  size_t k            = ids.size();
  const auto& objects = ctx.current_frame->objects;

  // Check for duplicate bindings in context
  for (const auto& id : ids) {
    if (ctx.bound_objects.count(id.name) > 0) {
      throw std::invalid_argument("Variable '" + id.name + "' is already bound in this scope");
    }
  }

  // Generate all k-permutations (with repetition) of objects
  // For k=1: iterate over each object
  // For k=2: iterate over each pair of objects, etc.

  if (k == 0) {
    return eval_impl(*e.body, ctx); // No quantification: just evaluate φ
  }

  // Simple case: k=1 (single quantified variable)
  if (k == 1) {
    for (const auto& [obj_id, obj] : objects) {
      ctx.bound_objects[ids[0].name] = obj_id;
      if (eval_impl(*e.body, ctx)) {
        return true; // Found an object satisfying φ
      }
      ctx.bound_objects.erase(ids[0].name);
    }
    return false;
  }

  // General case: k>1 (multiple quantified variables)
  // Generate k-permutations with repetition
  std::vector<std::string> object_ids;
  object_ids.reserve(objects.size());
  for (const auto& [obj_id, _] : objects) { object_ids.push_back(obj_id); }

  // Recursive permutation generation
  const std::function<bool(size_t)> generate_permutation = [&](size_t depth) -> bool {
    if (depth == k) {
      // Evaluate φ with this binding
      return eval_impl(*e.body, ctx);
    }
    for (const auto& obj_id : object_ids) {
      ctx.bound_objects[ids[depth].name] = obj_id;
      if (generate_permutation(depth + 1)) { return true; }
      ctx.bound_objects.erase(ids[depth].name);
    }
    return false;
  };

  return generate_permutation(0);
}

/**
 * @brief Evaluate universal quantifier over objects.
 *
 * **STQL**: `∀{id₁, id₂, ...}@φ`
 *
 * Iterates over all permutations of k objects from the current frame,
 * binds each to the quantified variables, and evaluates φ.
 * Returns true if φ is satisfied for ALL permutations.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_forall(const stql::ForallExpr& e, EvaluationContext<Iter, Sentinel> ctx)
    -> bool {
  if (ctx.current_frame->objects.empty()) {
    return true; // No objects to quantify over: vacuously true
  }

  const auto& ids     = e.variables;
  size_t k            = ids.size();
  const auto& objects = ctx.current_frame->objects;

  // Check for duplicate bindings in context
  for (const auto& id : ids) {
    if (ctx.bound_objects.count(id.name) > 0) {
      throw std::invalid_argument("Variable '" + id.name + "' is already bound in this scope");
    }
  }

  if (k == 0) {
    return eval_impl(*e.body, ctx); // No quantification: just evaluate φ
  }

  // Simple case: k=1
  if (k == 1) {
    for (const auto& [obj_id, obj] : objects) {
      ctx.bound_objects[ids[0].name] = obj_id;
      if (!eval_impl(*e.body, ctx)) {
        return false; // Found an object NOT satisfying φ
      }
      ctx.bound_objects.erase(ids[0].name);
    }
    return true;
  }

  // General case: k>1
  std::vector<std::string> object_ids;
  object_ids.reserve(objects.size());
  for (const auto& [obj_id, _] : objects) { object_ids.push_back(obj_id); }

  std::function<bool(size_t)> generate_permutation = [&](size_t depth) -> bool {
    if (depth == k) {
      // Evaluate φ with this binding
      return eval_impl(*e.body, ctx);
    }
    for (const auto& obj_id : object_ids) {
      ctx.bound_objects[ids[depth].name] = obj_id;
      if (!generate_permutation(depth + 1)) { return false; }
      ctx.bound_objects.erase(ids[depth].name);
    }
    return true;
  };

  return generate_permutation(0);
}

/**
 * @brief Evaluate freeze quantifier.
 *
 * **STQL**: `{x, f}.φ`
 *
 * Captures the current timestamp in frozen_times[x], then evaluates φ.
 * Captures the current frame in frozen_frames[f], then evaluates φ.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_freeze(const stql::FreezeExpr& e, EvaluationContext<Iter, Sentinel> ctx)
    -> bool {
  // If there is a time_var, update frozen_times accordingly
  if (e.time_var.has_value()) {
    if (ctx.frozen_times.contains(e.time_var->name)) {
      throw std::invalid_argument(
          "Time variable '" + e.time_var->name + "' is already bound in this scope");
    }
    ctx.frozen_times[e.time_var->name] = ctx.current_frame->timestamp;
  }

  // If there is a frame_var, update frozen_frames accordingly
  if (e.frame_var.has_value()) {
    if (ctx.frozen_frames.contains(e.frame_var->name)) {
      throw std::invalid_argument(
          "frame variable '" + e.frame_var->name + "' is already bound in this scope");
    }
    ctx.frozen_frames[e.frame_var->name] = ctx.current_frame->frame_num;
  }

  // Evaluate the frozen subformula
  bool result = eval_impl(*e.body, ctx);

  // Clear the bindings
  if (e.time_var.has_value()) { ctx.frozen_times.erase(e.time_var->name); }
  if (e.frame_var.has_value()) { ctx.frozen_frames.erase(e.frame_var->name); }

  return result;
}

// ============================================================================
// Evaluation Implementation: Constraint Operators
// ============================================================================

/**
 * @brief Evaluate time-based constraint.
 *
 * **STQL**: `(x - C_TIME) ∼ t`
 *
 * Compares the difference between frozen time x and current time C_TIME.
 * Returns false if x is not in the frozen_times map (unbound variable).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_time_bound(const stql::TimeBoundExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // Set the value for the frozen time variables.
  const auto& diff = e.diff;

  auto lhs = std::numeric_limits<double>::infinity();
  if (auto it = ctx.frozen_times.find(diff.lhs.name); it != ctx.frozen_times.end()) {
    lhs = it->second;
  } else {
    throw std::logic_error(std::format("Variable not frozen {}", diff.lhs.name));
  }

  auto rhs = std::numeric_limits<double>::infinity();
  if (auto it = ctx.frozen_times.find(diff.rhs.name); it != ctx.frozen_times.end()) {
    rhs = it->second;
  } else {
    throw std::logic_error(std::format("Variable not frozen {}", diff.rhs.name));
  }

  return compare_op(lhs - rhs, e.op, e.value);
}

/**
 * @brief Evaluate frame-based constraint.
 *
 * **STQL**: `(f - C_FRAME) ∼ n`
 *
 * Compares the difference between frozen frame f and current frame C_FRAME.
 * Returns false if f is not in the frozen_frames map (unbound variable).
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_frame_bound(const stql::FrameBoundExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // Set the value for the frozen frame variables.
  const auto& diff = e.diff;

  int64_t lhs = UNBOUNDED;
  if (auto it = ctx.frozen_frames.find(diff.lhs.name); it != ctx.frozen_frames.end()) {
    lhs = it->second;
  } else {
    throw std::logic_error(std::format("Variable not frozen {}", diff.lhs.name));
  }

  int64_t rhs = UNBOUNDED;
  if (auto it = ctx.frozen_frames.find(diff.rhs.name); it != ctx.frozen_frames.end()) {
    rhs = it->second;
  } else {
    throw std::logic_error(std::format("Variable not frozen {}", diff.rhs.name));
  }

  return compare_op(static_cast<double>(lhs - rhs), e.op, static_cast<double>(e.value));
}

// ============================================================================
// Evaluation Implementation: Perception Operators
// ============================================================================
/**
 * @brief Evaluate object ID comparison.
 *
 * **STQL**: `id ∼ id`
 *
 * Extracts the object class and compares it with the expected class.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_obj_id_compare(
    const stql::ObjectIdCompareExpr& e,
    const EvaluationContext<Iter, Sentinel>& ctx) -> bool {
  // Resolve the lhs object ID
  std::string lhs_obj_id = get_object_binding(e.lhs, ctx);
  // Find the object in the frame
  int lhs_obj_class = 0;
  if (auto lhs_obj_opt = find_object_in_frame(*ctx.current_frame, lhs_obj_id);
      lhs_obj_opt.has_value()) {
    lhs_obj_class = lhs_obj_opt->get().object_class;
  } else {
    return false; // Object not found in frame
  }

  // Resolve the rhs object ID
  std::string rhs_obj_id = get_object_binding(e.rhs, ctx);
  // Find the object in the frame
  int rhs_obj_class = 0;
  if (auto rhs_obj_opt = find_object_in_frame(*ctx.current_frame, rhs_obj_id);
      rhs_obj_opt.has_value()) {
    rhs_obj_class = rhs_obj_opt->get().object_class;
  } else {
    return false; // Object not found in frame
  }

  // Compare
  switch (e.op) {
    case stql::CompareOp::Equal: return lhs_obj_id == rhs_obj_id && lhs_obj_class == rhs_obj_class;
    case stql::CompareOp::NotEqual:
      return lhs_obj_id != rhs_obj_id && lhs_obj_class != rhs_obj_class;
    default: throw std::logic_error("Object ID comparison only supports EQ and NE");
  }
}

/**
 * @brief Evaluate class comparison.
 *
 * **STQL**: `C(id) ∼ class`
 *
 * Extracts the object class and compares it with the expected class.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_class_compare(const stql::ClassCompareExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // Resolve the lhs object ID
  std::string lhs_obj_id = get_object_binding(e.lhs.object, ctx);
  // Find the object in the frame
  int lhs_obj_class = 0;
  if (auto lhs_obj_opt = find_object_in_frame(*ctx.current_frame, lhs_obj_id);
      lhs_obj_opt.has_value()) {
    lhs_obj_class = lhs_obj_opt->get().object_class;
  } else {
    return false; // Object not found in frame
  }

  auto rhs_obj_class = std::visit(
      [&ctx](const auto& rhs) -> std::optional<int> {
        using rhs_t = std::decay_t<decltype(rhs)>;
        if constexpr (std::is_same_v<rhs_t, int>) {
          return rhs;
        } else {
          std::string rhs_obj_id = get_object_binding(rhs.object, ctx);
          // Find the object in the frame
          if (auto rhs_obj_opt = find_object_in_frame(*ctx.current_frame, rhs_obj_id);
              rhs_obj_opt.has_value()) {
            return rhs_obj_opt->get().object_class;
          } else {
            return std::nullopt; // Object not found in frame
          }
        }
      },
      e.rhs);
  if (!rhs_obj_class.has_value()) {
    return false; // RHS object not found
  }

  // Compare
  switch (e.op) {
    case stql::CompareOp::Equal: return lhs_obj_class == *rhs_obj_class;
    case stql::CompareOp::NotEqual: return lhs_obj_class != *rhs_obj_class;
    default: throw std::logic_error("Class comparison only supports EQ and NE");
  }
}

/**
 * @brief Evaluate probability comparison.
 *
 * **STQL**: `P(id) ∼ threshold`
 *
 * Extracts the object probability and compares it with the threshold.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_prob_compare(const stql::ProbCompareExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // Resolve the object ID
  std::string lhs_obj_id = get_object_binding(e.lhs.object, ctx);
  // Find the object in the frame
  auto lhs_obj_opt = find_object_in_frame(*ctx.current_frame, lhs_obj_id);
  if (!lhs_obj_opt.has_value()) {
    return false; // Object not found in frame
  }
  double lhs_prob = lhs_obj_opt->get().probability;

  // Resolve the comparison probability
  double rhs_prob = NAN;
  if (const auto* threshold = std::get_if<double>(&e.rhs)) {
    rhs_prob = *threshold;
  } else if (const auto* rhs = std::get_if<stql::ProbFunc>(&e.rhs)) {
    // Resolve the object ID
    std::string rhs_obj_id = get_object_binding(rhs->object, ctx);
    // Find the object in the frame
    auto rhs_obj_opt = find_object_in_frame(*ctx.current_frame, rhs_obj_id);
    if (!rhs_obj_opt.has_value()) {
      return false; // Object not found in frame
    }
    rhs_prob = rhs_obj_opt->get().probability;

  } else {
    return false; // Cannot resolve right-hand side
  }

  return compare_op(lhs_prob, e.op, rhs_prob);
}

/**
 * @brief Evaluate Euclidean distance comparison.
 *
 * **STQL**: `ED(id₁, crt₁, id₂, crt₂) ∼ threshold`
 *
 * Extracts reference points from two objects' bounding boxes and compares the distance.
 *
 * @note This function doesn't actually make sense for CRTs that aren't centroids. If
 * you think otherwise, contributions are welcome.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_dist_compare(const stql::DistCompareExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // if (!ctx.current_frame) { return false; }
  // TODO(anand): I am realizing this is ill defined when it comes to non-centroid reference
  // points... I am actually just going to disregard the CRTs altogether and just compare centroid
  // If people need something else, they should use Lat or Lon.

  // Resolve first object ID
  const auto& a = e.lhs.from;
  const auto& b = e.lhs.to;

  const std::string a_id = get_object_binding(a.object, ctx);
  const std::string b_id = get_object_binding(b.object, ctx);

  // Find both objects
  auto obj1_opt = find_object_in_frame(*ctx.current_frame, a_id);
  auto obj2_opt = find_object_in_frame(*ctx.current_frame, b_id);
  if (!obj1_opt.has_value() || !obj2_opt.has_value()) {
    return false; // One or both objects not found
  }

  const auto& bbox1 = obj1_opt->get().bbox;
  const auto& bbox2 = obj2_opt->get().bbox;

  // Get reference points
  auto ref_pt1 = stql::CoordRefPoint::Center;
  auto ref_pt2 = stql::CoordRefPoint::Center;

  double x1 = get_reference_point_value(bbox1, ref_pt1, true);
  double y1 = get_reference_point_value(bbox1, ref_pt1, false);
  double x2 = get_reference_point_value(bbox2, ref_pt2, true);
  double y2 = get_reference_point_value(bbox2, ref_pt2, false);

  // Compute Euclidean distance (squared)
  double dist_sq = ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  double compare = e.rhs * e.rhs;
  return compare_op(dist_sq, e.op, compare);
}

/**
 * @brief Evaluate lateral position comparison.
 *
 * **STQL**: `Lat(id, crt) ∼ threshold`
 * **STQL**: `Lon(id, crt) ∼ threshold`
 * **STQL**: `Lat(id, crt) ∼ Lat(id, crt)`
 * **STQL**: `Lon(id, crt) ∼ Lat(id, crt)`
 * **STQL**: `Lat(id, crt) ∼ Lon(id, crt)`
 * **STQL**: `Lon(id, crt) ∼ Lon(id, crt)`
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_latlon_compare(const stql::LatLonCompareExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // if (!ctx.current_frame) { return false; }

  const auto get_val = [&ctx](const auto& e) -> std::optional<double> {
    using T = std::decay_t<decltype(e)>;
    if constexpr (std::is_same_v<double, T>) {
      return e;
    } else {
      auto obj_id  = get_object_binding(e.point.object, ctx);
      auto obj_opt = find_object_in_frame(*ctx.current_frame, obj_id);
      if (!obj_opt.has_value()) { return std::nullopt; }
      auto& bbox             = obj_opt->get().bbox;
      constexpr bool lateral = std::is_same_v<stql::LatFunc, T>;
      auto crt               = e.point.crt;
      return get_reference_point_value(bbox, crt, lateral);
    }
  };

  std::optional<double> lhs = std::visit(get_val, e.lhs);
  std::optional<double> rhs = std::visit(get_val, e.rhs);
  if (!lhs.has_value() || !rhs.has_value()) { return false; }

  return compare_op(*lhs, e.op, *rhs);
}

// ============================================================================
// Evaluation Implementation: Spatial Operators
// ============================================================================

/**
 * @brief Extract a bounding box from an object.
 *
 * **STQL**: `BB(id)`
 *
 * Resolves the object ID and returns its bounding box as a spatial Region.
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_bbox(const stql::BBoxExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> spatial::Region {
  // if (!ctx.current_frame) { return spatial::Region{spatial::Empty{}}; }

  auto obj_id = get_object_binding(e.object, ctx);
  // Find object and return its bounding box
  auto obj_opt = find_object_in_frame(*ctx.current_frame, obj_id);
  if (!obj_opt.has_value()) { return spatial::Region{spatial::Empty{}}; }

  const auto& bbox = obj_opt->get().bbox;
  return spatial::Region{spatial::BBox{
      bbox.xmin,
      bbox.xmax,
      bbox.ymin,
      bbox.ymax,
      false, // open_left
      false, // open_right
      false, // open_bottom
      false  // open_top
  }};
}

/**
 * @brief Evaluate spatial union.
 *
 * **STQL**: `Ω₁ ⊔ Ω₂`
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_spatial_union(const stql::SpatialUnionExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> spatial::Region {
  // if (!ctx.current_frame) { return spatial::Region{spatial::Empty{}}; }

  spatial::Region ret = spatial::Empty{};
  for (const auto& arg : e.args) {
    auto arg_eval = eval_impl(arg, ctx);
    ret           = spatial::union_of(ret, arg_eval);
    // Short-circuit if we max out
    if (std::holds_alternative<spatial::Universe>(ret)) { return ret; }
  }
  return ret;
}

/**
 * @brief Evaluate spatial intersection.
 *
 * **STQL**: `Ω₁ ⊓ Ω₂`
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_spatial_intersect(
    const stql::SpatialIntersectExpr& e,
    const EvaluationContext<Iter, Sentinel>& ctx) -> spatial::Region {
  spatial::Region ret = spatial::Universe{};
  for (const auto& arg : e.args) {
    auto arg_eval = eval_impl(arg, ctx);
    ret           = spatial::intersect(ret, arg_eval);
    // Short-circuit if we empty out
    if (std::holds_alternative<spatial::Empty>(ret)) { return ret; }
  }
  return ret;
}

/**
 * @brief Evaluate spatial complement.
 *
 * **STQL**: `Ω̅`
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto eval_spatial_complement(
    const stql::SpatialComplementExpr& e,
    const EvaluationContext<Iter, Sentinel>& ctx) -> spatial::Region {
  auto arg_region = eval_impl(*e.arg, ctx);
  return spatial::complement(
      arg_region, spatial::from_datastream(ctx.current_frame->universe_bbox()));
}

/**
 * @brief Evaluate spatial area.
 *
 * **STQL**: `Area(Ω) ∼ threshold`
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_area_compare(const stql::AreaCompareExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // if (!ctx.current_frame) { return false; }

  auto lhs_region = eval_impl(*e.lhs.spatial_expr, ctx);
  lhs_region      = spatial::simplify(lhs_region);
  auto lhs_area   = spatial::area(lhs_region);

  auto rhs_area = std::visit(
      utils::overloaded{
          [](const double val) { return val; },
          [&ctx](const stql::AreaFunc& rhs) {
            auto rhs_region = eval_impl(*rhs.spatial_expr, ctx);
            rhs_region      = spatial::simplify(rhs_region);
            return spatial::area(rhs_region);
          }},
      e.rhs);

  return compare_op(lhs_area, e.op, rhs_area);
}

/**
 * @brief Evaluate spatial existence.
 *
 * **STQL**: `∃Ω` (spatial exists)
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_spatial_exists(const stql::SpatialExistsExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // if (!ctx.current_frame) { return false; }

  auto arg_region = eval_impl(*e.arg, ctx);
  arg_region      = spatial::simplify(arg_region);
  return (spatial::area(arg_region) > 0.0);
}

/**
 * @brief Evaluate spatial universal.
 *
 * **STQL**: `∀Ω` (spatial forall)
 */
template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel = Iter>
[[nodiscard]] auto
eval_spatial_forall(const stql::SpatialForallExpr& e, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  // if (!ctx.current_frame) { return false; }

  auto arg_region     = eval_impl(*e.arg, ctx);
  arg_region          = spatial::simplify(arg_region);
  const auto universe = spatial::from_datastream(ctx.current_frame->universe_bbox());

  return std::visit(
      utils::overloaded{
          [&universe](const spatial::BBox& box) { return box == universe; },
          [](const spatial::Universe&) { return true; },
          [](const auto&) {
            return false;
          }},
      arg_region);
}
// ============================================================================
// Spatial Recursive Evaluator
// ============================================================================

template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel>
[[nodiscard]] auto
eval_impl(const stql::SpatialExpr& expr, const EvaluationContext<Iter, Sentinel>& ctx)
    -> spatial::Region {
  using spatial::Region;
  struct EvalDispatcher {
    const EvaluationContext<Iter, Sentinel>* ctx;

    [[nodiscard]] auto operator()(const stql::EmptySetExpr& /*e*/) -> Region {
      return Region{spatial::Empty{}};
    }

    [[nodiscard]] auto operator()(const stql::UniverseSetExpr& /*e*/) -> Region {
      return Region{spatial::Universe{}};
    }

    [[nodiscard]] auto operator()(const stql::BBoxExpr& e) -> Region { return eval_bbox(e, *ctx); }

    [[nodiscard]] auto operator()(const stql::SpatialComplementExpr& e) -> Region {
      return eval_spatial_complement(e, *ctx);
    }

    [[nodiscard]] auto operator()(const stql::SpatialUnionExpr& e) -> Region {
      return eval_spatial_union(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::SpatialIntersectExpr& e) -> Region {
      return eval_spatial_intersect(e, *ctx);
    }
  };
  auto eval_visitor = EvalDispatcher{.ctx = &ctx};
  return std::visit(eval_visitor, expr);
}

// ============================================================================
// Main Recursive Evaluator
// ============================================================================

template <std::bidirectional_iterator Iter, std::sized_sentinel_for<Iter> Sentinel>
[[nodiscard]] auto eval_impl(const stql::Expr& expr, const EvaluationContext<Iter, Sentinel>& ctx)
    -> bool {
  struct EvalDispatcher {
    const EvaluationContext<Iter, Sentinel>* ctx;

    // Propositional operators
    [[nodiscard]] auto operator()(const stql::ConstExpr& e) -> bool { return eval_const(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::NotExpr& e) -> bool { return eval_not(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::AndExpr& e) -> bool { return eval_and(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::OrExpr& e) -> bool { return eval_or(e, *ctx); }
    // Temporal operators (future-time)
    [[nodiscard]] auto operator()(const stql::NextExpr& e) -> bool { return eval_next(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::AlwaysExpr& e) -> bool {
      return eval_always(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::EventuallyExpr& e) -> bool {
      return eval_eventually(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::UntilExpr& e) -> bool { return eval_until(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::ReleaseExpr& e) -> bool {
      return eval_release(e, *ctx);
    }
    // Temporal operators (past-time)
    [[nodiscard]] auto operator()(const stql::PreviousExpr& e) -> bool {
      return eval_previous(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::HoldsExpr& e) -> bool { return eval_holds(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::SometimesExpr& e) -> bool {
      return eval_sometimes(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::SinceExpr& e) -> bool { return eval_since(e, *ctx); }
    [[nodiscard]] auto operator()(const stql::BackToExpr& e) -> bool {
      return eval_backto(e, *ctx);
    }
    // Quantifiers
    [[nodiscard]] auto operator()(const stql::ExistsExpr& e) -> bool {
      return eval_exists(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::ForallExpr& e) -> bool {
      return eval_forall(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::FreezeExpr& e) -> bool {
      return eval_freeze(e, *ctx);
    }
    // Constraints
    [[nodiscard]] auto operator()(const stql::TimeBoundExpr& e) -> bool {
      return eval_time_bound(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::FrameBoundExpr& e) -> bool {
      return eval_frame_bound(e, *ctx);
    }
    // Perception operators
    [[nodiscard]] auto operator()(const stql::ObjectIdCompareExpr& e) -> bool {
      return eval_obj_id_compare(e, *ctx);
    }

    [[nodiscard]] auto operator()(const stql::ClassCompareExpr& e) -> bool {
      return eval_class_compare(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::ProbCompareExpr& e) -> bool {
      return eval_prob_compare(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::DistCompareExpr& e) -> bool {
      return eval_dist_compare(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::LatLonCompareExpr& e) -> bool {
      return eval_latlon_compare(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::AreaCompareExpr& e) -> bool {
      return eval_area_compare(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::SpatialExistsExpr& e) -> bool {
      return eval_spatial_exists(e, *ctx);
    }
    [[nodiscard]] auto operator()(const stql::SpatialForallExpr& e) -> bool {
      return eval_spatial_forall(e, *ctx);
    }
  };
  auto eval_visitor = EvalDispatcher{.ctx = &ctx};

  return std::visit(eval_visitor, expr);
}

} // anonymous namespace

// ============================================================================
// Public API Implementation
// ============================================================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto BooleanEvaluator::evaluate(
    const stql::Expr& formula,
    const datastream::Frame& current_frame,
    const std::deque<datastream::Frame>& history,
    const std::deque<datastream::Frame>& horizon) const -> bool {
  const auto cur_frame_queue = std::deque{current_frame};

  auto history_begin = ranges::cbegin(history);
  auto history_end   = ranges::cend(history);
  auto horizon_begin = ranges::cbegin(horizon);
  auto horizon_end   = ranges::cend(horizon);

  auto current_frame_it = ranges::cbegin(cur_frame_queue);

  using Iter     = decltype(history_begin);
  using Sentinel = decltype(history_end);
  static_assert(std::is_same_v<decltype(horizon_begin), Iter>);
  static_assert(std::is_same_v<decltype(horizon_end), Sentinel>);
  static_assert(std::is_same_v<decltype(current_frame_it), Iter>);
  static_assert(std::is_same_v<decltype(std::next(current_frame_it)), Sentinel>);

  EvaluationContext<Iter, Sentinel> ctx{
      .current_frame = current_frame_it,
      .history_begin = history_begin,
      .history_end   = history_end,
      .horizon_begin = horizon_begin,
      .horizon_end   = horizon_end,
      .frozen_times  = {},
      .frozen_frames = {},
      .bound_objects = {},
  };
  return eval_impl(formula, ctx);
}

} // namespace percemon::monitoring
