#include <cppitertools/imap.hpp>
#include <cppitertools/starmap.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <type_traits>
#include <variant>

#include "percemon/ast.hpp"
// #include "percemon/fmt.hpp"

#include "utils.hpp"

#define FMT_UNARY_OP(NODE_NAME, NODE_REPR)                         \
  [[nodiscard]] std::string NODE_NAME::to_string() const {         \
    return fmt::format(#NODE_REPR "({})", this->arg->to_string()); \
  }

#define FMT_BINARY_OP(NODE_NAME, NODE_REPR)                                        \
  [[nodiscard]] std::string NODE_NAME::to_string() const {                         \
    const auto [a, b] = this->args;                                                \
    return fmt::format("({}) " #NODE_REPR "({})", a->to_string(), b->to_string()); \
  }

#define FMT_UNARY_OP_WITH_INTERVAL(NODE_NAME, NODE_REPR)                         \
  [[nodiscard]] std::string NODE_NAME::to_string() const {                       \
    const auto interval =                                                        \
        (this->interval) ? fmt::format("_{}", this->interval->to_string()) : ""; \
    return fmt::format(#NODE_REPR "{}({})", interval, this->arg->to_string());   \
  }

#define FMT_BINARY_OP_WITH_INTERVAL(NODE_NAME, NODE_REPR)                        \
  [[nodiscard]] std::string NODE_NAME::to_string() const {                       \
    const auto [a, b] = this->args;                                              \
    const auto interval =                                                        \
        (this->interval) ? fmt::format("_{}", this->interval->to_string()) : ""; \
    return fmt::format(                                                          \
        "({}) " #NODE_REPR "{} ({})", a->to_string(), interval, b->to_string()); \
  }

namespace percemon::ast {
template <typename T>
auto format_as(const T& node) -> decltype(node.to_string()) {
  return node.to_string();
}

namespace {

template <typename T>
auto into_string(const T& v) -> decltype(fmt::to_string(v)) {
  return fmt::to_string(v);
}

template <typename T>
auto into_string(const std::shared_ptr<T>& v) -> decltype(v->to_string()) {
  return v->to_string();
}

template <typename T>
auto into_string(const Difference<T>& v) -> std::string {
  return fmt::format("{} - {}", v.lhs, v.rhs);
}

auto into_string(const RefPoint& v) -> std::string {
  return fmt::format("({}, {})", v.id, v.crt);
}

template <typename... Args>
auto into_string(const std::variant<Args...>& v) -> std::string {
  return std::visit([](const auto& val) { return into_string(val); }, v);
}

template <typename Seq>
auto args_into_string(const Seq& seq) -> std::vector<std::string> {
  const auto nargs  = seq.size();
  auto args_str_vec = std::vector<std::string>();
  args_str_vec.reserve(nargs);
  for (const auto& arg : seq) { args_str_vec.push_back(into_string(arg)); }
  return args_str_vec;
}
template <typename... Args>
auto args_into_string(const std::tuple<Args...>& seq) -> std::vector<std::string> {
  constexpr auto nargs = sizeof...(Args);
  auto args_str_vec    = std::vector<std::string>();
  args_str_vec.reserve(nargs);

  std::apply(
      [&args_str_vec](Args const&... args) {
        ((args_str_vec.push_back(into_string(args))), ...);
      },
      seq);
  return args_str_vec;
}

// template <typename... Args>
// auto args_into_string(const std::tuple<Args...>& seq) -> std::vector<std::string> {
//   auto args_str_vec = std::vector<std::string>();
//   for (const auto& arg : seq) { args_str_vec.push_back(arg->to_string()); }
//   return args_str_vec;
// }
} // namespace

template <typename T>
[[nodiscard]] std::string Interval<T>::to_string() const {
  auto fmt_str = "";
  switch (this->bound) {
    case Bound::OPEN: fmt_str = "({}, {})"; break;
    case Bound::LOPEN: fmt_str = "({}, {}]"; break;
    case Bound::ROPEN: fmt_str = "[{}, {})"; break;
    case Bound::CLOSED: fmt_str = "[{}, {}]"; break;
  }
  return fmt::format(fmt_str, this->low, this->high);
}
template struct Interval<double>;
template struct Interval<std::int64_t>;

template <typename T>
[[nodiscard]] std::string Difference<T>::to_string() const {
  return fmt::format("{} - {}", lhs, rhs);
}

template struct Difference<Var_x>;
template struct Difference<Var_f>;

template <typename... Args>
[[nodiscard]] std::string FuncNode<Args...>::to_string() const {
  const auto name = this->get_name();

  std::vector<std::string> args_str_vec = args_into_string(this->args);
  const auto args_str                   = fmt::join(args_str_vec, ", ");

  return fmt::format("{}({})", args_str_vec);
}
template struct FuncNode<Var_id>;
template struct FuncNode<RefPoint, RefPoint>;
template struct FuncNode<RefPoint>;
template struct FuncNode<SpatialExpr>;

[[nodiscard]] std::string Class::to_string() const {
  return FuncNode<Var_id>::to_string();
}
[[nodiscard]] std::string Prob::to_string() const {
  return FuncNode<Var_id>::to_string();
}
[[nodiscard]] std::string ED::to_string() const {
  return FuncNode<RefPoint, RefPoint>::to_string();
}
[[nodiscard]] std::string Lat::to_string() const {
  return FuncNode<RefPoint>::to_string();
}
[[nodiscard]] std::string Lon::to_string() const {
  return FuncNode<RefPoint>::to_string();
}
[[nodiscard]] std::string AreaOf::to_string() const {
  return FuncNode<SpatialExpr>::to_string();
}

[[nodiscard]] std::string Const::to_string() const {
  return fmt::to_string(this->value);
}
[[nodiscard]] std::string EmptySet::to_string() const { return "EMPTYSET"; }
[[nodiscard]] std::string UniverseSet::to_string() const { return "UNIVERSESET"; }

template <typename Lhs, typename Rhs, ComparisonOp... Ops>
[[nodiscard]] std::string CompareNode<Lhs, Rhs, Ops...>::to_string() const {
  const auto lhs_str = this->lhs.to_string();
  const auto rhs_str = into_string(this->rhs);
  const auto op_str  = format_as(this->op);
  return fmt::format("({:s} {} {:s})", lhs_str, this->op, rhs_str);
}
template struct OrderingOpNode<TimeDiff, double>;
template struct OrderingOpNode<FrameDiff, std::int64_t>;
template struct EqualityOpNode<Var_id, Var_id>;
template struct EqualityOpNode<Class, MaybeClass>;
template struct OrderingOpNode<Prob, MaybeProb>;
template struct OrderingOpNode<ED, double>;
template struct OrderingOpNode<Lat, MaybeLatLon>;
template struct OrderingOpNode<Lon, MaybeLatLon>;
template struct OrderingOpNode<AreaOf, MaybeAreaOf>;

[[nodiscard]] std::string Exists::to_string() const {
  std::vector<std::string> ids_vec = args_into_string(this->ids);
  return fmt::format(
      "Exists {{{0}}} . {1}", fmt::join(ids_vec, ", "), this->arg->to_string());
}

[[nodiscard]] std::string Forall::to_string() const {
  std::vector<std::string> ids_vec = args_into_string(this->ids);
  return fmt::format(
      "Forall {{{0}}} . {1}", fmt::join(ids_vec, ", "), this->arg->to_string());
}

[[nodiscard]] std::string Pin::to_string() const {
  std::string x = "_", f = "_";
  if (this->x) { x = this->x->to_string(); }
  if (this->f) { f = this->f->to_string(); }
  return fmt::format("@ {{{0}, {1}}} . {2}", x, f, this->arg->to_string());
}

[[nodiscard]] std::string Not::to_string() const {
  return fmt::format("~{}", this->arg->to_string());
}

[[nodiscard]] std::string And::to_string() const {
  const std::vector<std::string> args_str = args_into_string(this->args);
  return fmt::format("({})", fmt::join(args_str, " & "));
}

[[nodiscard]] std::string Or::to_string() const {
  const std::vector<std::string> args_str = args_into_string(this->args);
  return fmt::format("({})", fmt::join(args_str, " | "));
}

[[nodiscard]] std::string Next::to_string() const {
  const auto interval = (this->steps > 1) ? fmt::format("[{}]", this->steps) : "";
  return fmt::format("next{}({})", interval, this->arg->to_string());
}

[[nodiscard]] std::string Previous::to_string() const {
  const auto interval = (this->steps > 1) ? fmt::format("[{}]", this->steps) : "";
  return fmt::format("prev{}({})", interval, this->arg->to_string());
}

FMT_UNARY_OP(percemon::ast::Always, always);
FMT_UNARY_OP(percemon::ast::Holds, holds);
FMT_UNARY_OP(percemon::ast::Eventually, eventually);
FMT_UNARY_OP(percemon::ast::Sometimes, sometimes);
FMT_BINARY_OP(percemon::ast::Until, until);
FMT_BINARY_OP(percemon::ast::Since, since);
FMT_BINARY_OP(percemon::ast::Release, release);
FMT_BINARY_OP(percemon::ast::BackTo, back_to);

[[nodiscard]] std::string SpExists::to_string() const {
  return fmt::format("SpExists ({})", this->arg->to_string());
}

[[nodiscard]] std::string SpForall::to_string() const {
  return fmt::format("SpForall ({})", this->arg->to_string());
}

[[nodiscard]] std::string BBox::to_string() const {
  return fmt::format("BBox({})", this->id.to_string());
}

[[nodiscard]] std::string Complement::to_string() const {
  return fmt::format("~{}", this->arg->to_string());
}

[[nodiscard]] std::string Interior::to_string() const {
  return fmt::format("Interior ({})", this->arg->to_string());
}

[[nodiscard]] std::string Closure::to_string() const {
  return fmt::format("Closure ({})", this->arg->to_string());
}

[[nodiscard]] std::string Intersect::to_string() const {
  std::vector<std::string> args_str_vec = args_into_string(this->args);
  return fmt::format("({})", fmt::join(args_str_vec, " & "));
}

[[nodiscard]] std::string Union::to_string() const {
  std::vector<std::string> args_str_vec = args_into_string(this->args);
  return fmt::format("({})", fmt::join(args_str_vec, " | "));
}

[[nodiscard]] std::string SpNext::to_string() const {
  return fmt::format("sp_next({})", this->arg->to_string());
}

[[nodiscard]] std::string SpPrevious::to_string() const {
  return fmt::format("sp_prev({})", this->arg->to_string());
}
} // namespace percemon::ast

FMT_UNARY_OP_WITH_INTERVAL(percemon::ast::SpAlways, sp_always);
FMT_UNARY_OP_WITH_INTERVAL(percemon::ast::SpHolds, sp_holds);
FMT_UNARY_OP_WITH_INTERVAL(percemon::ast::SpEventually, sp_eventually);
FMT_UNARY_OP_WITH_INTERVAL(percemon::ast::SpSometimes, sp_sometimes);
FMT_BINARY_OP_WITH_INTERVAL(percemon::ast::SpUntil, sp_until);
FMT_BINARY_OP_WITH_INTERVAL(percemon::ast::SpSince, sp_since);
FMT_BINARY_OP_WITH_INTERVAL(percemon::ast::SpRelease, sp_release);
FMT_BINARY_OP_WITH_INTERVAL(percemon::ast::SpBackTo, sp_back_to);

#undef FMT_UNARY_OP
#undef FMT_BINARY_OP
#undef FMT_UNARY_OP_WITH_INTERVAL
#undef FMT_BINARY_OP_WITH_INTERVAL
