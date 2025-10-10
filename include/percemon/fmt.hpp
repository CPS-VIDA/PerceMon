#pragma once

#ifndef __PERCEMON_FMT2_HPP__
#define __PERCEMON_FMT2_HPP__

#include <fmt/format.h>

#include "percemon/ast.hpp"

namespace percemon::ast::primitives {

auto format_as(ComparisonOp) -> std::string;
auto format_as(Const) -> bool;
auto format_as(C_TIME) -> std::string;
auto format_as(C_FRAME) -> std::string;
auto format_as(const Var_f&) -> std::string;
auto format_as(const Var_x&) -> std::string;
auto format_as(const Var_id&) -> std::string;
auto format_as(EmptySet) -> std::string;
auto format_as(UniverseSet) -> std::string;
auto format_as(CRT e) -> std::string;
} // namespace percemon::ast::primitives

#define AST_FMT(AST_T)                                         \
  template <>                                                  \
  struct fmt::formatter<AST_T> : fmt::formatter<std::string> { \
    auto format(const AST_T&, fmt::format_context& ctx) const  \
        -> fmt::format_context::iterator;                      \
  }

AST_FMT(percemon::ast::TimeInterval);
AST_FMT(percemon::ast::FrameInterval);
AST_FMT(percemon::ast::TimeBound);
AST_FMT(percemon::ast::FrameBound);
AST_FMT(percemon::ast::CompareId);
AST_FMT(percemon::ast::Class);
AST_FMT(percemon::ast::CompareClass);
AST_FMT(percemon::ast::Prob);
AST_FMT(percemon::ast::CompareProb);
AST_FMT(percemon::ast::BBox);
AST_FMT(percemon::ast::ED);
AST_FMT(percemon::ast::Lat);
AST_FMT(percemon::ast::Lon);
AST_FMT(percemon::ast::AreaOf);
AST_FMT(percemon::ast::CompareED);
AST_FMT(percemon::ast::CompareLon);
AST_FMT(percemon::ast::CompareLat);
AST_FMT(percemon::ast::CompareArea);
AST_FMT(percemon::ast::SpArea);
AST_FMT(percemon::ast::CompareSpArea);
AST_FMT(percemon::ast::Complement);
AST_FMT(percemon::ast::Interior);
AST_FMT(percemon::ast::Closure);
AST_FMT(percemon::ast::Intersect);
AST_FMT(percemon::ast::Union);
AST_FMT(percemon::ast::SpExists);
AST_FMT(percemon::ast::SpForall);
AST_FMT(percemon::ast::SpPrevious);
AST_FMT(percemon::ast::SpAlways);
AST_FMT(percemon::ast::SpSometimes);
AST_FMT(percemon::ast::SpSince);
AST_FMT(percemon::ast::SpBackTo);
AST_FMT(percemon::ast::Pin);
AST_FMT(percemon::ast::Exists);
AST_FMT(percemon::ast::Forall);
AST_FMT(percemon::ast::Not);
AST_FMT(percemon::ast::And);
AST_FMT(percemon::ast::Or);
AST_FMT(percemon::ast::Previous);
AST_FMT(percemon::ast::Always);
AST_FMT(percemon::ast::Sometimes);
AST_FMT(percemon::ast::Since);
AST_FMT(percemon::ast::BackTo);
AST_FMT(percemon::ast::TemporalBoundExpr);
AST_FMT(percemon::ast::SpatialExpr);
AST_FMT(percemon::ast::Expr);

AST_FMT(percemon::ast::ExistsPtr);
AST_FMT(percemon::ast::ForallPtr);
AST_FMT(percemon::ast::PinPtr);
AST_FMT(percemon::ast::NotPtr);
AST_FMT(percemon::ast::AndPtr);
AST_FMT(percemon::ast::OrPtr);
AST_FMT(percemon::ast::PreviousPtr);
AST_FMT(percemon::ast::AlwaysPtr);
AST_FMT(percemon::ast::SometimesPtr);
AST_FMT(percemon::ast::SincePtr);
AST_FMT(percemon::ast::BackToPtr);
AST_FMT(percemon::ast::CompareSpAreaPtr);
AST_FMT(percemon::ast::SpExistsPtr);
AST_FMT(percemon::ast::SpForallPtr);
AST_FMT(percemon::ast::ComplementPtr);
AST_FMT(percemon::ast::IntersectPtr);
AST_FMT(percemon::ast::UnionPtr);
AST_FMT(percemon::ast::InteriorPtr);
AST_FMT(percemon::ast::ClosurePtr);
AST_FMT(percemon::ast::SpPreviousPtr);
AST_FMT(percemon::ast::SpAlwaysPtr);
AST_FMT(percemon::ast::SpSometimesPtr);
AST_FMT(percemon::ast::SpSincePtr);
AST_FMT(percemon::ast::SpBackToPtr);

#undef AST_FMT
#endif // __PERCEMON_FMT2_HPP__
