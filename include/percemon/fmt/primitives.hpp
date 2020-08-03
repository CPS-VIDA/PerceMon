#pragma once

#ifndef __PERCEMON_FMT_PRIMITIVES_HPP__
#define __PERCEMON_FMT_PRIMITIVES_HPP__

#include <fmt/format.h>

#include "percemon/fmt/fmt.hpp"

#include "percemon/ast/primitives.hpp"

template <>
struct fmt::formatter<percemon::ast::ComparisonOp>
    : percemon::ast::formatter<percemon::ast::ComparisonOp> {
  template <typename FormatContext>
  auto format(const percemon::ast::ComparisonOp& op, FormatContext& ctx) {
    std::string op_str;
    switch (op) {
      case percemon::ast::ComparisonOp::GE: op_str = ">="; break;
      case percemon::ast::ComparisonOp::GT: op_str = ">"; break;
      case percemon::ast::ComparisonOp::LE: op_str = "<="; break;
      case percemon::ast::ComparisonOp::LT: op_str = "<"; break;
      case percemon::ast::ComparisonOp::EQ: op_str = "=="; break;
      case percemon::ast::ComparisonOp::NE: op_str = "!="; break;
    }
    return format_to(ctx.out(), "{}", op_str);
  }
};

template <>
struct fmt::formatter<percemon::ast::C_TIME>
    : percemon::ast::formatter<percemon::ast::C_TIME> {
  template <typename FormatContext>
  auto format(const percemon::ast::C_TIME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_TIME");
  }
};

template <>
struct fmt::formatter<percemon::ast::C_FRAME>
    : percemon::ast::formatter<percemon::ast::C_FRAME> {
  template <typename FormatContext>
  auto format(const percemon::ast::C_FRAME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_FRAME");
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_f>
    : percemon::ast::formatter<percemon::ast::Var_f> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_f& e, FormatContext& ctx) {
    return format_to(ctx.out(), "f_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_x>
    : percemon::ast::formatter<percemon::ast::Var_x> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_x& e, FormatContext& ctx) {
    return format_to(ctx.out(), "x_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Var_id>
    : percemon::ast::formatter<percemon::ast::Var_id> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_id& e, FormatContext& ctx) {
    return format_to(ctx.out(), "id_{}", e.name);
  }
};

template <>
struct fmt::formatter<percemon::ast::Const>
    : percemon::ast::formatter<percemon::ast::Const> {
  template <typename FormatContext>
  auto format(const percemon::ast::Const& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", e.value);
  }
};

template <>
struct fmt::formatter<percemon::ast::EmptySet>
    : percemon::ast::formatter<percemon::ast::EmptySet> {
  template <typename FormatContext>
  auto format(const percemon::ast::EmptySet&, FormatContext& ctx) {
    return format_to(ctx.out(), "EMPTYSET");
  }
};

template <>
struct fmt::formatter<percemon::ast::UniverseSet>
    : percemon::ast::formatter<percemon::ast::UniverseSet> {
  template <typename FormatContext>
  auto format(const percemon::ast::UniverseSet&, FormatContext& ctx) {
    return format_to(ctx.out(), "UNIVERSE");
  }
};

template <>
struct fmt::formatter<percemon::ast::CRT>
    : percemon::ast::formatter<percemon::ast::CRT> {
  template <typename FormatContext>
  auto format(const percemon::ast::CRT& e, FormatContext& ctx) {
    switch (e) {
      case percemon::ast::CRT::LM: return format_to(ctx.out(), "LM");
      case percemon::ast::CRT::RM: return format_to(ctx.out(), "RM");
      case percemon::ast::CRT::TM: return format_to(ctx.out(), "TM");
      case percemon::ast::CRT::BM: return format_to(ctx.out(), "BM");
      case percemon::ast::CRT::CT: return format_to(ctx.out(), "CT");
    }
  }
};

#endif /* end of include guard: __PERCEMON_FMT_PRIMITIVES_HPP__ */
