#pragma once

#ifndef __PERCEMON_FMT_FMT_HPP__
#define __PERCEMON_FMT_FMT_HPP__

#include <fmt/format.h>

namespace percemon {
namespace ast {
// std::ostream& operator<<(std::ostream& os, const percemon::ast::Expr& expr);
// std::ostream&
// operator<<(std::ostream& os, const percemon::ast::TemporalBoundExpr& expr);

template <typename Node>
struct formatter {
  constexpr fmt::format_parse_context::iterator
  parse(fmt::format_parse_context& ctx) const {
    return ctx.begin();
  }
};

} // namespace ast
} // namespace percemon

#endif /* end of include guard: __PERCEMON_FMT_FMT_HPP__ */
