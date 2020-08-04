#pragma once

#ifndef __PERCEMON_S4U_HPP__
#define __PERCEMON_S4U_HPP__

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <exception>

#include "percemon/ast/ast.hpp"

namespace percemon::ast {

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

} // namespace percemon::ast

#endif /* end of include guard: __PERCEMON_S4U_HPP__ */
