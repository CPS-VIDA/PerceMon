#pragma once

#ifndef __PERCEMON_FMT_FUNCTIONS_HPP__
#define __PERCEMON_FMT_FUNCTIONS_HPP__

#include <fmt/format.h>

#include "percemon/ast.hpp"

#include "percemon/fmt/fmt.hpp"

template <>
struct fmt::formatter<percemon::ast::TimeInterval>
    : percemon::ast::formatter<percemon::ast::TimeInterval> {
  template <typename FormatContext>
  auto format(const percemon::ast::TimeInterval& e, FormatContext& ctx) {
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
};

template <>
struct fmt::formatter<percemon::ast::FrameInterval>
    : percemon::ast::formatter<percemon::ast::FrameInterval> {
  template <typename FormatContext>
  auto format(const percemon::ast::FrameInterval& e, FormatContext& ctx) {
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
};

template <>
struct fmt::formatter<percemon::ast::TimeBound>
    : percemon::ast::formatter<percemon::ast::TimeBound> {
  template <typename FormatContext>
  auto format(const percemon::ast::TimeBound& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({} - C_TIME {} {})", e.x, e.op, e.bound);
  }
};

template <>
struct fmt::formatter<percemon::ast::FrameBound>
    : percemon::ast::formatter<percemon::ast::FrameBound> {
  template <typename FormatContext>
  auto format(const percemon::ast::FrameBound& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({} - C_FRAME {} {})", e.f, e.op, e.bound);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareId>
    : percemon::ast::formatter<percemon::ast::CompareId> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareId& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, e.rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::Class>
    : percemon::ast::formatter<percemon::ast::Class> {
  template <typename FormatContext>
  auto format(const percemon::ast::Class& e, FormatContext& ctx) {
    return format_to(ctx.out(), "Class({})", e.id);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareClass>
    : percemon::ast::formatter<percemon::ast::CompareClass> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareClass& e, FormatContext& ctx) {
    std::string rhs = std::visit([](auto r) { return fmt::to_string(r); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::Prob>
    : percemon::ast::formatter<percemon::ast::Prob> {
  template <typename FormatContext>
  auto format(const percemon::ast::Prob& e, FormatContext& ctx) {
    if (e.scale == 1.0) { return format_to(ctx.out(), "Prob({})", e.id); }
    return format_to(ctx.out(), "{} * Prob({})", e.scale, e.id);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareProb>
    : percemon::ast::formatter<percemon::ast::CompareProb> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareProb& e, FormatContext& ctx) {
    std::string rhs = std::visit([](auto r) { return fmt::to_string(r); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::BBox>
    : percemon::ast::formatter<percemon::ast::BBox> {
  template <typename FormatContext>
  auto format(const percemon::ast::BBox& e, FormatContext& ctx) {
    return format_to(ctx.out(), "BBox({})", e.id);
  }
};

template <>
struct fmt::formatter<percemon::ast::ED> : percemon::ast::formatter<percemon::ast::ED> {
  template <typename FormatContext>
  auto format(const percemon::ast::ED& e, FormatContext& ctx) {
    return format_to(
        ctx.out(), "{} * ED({}, {}, {}, {})", e.scale, e.id1, e.crt1, e.id2, e.crt2);
  }
};

template <>
struct fmt::formatter<percemon::ast::Lat>
    : percemon::ast::formatter<percemon::ast::Lat> {
  template <typename FormatContext>
  auto format(const percemon::ast::Lat& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{} * Lat({}, {})", e.scale, e.id, e.crt);
  }
};

template <>
struct fmt::formatter<percemon::ast::Lon>
    : percemon::ast::formatter<percemon::ast::Lon> {
  template <typename FormatContext>
  auto format(const percemon::ast::Lon& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{} * Lon({}, {})", e.scale, e.id, e.crt);
  }
};

template <>
struct fmt::formatter<percemon::ast::AreaOf>
    : percemon::ast::formatter<percemon::ast::AreaOf> {
  template <typename FormatContext>
  auto format(const percemon::ast::AreaOf& e, FormatContext& ctx) {
    return format_to(ctx.out(), "{} * Area({})", e.scale, e.id);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareED>
    : percemon::ast::formatter<percemon::ast::CompareED> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareED& e, FormatContext& ctx) {
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, e.rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareLon>
    : percemon::ast::formatter<percemon::ast::CompareLon> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareLon& e, FormatContext& ctx) {
    auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareLat>
    : percemon::ast::formatter<percemon::ast::CompareLat> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareLat& e, FormatContext& ctx) {
    auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

template <>
struct fmt::formatter<percemon::ast::CompareArea>
    : percemon::ast::formatter<percemon::ast::CompareArea> {
  template <typename FormatContext>
  auto format(const percemon::ast::CompareArea& e, FormatContext& ctx) {
    auto rhs = std::visit([](const auto& r) { return fmt::to_string(r); }, e.rhs);
    return format_to(ctx.out(), "({} {} {})", e.lhs, e.op, rhs);
  }
};

#endif /* end of include guard: __PERCEMON_FMT_FUNCTIONS_HPP__ */
