#pragma once

#ifndef __PERCEMON_AST_FMT_HPP__
#define __PERCEMON_AST_FMT_HPP__

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "percemon/ast/s4u.hpp"
#include "percemon/ast/tqtl.hpp"

#include "percemon/utils.hpp"

namespace percemon {
namespace ast {
std::ostream& operator<<(std::ostream& os, const percemon::ast::Expr& expr);

template <typename Node>
struct formatter {
  constexpr auto parse(fmt::format_parse_context& ctx) {
    return ctx.begin();
  }
};

} // namespace ast
} // namespace percemon

template <>
struct fmt::formatter<percemon::ast::C_TIME>
    : percemon::ast::formatter<percemon::ast::C_TIME> {
  template <typename FormatContext>
  auto format(const percemon::ast::C_TIME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_TIME");
  };
};

template <>
struct fmt::formatter<percemon::ast::C_FRAME>
    : percemon::ast::formatter<percemon::ast::C_FRAME> {
  template <typename FormatContext>
  auto format(const percemon::ast::C_FRAME&, FormatContext& ctx) {
    return format_to(ctx.out(), "C_FRAME");
  };
};

template <>
struct fmt::formatter<percemon::ast::Var_f>
    : percemon::ast::formatter<percemon::ast::Var_f> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_f& e, FormatContext& ctx) {
    return format_to(ctx.out(), "f_{}", e.name);
  };
};

template <>
struct fmt::formatter<percemon::ast::Var_x>
    : percemon::ast::formatter<percemon::ast::Var_x> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_x& e, FormatContext& ctx) {
    return format_to(ctx.out(), "x_{}", e.name);
  };
};

template <>
struct fmt::formatter<percemon::ast::Var_id>
    : percemon::ast::formatter<percemon::ast::Var_id> {
  template <typename FormatContext>
  auto format(const percemon::ast::Var_id& e, FormatContext& ctx) {
    return format_to(ctx.out(), "id_{}", e.name);
  };
};

template <>
struct fmt::formatter<percemon::ast::TimeBound>
    : percemon::ast::formatter<percemon::ast::TimeBound> {
  template <typename FormatContext>
  auto format(const percemon::ast::TimeBound& e, FormatContext& ctx) {
    std::string op = ">=";
    switch (e.op) {
      case percemon::ast::ComparisonOp::GE: op = ">="; break;
      case percemon::ast::ComparisonOp::GT: op = ">"; break;
      case percemon::ast::ComparisonOp::LE: op = "<="; break;
      case percemon::ast::ComparisonOp::LT: op = "<"; break;
    }
    return format_to(ctx.out(), "{} - C_TIME {} {}", e.x, op, e.bound);
  };
};

template <>
struct fmt::formatter<percemon::ast::FrameBound>
    : percemon::ast::formatter<percemon::ast::FrameBound> {
  template <typename FormatContext>
  auto format(const percemon::ast::FrameBound& e, FormatContext& ctx) {
    std::string op = ">=";
    switch (e.op) {
      case percemon::ast::ComparisonOp::GE: op = ">="; break;
      case percemon::ast::ComparisonOp::GT: op = ">"; break;
      case percemon::ast::ComparisonOp::LE: op = "<="; break;
      case percemon::ast::ComparisonOp::LT: op = "<"; break;
    }
    return format_to(ctx.out(), "{} - C_FRAME {} {}", e.f, op, e.bound);
  };
};

template <>
struct fmt::formatter<percemon::ast::Const>
    : percemon::ast::formatter<percemon::ast::Const> {
  template <typename FormatContext>
  auto format(const percemon::ast::Const& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{}", e.value);
  };
};

template <>
struct fmt::formatter<percemon::ast::Pin>
    : percemon::ast::formatter<percemon::ast::Pin> {
  template <typename FormatContext>
  auto format(const percemon::ast::Pin& e, FormatContext& ctx) {
    std::string x = "_", f = "_";
    if (e.x) {
      x = fmt::to_string(*e.x);
    }
    if (e.f) {
      f = fmt::to_string(*e.f);
    }
    if (e.phi) {
      return format_to(ctx.out(), "{{{0}, {1}}} . ({2})", x, f, *e.phi);
    }
    return format_to(ctx.out(), "{{{0}, {1}}}", x, f);
  };
};

template <>
struct fmt::formatter<percemon::ast::Exists>
    : percemon::ast::formatter<percemon::ast::Exists> {
  template <typename FormatContext>
  auto format(const percemon::ast::Exists& e, FormatContext& ctx) {
    if (e.pinned_at.has_value()) {
      return format_to(
          ctx.out(), "EXISTS {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
    }
    return format_to(ctx.out(), "EXISTS {{{}}}", fmt::join(e.ids, ", "));
  };
};

template <>
struct fmt::formatter<percemon::ast::Forall>
    : percemon::ast::formatter<percemon::ast::Forall> {
  template <typename FormatContext>
  auto format(const percemon::ast::Forall& e, FormatContext& ctx) {
    if (e.pinned_at.has_value()) {
      return format_to(
          ctx.out(), "FORALL {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
    }
    return format_to(ctx.out(), "FORALL {{{}}}", fmt::join(e.ids, ", "));
  };
};

template <>
struct fmt::formatter<percemon::ast::Not>
    : percemon::ast::formatter<percemon::ast::Not> {
  template <typename FormatContext>
  auto format(const percemon::ast::Not& e, FormatContext& ctx) {
    return format_to(ctx.out(), "~{}", e.arg);
  };
};

template <>
struct fmt::formatter<percemon::ast::And>
    : percemon::ast::formatter<percemon::ast::And> {
  template <typename FormatContext>
  auto format(const percemon::ast::And& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({})", fmt::join(e.args, " & "));
  };
};

template <>
struct fmt::formatter<percemon::ast::Or> : percemon::ast::formatter<percemon::ast::Or> {
  template <typename FormatContext>
  auto format(const percemon::ast::Or& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({})", fmt::join(e.args, " | "));
  };
};

template <>
struct fmt::formatter<percemon::ast::Previous>
    : percemon::ast::formatter<percemon::ast::Previous> {
  template <typename FormatContext>
  auto format(const percemon::ast::Previous& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Prev {}", e.arg);
  };
};

template <>
struct fmt::formatter<percemon::ast::Always>
    : percemon::ast::formatter<percemon::ast::Always> {
  template <typename FormatContext>
  auto format(const percemon::ast::Always& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Alw {}", e.arg);
  };
};

template <>
struct fmt::formatter<percemon::ast::Sometimes>
    : percemon::ast::formatter<percemon::ast::Sometimes> {
  template <typename FormatContext>
  auto format(const percemon::ast::Sometimes& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Sometimes {}", e.arg);
  };
};

template <>
struct fmt::formatter<percemon::ast::Since>
    : percemon::ast::formatter<percemon::ast::Since> {
  template <typename FormatContext>
  auto format(const percemon::ast::Since& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    return format_to(ctx.out(), "{} S {}", a, b);
  };
};

template <>
struct fmt::formatter<percemon::ast::BackTo>
    : percemon::ast::formatter<percemon::ast::BackTo> {
  template <typename FormatContext>
  auto format(const percemon::ast::BackTo& e, FormatContext& ctx) {
    const auto [a, b] = e.args;
    return format_to(ctx.out(), "{} B {}", a, b);
  };
};

inline std::ostream& operator<<(std::ostream& os, const percemon::ast::Expr& expr) {
  std::string s = std::visit(
      percemon::utils::overloaded{
          [](const percemon::ast::Const& e) { return fmt::to_string(e); },
          [](const percemon::ast::TimeBound& e) { return fmt::to_string(e); },
          [](const percemon::ast::FrameBound& e) { return fmt::to_string(e); },
          [](const auto e) {
            return fmt::to_string(*e);
          }},
      expr);

  return os << s;
}

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
                                    [&](const auto& e) {
                                      return format_to(ctx.out(), "{}", *e);
                                    }},
        expr);
  };
};

#endif /* end of include guard: __PERCEMON_AST_FMT_HPP__ */
