#include "percemon/ast.hpp"
#include "percemon/utils.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>

namespace percemon {
namespace ast {

namespace functions {

TimeBound operator-(const Var_x& lhs, C_TIME) { return TimeBound{lhs}; }
FrameBound operator-(const Var_f& lhs, C_FRAME) { return FrameBound{lhs}; }
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
TimeBound operator<(const double bound, const TimeBound& rhs) { return rhs > bound; }
TimeBound operator<=(const double bound, const TimeBound& rhs) { return rhs >= bound; }
TimeBound operator>(const double bound, const TimeBound& rhs) { return rhs < bound; }
TimeBound operator>=(const double bound, const TimeBound& rhs) { return rhs <= bound; }
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
FrameBound operator<(const size_t bound, const FrameBound& rhs) { return rhs > bound; }
FrameBound operator<=(const size_t bound, const FrameBound& rhs) {
  return rhs >= bound;
}
FrameBound operator>(const size_t bound, const FrameBound& rhs) { return rhs < bound; }
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

CompareED operator>(const ED& lhs, const double rhs) {
  return CompareED{lhs, ComparisonOp::GT, rhs};
}
CompareED operator>=(const ED& lhs, const double rhs) {
  return CompareED{lhs, ComparisonOp::GE, rhs};
}
CompareED operator<(const ED& lhs, const double rhs) {
  return CompareED{lhs, ComparisonOp::LT, rhs};
}
CompareED operator<=(const ED& lhs, const double rhs) {
  return CompareED{lhs, ComparisonOp::LE, rhs};
}
CompareED operator>(const ED& lhs, const ED& rhs) {
  return CompareED{lhs, ComparisonOp::GT, rhs};
}
CompareED operator>=(const ED& lhs, const ED& rhs) {
  return CompareED{lhs, ComparisonOp::GE, rhs};
}
CompareED operator<(const ED& lhs, const ED& rhs) {
  return CompareED{lhs, ComparisonOp::LT, rhs};
}
CompareED operator<=(const ED& lhs, const ED& rhs) {
  return CompareED{lhs, ComparisonOp::LE, rhs};
}

CompareLat operator>(const Lat& lhs, const double rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLat operator>=(const Lat& lhs, const double rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLat operator<(const Lat& lhs, const double rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLat operator<=(const Lat& lhs, const double rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}
CompareLat operator>(const double lhs, const Lat& rhs) { return rhs < lhs; }
CompareLat operator>=(const double lhs, const Lat& rhs) { return rhs <= lhs; }
CompareLat operator<(const double lhs, const Lat& rhs) { return rhs > lhs; }
CompareLat operator<=(const double lhs, const Lat& rhs) { return rhs >= lhs; }

CompareLat operator>(const Lat& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLat operator>=(const Lat& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLat operator<(const Lat& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLat operator<=(const Lat& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}
CompareLat operator>(const Lat& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLat operator>=(const Lat& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLat operator<(const Lat& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLat operator<=(const Lat& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}

CompareLon operator>(const Lon& lhs, const double rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLon operator>=(const Lon& lhs, const double rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLon operator<(const Lon& lhs, const double rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLon operator<=(const Lon& lhs, const double rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}
CompareLon operator>(const double lhs, const Lon& rhs) { return rhs < lhs; }
CompareLon operator>=(const double lhs, const Lon& rhs) { return rhs <= lhs; }
CompareLon operator<(const double lhs, const Lon& rhs) { return rhs > lhs; }
CompareLon operator<=(const double lhs, const Lon& rhs) { return rhs >= lhs; }

CompareLon operator>(const Lon& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLon operator>=(const Lon& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLon operator<(const Lon& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLon operator<=(const Lon& lhs, const Lon& rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}
CompareLon operator>(const Lon& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareLon operator>=(const Lon& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareLon operator<(const Lon& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareLon operator<=(const Lon& lhs, const Lat& rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}

CompareArea operator>(const Area& lhs, const double rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareArea operator>=(const Area& lhs, const double rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareArea operator<(const Area& lhs, const double rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareArea operator<=(const Area& lhs, const double rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}
CompareArea operator>(const double lhs, const Area& rhs) { return rhs < lhs; }
CompareArea operator>=(const double lhs, const Area& rhs) { return rhs <= lhs; }
CompareArea operator<(const double lhs, const Area& rhs) { return rhs > lhs; }
CompareArea operator<=(const double lhs, const Area& rhs) { return rhs >= lhs; }

CompareArea operator>(const Area& lhs, const Area& rhs) {
  return {lhs, ComparisonOp::GT, rhs};
}
CompareArea operator>=(const Area& lhs, const Area& rhs) {
  return {lhs, ComparisonOp::GE, rhs};
}
CompareArea operator<(const Area& lhs, const Area& rhs) {
  return {lhs, ComparisonOp::LT, rhs};
}
CompareArea operator<=(const Area& lhs, const Area& rhs) {
  return {lhs, ComparisonOp::LE, rhs};
}

} // namespace functions

namespace {
using percemon::utils::overloaded;

Expr AndHelper(const AndPtr& lhs, const Expr& rhs) {
  if (const Const* c_ptr = std::get_if<Const>(&rhs)) {
    return (c_ptr->value) ? Expr{lhs}
                          : Expr{*c_ptr}; // If True, return lhs, else return False
  }

  auto args = lhs->args;
  for (auto& e : lhs->temporal_bound_args) {
    args.push_back(std::visit([](auto&& c) { return Expr{c}; }, e));
  }

  if (const AndPtr* e_ptr = std::get_if<AndPtr>(&rhs)) {
    args.insert(args.end(), std::begin((*e_ptr)->args), std::end((*e_ptr)->args));
    for (auto& e : (*e_ptr)->temporal_bound_args) {
      args.push_back(std::visit([](auto&& c) { return Expr{c}; }, e));
    }
  } else {
    args.insert(args.end(), rhs);
  }
  return std::make_shared<And>(args);
}

Expr OrHelper(const OrPtr& lhs, const Expr& rhs) {
  if (const Const* c_ptr = std::get_if<Const>(&rhs)) {
    return (!c_ptr->value) ? Expr{lhs}
                           : Expr{*c_ptr}; // If False, return lhs, else return True
  }

  std::vector<Expr> args{lhs->args};
  for (auto& e : lhs->temporal_bound_args) {
    args.push_back(std::visit([](auto&& c) { return Expr{c}; }, e));
  }

  if (const OrPtr* e_ptr = std::get_if<OrPtr>(&rhs)) {
    args.insert(args.end(), std::begin((*e_ptr)->args), std::end((*e_ptr)->args));
    for (auto& e : (*e_ptr)->temporal_bound_args) {
      args.push_back(std::visit([](auto&& c) { return Expr{c}; }, e));
    }
  } else {
    args.insert(args.end(), rhs);
  }
  return std::make_shared<Or>(args);
}

} // namespace

Expr operator&(const Expr& lhs, const Expr& rhs) {
  if (const Const* c_ptr = std::get_if<Const>(&lhs)) {
    return (c_ptr->value) ? rhs : *c_ptr;
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
  if (const auto e = std::get_if<Const>(&expr)) { return Const{!(*e).value}; }
  return std::make_shared<Not>(expr);
}

Expr operator>>(const Expr& lhs, const Expr& rhs) { return ~(lhs) | rhs; }

} // namespace ast

ast::PinPtr Pinned(Var_x x, Var_f f) { return std::make_shared<Pin>(x, f); }
ast::PinPtr Pinned(Var_x x) { return std::make_shared<Pin>(x); }
ast::PinPtr Pinned(Var_f f) { return std::make_shared<Pin>(f); }

ast::ExistsPtr Exists(std::initializer_list<Var_id> id_list) {
  return std::make_shared<ast::Exists>(id_list);
}

ast::ForallPtr Forall(std::initializer_list<Var_id> id_list) {
  return std::make_shared<ast::Forall>(id_list);
}

ast::NotPtr Not(const Expr& arg) { return std::make_shared<ast::Not>(arg); }

ast::AndPtr And(const std::vector<Expr>& args) {
  return std::make_shared<ast::And>(args);
}

ast::OrPtr Or(const std::vector<Expr>& args) { return std::make_shared<ast::Or>(args); }

ast::PreviousPtr Previous(const Expr& arg) {
  return std::make_shared<ast::Previous>(arg);
}

ast::AlwaysPtr Always(const Expr& arg) { return std::make_shared<ast::Always>(arg); }

ast::SometimesPtr Sometimes(const Expr& arg) {
  return std::make_shared<ast::Sometimes>(arg);
}

ast::SincePtr Since(const Expr& a, const Expr& b) {
  return std::make_shared<ast::Since>(a, b);
}

ast::BackToPtr BackTo(const Expr& a, const Expr& b) {
  return std::make_shared<ast::BackTo>(a, b);
}

} // namespace percemon
