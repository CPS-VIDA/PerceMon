#pragma once

#ifndef __PERCEMON_AST_HPP__
#define __PERCEMON_AST_HPP__

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace percemon {
namespace ast {

namespace primitives {
// Some basic primitives

enum class ComparisonOp : std::uint8_t { GT, GE, LT, LE, EQ, NE };

/**
 * Node that holds a constant value, true or false.
 */
struct Const {
  bool value = false;

  Const(bool value_) : value{value_} {};

  inline bool operator==(const Const& other) const {
    return this->value == other.value;
  };

  inline bool operator!=(const Const& other) const { return !(*this == other); };
};

/**
 * Placeholder value for Current Time
 */
struct C_TIME {};
/**
 * Placeholder value for Current Frame
 */
struct C_FRAME {};

/**
 * Variable place holder for frames.
 */
struct Var_f {
  std::string name;

  Var_f(std::string name_) : name{std::move(name_)} {}

  inline bool operator==(const Var_f& other) const { return name == other.name; };

  inline bool operator!=(const Var_f& other) const { return !(*this == other); };
};

/**
 * Variable place holder for timestamps.
 */
struct Var_x {
  std::string name;

  Var_x(std::string name_) : name{std::move(name_)} {}

  inline bool operator==(const Var_x& other) const { return name == other.name; };

  inline bool operator!=(const Var_x& other) const { return !(*this == other); };
};

/**
 * Variable place holder for object IDs
 */
struct Var_id {
  std::string name;

  Var_id(std::string name_) : name{std::move(name_)} {}
};

// Spatial primitives.

struct EmptySet {};
struct UniverseSet {};

// Topological identifiers.
enum struct CRT : std::uint8_t { LM, RM, TM, BM, CT };

/**
 * A bound on a Var_x of the form
 *
 *    x - C_TIME ~ c
 *
 * where, `~` is a relational operator: `<`,`>`,`<=`,`>=`.
 */
struct TimeBound {
  Var_x x;
  ComparisonOp op = ComparisonOp::GE;
  double bound    = 0.0;

  TimeBound() = delete;
  TimeBound(
      Var_x x_,
      const ComparisonOp op_ = ComparisonOp::GE,
      const double bound_    = 0.0) :
      x{std::move(x_)}, op{op_}, bound{bound_} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "TimeBound (Var_x - C_TIME ~ c) cannot have == and != constraints.");
    }
    if (bound_ <= 0.0) {
      bound = -bound_;
      switch (op_) {
        case ComparisonOp::GE: op = ComparisonOp::LE; break;
        case ComparisonOp::GT: op = ComparisonOp::LT; break;
        case ComparisonOp::LE: op = ComparisonOp::GE; break;
        case ComparisonOp::LT: op = ComparisonOp::GT; break;
        default: {
          throw std::invalid_argument(
              "TimeBound (Var_x - C_TIME ~ c) cannot have == and != constraints.");
        }
      }
    }
  };

  inline bool operator==(const TimeBound& other) const {
    return (x == other.x) && (op == other.op) && (bound == other.bound);
  };

  inline bool operator!=(const TimeBound& other) const { return !(*this == other); };
};
TimeBound operator-(const Var_x& lhs, C_TIME);
TimeBound operator>(const TimeBound& lhs, const double bound);
TimeBound operator>=(const TimeBound& lhs, const double bound);
TimeBound operator<(const TimeBound& lhs, const double bound);
TimeBound operator<=(const TimeBound& lhs, const double bound);
TimeBound operator>(const double bound, const TimeBound& rhs);
TimeBound operator>=(const double bound, const TimeBound& rhs);
TimeBound operator<(const double bound, const TimeBound& rhs);
TimeBound operator<=(const double bound, const TimeBound& rhs);

/**
 * A bound on a Var_f of the form
 *
 *    f - C_FRAME ~ c
 *
 * where, `~` is a relational operator: `<`,`>`,`<=`,`>=`.
 */
struct FrameBound {
  Var_f f;
  ComparisonOp op = ComparisonOp::GE;
  size_t bound    = 0;

  FrameBound() = delete;
  FrameBound(
      Var_f f_,
      const ComparisonOp op_ = ComparisonOp::GE,
      const size_t bound_    = 0) :
      f{std::move(f_)}, op{op_}, bound{bound_} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "FrameBound (Var_f - C_FRAME ~ c) cannot have == and != constraints.");
    }
  };

  inline bool operator==(const FrameBound& other) const {
    return (f == other.f) && (op == other.op) && (bound == other.bound);
  };

  inline bool operator!=(const FrameBound& other) const { return !(*this == other); };
};
FrameBound operator-(const Var_f& lhs, C_FRAME);
FrameBound operator>(const FrameBound& lhs, const size_t bound);
FrameBound operator>=(const FrameBound& lhs, const size_t bound);
FrameBound operator<(const FrameBound& lhs, const size_t bound);
FrameBound operator<=(const FrameBound& lhs, const size_t bound);
FrameBound operator>(const size_t bound, const FrameBound& rhs);
FrameBound operator>=(const size_t bound, const FrameBound& rhs);
FrameBound operator<(const size_t bound, const FrameBound& rhs);
FrameBound operator<=(const size_t bound, const FrameBound& rhs);

using TemporalBoundExpr = std::variant<TimeBound, FrameBound>;

struct TimeInterval {
  double low, high;

  enum Bound : std::uint8_t { OPEN, LOPEN, ROPEN, CLOSED };
  Bound bound;

  static TimeInterval open(double low, double high) {
    return TimeInterval{low, high, OPEN};
  }
  static TimeInterval lopen(double low, double high) {
    return TimeInterval{low, high, LOPEN};
  }
  static TimeInterval ropen(double low, double high) {
    return TimeInterval{low, high, ROPEN};
  }
  static TimeInterval closed(double low, double high) {
    return TimeInterval{low, high, CLOSED};
  }
};

struct FrameInterval {
  size_t low, high;

  enum Bound : std::uint8_t { OPEN, LOPEN, ROPEN, CLOSED };
  Bound bound;

  static FrameInterval open(size_t low, size_t high) {
    return FrameInterval{low, high, OPEN};
  }
  static FrameInterval lopen(size_t low, size_t high) {
    return FrameInterval{low, high, LOPEN};
  }
  static FrameInterval ropen(size_t low, size_t high) {
    return FrameInterval{low, high, ROPEN};
  }
  static FrameInterval closed(size_t low, size_t high) {
    return FrameInterval{low, high, CLOSED};
  }
};

// Some primitive operations on IDs

/**
 * Node comparing objects.
 */
struct CompareId {
  Var_id lhs;
  ComparisonOp op;
  Var_id rhs;

  CompareId() = delete;
  CompareId(Var_id lhs_, ComparisonOp op_, Var_id rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op != ComparisonOp::EQ && op != ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators <, >, <=, >= to compare Var_id");
    }
  };
};
CompareId operator==(const Var_id& lhs, const Var_id& rhs);
CompareId operator!=(const Var_id& lhs, const Var_id& rhs);

/**
 * Node to represent the Class(id) function.
 */
struct Class {
  Var_id id;

  Class() = delete;
  Class(Var_id id_) : id{std::move(id_)} {}
};

/**
 * Node to compare equality of object class between either two IDs or an ID and a
 * class literal.
 */
struct CompareClass {
  Class lhs;
  ComparisonOp op;
  std::variant<int, Class> rhs;

  CompareClass() = delete;
  CompareClass(Class lhs_, ComparisonOp op_, std::variant<int, Class> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op != ComparisonOp::EQ && op != ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators <, >, <=, >= to compare Class(id)");
    }
  };
};
CompareClass operator==(const Class& lhs, const int rhs);
CompareClass operator==(const int lhs, const Class& rhs);
CompareClass operator==(const Class& lhs, const Class& rhs);
CompareClass operator!=(const Class& lhs, const int rhs);
CompareClass operator!=(const int lhs, const Class& rhs);
CompareClass operator!=(const Class& lhs, const Class& rhs);

/**
 * Node representing the Prob(id) function. Providing a multiplier is equivalent to
 * Prob(id) * scale.
 */
struct Prob {
  Var_id id;
  double scale = 1.0;

  Prob() = delete;
  Prob(Var_id id_, double scale_ = 1.0) : id{std::move(id_)}, scale{scale_} {}

  Prob& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend Prob operator*(const Prob& lhs, const double rhs) {
    return Prob{lhs.id, lhs.scale * rhs};
  }

  friend Prob operator*(const double lhs, const Prob& rhs) { return rhs * lhs; }
};

struct CompareProb {
  Prob lhs;
  ComparisonOp op;
  std::variant<double, Prob> rhs;

  CompareProb() = delete;
  CompareProb(Prob lhs_, ComparisonOp op_, std::variant<double, Prob> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare Prob(id)");
    }
  };
};
CompareProb operator>(const Prob& lhs, const double rhs);
CompareProb operator>=(const Prob& lhs, const double rhs);
CompareProb operator<(const Prob& lhs, const double rhs);
CompareProb operator<=(const Prob& lhs, const double rhs);
CompareProb operator>(const Prob& lhs, const Prob& rhs);
CompareProb operator>=(const Prob& lhs, const Prob& rhs);
CompareProb operator<(const Prob& lhs, const Prob& rhs);
CompareProb operator<=(const Prob& lhs, const Prob& rhs);

// Spatial primitive operations
struct BBox {
  Var_id id;
};

struct ED {
  Var_id id1;
  CRT crt1;
  Var_id id2;
  CRT crt2;
  double scale = 1.0;

  ED() = delete;
  ED(Var_id id1_, CRT crt1_, Var_id id2_, CRT crt2_, double scale_ = 1.0) :
      id1{std::move(id1_)},
      crt1{crt1_},
      id2{std::move(id2_)},
      crt2{crt2_},
      scale{scale_} {}

  ED& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend ED operator*(const ED& lhs, const double rhs) {
    return ED{lhs.id1, lhs.crt1, lhs.id2, lhs.crt2, lhs.scale * rhs};
  }

  friend ED operator*(const double lhs, const ED& rhs) { return rhs * lhs; }
};

struct CompareED {
  ED lhs;
  ComparisonOp op;
  double rhs;

  CompareED() = delete;
  CompareED(ED lhs_, ComparisonOp op_, double rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{rhs_} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare Euclidean Distance");
    }
  };
};

CompareED operator>(const ED& lhs, const double rhs);
CompareED operator>=(const ED& lhs, const double rhs);
CompareED operator<(const ED& lhs, const double rhs);
CompareED operator<=(const ED& lhs, const double rhs);
CompareED operator>(const double lhs, const ED& rhs);
CompareED operator>=(const double lhs, const ED& rhs);
CompareED operator<(const double lhs, const ED& rhs);
CompareED operator<=(const double lhs, const ED& rhs);
CompareED operator>(const ED& lhs, const ED& rhs);
CompareED operator>=(const ED& lhs, const ED& rhs);
CompareED operator<(const ED& lhs, const ED& rhs);
CompareED operator<=(const ED& lhs, const ED& rhs);

struct Lat {
  Var_id id;
  CRT crt;
  double scale = 1.0;

  Lat() = delete;
  Lat(Var_id id_, CRT crt_, double scale_ = 1.0) :
      id{std::move(id_)}, crt{crt_}, scale{scale_} {}

  Lat& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend Lat operator*(Lat lhs, const double rhs) {
    lhs *= rhs;
    return lhs;
  }

  friend Lat operator*(const double lhs, const Lat& rhs) { return rhs * lhs; }
};

struct Lon {
  Var_id id;
  CRT crt;
  double scale = 1.0;

  Lon() = delete;
  Lon(Var_id id_, CRT crt_, double scale_ = 1.0) :
      id{std::move(id_)}, crt{crt_}, scale{scale_} {}

  Lon& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend Lon operator*(Lon lhs, const double rhs) {
    lhs *= rhs;
    return lhs;
  }

  friend Lon operator*(const double lhs, const Lon& rhs) { return rhs * lhs; }
};

struct CompareLat {
  Lat lhs;
  ComparisonOp op;
  std::variant<double, Lat, Lon> rhs;

  CompareLat() = delete;
  CompareLat(Lat lhs_, ComparisonOp op_, std::variant<double, Lat, Lon> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare Lat(id)");
    }
  };
};

struct CompareLon {
  Lon lhs;
  ComparisonOp op;
  std::variant<double, Lat, Lon> rhs;

  CompareLon() = delete;
  CompareLon(Lon lhs_, ComparisonOp op_, std::variant<double, Lat, Lon> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare Lon(id)");
    }
  };
};

CompareLat operator>(const double lhs, const Lat& rhs);
CompareLat operator>=(const double lhs, const Lat& rhs);
CompareLat operator<(const double lhs, const Lat& rhs);
CompareLat operator<=(const double lhs, const Lat& rhs);
CompareLat operator>(const Lat& lhs, const double rhs);
CompareLat operator>=(const Lat& lhs, const double rhs);
CompareLat operator<(const Lat& lhs, const double rhs);
CompareLat operator<=(const Lat& lhs, const double rhs);
CompareLat operator>(const Lat& lhs, const Lat& rhs);
CompareLat operator>=(const Lat& lhs, const Lat& rhs);
CompareLat operator<(const Lat& lhs, const Lat& rhs);
CompareLat operator<=(const Lat& lhs, const Lat& rhs);
CompareLat operator>(const Lat& lhs, const Lon& rhs);
CompareLat operator>=(const Lat& lhs, const Lon& rhs);
CompareLat operator<(const Lat& lhs, const Lon& rhs);
CompareLat operator<=(const Lat& lhs, const Lon& rhs);

CompareLon operator>(const double lhs, const Lon& rhs);
CompareLon operator>=(const double lhs, const Lon& rhs);
CompareLon operator<(const double lhs, const Lon& rhs);
CompareLon operator<=(const double lhs, const Lon& rhs);
CompareLon operator>(const Lon& lhs, const double rhs);
CompareLon operator>=(const Lon& lhs, const double rhs);
CompareLon operator<(const Lon& lhs, const double rhs);
CompareLon operator<=(const Lon& lhs, const double rhs);
CompareLon operator>(const Lon& lhs, const Lon& rhs);
CompareLon operator>=(const Lon& lhs, const Lon& rhs);
CompareLon operator<(const Lon& lhs, const Lon& rhs);
CompareLon operator<=(const Lon& lhs, const Lon& rhs);
CompareLon operator>(const Lon& lhs, const Lat& rhs);
CompareLon operator>=(const Lon& lhs, const Lat& rhs);
CompareLon operator<(const Lon& lhs, const Lat& rhs);
CompareLon operator<=(const Lon& lhs, const Lat& rhs);

struct AreaOf {
  Var_id id;
  double scale = 1.0;

  AreaOf() = delete;
  AreaOf(Var_id id_, double scale_ = 1.0) : id{std::move(id_)}, scale{scale_} {}

  AreaOf& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend AreaOf operator*(const AreaOf& lhs, const double rhs) {
    return AreaOf{lhs.id, lhs.scale * rhs};
  }

  friend AreaOf operator*(const double lhs, const AreaOf& rhs) { return rhs * lhs; }
};

struct CompareArea {
  AreaOf lhs;
  ComparisonOp op;
  std::variant<double, AreaOf> rhs;

  CompareArea() = delete;
  CompareArea(AreaOf lhs_, ComparisonOp op_, std::variant<double, AreaOf> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare AreaOf(id)");
    }
  };
};
CompareArea operator>(const AreaOf& lhs, const double rhs);
CompareArea operator>=(const AreaOf& lhs, const double rhs);
CompareArea operator<(const AreaOf& lhs, const double rhs);
CompareArea operator<=(const AreaOf& lhs, const double rhs);
CompareArea operator>(const double lhs, const AreaOf& rhs);
CompareArea operator>=(const double lhs, const AreaOf& rhs);
CompareArea operator<(const double lhs, const AreaOf& rhs);
CompareArea operator<=(const double lhs, const AreaOf& rhs);
CompareArea operator>(const AreaOf& lhs, const AreaOf& rhs);
CompareArea operator>=(const AreaOf& lhs, const AreaOf& rhs);
CompareArea operator<(const AreaOf& lhs, const AreaOf& rhs);
CompareArea operator<=(const AreaOf& lhs, const AreaOf& rhs);
} // namespace primitives

// Forward declarations for std::variant

struct Exists;
using ExistsPtr = std::shared_ptr<Exists>;
struct Forall;
using ForallPtr = std::shared_ptr<Forall>;
struct Pin;
using PinPtr = std::shared_ptr<Pin>;
struct Not;
using NotPtr = std::shared_ptr<Not>;
struct And;
using AndPtr = std::shared_ptr<And>;
struct Or;
using OrPtr = std::shared_ptr<Or>;
struct Previous;
using PreviousPtr = std::shared_ptr<Previous>;
struct Always;
using AlwaysPtr = std::shared_ptr<Always>;
struct Sometimes;
using SometimesPtr = std::shared_ptr<Sometimes>;
struct Since;
using SincePtr = std::shared_ptr<Since>;
struct BackTo;
using BackToPtr = std::shared_ptr<BackTo>;

// Spatio Temporal subset
struct Complement;
using ComplementPtr = std::shared_ptr<Complement>;
struct Intersect;
using IntersectPtr = std::shared_ptr<Intersect>;
struct Union;
using UnionPtr = std::shared_ptr<Union>;
struct Interior;
using InteriorPtr = std::shared_ptr<Interior>;
struct Closure;
using ClosurePtr = std::shared_ptr<Closure>;

struct SpExists;
using SpExistsPtr = std::shared_ptr<SpExists>;
struct SpForall;
using SpForallPtr = std::shared_ptr<SpForall>;

struct SpPrevious;
using SpPreviousPtr = std::shared_ptr<SpPrevious>;
struct SpAlways;
using SpAlwaysPtr = std::shared_ptr<SpAlways>;
struct SpSometimes;
using SpSometimesPtr = std::shared_ptr<SpSometimes>;
struct SpSince;
using SpSincePtr = std::shared_ptr<SpSince>;
struct SpBackTo;
using SpBackToPtr = std::shared_ptr<SpBackTo>;

struct CompareSpArea;
using CompareSpAreaPtr = std::shared_ptr<CompareSpArea>;

using namespace primitives;

using Expr = std::variant<
    // Primitives.
    Const,
    // Functions on Primitives
    TimeBound,
    FrameBound,
    CompareId,
    CompareProb,
    CompareClass,
    CompareED,
    CompareLat,
    CompareLon,
    CompareArea,
    // Quantifiers
    ExistsPtr,
    ForallPtr,
    PinPtr,
    // Temporal Operators
    NotPtr,
    AndPtr,
    OrPtr,
    PreviousPtr,
    AlwaysPtr,
    SometimesPtr,
    SincePtr,
    BackToPtr,
    // Spatio-temporal operators
    CompareSpAreaPtr,
    SpExistsPtr,
    SpForallPtr>;

using SpatialExpr = std::variant<
    EmptySet,
    UniverseSet,
    BBox,
    ComplementPtr,
    IntersectPtr,
    UnionPtr,
    InteriorPtr,
    ClosurePtr,
    SpPreviousPtr,
    SpAlwaysPtr,
    SpSometimesPtr,
    SpSincePtr,
    SpBackToPtr>;

Expr operator~(const Expr& e);
Expr operator&(const Expr& lhs, const Expr& rhs);
Expr operator|(const Expr& lhs, const Expr& rhs);
Expr operator>>(const Expr& lhs, const Expr& rhs);

/**
 * Datastructure to pin frames
 */
struct Pin {
  std::optional<Var_x> x;
  std::optional<Var_f> f;

  Expr phi = Expr{Const{false}};

  Pin(const std::optional<Var_x>& x_, const std::optional<Var_f>& f_) : x{x_}, f{f_} {
    if (!x_.has_value() and !f_.has_value()) {
      throw std::invalid_argument(
          "Either time variable or frame variable must be specified when a frame is pinned.");
    }
  };

  Pin(const std::optional<Var_x>& x_) : Pin{x_, {}} {};
  Pin(const std::optional<Var_f>& f_) : Pin{{}, f_} {};

  Pin(const Pin&)            = default;
  Pin& operator=(const Pin&) = default;
  Pin(Pin&&)                 = default;
  Pin& operator=(Pin&&)      = default;

  PinPtr dot(const Expr& e) {
    auto ret = std::make_shared<Pin>(x, f);
    ret->phi = e;
    return ret;
  };
};

/**
 * Existential quantifier over IDs in either the current frame or a pinned frame.
 */
struct Exists {
  std::vector<Var_id> ids;
  std::optional<Pin> pinned_at = {};
  std::optional<Expr> phi      = {};

  Exists(std::vector<Var_id> id_list) : ids{std::move(id_list)}, pinned_at{}, phi{} {};

  ExistsPtr at(const Pin& pin) {
    auto ret       = std::make_shared<Exists>(ids);
    ret->pinned_at = pin;
    return ret;
  };

  ExistsPtr at(Pin&& pin) {
    auto ret       = std::make_shared<Exists>(ids);
    ret->pinned_at = std::move(pin);
    return ret;
  };

  ExistsPtr dot(const Expr& e) {
    auto ret = std::make_shared<Exists>(ids);
    if (auto& pin = this->pinned_at) {
      ret->pinned_at      = pin;
      ret->pinned_at->phi = e;
    } else {
      ret->phi = e;
    }
    return ret;
  };
};

/**
 * Universal quantifier over IDs in either the current frame or a pinned frame.
 */
struct Forall {
  std::vector<Var_id> ids;
  std::optional<Pin> pinned_at = {};
  std::optional<Expr> phi      = {};

  Forall(std::vector<Var_id> id_list) : ids{std::move(id_list)}, pinned_at{}, phi{} {};

  ForallPtr at(const Pin& pin) {
    auto ret       = std::make_shared<Forall>(ids);
    ret->pinned_at = pin;
    return ret;
  };

  ForallPtr at(Pin&& pin) {
    auto ret       = std::make_shared<Forall>(ids);
    ret->pinned_at = std::move(pin);
    return ret;
  };

  ForallPtr dot(const Expr& e) {
    auto ret = std::make_shared<Forall>(ids);
    if (auto& pin = this->pinned_at) {
      ret->pinned_at      = pin;
      ret->pinned_at->phi = e;
    } else {
      ret->phi = e;
    }
    return ret;
  };
};

struct Not {
  Expr arg;

  Not() = delete;
  Not(Expr arg_) : arg{std::move(arg_)} {};
};

struct And {
  std::vector<Expr> args;
  std::vector<TemporalBoundExpr> temporal_bound_args;

  And() = delete;
  And(const std::vector<Expr>& args_) {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an And operator with < 2 operands");
    }
    for (auto&& e : args_) {
      if (auto tb_ptr = std::get_if<TimeBound>(&e)) {
        temporal_bound_args.emplace_back(*tb_ptr);
      } else if (auto fb_ptr = std::get_if<FrameBound>(&e)) {
        temporal_bound_args.emplace_back(*fb_ptr);
      } else {
        args.push_back(e);
      }
    }
  };
};

struct Or {
  std::vector<Expr> args;
  std::vector<TemporalBoundExpr> temporal_bound_args;

  Or() = delete;
  Or(const std::vector<Expr>& args_) {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Or operator with < 2 operands");
    }
    for (auto&& e : args_) {
      if (auto tb_ptr = std::get_if<TimeBound>(&e)) {
        temporal_bound_args.emplace_back(*tb_ptr);
      } else if (auto fb_ptr = std::get_if<FrameBound>(&e)) {
        temporal_bound_args.emplace_back(*fb_ptr);
      } else {
        args.push_back(e);
      }
    }
  };
};

struct Previous {
  Expr arg;

  Previous() = delete;
  Previous(Expr arg_) : arg{std::move(arg_)} {};
};

struct Always {
  Expr arg;

  Always() = delete;
  Always(Expr arg_) : arg{std::move(arg_)} {};
};

struct Sometimes {
  Expr arg;

  Sometimes() = delete;
  Sometimes(Expr arg_) : arg{std::move(arg_)} {};
};

struct Since {
  std::pair<Expr, Expr> args;

  Since() = delete;
  Since(const Expr& arg0, const Expr& arg1) : args{std::make_pair(arg0, arg1)} {};
};

struct BackTo {
  std::pair<Expr, Expr> args;

  BackTo() = delete;
  BackTo(const Expr& arg0, const Expr& arg1) : args{std::make_pair(arg0, arg1)} {};
};

struct SpArea {
  SpatialExpr arg;
  double scale = 1.0;

  SpArea(SpatialExpr arg_, double scale_ = 1.0) : arg{std::move(arg_)}, scale{scale_} {}

  SpArea& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend SpArea operator*(const SpArea& lhs, const double rhs) {
    return SpArea{lhs.arg, lhs.scale * rhs};
  }
  friend SpArea operator*(const double lhs, const SpArea& rhs) { return rhs * lhs; }
};

struct CompareSpArea {
  SpArea lhs;
  ComparisonOp op;
  std::variant<double, SpArea> rhs;

  CompareSpArea() = delete;
  CompareSpArea(SpArea lhs_, ComparisonOp op_, std::variant<double, SpArea> rhs_) :
      lhs{std::move(lhs_)}, op{op_}, rhs{std::move(rhs_)} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to compare SpArea(id)");
    }
  };
};
CompareSpArea operator>(const SpArea& lhs, const double rhs);
CompareSpArea operator>=(const SpArea& lhs, const double rhs);
CompareSpArea operator<(const SpArea& lhs, const double rhs);
CompareSpArea operator<=(const SpArea& lhs, const double rhs);
CompareSpArea operator>(const double lhs, const SpArea& rhs);
CompareSpArea operator>=(const double lhs, const SpArea& rhs);
CompareSpArea operator<(const double lhs, const SpArea& rhs);
CompareSpArea operator<=(const double lhs, const SpArea& rhs);
CompareSpArea operator>(const SpArea& lhs, const SpArea& rhs);
CompareSpArea operator>=(const SpArea& lhs, const SpArea& rhs);
CompareSpArea operator<(const SpArea& lhs, const SpArea& rhs);
CompareSpArea operator<=(const SpArea& lhs, const SpArea& rhs);

struct Complement {
  SpatialExpr arg;
  Complement() = delete;
  Complement(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct Intersect {
  std::vector<SpatialExpr> args;
  Intersect() = delete;
  Intersect(const std::vector<SpatialExpr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Intersect operator with < 2 operands");
    }
  };
};

struct Union {
  std::vector<SpatialExpr> args;
  Union() = delete;
  Union(const std::vector<SpatialExpr>& args_) : args{args_} {
    if (args_.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Union operator with < 2 operands");
    }
  };
};

struct Interior {
  SpatialExpr arg;
  Interior() = delete;
  Interior(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct Closure {
  SpatialExpr arg;
  Closure() = delete;
  Closure(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct SpExists {
  SpatialExpr arg;
  SpExists() = delete;
  SpExists(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct SpForall {
  SpatialExpr arg;
  SpForall() = delete;
  SpForall(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct SpPrevious {
  SpatialExpr arg;

  SpPrevious() = delete;
  SpPrevious(SpatialExpr arg_) : arg{std::move(arg_)} {};
};

struct SpAlways {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpAlways() = delete;
  SpAlways(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpAlways(FrameInterval i, SpatialExpr arg_) : interval{i}, arg{std::move(arg_)} {};
};

struct SpSometimes {
  std::optional<FrameInterval> interval = {};
  SpatialExpr arg;

  SpSometimes() = delete;
  SpSometimes(SpatialExpr arg_) : arg{std::move(arg_)} {};
  SpSometimes(FrameInterval i, SpatialExpr arg_) : interval{i}, arg{std::move(arg_)} {};
};

struct SpSince {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpSince() = delete;
  SpSince(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpSince(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
};

struct SpBackTo {
  std::optional<FrameInterval> interval = {};
  std::pair<SpatialExpr, SpatialExpr> args;

  SpBackTo() = delete;
  SpBackTo(const SpatialExpr& arg0, const SpatialExpr& arg1) :
      args{std::make_pair(arg0, arg1)} {};
  SpBackTo(FrameInterval i, const SpatialExpr& arg0, const SpatialExpr& arg1) :
      interval{i}, args{std::make_pair(arg0, arg1)} {};
};

} // namespace ast

using ast::Expr;
using ast::Pin;
using namespace ast::primitives;

ast::PinPtr Pinned(Var_x x, Var_f f);
ast::PinPtr Pinned(Var_x x);
ast::PinPtr Pinned(Var_f f);

ast::ExistsPtr Exists(std::initializer_list<Var_id> id_list);
ast::ForallPtr Forall(std::initializer_list<Var_id> id_list);

ast::NotPtr Not(const Expr& arg);
ast::AndPtr And(const std::vector<Expr>& args);
ast::OrPtr Or(const std::vector<Expr>& args);
ast::PreviousPtr Previous(const Expr& arg);
ast::AlwaysPtr Always(const Expr& arg);
ast::SometimesPtr Sometimes(const Expr& arg);
ast::SincePtr Since(const Expr& a, const Expr& b);
ast::BackToPtr BackTo(const Expr& a, const Expr& b);

ast::AreaOf Area(Var_id id, double scale = 1.0);
ast::SpArea Area(const ast::SpatialExpr& expr, double scale = 1.0);

ast::SpExistsPtr SpExists(const ast::SpatialExpr&);
ast::SpForallPtr SpForall(const ast::SpatialExpr&);
ast::ComplementPtr Complement(const ast::SpatialExpr&);
ast::IntersectPtr Intersect(const std::vector<ast::SpatialExpr>&);
ast::UnionPtr Union(const std::vector<ast::SpatialExpr>&);
ast::InteriorPtr Interior(const ast::SpatialExpr&);
ast::ClosurePtr Closure(const ast::SpatialExpr&);

ast::SpPreviousPtr SpPrevious(const ast::SpatialExpr&);

ast::SpAlwaysPtr SpAlways(const ast::SpatialExpr&);
ast::SpAlwaysPtr SpAlways(const FrameInterval&, const ast::SpatialExpr&);

ast::SpSometimesPtr SpSometimes(const ast::SpatialExpr&);
ast::SpSometimesPtr SpSometimes(const FrameInterval&, const ast::SpatialExpr&);

ast::SpSincePtr SpSince(const ast::SpatialExpr&, const ast::SpatialExpr&);
ast::SpSincePtr
SpSince(const FrameInterval&, const ast::SpatialExpr&, const ast::SpatialExpr&);

ast::SpBackToPtr SpBackTo(const ast::SpatialExpr&, const ast::SpatialExpr&);
ast::SpBackToPtr
SpBackTo(const FrameInterval&, const ast::SpatialExpr&, const ast::SpatialExpr&);

} // namespace percemon

#endif /* end of include guard: __PERCEMON_AST_HPP__ */
