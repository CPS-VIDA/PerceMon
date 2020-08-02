#pragma once

#ifndef __PERCEMON_FMT_TQTL_HPP__
#define __PERCEMON_FMT_TQTL_HPP__

#include "percemon/fmt/fmt.hpp"

#include "percemon/ast.hpp"

template <>
struct fmt::formatter<percemon::ast::Pin>
    : percemon::ast::formatter<percemon::ast::Pin> {
  template <typename FormatContext>
  auto format(const percemon::ast::Pin& e, FormatContext& ctx) {
    std::string x = "_", f = "_";
    if (e.x) { x = fmt::to_string(*e.x); }
    if (e.f) { f = fmt::to_string(*e.f); }
    return format_to(ctx.out(), "{{{0}, {1}}} . {2}", x, f, e.phi);
  }
};

template <>
struct fmt::formatter<percemon::ast::Exists>
    : percemon::ast::formatter<percemon::ast::Exists> {
  template <typename FormatContext>
  auto format(const percemon::ast::Exists& e, FormatContext& ctx) {
    if (e.pinned_at.has_value()) {
      return format_to(
          ctx.out(), "EXISTS {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
    } else if (e.phi.has_value()) {
      return format_to(
          ctx.out(), "EXISTS {{{0}}} . {1}", fmt::join(e.ids, ", "), *e.phi);
    }
    return format_to(ctx.out(), "EXISTS {{{}}}", fmt::join(e.ids, ", "));
  }
};

template <>
struct fmt::formatter<percemon::ast::Forall>
    : percemon::ast::formatter<percemon::ast::Forall> {
  template <typename FormatContext>
  auto format(const percemon::ast::Forall& e, FormatContext& ctx) {
    if (e.pinned_at.has_value()) {
      return format_to(
          ctx.out(), "FORALL {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
    } else if (e.phi.has_value()) {
      return format_to(
          ctx.out(), "FORALL {{{0}}} . {1}", fmt::join(e.ids, ", "), *e.phi);
    }
    return format_to(ctx.out(), "FORALL {{{}}}", fmt::join(e.ids, ", "));
  }
};

template <>
struct fmt::formatter<percemon::ast::Not>
    : percemon::ast::formatter<percemon::ast::Not> {
  template <typename FormatContext>
  auto format(const percemon::ast::Not& e, FormatContext& ctx) {
    return format_to(ctx.out(), "~{}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::And>
    : percemon::ast::formatter<percemon::ast::And> {
  template <typename FormatContext>
  auto format(const percemon::ast::And& e, FormatContext& ctx) {
    if (e.temporal_bound_args.empty()) {
      return format_to(ctx.out(), "({})", fmt::join(e.args, " & "));
    } else if (e.args.empty()) {
      return format_to(ctx.out(), "({})", fmt::join(e.temporal_bound_args, " & "));
    } else {
      return format_to(
          ctx.out(),
          "({} & {})",
          fmt::join(e.temporal_bound_args, " & "),
          fmt::join(e.args, " & "));
    }
  }
};

template <>
struct fmt::formatter<percemon::ast::Or> : percemon::ast::formatter<percemon::ast::Or> {
  template <typename FormatContext>
  auto format(const percemon::ast::Or& e, FormatContext& ctx) {
    if (e.temporal_bound_args.empty()) {
      return format_to(ctx.out(), "({})", fmt::join(e.args, " | "));
    } else if (e.args.empty()) {
      return format_to(ctx.out(), "({})", fmt::join(e.temporal_bound_args, " | "));
    } else {
      return format_to(
          ctx.out(),
          "({} | {})",
          fmt::join(e.temporal_bound_args, " | "),
          fmt::join(e.args, " | "));
    }
  }
};

template <>
struct fmt::formatter<percemon::ast::Previous>
    : percemon::ast::formatter<percemon::ast::Previous> {
  template <typename FormatContext>
  auto format(const percemon::ast::Previous& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Prev {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Always>
    : percemon::ast::formatter<percemon::ast::Always> {
  template <typename FormatContext>
  auto format(const percemon::ast::Always& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Alw {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Sometimes>
    : percemon::ast::formatter<percemon::ast::Sometimes> {
  template <typename FormatContext>
  auto format(const percemon::ast::Sometimes& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Sometimes {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Since>
    : percemon::ast::formatter<percemon::ast::Since> {
  template <typename FormatContext>
  auto format(const percemon::ast::Since& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    return format_to(ctx.out(), "{} S {}", a, b);
  }
};

template <>
struct fmt::formatter<percemon::ast::BackTo>
    : percemon::ast::formatter<percemon::ast::BackTo> {
  template <typename FormatContext>
  auto format(const percemon::ast::BackTo& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    return format_to(ctx.out(), "{} B {}", a, b);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_TQTL_HPP__ */
