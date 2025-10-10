#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <variant>

#include "percemon/ast.hpp"
#include "percemon/fmt.hpp"

namespace percemon::ast::primitives {

auto format_as(ComparisonOp op) -> std::string {
  switch (op) {
    case percemon::ast::ComparisonOp::GE: return (">=");
    case percemon::ast::ComparisonOp::GT: return (">");
    case percemon::ast::ComparisonOp::LE: return ("<=");
    case percemon::ast::ComparisonOp::LT: return ("<");
    case percemon::ast::ComparisonOp::EQ: return ("==");
    case percemon::ast::ComparisonOp::NE: return ("!=");
    default: std::abort();
  }
}

auto format_as(Const e) -> bool { return e.value; }
auto format_as(C_TIME) -> std::string { return "C_TIME"; }
auto format_as(C_FRAME) -> std::string { return "C_FRAME"; }
auto format_as(const Var_f& v) -> std::string { return fmt::format("f_{}", v.name); }
auto format_as(const Var_x& v) -> std::string { return fmt::format("x_{}", v.name); }
auto format_as(const Var_id& v) -> std::string { return fmt::format("id_{}", v.name); }
auto format_as(EmptySet) -> std::string { return "EMPTYSET"; }
auto format_as(UniverseSet) -> std::string { return "UNIVERSESET"; }

auto format_as(CRT e) -> std::string {
  switch (e) {
    case percemon::ast::CRT::LM: return "LM";
    case percemon::ast::CRT::RM: return "RM";
    case percemon::ast::CRT::TM: return "TM";
    case percemon::ast::CRT::BM: return "BM";
    case percemon::ast::CRT::CT: return "CT";
    default: std::abort();
  }
}
} // namespace percemon::ast::primitives

auto fmt::formatter<percemon::ast::TimeInterval>::format(
    const percemon::ast::TimeInterval& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  using namespace percemon::ast;
  auto fmt_str = "";
  switch (e.bound) {
    case TimeInterval::OPEN: fmt_str = "({}, {})"; break;
    case TimeInterval::LOPEN: fmt_str = "({}, {}]"; break;
    case TimeInterval::ROPEN: fmt_str = "[{}, {})"; break;
    case TimeInterval::CLOSED: fmt_str = "[{}, {}]"; break;
  }
  return format_to(ctx.out(), fmt_str, e.low, e.high);
}

auto fmt::formatter<percemon::ast::FrameInterval>::format(
    const percemon::ast::FrameInterval& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  using namespace percemon::ast;
  auto fmt_str = "";
  switch (e.bound) {
    case FrameInterval::OPEN: fmt_str = "({}, {})"; break;
    case FrameInterval::LOPEN: fmt_str = "({}, {}]"; break;
    case FrameInterval::ROPEN: fmt_str = "[{}, {})"; break;
    case FrameInterval::CLOSED: fmt_str = "[{}, {}]"; break;
  }
  return format_to(ctx.out(), fmt_str, e.low, e.high);
}

auto fmt::formatter<percemon::ast::TimeBound>::format(
    const percemon::ast::TimeBound& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({} - C_TIME {} {})", e.x, e.op, e.bound);
}

auto fmt::formatter<percemon::ast::FrameBound>::format(
    const percemon::ast::FrameBound& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({} - C_FRAME {} {})", e.f, e.op, e.bound);
}

auto fmt::formatter<percemon::ast::CompareId>::format(
    const percemon::ast::CompareId& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, e.rhs);
}

auto fmt::formatter<percemon::ast::Class>::format(
    const percemon::ast::Class& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Class({})", e.id);
}

auto fmt::formatter<percemon::ast::CompareClass>::format(
    const percemon::ast::CompareClass& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  std::string rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::Prob>::format(
    const percemon::ast::Prob& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  if (e.scale == 1.0) { return format_to(ctx.out(), "Prob({})", e.id); }
  return format_to(ctx.out(), "{} * Prob({})", e.scale, e.id);
}

auto fmt::formatter<percemon::ast::CompareProb>::format(
    const percemon::ast::CompareProb& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  std::string rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::BBox>::format(
    const percemon::ast::BBox& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "BBox({})", e.id);
}

auto fmt::formatter<percemon::ast::ED>::format(
    const percemon::ast::ED& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(
      ctx.out(), "{} * ED({}, {}, {}, {})", e.scale, e.id1, e.crt1, e.id2, e.crt2);
}

auto fmt::formatter<percemon::ast::Lat>::format(
    const percemon::ast::Lat& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "{} * Lat({}, {})", e.scale, e.id, e.crt);
}

auto fmt::formatter<percemon::ast::Lon>::format(
    const percemon::ast::Lon& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "{} * Lon({}, {})", e.scale, e.id, e.crt);
}

auto fmt::formatter<percemon::ast::AreaOf>::format(
    const percemon::ast::AreaOf& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "{} * Area({})", e.scale, e.id);
}

auto fmt::formatter<percemon::ast::CompareED>::format(
    const percemon::ast::CompareED& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, e.rhs);
}

auto fmt::formatter<percemon::ast::CompareLon>::format(
    const percemon::ast::CompareLon& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::CompareLat>::format(
    const percemon::ast::CompareLat& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::CompareArea>::format(
    const percemon::ast::CompareArea& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::SpArea>::format(
    const percemon::ast::SpArea& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "{} * Area({})", e.scale, e.arg);
}

auto fmt::formatter<percemon::ast::CompareSpArea>::format(
    const percemon::ast::CompareSpArea& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  auto rhs = std::visit([](const auto& a) { return fmt::to_string(a); }, e.rhs);
  return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
}

auto fmt::formatter<percemon::ast::Complement>::format(
    const percemon::ast::Complement& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "~{}", e.arg);
}

auto fmt::formatter<percemon::ast::Interior>::format(
    const percemon::ast::Interior& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Interior ({})", e.arg);
}

auto fmt::formatter<percemon::ast::Closure>::format(
    const percemon::ast::Closure& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Closure ({})", e.arg);
}

auto fmt::formatter<percemon::ast::Intersect>::format(
    const percemon::ast::Intersect& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({})", fmt::join(e.args, " & "));
}

auto fmt::formatter<percemon::ast::Union>::format(
    const percemon::ast::Union& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "({})", fmt::join(e.args, " | "));
}

auto fmt::formatter<percemon::ast::SpExists>::format(
    const percemon::ast::SpExists& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "SPEXISTS ({})", e.arg);
}

auto fmt::formatter<percemon::ast::SpForall>::format(
    const percemon::ast::SpForall& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "SPFORALL ({})", e.arg);
}

auto fmt::formatter<percemon::ast::SpPrevious>::format(
    const percemon::ast::SpPrevious& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "SpPrev {}", e.arg);
}

auto fmt::formatter<percemon::ast::SpAlways>::format(
    const percemon::ast::SpAlways& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  if (e.interval.has_value()) {
    return format_to(ctx.out(), "SpAlw_{} {}", *(e.interval), e.arg);
  }
  return format_to(ctx.out(), "SpAlw {}", e.arg);
}

auto fmt::formatter<percemon::ast::SpSometimes>::format(
    const percemon::ast::SpSometimes& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  if (e.interval.has_value()) {
    return format_to(ctx.out(), "SpSometimes_{} {}", *(e.interval), e.arg);
  }
  return format_to(ctx.out(), "SpSometimes {}", e.arg);
}

auto fmt::formatter<percemon::ast::SpSince>::format(
    const percemon::ast::SpSince& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  const auto [a, b] = e.args;
  if (e.interval.has_value()) {
    return format_to(ctx.out(), "{} SpSince_{} {}", a, *(e.interval), b);
  }
  return format_to(ctx.out(), "{} SpSince {}", a, b);
}

auto fmt::formatter<percemon::ast::SpBackTo>::format(
    const percemon::ast::SpBackTo& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  const auto [a, b] = e.args;
  if (e.interval.has_value()) {
    return format_to(ctx.out(), "{} SpBackTo_{} {}", a, *(e.interval), b);
  }
  return format_to(ctx.out(), "{} SpBackTo {}", a, b);
}

auto fmt::formatter<percemon::ast::Pin>::format(
    const percemon::ast::Pin& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  std::string x = "_", f = "_";
  if (e.x) { x = fmt::to_string(*e.x); }
  if (e.f) { f = fmt::to_string(*e.f); }
  return format_to(ctx.out(), "{{{0}, {1}}} . {2}", x, f, e.phi);
}

auto fmt::formatter<percemon::ast::Exists>::format(
    const percemon::ast::Exists& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  if (e.pinned_at.has_value()) {
    return format_to(
        ctx.out(), "EXISTS {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
  } else if (e.phi.has_value()) {
    return format_to(ctx.out(), "EXISTS {{{0}}} . {1}", fmt::join(e.ids, ", "), *e.phi);
  }
  return format_to(ctx.out(), "EXISTS {{{}}}", fmt::join(e.ids, ", "));
}

auto fmt::formatter<percemon::ast::Forall>::format(
    const percemon::ast::Forall& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  if (e.pinned_at.has_value()) {
    return format_to(
        ctx.out(), "FORALL {{{0}}} @ {1}", fmt::join(e.ids, ", "), *e.pinned_at);
  } else if (e.phi.has_value()) {
    return format_to(ctx.out(), "FORALL {{{0}}} . {1}", fmt::join(e.ids, ", "), *e.phi);
  }
  return format_to(ctx.out(), "FORALL {{{}}}", fmt::join(e.ids, ", "));
}

auto fmt::formatter<percemon::ast::Not>::format(
    const percemon::ast::Not& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "~{}", e.arg);
}

auto fmt::formatter<percemon::ast::And>::format(
    const percemon::ast::And& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
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

auto fmt::formatter<percemon::ast::Or>::format(
    const percemon::ast::Or& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
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

auto fmt::formatter<percemon::ast::Previous>::format(
    const percemon::ast::Previous& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Prev {}", e.arg);
}

auto fmt::formatter<percemon::ast::Always>::format(
    const percemon::ast::Always& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Alw {}", e.arg);
}

auto fmt::formatter<percemon::ast::Sometimes>::format(
    const percemon::ast::Sometimes& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return format_to(ctx.out(), "Sometimes {}", e.arg);
}

auto fmt::formatter<percemon::ast::Since>::format(
    const percemon::ast::Since& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  const auto [a, b] = e.args;
  return format_to(ctx.out(), "{} S {}", a, b);
}

auto fmt::formatter<percemon::ast::BackTo>::format(
    const percemon::ast::BackTo& e,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  const auto [a, b] = e.args;
  return format_to(ctx.out(), "{} B {}", a, b);
}

auto fmt::formatter<percemon::ast::TemporalBoundExpr>::format(
    const percemon::ast::TemporalBoundExpr& expr,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return std::visit(
      [&ctx](auto const& v) { return fmt::format_to(ctx.out(), "{}", v); }, expr);
}

auto fmt::formatter<percemon::ast::SpatialExpr>::format(
    const percemon::ast::SpatialExpr& expr,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return std::visit(
      [&ctx](auto const& v) { return fmt::format_to(ctx.out(), "{}", v); }, expr);
}

auto fmt::formatter<percemon::ast::Expr>::format(
    const percemon::ast::Expr& expr,
    fmt::format_context& ctx) const -> fmt::format_context::iterator {
  return std::visit(
      [&ctx](auto const& v) { return fmt::format_to(ctx.out(), "{}", v); }, expr);
}

#define FMT_AST_SHARED_PTR(AST_PTR_T)                        \
  auto fmt::formatter<AST_PTR_T>::format(                    \
      const AST_PTR_T& expr, fmt::format_context& ctx) const \
      -> fmt::format_context::iterator {                     \
    return fmt::format_to(ctx.out(), "{}", *expr);           \
  }

FMT_AST_SHARED_PTR(percemon::ast::ExistsPtr);
FMT_AST_SHARED_PTR(percemon::ast::ForallPtr);
FMT_AST_SHARED_PTR(percemon::ast::PinPtr);
FMT_AST_SHARED_PTR(percemon::ast::NotPtr);
FMT_AST_SHARED_PTR(percemon::ast::AndPtr);
FMT_AST_SHARED_PTR(percemon::ast::OrPtr);
FMT_AST_SHARED_PTR(percemon::ast::PreviousPtr);
FMT_AST_SHARED_PTR(percemon::ast::AlwaysPtr);
FMT_AST_SHARED_PTR(percemon::ast::SometimesPtr);
FMT_AST_SHARED_PTR(percemon::ast::SincePtr);
FMT_AST_SHARED_PTR(percemon::ast::BackToPtr);
FMT_AST_SHARED_PTR(percemon::ast::CompareSpAreaPtr);
FMT_AST_SHARED_PTR(percemon::ast::SpExistsPtr);
FMT_AST_SHARED_PTR(percemon::ast::SpForallPtr);
FMT_AST_SHARED_PTR(percemon::ast::ComplementPtr);
FMT_AST_SHARED_PTR(percemon::ast::IntersectPtr);
FMT_AST_SHARED_PTR(percemon::ast::UnionPtr);
FMT_AST_SHARED_PTR(percemon::ast::InteriorPtr);
FMT_AST_SHARED_PTR(percemon::ast::ClosurePtr);
FMT_AST_SHARED_PTR(percemon::ast::SpPreviousPtr);
FMT_AST_SHARED_PTR(percemon::ast::SpAlwaysPtr);
FMT_AST_SHARED_PTR(percemon::ast::SpSometimesPtr);
FMT_AST_SHARED_PTR(percemon::ast::SpSincePtr);
FMT_AST_SHARED_PTR(percemon::ast::SpBackToPtr);

#undef FMT_AST_SHARED_PTR
