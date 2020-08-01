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

namespace percemon {
namespace ast {

// TODO(anand): Add MTL x S4U ops

struct Complement {};

struct Intersect;
struct Union;
struct Interior;
struct Closure;
struct SpExists;
struct SpForall;

} // namespace ast
} // namespace percemon

#endif /* end of include guard: __PERCEMON_S4U_HPP__ */
