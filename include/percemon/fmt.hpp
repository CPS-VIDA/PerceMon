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
struct fmt::formatter<percemon::ast::SpatialExpr>
    : percemon::ast::formatter<percemon::ast::SpatialExpr> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpatialExpr& expr, FormatContext& ctx) {
    return std::visit(
        percemon::utils::overloaded{
            [&](const auto& e) { return format_to(ctx.out(), "{}", *e); },
            [&](const percemon::ast::EmptySet& e) {
              return format_to(ctx.out(), "{}", e);
            },
            [&](const percemon::ast::UniverseSet& e) {
              return format_to(ctx.out(), "{}", e);
            },
            [&](const percemon::ast::BBox& e) {
              return format_to(ctx.out(), "{}", e);
            }},
        expr);
  }
};

template <>
struct fmt::formatter<percemon::ast::Expr>
    : percemon::ast::formatter<percemon::ast::Expr> {
  template <typename FormatContext>
  auto format(const percemon::ast::Expr& expr, FormatContext& ctx) {
    using namespace percemon::ast;
    std::string expr_str = "";
    if (std::holds_alternative<Const>(expr)) {
      return format_to(ctx.out(), "{}", std::get<Const>(expr));
    }
    if (std::holds_alternative<TimeBound>(expr)) {
      return format_to(ctx.out(), "{}", std::get<TimeBound>(expr));
    }
    if (std::holds_alternative<FrameBound>(expr)) {
      return format_to(ctx.out(), "{}", std::get<FrameBound>(expr));
    }
    if (std::holds_alternative<CompareId>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareId>(expr));
    }
    if (std::holds_alternative<CompareClass>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareClass>(expr));
    }
    if (std::holds_alternative<CompareED>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareED>(expr));
    }
    if (std::holds_alternative<CompareLat>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareLat>(expr));
    }
    if (std::holds_alternative<CompareLon>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareLon>(expr));
    }
    if (std::holds_alternative<CompareArea>(expr)) {
      return format_to(ctx.out(), "{}", std::get<CompareArea>(expr));
    }
    if (std::holds_alternative<ExistsPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<ExistsPtr>(expr));
    }
    if (std::holds_alternative<AlwaysPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<AlwaysPtr>(expr));
    }
    if (std::holds_alternative<PinPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<PinPtr>(expr));
    }
    if (std::holds_alternative<NotPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<NotPtr>(expr));
    }
    if (std::holds_alternative<AndPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<AndPtr>(expr));
    }
    if (std::holds_alternative<OrPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<OrPtr>(expr));
    }
    if (std::holds_alternative<PreviousPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<PreviousPtr>(expr));
    }
    if (std::holds_alternative<AlwaysPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<AlwaysPtr>(expr));
    }
    if (std::holds_alternative<SometimesPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<SometimesPtr>(expr));
    }
    if (std::holds_alternative<SincePtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<SincePtr>(expr));
    }
    if (std::holds_alternative<BackToPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<BackToPtr>(expr));
    }
    // Spatio-temporal operators
    if (std::holds_alternative<CompareSpAreaPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<CompareSpAreaPtr>(expr));
    }
    if (std::holds_alternative<SpExistsPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<SpExistsPtr>(expr));
    }
    if (std::holds_alternative<SpForallPtr>(expr)) {
      return format_to(ctx.out(), "{}", *std::get<SpForallPtr>(expr));
    }
    return format_to(ctx.out(), "");
  }
};

#endif /* end of include guard: __PERCEMON_FMT_HPP__ */
