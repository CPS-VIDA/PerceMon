#include "percemon/ast.hpp"
#include "percemon/fmt.hpp"

#include "utils.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>

namespace percemon::ast {

namespace {
template <typename BinOp, bool identity>
Expr BinOpFlatten(const Expr& lhs, const Expr& rhs) {
  // Check if lhs is constant
  if (auto c_ptr = std::dynamic_pointer_cast<Const>(lhs)) {
    // If lhs is the identity value, return the rhs, otherwise, return the lhs
    return (c_ptr->value == identity) ? rhs : lhs;
  }
  // Check if rhs is constant
  if (auto c_ptr = std::dynamic_pointer_cast<Const>(rhs)) {
    return (c_ptr->value == identity) ? lhs : rhs;
  }
  if (const auto& cast_lhs = std::dynamic_pointer_cast<BinOp>(lhs)) {
    std::vector<Expr> args = cast_lhs->args;
    if (const auto rhs_ptr = std::dynamic_pointer_cast<BinOp>(rhs)) {
      args.reserve(args.size() + rhs_ptr->args.size());
      args.insert(args.end(), std::begin(rhs_ptr->args), std::end(rhs_ptr->args));
    } else {
      args.insert(args.end(), rhs);
    }
    return std::make_shared<BinOp>(args);
  }
  return std::make_shared<BinOp>(std::vector{lhs, rhs});
}

template <typename BinOp, typename Identity, typename Annihilator>
SpatialExpr SpBinOpFlatten(const SpatialExpr& lhs, const SpatialExpr& rhs) {
  // If lhs is the identity value, return the rhs
  if (std::dynamic_pointer_cast<Identity>(lhs)) { return rhs; }
  // If rhs is the identity value, return the lhs
  if (std::dynamic_pointer_cast<Identity>(rhs)) { return lhs; }

  // If rhs or lhs is the annihilator, return the annihilator
  if (std::dynamic_pointer_cast<Annihilator>(rhs) == nullptr ||
      std::dynamic_pointer_cast<Annihilator>(lhs) == nullptr) {
    return std::make_shared<Annihilator>();
  }

  if (const auto& lhs_ptr = std::dynamic_pointer_cast<BinOp>(lhs)) {
    std::vector<SpatialExpr> args = lhs_ptr->args;

    if (const auto rhs_ptr = std::dynamic_pointer_cast<BinOp>(rhs)) {
      args.reserve(args.size() + rhs_ptr->args.size());
      args.insert(args.end(), std::begin(rhs_ptr->args), std::end(rhs_ptr->args));
    } else {
      args.insert(args.end(), rhs);
    }
    return std::make_shared<BinOp>(args);
  }
  return std::make_shared<BinOp>(std::vector{lhs, rhs});
}

} // namespace

Expr BaseExpr::operator~() {
  auto expr = shared_from_this();
  if (auto constant = std::dynamic_pointer_cast<Const>(expr)) {
    return Const{!constant->value}.ptr();
  }
  return Not(expr).ptr();
}

Expr operator&(const Expr& lhs, const Expr& rhs) {
  return BinOpFlatten<And, true>(lhs, rhs);
}

Expr operator|(const Expr& lhs, const Expr& rhs) {
  return BinOpFlatten<Or, false>(lhs, rhs);
}

SpatialExpr BaseSpatialExpr::operator~() {
  auto expr = shared_from_this();
  if (std::dynamic_pointer_cast<EmptySet>(expr)) {
    return std::make_shared<UniverseSet>();
  }
  if (std::dynamic_pointer_cast<UniverseSet>(expr)) {
    return std::make_shared<EmptySet>();
  }
  return Complement{expr}.ptr();
}

SpatialExpr operator&(const SpatialExpr& lhs, const SpatialExpr& rhs) {
  return SpBinOpFlatten<Intersect, UniverseSet, EmptySet>(lhs, rhs);
}

SpatialExpr operator|(const SpatialExpr& lhs, const SpatialExpr& rhs) {
  return SpBinOpFlatten<Union, EmptySet, UniverseSet>(lhs, rhs);
}

} // namespace percemon::ast
