#pragma once

#ifndef __PERCEMON_S4U_HPP__
#define __PERCEMON_S4U_HPP__

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <exception>

#include "percemon/ast/primitives.hpp"

namespace percemon {
namespace ast {

struct EmptySet {};
struct UniverseSet {};

struct BBox {
  Var_id id;
};

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

// TODO(anand): Add MTL x S4U ops

using SpExpr = std::variant<
    EmptySet,
    UniverseSet,
    BBox,
    ComplementPtr,
    IntersectPtr,
    UnionPtr,
    InteriorPtr,
    ClosurePtr,
    SpExistsPtr,
    SpForallPtr>;

} // namespace ast
} // namespace percemon

#endif /* end of include guard: __PERCEMON_S4U_HPP__ */
