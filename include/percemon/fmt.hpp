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
  struct FormatterHelp {
    template <typename A>
    std::string operator()(const A& a) {
      return fmt::to_string(a);
    }

    template <typename A>
    std::string operator()(const std::shared_ptr<A>& a) {
      return fmt::to_string(*a);
    }
  };

  template <typename FormatContext>
  auto format(const percemon::ast::SpatialExpr& expr, FormatContext& ctx) {
    std::string e = std::visit(FormatterHelp{}, expr);
    return format_to(ctx.out(), "{}", e);
  }
};

template <>
struct fmt::formatter<percemon::ast::Expr>
    : percemon::ast::formatter<percemon::ast::Expr> {
  struct FormatterHelp {
    template <typename A>
    std::string operator()(const A& a) {
      return fmt::to_string(a);
    }

    template <typename A>
    std::string operator()(const std::shared_ptr<A>& a) {
      return fmt::to_string(*a);
    }
  };

  template <typename FormatContext>
  auto format(const percemon::ast::Expr& expr, FormatContext& ctx) {
    std::string e = std::visit(FormatterHelp{}, expr);
    return format_to(ctx.out(), "{}", e);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_HPP__ */
