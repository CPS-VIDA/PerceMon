#pragma once

#ifndef __STQL_AST_PRIMITIVES_HH__
#define __STQL_AST_PRIMITIVES_HH__

#include <memory>
#include <string>
#include <variant>

namespace stql {
namespace ast {

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
 * Variable place holder for object IDs
 */
template <typename T>
struct Var_id {
  T id;
};

struct Const;
using ConstPtr = std::shared_ptr<Const>;

} // namespace ast
} // namespace stql

#endif /* end of include guard: __STQL_AST_PRIMITIVES_HH__ */
