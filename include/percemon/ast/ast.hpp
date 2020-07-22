#pragma once

#ifndef __PERCEMON_AST_AST_HPP__
#define __PERCEMON_AST_AST_HPP__

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <exception>

#include "percemon/ast/primitives.hpp"

namespace percemon {
namespace ast {

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
      const Var_x& x_,
      const ComparisonOp op_ = ComparisonOp::GE,
      const double bound_    = 0.0) :
      x{x_}, op{op_}, bound{bound_} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to create Time Bounds");
    }
  };

  inline bool operator==(const TimeBound& other) const {
    return (x == other.x) && (op == other.op) && (bound == other.bound);
  };

  inline bool operator!=(const TimeBound& other) const {
    return !(*this == other);
  };
};
TimeBound operator-(const Var_x& lhs, C_TIME);
TimeBound operator>(const TimeBound& lhs, const double bound);
TimeBound operator>=(const TimeBound& lhs, const double bound);
TimeBound operator<(const TimeBound& lhs, const double bound);
TimeBound operator<=(const TimeBound& lhs, const double bound);

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
  double bound    = 0.0;

  FrameBound() = delete;
  FrameBound(
      const Var_f& f_,
      const ComparisonOp op_ = ComparisonOp::GE,
      const double bound_    = 0.0) :
      f{f_}, op{op_}, bound{bound_} {
    if (op == ComparisonOp::EQ || op == ComparisonOp::NE) {
      throw std::invalid_argument(
          "Cannot use relational operators ==, != to create Frame Bounds");
    }
  };

  inline bool operator==(const FrameBound& other) const {
    return (f == other.f) && (op == other.op) && (bound == other.bound);
  };

  inline bool operator!=(const FrameBound& other) const {
    return !(*this == other);
  };
};
FrameBound operator-(const Var_f& lhs, C_FRAME);
FrameBound operator>(const FrameBound& lhs, const double bound);
FrameBound operator>=(const FrameBound& lhs, const double bound);
FrameBound operator<(const FrameBound& lhs, const double bound);
FrameBound operator<=(const FrameBound& lhs, const double bound);

/**
 * Node comparing objects.
 */
struct CompareId {
  Var_id lhs;
  ComparisonOp op;
  Var_id rhs;

  CompareId() = delete;
  CompareId(Var_id lhs_, ComparisonOp op_, Var_id rhs_) :
      lhs{lhs_}, op{op_}, rhs{rhs_} {
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
  Class(Var_id id_) : id{id_} {}
};

/**
 * Node to compare equality of object class between either two IDs or an ID and a class
 * literal.
 */
struct CompareClass {
  Class lhs;
  ComparisonOp op;
  std::variant<int, Class> rhs;

  CompareClass() = delete;
  CompareClass(Class lhs_, ComparisonOp op_, std::variant<int, Class> rhs_) :
      lhs{lhs_}, op{op_}, rhs{rhs_} {
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
  Prob(Var_id id_, double scale_ = 1.0) : id{id_}, scale{scale_} {}

  Prob& operator*=(const double rhs) {
    this->scale *= rhs;
    return *this;
  };
  friend Prob operator*(Prob lhs, const double rhs) {
    return Prob{lhs.id, lhs.scale * rhs};
  }

  friend Prob operator*(const double lhs, const Prob& rhs) {
    return rhs * lhs;
  }
};

struct CompareProb {
  Prob lhs;
  ComparisonOp op;
  std::variant<double, Prob> rhs;

  CompareProb() = delete;
  CompareProb(Prob lhs_, ComparisonOp op_, std::variant<double, Prob> rhs_) :
      lhs{lhs_}, op{op_}, rhs{rhs_} {
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

using Expr = std::variant<
    Const,
    TimeBound,
    FrameBound,
    CompareId,
    CompareClass,
    ExistsPtr,
    AlwaysPtr,
    PinPtr,
    NotPtr,
    AndPtr,
    OrPtr,
    PreviousPtr,
    AlwaysPtr,
    SometimesPtr,
    SincePtr,
    BackToPtr>;

/**
 * Datastructure to pin frames
 */
struct Pin {
  std::optional<Var_x> x;
  std::optional<Var_f> f;

  std::optional<Expr> phi = {};

  Pin(std::optional<Var_x> x_, std::optional<Var_f> f_) : x{x_}, f{f_} {
    if (!x_.has_value() and !f_.has_value()) {
      throw std::invalid_argument(
          "Either time variable or frame variable must be specified when a frame is pinned.");
    }
  };

  Pin(std::optional<Var_x> x_) : Pin{x_, {}} {};
  Pin(std::optional<Var_f> f_) : Pin{{}, f_} {};

  Pin(const Pin& other) : x{other.x}, f{other.f}, phi{other.phi} {};
  Pin& operator=(const Pin& other) noexcept {
    if (this != &other) {
      x   = other.x;
      f   = other.f;
      phi = other.phi;
    }
    return *this;
  };

  Pin(Pin&& other) noexcept :
      x{std::move(other.x)}, f{std::move(other.f)}, phi{std::move(other.phi)} {};
  Pin& operator=(Pin&& other) noexcept {
    if (this != &other) {
      x   = std::move(other.x);
      f   = std::move(other.f);
      phi = std::move(other.phi);
    }
    return *this;
  };

  Pin& dot(const Expr& e) {
    this->phi = e;
    return *this;
  };
};

/**
 * Existential quantifier over IDs in either the current frame or a pinned frame.
 */
struct Exists {
  std::vector<Var_id> ids;
  std::optional<Pin> pinned_at = {};
  std::optional<Expr> phi      = {};

  Exists(std::initializer_list<Var_id> id_list) : ids{id_list}, pinned_at{}, phi{} {};

  Exists& at(const Pin& pin) {
    this->pinned_at = pin;
    return *this;
  };

  Exists& at(Pin&& pin) {
    this->pinned_at = std::move(pin);
    return *this;
  };

  Exists& dot(const Expr& e) {
    if (this->pinned_at && this->pinned_at->phi) {
      throw std::invalid_argument(
          "An expression is already being used by the pinned subexpression. Malformed formula construction!");
    }
    this->phi = e;
    return *this;
  };
};

/**
 * Universal quantifier over IDs in either the current frame or a pinned frame.
 */
struct Forall {
  std::vector<Var_id> ids;
  std::optional<Pin> pinned_at = {};
  std::optional<Expr> phi      = {};

  Forall(std::initializer_list<Var_id> id_list) : ids{id_list}, pinned_at{}, phi{} {};

  Forall& at(const Pin& pin) {
    this->pinned_at = pin;
    return *this;
  };

  Forall& at(Pin&& pin) {
    this->pinned_at = std::move(pin);
    return *this;
  };

  Forall& dot(const Expr& e) {
    if (this->pinned_at && this->pinned_at->phi) {
      throw std::invalid_argument(
          "An expression is already being used by the pinned subexpression. Malformed formula construction!");
    }
    this->phi = e;
    return *this;
  };
};

struct Not {
  Expr arg;

  Not() = delete;
  Not(const Expr& arg_) : arg{arg_} {};
};

struct And {
  std::vector<Expr> args;

  And() = delete;
  And(std::vector<Expr> args_) : args{args_} {
    if (args.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an And operator with < 2 operands");
    }
  };
};

struct Or {
  std::vector<Expr> args;

  Or() = delete;
  Or(std::vector<Expr> args_) : args{args_} {
    if (args.size() < 2) {
      throw std::invalid_argument(
          "It doesn't make sense to have an Or operator with < 2 operands");
    }
  };
};

struct Previous {
  Expr arg;

  Previous() = delete;
  Previous(const Expr& arg_) : arg{arg_} {};
};

struct Always {
  Expr arg;

  Always() = delete;
  Always(const Expr& arg_) : arg{arg_} {};
};

struct Sometimes {
  Expr arg;

  Sometimes() = delete;
  Sometimes(const Expr& arg_) : arg{arg_} {};
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

Expr operator~(const Expr& e);
Expr operator&(const Expr& lhs, const Expr& rhs);
Expr operator|(const Expr& lhs, const Expr& rhs);
Expr operator>>(const Expr& lhs, const Expr& rhs);

} // namespace ast
} // namespace percemon

#endif /* end of include guard: __PERCEMON_AST_AST_HPP__ */
