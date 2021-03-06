#pragma once

#ifndef __PERCEMON_AST_PRIMITIVES_HPP__
#define __PERCEMON_AST_PRIMITIVES_HPP__

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace percemon::ast::primitives {

// Some basic primitives

enum class ComparisonOp { GT, GE, LT, LE, EQ, NE };

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

enum struct CRT { LM, RM, TM, BM, CT };

} // namespace percemon::ast::primitives

#endif /* end of include guard: __PERCEMON_AST_PRIMITIVES_HPP__ */
