#pragma once

#ifndef __PERCEMON_AST_HPP__
#define __PERCEMON_AST_HPP__

#include "percemon/ast/functions.hpp"
#include "percemon/ast/primitives.hpp"
#include "percemon/ast/s4u.hpp"
#include "percemon/ast/tqtl.hpp"

namespace percemon {
// TODO(anand): Create functions that return the Node pointers.
using namespace ast::primitives;
using namespace ast::functions;

using ast::Expr;

using ast::Pin;

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

} // namespace percemon

#endif /* end of include guard: __PERCEMON_AST_HPP__ */
