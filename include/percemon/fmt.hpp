#pragma once

#ifndef __PERCEMON_FMT_HPP__
#define __PERCEMON_FMT_HPP__

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "percemon/ast.hpp"

#include "percemon/utils.hpp"

#include "percemon/fmt/functions.hpp"
#include "percemon/fmt/primitives.hpp"
#include "percemon/fmt/s4u.hpp"
#include "percemon/fmt/tqtl.hpp"

template <>
struct fmt::formatter<percemon::ast::TemporalBoundExpr>
    : percemon::ast::formatter<percemon::ast::TemporalBoundExpr> {
  template <typename FormatContext>
  auto format(const percemon::ast::TemporalBoundExpr& expr, FormatContext& ctx) {
    return std::visit(
        [&](const auto& e) { return format_to(ctx.out(), "{}", e); }, expr);
  }
};

template <>
struct fmt::formatter<percemon::ast::Expr>
    : percemon::ast::formatter<percemon::ast::Expr> {
  template <typename FormatContext>
  auto format(const percemon::ast::Expr& expr, FormatContext& ctx) {
    using namespace percemon;
    std::string expr_str = std::visit(
        utils::overloaded{[](const ast::Const& e) { return fmt::to_string(e); },
                          [](const ast::TimeBound& e) { return fmt::to_string(e); },
                          [](const ast::FrameBound& e) { return fmt::to_string(e); },
                          [](const ast::CompareId& e) { return fmt::to_string(e); },
                          [](const ast::CompareClass& e) { return fmt::to_string(e); },
                          [](const ast::CompareED& e) { return fmt::to_string(e); },
                          [](const ast::CompareLat& e) { return fmt::to_string(e); },
                          [](const ast::CompareLon& e) { return fmt::to_string(e); },
                          [](const ast::CompareArea& e) { return fmt::to_string(e); },
                          [](const auto&& e) {
                            return fmt::to_string(*e);
                          }},
        expr);
    return format_to(ctx.out(), "{}", expr_str);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_HPP__ */
