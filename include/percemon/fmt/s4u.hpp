#pragma once

#ifndef __PERCEMON_FMT_S4U_HPP__
#define __PERCEMON_FMT_S4U_HPP__

#include <fmt/format.h>

#include "percemon/ast.hpp"

#include "percemon/fmt/fmt.hpp"

template <>
struct fmt::formatter<percemon::ast::SpArea>
    : percemon::ast::formatter<percemon::ast::SpArea> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpArea& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{} * Area({})", e.scale, e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareSpArea>
    : percemon::ast::formatter<percemon::ast::CompareSpArea> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareSpArea& e, FormatContext& ctx) {
    auto rhs = std::visit([](const auto& a) { return fmt::to_string(a); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::Complement>
    : percemon::ast::formatter<percemon::ast::Complement> {
  template <typename FormatContext>
  auto format(const percemon::ast::Complement& e, FormatContext& ctx) {
    return format_to(ctx.out(), "~{}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Interior>
    : percemon::ast::formatter<percemon::ast::Interior> {
  template <typename FormatContext>
  auto format(const percemon::ast::Interior& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Interior ({})", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Closure>
    : percemon::ast::formatter<percemon::ast::Closure> {
  template <typename FormatContext>
  auto format(const percemon::ast::Closure& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Closure ({})", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::Intersect>
    : percemon::ast::formatter<percemon::ast::Intersect> {
  template <typename FormatContext>
  auto format(const percemon::ast::Intersect& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({})", fmt::join(e.args, " & "));
  }
};

template <>
struct fmt::formatter<percemon::ast::Union>
    : percemon::ast::formatter<percemon::ast::Union> {
  template <typename FormatContext>
  auto format(const percemon::ast::Union& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({})", fmt::join(e.args, " | "));
  }
};

template <>
struct fmt::formatter<percemon::ast::SpExists>
    : percemon::ast::formatter<percemon::ast::SpExists> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpExists& e, FormatContext& ctx) {
    return format_to(ctx.out(), "SPEXISTS ({})", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpForall>
    : percemon::ast::formatter<percemon::ast::SpForall> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpForall& e, FormatContext& ctx) {
    return format_to(ctx.out(), "SPFORALL ({})", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpPrevious>
    : percemon::ast::formatter<percemon::ast::SpPrevious> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpPrevious& e, FormatContext& ctx) {
    return format_to(ctx.out(), "SpPrev {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpAlways>
    : percemon::ast::formatter<percemon::ast::SpAlways> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpAlways& e, FormatContext& ctx) {
    if (e.interval.has_value()) {
      return format_to(ctx.out(), "SpAlw_{} {}", *(e.interval), e.arg);
    }
    return format_to(ctx.out(), "SpAlw {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpSometimes>
    : percemon::ast::formatter<percemon::ast::SpSometimes> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpSometimes& e, FormatContext& ctx) {
    if (e.interval.has_value()) {
      return format_to(ctx.out(), "SpSometimes_{} {}", *(e.interval), e.arg);
    }
    return format_to(ctx.out(), "SpSometimes {}", e.arg);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpSince>
    : percemon::ast::formatter<percemon::ast::SpSince> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpSince& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    if (e.interval.has_value()) {
      return format_to(ctx.out(), "{} SpSince_{} {}", a, *(e.interval), b);
    }
    return format_to(ctx.out(), "{} SpSince {}", a, b);
  }
};

template <>
struct fmt::formatter<percemon::ast::SpBackTo>
    : percemon::ast::formatter<percemon::ast::SpBackTo> {
  template <typename FormatContext>
  auto format(const percemon::ast::SpBackTo& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    if (e.interval.has_value()) {
      return format_to(ctx.out(), "{} SpBackTo_{} {}", a, *(e.interval), b);
    }
    return format_to(ctx.out(), "{} SpBackTo {}", a, b);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_S4U_HPP__ */
