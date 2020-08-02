#pragma once

#ifndef __PERCEMON_AST_AST_HPP__
#define __PERCEMON_AST_AST_HPP__

#include "percemon/ast/primitives.hpp"

#include "percemon/ast/functions.hpp"

#include <exception>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace percemon::ast {

using namespace primitives;
using namespace functions;

// TQTL Subset
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

using Expr = std::variant<
    // Primitives.
    Const,
    // Functions on Primitives
    TimeBound,
    FrameBound,
    CompareId,
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
    BackToPtr>;

Expr operator~(const Expr& e);
Expr operator&(const Expr& lhs, const Expr& rhs);
Expr operator|(const Expr& lhs, const Expr& rhs);
Expr operator>>(const Expr& lhs, const Expr& rhs);

} // namespace percemon::ast

#endif /* end of include guard: __PERCEMON_AST_AST_HPP__ */
