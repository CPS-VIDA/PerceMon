#pragma once

#ifndef __PERCEMON_AST_PRIMITIVES_HH__
#define __PERCEMON_AST_PRIMITIVES_HH__

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <exception>

namespace percemon {
namespace ast {

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
};

/**
 * Variable place holder for timestamps.
 */
struct Var_x {
  std::string name;
};

/**
 * Datastructure to pin frames
 */
struct Pin {
  std::optional<Var_x> x;
  std::optional<Var_f> f;

  Pin() = delete;
  Pin(std::optional<Var_x> x_, std::optional<Var_f> f_) : x{x_}, f{f_} {
    if (!x_.has_value() and !f_.has_value()) {
      throw std::invalid_argument(
          "Either time variable or frame variable must be specified when a frame is pinned.");
    }
  };

  Pin(std::optional<Var_x> x_) : Pin{x_, {}} {};
  Pin(std::optional<Var_f> f_) : Pin{{}, f_} {};
};

/**
 * Variable place holder for object IDs
 */
struct Var_id {
  std::string name;
};

struct Exists {
  std::vector<Var_id> ids;

  Exists() = delete;
  Exists(std::initializer_list<Var_id> id_list) : ids{id_list} {};
};

struct Forall {
  std::vector<Var_id> ids;

  Forall() = delete;
  Forall(std::initializer_list<Var_id> id_list) : ids{id_list} {};
};

struct Const {
  bool value;
};
using ConstPtr = std::shared_ptr<Const>;

} // namespace ast
} // namespace percemon

#endif /* end of include guard: __PERCEMON_AST_PRIMITIVES_HH__ */
