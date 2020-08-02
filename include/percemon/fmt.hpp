#pragma once

#ifndef __PERCEMON_FMT_HPP__
#define __PERCEMON_FMT_HPP__

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "percemon/fmt/functions.hpp"
#include "percemon/fmt/primitives.hpp"
#include "percemon/fmt/s4u.hpp"
#include "percemon/fmt/tqtl.hpp"

#include "percemon/utils.hpp"

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
    return std::visit(
        percemon::utils::overloaded{[&](const percemon::ast::Const& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::TimeBound& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::FrameBound& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareId& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareClass& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareED& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareLat& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareLon& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const percemon::ast::CompareArea& e) {
                                      return format_to(ctx.out(), "{}", e);
                                    },
                                    [&](const auto& e) {
                                      return format_to(ctx.out(), "{}", *e);
                                    }},
        expr);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_HPP__ */
