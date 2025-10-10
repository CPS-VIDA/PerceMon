#pragma once

#ifndef __PERCEMON_AST_HPP__
#define __PERCEMON_AST_HPP__

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#define NODE_DECL_AND_DEF_OVERRIDES                                                \
  [[nodiscard]] std::string to_string() const final;                               \
  [[nodiscard]] std::optional<std::int64_t> history(std::int64_t fps) const final; \
  [[nodiscard]] std::optional<std::int64_t> horizon(std::int64_t fps) const final;

namespace percemon::ast {

// Some forward declarations
struct BaseExpr;
using Expr = std::shared_ptr<BaseExpr>;
struct BaseSpatialExpr;
using SpatialExpr = std::shared_ptr<BaseSpatialExpr>;

template <typename Base>
struct AstVisitor {
  virtual ~AstVisitor() = default;

  virtual void visit(const std::shared_ptr<Base>&) = 0;
};

/**
 * Abstract Expression type
 */
struct BaseExpr : std::enable_shared_from_this<BaseExpr> {
  virtual ~BaseExpr() = default;

  Expr ptr() { return shared_from_this(); }
  std::shared_ptr<const BaseExpr> ptr() const { return shared_from_this(); }

  /* Utility method to easily downcast.
   * Useful when a child doesn't inherit directly from enable_shared_from_this
   * but wants to use the feature.
   */
  template <class Down>
  std::shared_ptr<Down> downcast_ptr() {
    return std::dynamic_pointer_cast<Down>(BaseExpr::shared_from_this());
  }

  /**
   * Convert the given expression to a string
   */
  [[nodiscard]] virtual std::string to_string() const = 0;

  /**
   * Compute the horizon requirment for the expression
   */
  [[nodiscard]] virtual std::optional<std::int64_t> horizon(std::int64_t fps) const = 0;
  /**
   * Compute the history requirment for the expression
   */
  [[nodiscard]] virtual std::optional<std::int64_t> history(std::int64_t fps) const = 0;

  Expr operator~();
};
Expr operator&(const Expr& lhs, const Expr& rhs);
Expr operator|(const Expr& lhs, const Expr& rhs);

struct BaseSpatialExpr : std::enable_shared_from_this<BaseSpatialExpr> {
  virtual ~BaseSpatialExpr() = default;

  SpatialExpr ptr() { return shared_from_this(); }

  /* Utility method to easily downcast.
   * Useful when a child doesn't inherit directly from enable_shared_from_this
   * but wants to use the feature.
   */
  template <class Down>
  std::shared_ptr<Down> downcast_ptr() {
    return std::dynamic_pointer_cast<Down>(BaseSpatialExpr::shared_from_this());
  }

  /**
   * Convert the given expression to a string
   */
  [[nodiscard]] virtual std::string to_string() const = 0;

  /**
   * Compute the horizon requirment for the expression
   */
  [[nodiscard]] virtual std::optional<std::int64_t> horizon(std::int64_t fps) const = 0;
  /**
   * Compute the history requirment for the expression
   */
  [[nodiscard]] virtual std::optional<std::int64_t> history(std::int64_t fps) const = 0;

  SpatialExpr operator~();
};
SpatialExpr operator&(const SpatialExpr& lhs, const SpatialExpr& rhs);
SpatialExpr operator|(const SpatialExpr& lhs, const SpatialExpr& rhs);

// Some basic primitives
// These are needed for mixins/CRTP templates defined later.

enum class ComparisonOp : std::uint8_t { GT, GE, LT, LE, EQ, NE };

/**
 * Allows one to flip the comparison operation such that the lhs and rhs can be
 * swapped.
 *
 * NOTE: This is not a negation, it just changes the direction of the comparison.
 */
inline ComparisonOp flip_comparison(ComparisonOp op) {
  switch (op) {
    case ComparisonOp::GT: return ComparisonOp::LT;
    case ComparisonOp::GE: return ComparisonOp::LE;
    case ComparisonOp::LT: return ComparisonOp::GT;
    case ComparisonOp::LE: return ComparisonOp::GE;
    default: return op;
  }
}

/**
 * Return the comparison if the expression is negated.
 */
inline ComparisonOp negate_comparison(ComparisonOp op) {
  switch (op) {
    case ComparisonOp::GT: return ComparisonOp::LE;
    case ComparisonOp::GE: return ComparisonOp::LT;
    case ComparisonOp::LT: return ComparisonOp::GE;
    case ComparisonOp::LE: return ComparisonOp::GT;
    case ComparisonOp::NE: return ComparisonOp::EQ;
    case ComparisonOp::EQ: return ComparisonOp::NE;
    default: return op;
  }
}

inline auto format_as(ComparisonOp op) {
  switch (op) {
    case ComparisonOp::GE: return ">=";
    case ComparisonOp::GT: return ">";
    case ComparisonOp::LE: return "<=";
    case ComparisonOp::LT: return "<";
    case ComparisonOp::EQ: return "==";
    case ComparisonOp::NE: return "!=";
    default: std::abort();
  }
}

/**
 * Abstract variable node
 */
struct VarNode {
  std::string name;

  VarNode()                          = delete;
  VarNode(const VarNode&)            = default;
  VarNode(VarNode&&)                 = delete;
  VarNode& operator=(const VarNode&) = default;
  VarNode& operator=(VarNode&&)      = delete;
  VarNode(std::string name_) : name{std::move(name_)} {}

  virtual ~VarNode() = default;
  [[nodiscard]] virtual std::string to_string() const { return this->name; }
};

inline bool operator==(const VarNode& a, const VarNode& b) { return a.name == b.name; }
inline bool operator!=(const VarNode& a, const VarNode& b) { return a.name != b.name; }

/**
 * Variable place holder for frames.
 */
struct Var_f final : public virtual VarNode {
  using VarNode::VarNode;
  static Var_f current() { return Var_f{"C_FRAME"}; }
};
const auto C_FRAME = Var_f::current();

/**
 * Variable place holder for timestamps.
 */
struct Var_x final : public virtual VarNode {
  using VarNode::VarNode;
  static Var_x current() { return Var_x{"C_TIME"}; }
};
const auto C_TIME = Var_x::current();

/**
 * Variable place holder for object IDs
 */
struct Var_id final : public virtual VarNode {
  using VarNode::VarNode;
};

/**
 * Topological identifiers.
 */
enum struct CRT : std::uint8_t { LM, RM, TM, BM, CT };

inline auto format_as(CRT ref_point) {
  switch (ref_point) {
    case CRT::LM: return "Left";
    case CRT::RM: return "Right";
    case CRT::TM: return "Top";
    case CRT::BM: return "Bottom";
    case CRT::CT: return "Center";
    default: std::abort();
  }
}

enum struct Bound : std::uint8_t { OPEN, LOPEN, ROPEN, CLOSED };

template <typename T>
struct Interval {
  T low, high;
  Bound bound;
  static Interval open(T low, T high) { return Interval{low, high, Bound::OPEN}; }
  static Interval lopen(T low, T high) { return Interval{low, high, Bound::LOPEN}; }
  static Interval ropen(T low, T high) { return Interval{low, high, Bound::ROPEN}; }
  static Interval closed(T low, T high) { return Interval{low, high, Bound::CLOSED}; }

  [[nodiscard]] std::string to_string() const;
};

using TimeInterval  = Interval<double>;
using FrameInterval = Interval<std::int64_t>;

template <typename... Args>
struct FuncNode {
  static_assert(sizeof...(Args) > 0, "can't have a 0-ary function");

  static constexpr std::size_t arity = sizeof...(Args);

  std::tuple<Args...> args;

  FuncNode(const Args&... args_) : args{std::make_tuple(args_...)} {};
  FuncNode(const FuncNode&)            = default;
  FuncNode& operator=(const FuncNode&) = default;
  // FuncNode(FuncNode&&)                 = default;
  // FuncNode& operator=(FuncNode&&)      = default;
  virtual ~FuncNode() = default;

  [[nodiscard]] virtual std::string get_name() const { return "UnknownFunc"; };
  [[nodiscard]] virtual std::string to_string() const;
  [[nodiscard]] std::optional<std::int64_t> history(std::int64_t fps) const;
  [[nodiscard]] std::optional<std::int64_t> horizon(std::int64_t fps) const;
};

// template <typename... Args>
// struct ScaledFuncNode : public virtual FuncNode<Args...> {
//   ScaledFuncNode(const Args&... args, double scale_ = 1.0) :
//       FuncNode<Args...>(args...), scale{scale_} {}

//   ScaledFuncNode(const ScaledFuncNode&)            = default;
//   ScaledFuncNode& operator=(const ScaledFuncNode&) = default;
//   // ScaledFuncNode(ScaledFuncNode&&)                 = default;
//   // ScaledFuncNode& operator=(ScaledFuncNode&&)      = default;

//   double scale = 1.0;
// };

/**
 * Node to represent the Class(id) function.
 */
struct Class final : public virtual FuncNode<Var_id> {
  using FuncNode<Var_id>::FuncNode;
  using FuncNode<Var_id>::operator=;

  [[nodiscard]] std::string get_name() const final { return "Class"; }
  [[nodiscard]] std::string to_string() const final;
};

/**
 * Node representing the Prob(id) function. Providing a multiplier is equivalent to
 * Prob(id) * scale.
 */
struct Prob final : public virtual FuncNode<Var_id> {
  using FuncNode<Var_id>::FuncNode;
  using FuncNode<Var_id>::operator=;
  [[nodiscard]] std::string get_name() const final { return "Prob"; }
  [[nodiscard]] std::string to_string() const final;
};

/**
 * Set the reference point/anchor of the object
 */
struct RefPoint {
  Var_id id;
  CRT crt;

  RefPoint() = delete;
  RefPoint(Var_id id, CRT crt) : id(std::move(id)), crt(crt) {}
};

/**
 * Compute the Euclidean distance between two objects given their reference points
 */
struct ED final : public virtual FuncNode<RefPoint, RefPoint> {
  using FuncNode<RefPoint, RefPoint>::FuncNode;
  using FuncNode<RefPoint, RefPoint>::operator=;
  [[nodiscard]] std::string get_name() const final { return "ED"; }
  [[nodiscard]] std::string to_string() const final;
};

/**
 * Access the (scaled) lateral position of the reference point.
 */
struct Lat final : public virtual FuncNode<RefPoint> {
  using FuncNode<RefPoint>::FuncNode;
  using FuncNode<RefPoint>::operator=;
  [[nodiscard]] std::string get_name() const final { return "Lat"; }
  [[nodiscard]] std::string to_string() const final;
};

/**
 * Access the (scaled) longitudinal position of the reference point.
 */
struct Lon final : public virtual FuncNode<RefPoint> {
  using FuncNode<RefPoint>::FuncNode;
  using FuncNode<RefPoint>::operator=;
  [[nodiscard]] std::string get_name() const final { return "Lon"; }
  [[nodiscard]] std::string to_string() const final;
};

struct AreaOf final : public virtual FuncNode<SpatialExpr> {
  using FuncNode<SpatialExpr>::FuncNode;
  using FuncNode<SpatialExpr>::operator=;
  [[nodiscard]] std::string get_name() const final { return "AreaOf"; }
  [[nodiscard]] std::string to_string() const final;
};

/**
 * Node that holds a constant value, true or false.
 */
struct Const final : public virtual BaseExpr {
  bool value = false;

  Const(bool value_) : value{value_} {};

  NODE_DECL_AND_DEF_OVERRIDES
};
const auto CONST_TRUE  = std::make_shared<Const>(true);
const auto CONST_FALSE = std::make_shared<Const>(false);

struct EmptySet final : public virtual BaseSpatialExpr {
  NODE_DECL_AND_DEF_OVERRIDES
};
struct UniverseSet final : public virtual BaseSpatialExpr {
  NODE_DECL_AND_DEF_OVERRIDES
};

template <typename T>
struct Difference {
  T lhs;
  T rhs;

  Difference() = delete;
  Difference(T lhs_, T rhs_) : lhs{std::move(lhs_)}, rhs{std::move(rhs_)} {}

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] std::optional<std::int64_t> history(std::int64_t fps) const;
  [[nodiscard]] std::optional<std::int64_t> horizon(std::int64_t fps) const;
};
template <typename T>
inline Difference<T> operator-(const T& lhs, const T& rhs) {
  return Difference<T>{lhs, rhs};
}

using TimeDiff  = Difference<Var_x>;
using FrameDiff = Difference<Var_f>;

template <typename Lhs, typename Rhs, ComparisonOp... Ops>
struct CompareNode : public virtual BaseExpr {
  Lhs lhs;
  ComparisonOp op;
  Rhs rhs;

  CompareNode()                              = delete;
  CompareNode(const CompareNode&)            = default;
  CompareNode(CompareNode&&)                 = default;
  CompareNode& operator=(const CompareNode&) = default;
  CompareNode(const Lhs& lhs, ComparisonOp op, const Rhs& rhs) :
      lhs{lhs}, op{op}, rhs{rhs} {
    if (!((op == Ops) || ...)) { throw std::invalid_argument("unsupported operation"); }
  }

  [[nodiscard]] std::string to_string() const override;
  [[nodiscard]] std::optional<std::int64_t> history(std::int64_t fps) const override;
  [[nodiscard]] std::optional<std::int64_t> horizon(std::int64_t fps) const override;
};

template <typename Lhs, typename Rhs>
struct OrderingOpNode final : public virtual CompareNode<
                                  Lhs,
                                  Rhs,
                                  ComparisonOp::LT,
                                  ComparisonOp::LE,
                                  ComparisonOp::GT,
                                  ComparisonOp::GE> {
  using CompareNode<
      Lhs,
      Rhs,
      ComparisonOp::LT,
      ComparisonOp::LE,
      ComparisonOp::GT,
      ComparisonOp::GE>::CompareNode;

  using CompareNode<
      Lhs,
      Rhs,
      ComparisonOp::LT,
      ComparisonOp::LE,
      ComparisonOp::GT,
      ComparisonOp::GE>::operator=;
};
template <typename Lhs, typename Rhs>
inline OrderingOpNode<Lhs, Rhs> less_than(const Lhs& lhs, const Rhs& rhs) {
  return OrderingOpNode<Lhs, Rhs>{lhs, ComparisonOp::LT, rhs};
}
template <typename Lhs, typename Rhs>
inline OrderingOpNode<Lhs, Rhs> less_than_eq(const Lhs& lhs, const Rhs& rhs) {
  return OrderingOpNode<Lhs, Rhs>{lhs, ComparisonOp::LE, rhs};
}
template <typename Lhs, typename Rhs>
inline OrderingOpNode<Lhs, Rhs> greater_than(const Lhs& lhs, const Rhs& rhs) {
  return OrderingOpNode<Lhs, Rhs>(lhs, ComparisonOp::GT, rhs);
}
template <typename Lhs, typename Rhs>
inline OrderingOpNode<Lhs, Rhs> greater_than_eq(const Lhs& lhs, const Rhs& rhs) {
  return OrderingOpNode<Lhs, Rhs>{lhs, ComparisonOp::GE, rhs};
}

// template <typename Lhs, typename Rhs> OrderingOpNode<Lhs, Rhs> operator<(const Lhs&
// lhs, const Rhs& rhs) {return OrderingOpNode{lhs, ComparisonOp::LT, rhs};} template
// <typename Lhs, typename Rhs> OrderingOpNode<Lhs, Rhs> operator<=(const Lhs& lhs,
// const Rhs& rhs) {return OrderingOpNode{lhs, ComparisonOp::LE, rhs};} template
// <typename Lhs, typename Rhs> OrderingOpNode<Lhs, Rhs> operator>(const Lhs& lhs, const
// Rhs& rhs) {return OrderingOpNode{lhs, ComparisonOp::GT, rhs};} template <typename
// Lhs, typename Rhs> OrderingOpNode<Lhs, Rhs> operator>=(const Lhs& lhs, const Rhs&
// rhs) {return OrderingOpNode{lhs, ComparisonOp::GE, rhs};}

template <typename Lhs, typename Rhs>
struct EqualityOpNode final
    : public virtual CompareNode<Lhs, Rhs, ComparisonOp::NE, ComparisonOp::EQ> {
  using CompareNode<Lhs, Rhs, ComparisonOp::NE, ComparisonOp::EQ>::CompareNode;
  using CompareNode<Lhs, Rhs, ComparisonOp::NE, ComparisonOp::EQ>::operator=;
};
template <typename Lhs, typename Rhs>
inline EqualityOpNode<Lhs, Rhs> not_equal(const Lhs& lhs, const Rhs& rhs) {
  return EqualityOpNode<Lhs, Rhs>{lhs, ComparisonOp::NE, rhs};
}
template <typename Lhs, typename Rhs>
inline EqualityOpNode<Lhs, Rhs> equal(const Lhs& lhs, const Rhs& rhs) {
  return EqualityOpNode<Lhs, Rhs>{lhs, ComparisonOp::EQ, rhs};
}

// template <typename Lhs, typename Rhs> EqualityOpNode<Lhs, Rhs> operator!=(const Lhs&
// lhs, const Rhs& rhs) {return EqualityOpNode{lhs, ComparisonOp::NE, rhs};} template
// <typename Lhs, typename Rhs> EqualityOpNode<Lhs, Rhs> operator==(const Lhs& lhs,
// const Rhs& rhs) {return EqualityOpNode{lhs, ComparisonOp::EQ, rhs};}

/**
 * A bound on time frozen variables
 *
 *    x - y ~ c
 *
 * where, `~` is a relational operator: `<`,`>`,`<=`,`>=`.
 */
using TimeBound = OrderingOpNode<TimeDiff, double>;

/**
 * A bound on frame frozen variables of the form
 *
 *    f1 - f2 ~ c
 *
 * where, `~` is a relational operator: `<`,`>`,`<=`,`>=`.
 */
using FrameBound = OrderingOpNode<FrameDiff, std::int64_t>;

/**
 * Node comparing objects.
 */
using CompareId = EqualityOpNode<Var_id, Var_id>;

using MaybeClass = std::variant<int, Class>;
/**
 * Node to compare equality of object class between either two IDs or an ID and a
 * class literal.
 */
using CompareClass = EqualityOpNode<Class, MaybeClass>;

using MaybeProb = std::variant<Prob, double>;
/**
 * Node to compare the confidence associated with a given object to either another
 * confidence variable, or a constant double.
 */
using CompareProb = OrderingOpNode<Prob, MaybeProb>;

using CompareED = OrderingOpNode<ED, double>;

using MaybeLatLon = std::variant<double, Lat, Lon>;
using CompareLat  = OrderingOpNode<Lat, MaybeLatLon>;
using CompareLon  = OrderingOpNode<Lon, MaybeLatLon>;

/**
 * Existential quantifier over object IDs
 */
struct Exists final : public virtual BaseExpr {
  std::vector<Var_id> ids;
  Expr arg;

  Exists(std::vector<Var_id> id_list, Expr subexpr) :
      ids{std::move(id_list)}, arg{std::move(subexpr)} {};

  NODE_DECL_AND_DEF_OVERRIDES
};

/**
 * Universal quantifier over IDs in either the current frame or a pinned frame.
 */
struct Forall final : public virtual BaseExpr {
  std::vector<Var_id> ids;
  Expr arg;

  Forall(std::vector<Var_id> id_list, Expr subexpr) :
      ids{std::move(id_list)}, arg{std::move(subexpr)} {};

  NODE_DECL_AND_DEF_OVERRIDES
};

/**
 * Datastructure to pin frames
 */
struct Pin final : public virtual BaseExpr {
  std::optional<Var_x> x;
  std::optional<Var_f> f;

  Expr arg;

  Pin(const std::optional<Var_x>& x_, const std::optional<Var_f>& f_, Expr subexpr) :
      x{x_}, f{f_}, arg{std::move(subexpr)} {
    if (!x_.has_value() and !f_.has_value()) {
      throw std::invalid_argument(
          "Either time variable or frame variable must be specified when a frame is pinned.");
    }
  };

  Pin(const std::optional<Var_x>& x_, Expr subexpr) :
      Pin{x_, {}, std::move(subexpr)} {};
  Pin(const std::optional<Var_f>& f_, Expr subexpr) :
      Pin{{}, f_, std::move(subexpr)} {};

  Pin(const Pin&)            = default;
  Pin& operator=(const Pin&) = default;
  Pin(Pin&&)                 = default;
  Pin& operator=(Pin&&)      = default;

  NODE_DECL_AND_DEF_OVERRIDES
};

struct Not final : public virtual BaseExpr {
  Expr arg;

  Not() = delete;
  Not(Expr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct And final : public virtual BaseExpr {
  std::vector<Expr> args;

  And() = delete;
  And(const std::vector<Expr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an And operator with < 2 operands");
    }
  };
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Or final : public virtual BaseExpr {
  std::vector<Expr> args;

  Or() = delete;
  Or(const std::vector<Expr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Or operator with < 2 operands");
    }
  };
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Next final : public virtual BaseExpr {
  Expr arg;
  size_t steps = 1;

  Next() = delete;
  Next(Expr arg, size_t steps = 0) : arg{std::move(arg)}, steps{steps} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Previous final : public virtual BaseExpr {
  Expr arg;
  size_t steps = 1;

  Previous() = delete;
  Previous(Expr arg, size_t steps = 0) : arg{std::move(arg)}, steps{steps} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Always final : public virtual BaseExpr {
  Expr arg;

  Always() = delete;
  Always(Expr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Holds final : public virtual BaseExpr {
  Expr arg;

  Holds() = delete;
  Holds(Expr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};
struct Eventually final : public virtual BaseExpr {
  Expr arg;

  Eventually() = delete;
  Eventually(Expr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Sometimes final : public virtual BaseExpr {
  Expr arg;

  Sometimes() = delete;
  Sometimes(Expr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Until final : public virtual BaseExpr {
  std::pair<Expr, Expr> args;

  Until() = delete;
  Until(const Expr& arg0, const Expr& arg1) : args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Since final : public virtual BaseExpr {
  std::pair<Expr, Expr> args;

  Since() = delete;
  Since(const Expr& arg0, const Expr& arg1) : args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Release final : public virtual BaseExpr {
  std::pair<Expr, Expr> args;

  Release() = delete;
  Release(const Expr& arg0, const Expr& arg1) : args{arg0, arg1} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct BackTo final : public virtual BaseExpr {
  std::pair<Expr, Expr> args;

  BackTo() = delete;
  BackTo(const Expr& arg0, const Expr& arg1) : args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

using MaybeAreaOf = std::variant<double, AreaOf>;
using CompareArea = OrderingOpNode<AreaOf, MaybeAreaOf>;

struct SpExists final : public virtual BaseExpr {
  SpatialExpr arg;
  SpExists() = delete;
  SpExists(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpForall final : public virtual BaseExpr {
  SpatialExpr arg;
  SpForall() = delete;
  SpForall(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct BBox final : public virtual BaseSpatialExpr {
  Var_id id;
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Complement final : public virtual BaseSpatialExpr {
  SpatialExpr arg;
  Complement() = delete;
  Complement(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Intersect final : public virtual BaseSpatialExpr {
  std::vector<SpatialExpr> args;
  Intersect() = delete;
  Intersect(const std::vector<SpatialExpr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Intersect operator with < 2 operands");
    }
  };
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Union final : public virtual BaseSpatialExpr {
  std::vector<SpatialExpr> args;
  Union() = delete;
  Union(const std::vector<SpatialExpr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Union operator with < 2 operands");
    }
  };
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Interior final : public virtual BaseSpatialExpr {
  SpatialExpr arg;
  Interior() = delete;
  Interior(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct Closure final : public virtual BaseSpatialExpr {
  SpatialExpr arg;
  Closure() = delete;
  Closure(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpNext final : public virtual BaseSpatialExpr {
  SpatialExpr arg;

  SpNext() = delete;
  SpNext(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpPrevious final : public virtual BaseSpatialExpr {
  SpatialExpr arg;

  SpPrevious() = delete;
  SpPrevious(SpatialExpr arg_) : arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpAlways final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpAlways() = delete;
  SpAlways(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpAlways(FrameInterval i, SpatialExpr arg_) : interval{i}, arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpHolds final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpHolds() = delete;
  SpHolds(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpHolds(FrameInterval i, SpatialExpr arg_) : interval{i}, arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpEventually final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpEventually() = delete;
  SpEventually(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpEventually(FrameInterval i, SpatialExpr arg_) :
      interval{i}, arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpSometimes final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpSometimes() = delete;
  SpSometimes(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpSometimes(FrameInterval i, SpatialExpr arg_) : interval{i}, arg{std::move(arg_)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpUntil final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpUntil() = delete;
  SpUntil(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpUntil(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpSince final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpSince() = delete;
  SpSince(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpSince(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpRelease final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpRelease() = delete;
  SpRelease(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpRelease(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

struct SpBackTo final : public virtual BaseSpatialExpr {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpBackTo() = delete;
  SpBackTo(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpBackTo(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
  NODE_DECL_AND_DEF_OVERRIDES
};

// Some convenience aliases
using PastNext       = Previous;
using PastAlways     = Holds;
using PastEventually = Sometimes;
using PastUntil      = Since;
using PastRelease    = BackTo;

using PastSpNext       = SpPrevious;
using PastSpAlways     = SpHolds;
using PastSpEventually = SpSometimes;
using PastSpUntil      = SpSince;
using PastSpRelease    = SpBackTo;

} // namespace percemon::ast

#undef NODE_DECL_AND_DEF_OVERRIDES

#endif /* end of include guard: __PERCEMON_AST_HPP__ */
