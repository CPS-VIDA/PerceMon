#pragma once

#ifndef __PERCEMON_AST_FMT_HPP__
#define __PERCEMON_AST_FMT_HPP__

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "percemon/ast/s4u.hpp"
#include "percemon/ast/tqtl.hpp"

template <>
struct fmt::formatter<percemon::ast::C_TIME> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::C_TIME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_TIME");
  }
};

template <>
struct fmt::formatter<percemon::ast::C_FRAME> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::C_FRAME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_FRAME");
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_f> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Var_f& e, FormatContext& ctx) {
    return format_to(ctx.out(), "f_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_x> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Var_x& e, FormatContext& ctx) {
    return format_to(ctx.out(), "x_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Pin> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Pin& e, FormatContext& ctx) {
    auto x = e.x.value_or(percemon::ast::Var_x{"_"});
    auto f = e.f.value_or(percemon::ast::Var_f{"_"});
    return format_to(ctx.out(), "{{{0}, {1}}}", x, f);
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_id> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Var_id& e, FormatContext& ctx) {
    return format_to(ctx.out(), "id_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Exists> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Exists& e, FormatContext& ctx) {
    return format_to(ctx.out(), "EXISTS {{{}}}", fmt::join(e.ids, ", "));
  }
};

template <>
struct fmt::formatter<percemon::ast::Forall> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const percemon::ast::Forall& e, FormatContext& ctx) {
    return format_to(ctx.out(), "FORALL {{{}}}", fmt::join(e.ids, ", "));
  }
};

#endif /* end of include guard: __PERCEMON_AST_FMT_HPP__ */
