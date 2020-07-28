#include "percemon/ast.hpp"
#include "percemon/utils.hpp"

#include <iterator>
#include <memory>

namespace percemon {
namespace ast {

TimeBound operator-(const Var_x& lhs, C_TIME) {
  return TimeBound{lhs};
}

FrameBound operator-(const Var_f& lhs, C_FRAME) {
  return FrameBound{lhs};
}

TimeBound operator>(const TimeBound& lhs, const double bound) {
  return TimeBound{lhs.x, ComparisonOp::GT, bound};
}

TimeBound operator>=(const TimeBound& lhs, const double bound) {
  return TimeBound{lhs.x, ComparisonOp::GE, bound};
}

TimeBound operator<(const TimeBound& lhs, const double bound) {
  return TimeBound{lhs.x, ComparisonOp::LT, bound};
}

TimeBound operator<=(const TimeBound& lhs, const double bound) {
  return TimeBound{lhs.x, ComparisonOp::LE, bound};
}

TimeBound operator<(const double bound, const TimeBound& rhs) {
  return rhs > bound;
}

TimeBound operator<=(const double bound, const TimeBound& rhs) {
  return rhs >= bound;
}

TimeBound operator>(const double bound, const TimeBound& rhs) {
  return rhs < bound;
}

TimeBound operator>=(const double bound, const TimeBound& rhs) {
  return rhs <= bound;
}

FrameBound operator>(const FrameBound& lhs, const size_t bound) {
  return FrameBound{lhs.f, ComparisonOp::GT, bound};
}

FrameBound operator>=(const FrameBound& lhs, const size_t bound) {
  return FrameBound{lhs.f, ComparisonOp::GE, bound};
}

FrameBound operator<(const FrameBound& lhs, const size_t bound) {
  return FrameBound{lhs.f, ComparisonOp::LT, bound};
}

FrameBound operator<=(const FrameBound& lhs, const size_t bound) {
  return FrameBound{lhs.f, ComparisonOp::LE, bound};
}

FrameBound operator<(const size_t bound, const FrameBound& rhs) {
  return rhs > bound;
}

FrameBound operator<=(const size_t bound, const FrameBound& rhs) {
  return rhs >= bound;
}

FrameBound operator>(const size_t bound, const FrameBound& rhs) {
  return rhs < bound;
}

FrameBound operator>=(const size_t bound, const FrameBound& rhs) {
  return rhs <= bound;
}

CompareId operator==(const Var_id& lhs, const Var_id& rhs) {
  return CompareId{lhs, ComparisonOp::EQ, rhs};
}

CompareId operator!=(const Var_id& lhs, const Var_id& rhs) {
  return CompareId{lhs, ComparisonOp::NE, rhs};
}

CompareClass operator==(const Class& lhs, const int rhs) {
  return CompareClass{lhs, ComparisonOp::EQ, rhs};
}

CompareClass operator==(const int lhs, const Class& rhs) {
  return CompareClass{rhs, ComparisonOp::EQ, lhs};
}

CompareClass operator==(const Class& lhs, const Class& rhs) {
  return CompareClass{lhs, ComparisonOp::EQ, rhs};
}

CompareClass operator!=(const Class& lhs, const int rhs) {
  return CompareClass{lhs, ComparisonOp::NE, rhs};
}

CompareClass operator!=(const int lhs, const Class& rhs) {
  return CompareClass{rhs, ComparisonOp::NE, lhs};
}

CompareClass operator!=(const Class& lhs, const Class& rhs) {
  return CompareClass{lhs, ComparisonOp::NE, rhs};
}

CompareProb operator>(const Prob& lhs, const double rhs) {
  return CompareProb{lhs, ComparisonOp::GT, rhs};
}
CompareProb operator>=(const Prob& lhs, const double rhs) {
  return CompareProb{lhs, ComparisonOp::GE, rhs};
}
CompareProb operator<(const Prob& lhs, const double rhs) {
  return CompareProb{lhs, ComparisonOp::LT, rhs};
}
CompareProb operator<=(const Prob& lhs, const double rhs) {
  return CompareProb{lhs, ComparisonOp::LE, rhs};
}
CompareProb operator>(const Prob& lhs, const Prob& rhs) {
  return CompareProb{lhs, ComparisonOp::GT, rhs};
}
CompareProb operator>=(const Prob& lhs, const Prob& rhs) {
  return CompareProb{lhs, ComparisonOp::GE, rhs};
}
CompareProb operator<(const Prob& lhs, const Prob& rhs) {
  return CompareProb{lhs, ComparisonOp::LT, rhs};
}
CompareProb operator<=(const Prob& lhs, const Prob& rhs) {
  return CompareProb{lhs, ComparisonOp::LE, rhs};
}

namespace {
using percemon::utils::overloaded;

Expr AndHelper(const AndPtr& lhs, const Expr& rhs) {
  std::vector<Expr> args{lhs->args};
  // TODO(anand): Bounds need to be Minkowski summed

  std::visit(
      overloaded{[&](const auto) { args.push_back(rhs); },
                 [&args](const Const& e) {
                   if (!e.value) {
                     args = {Expr{e}};
                   }
                 },
                 [&args](const AndPtr e) {
                   args.reserve(
                       args.size() + std::distance(e->args.begin(), e->args.end()));
                   args.insert(args.end(), e->args.begin(), e->args.end());
                 }},
      rhs);
  return std::make_shared<And>(args);
}

Expr OrHelper(const OrPtr& lhs, const Expr& rhs) {
  std::vector<Expr> args{lhs->args};
  // TODO(anand): Bounds need to be Minkowski summed

  std::visit(
      overloaded{[&](const auto) { args.push_back(rhs); },
                 [&args](const Const e) {
                   if (e.value) {
                     args = {Expr{e}};
                   }
                 },
                 [&args](const OrPtr e) {
                   args.reserve(
                       args.size() + std::distance(e->args.begin(), e->args.end()));
                   args.insert(args.end(), e->args.begin(), e->args.end());
                 }},
      rhs);
  return std::make_shared<Or>(args);
}

} // namespace

Expr operator&(const Expr& lhs, const Expr& rhs) {
  if (const auto e_ptr = std::get_if<Const>(&lhs)) {
    return (e_ptr->value) ? rhs : *e_ptr;
  } else if (const AndPtr* e_ptr = std::get_if<AndPtr>(&lhs)) {
    return AndHelper(*e_ptr, rhs);
  }
  return std::make_shared<And>(std::vector{lhs, rhs});
}

Expr operator|(const Expr& lhs, const Expr& rhs) {
  if (const auto e_ptr = std::get_if<Const>(&lhs)) {
    return (e_ptr->value) ? *e_ptr : rhs;
  } else if (const OrPtr* e_ptr = std::get_if<OrPtr>(&lhs)) {
    return OrHelper(*e_ptr, rhs);
  }
  return std::make_shared<Or>(std::vector{lhs, rhs});
}

Expr operator~(const Expr& expr) {
  if (const auto e = std::get_if<Const>(&expr)) {
    return Const{!(*e).value};
  }
  return std::make_shared<Not>(expr);
}

Expr operator>>(const Expr& lhs, const Expr& rhs) {
  return ~(lhs) | rhs;
}

} // namespace ast

} // namespace percemon
