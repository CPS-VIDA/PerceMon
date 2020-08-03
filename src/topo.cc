#include "percemon/topo.hpp"

#include <cassert>
#include <cmath>
#include <vector>

#include <itertools.hpp>

#include "percemon/utils.hpp"

using percemon::utils::overloaded;

namespace {
using namespace percemon::topo;

constexpr double TOP    = std::numeric_limits<double>::infinity();
constexpr double BOTTOM = -TOP;

/**
 * Helper class to compute the area of a union of BoundingBoxes.
 * Idea derived from
 * https://codercareer.blogspot.com/2011/12/no-27-area-of-rectangles.html
 */
struct TopoAreaCompute {
  /**
   * A one dimensional interval
   */
  struct Interval {
    double low, high;
    Interval(double l, double h) {
      low  = (l < h) ? l : h;
      high = (l + h) - low;
    }
    constexpr bool is_overlapping(const Interval& other) {
      return !(low > other.high || other.low > high);
    }
    void merge(const Interval& other) {
      if (is_overlapping(other)) {
        low  = std::min(low, other.low);
        high = std::max(high, other.high);
      }
    }
  };

  /**
   * Sort boxes by left margin
   */
  struct BoundOrder {
    constexpr bool operator()(const BoundingBox& a, const BoundingBox& b) {
      return a.xmin < b.xmin;
    }
  };

  double operator()(const TopoUnion& region);
};

Region intersection_of(const BoundingBox& a, const BoundingBox& b) {
  // First check if they intersect horizontally

  double xmin = BOTTOM;
  double xmax = BOTTOM;
  double ymin = BOTTOM;
  double ymax = BOTTOM;
  bool lopen  = false;
  bool ropen  = false;
  bool topen  = false;
  bool bopen  = false;

  // Check x axis intersection.
  if (a.xmin <= b.xmin && b.xmin <= a.xmax) {
    xmin  = b.xmin;
    lopen = b.lopen;
    if (a.xmin <= b.xmax && b.xmax <= a.xmax) {
      xmax  = b.xmax;
      ropen = b.ropen;
    } else if (b.xmin <= a.xmax && a.xmax <= b.xmax) {
      xmax  = a.xmax;
      ropen = a.ropen;
    }
  } else if (b.xmin <= a.xmin && a.xmin <= b.xmax) {
    xmin  = a.xmin;
    lopen = a.lopen;
    if (b.xmin <= a.xmax && a.xmax <= b.xmax) {
      xmax  = a.xmax;
      ropen = a.ropen;
    } else if (a.xmin <= b.xmax && b.xmax <= a.xmax) {
      xmax  = b.xmax;
      ropen = b.ropen;
    }
  } else {
    // Since x axis doesn't intersect, the boxes don't intersect
    return Empty{};
  }

  // Check if y axis intersects
  if (a.ymin <= b.ymin && b.ymin <= a.ymax) {
    ymin  = b.ymin;
    topen = b.topen;
    if (a.ymin <= b.ymax && b.ymax <= a.ymax) {
      ymax  = b.ymax;
      bopen = b.bopen;
    } else if (b.ymin <= a.ymax && a.ymax <= b.ymax) {
      ymax  = a.ymax;
      bopen = a.bopen;
    }
  } else if (b.ymin <= a.ymin && a.ymin <= b.ymax) {
    ymin  = a.ymin;
    topen = a.topen;
    if (b.ymin <= a.ymax && a.ymax <= b.ymax) {
      ymax  = a.ymax;
      bopen = a.bopen;
    } else if (a.ymin <= b.ymax && b.ymax <= a.ymax) {
      ymax  = b.ymax;
      bopen = b.bopen;
    }
  } else {
    // Since y axis doesn't intersect, the boxes don't intersect
    return Empty{};
  }

  return BoundingBox{xmin, xmax, ymin, ymax, lopen, ropen, topen, bopen};
}

Region intersection_of(const TopoUnion& a, const BoundingBox& b) {
  auto intersect_set = std::vector<BoundingBox>{};
  for (const BoundingBox& bbox : a) {
    auto int_box = intersection_of(b, bbox);
    if (auto box_p = std::get_if<BoundingBox>(&int_box)) {
      intersect_set.push_back(*box_p);
    } // Else do nothing as the current bboxes do not intersect.
  }
  if (intersect_set.size() == 0) { return Empty{}; }
  if (intersect_set.size() == 1) { return intersect_set.back(); }

  return TopoUnion{std::begin(intersect_set), std::end(intersect_set)};
}

Region intersection_of(const TopoUnion& lhs, const TopoUnion& rhs) {
  auto intersect_set = std::vector<BoundingBox>{};
  for (const BoundingBox& a : lhs) {
    for (const BoundingBox& b : rhs) {
      auto int_box = intersection_of(a, b);
      if (auto box_p = std::get_if<BoundingBox>(&int_box)) {
        intersect_set.push_back(*box_p);
      } // Else do nothing as the current bboxes do not intersect.
    }
  }
  if (intersect_set.size() == 0) { return Empty{}; }
  if (intersect_set.size() == 1) { return intersect_set.back(); }

  return TopoUnion{std::begin(intersect_set), std::end(intersect_set)};
}

Region union_of(const BoundingBox& a, const BoundingBox& b) {
  // Check if one box is  inside the other.

  if ((a.xmin <= b.xmin && b.xmin <= a.xmax) &&
      (a.xmin <= b.xmax && b.xmax <= a.xmax)) {
    // b is inside a in the horizontal axis

    // Left is open if the left align and both of them are open.
    // Else just pick a (the outer one)
    bool lopen = (a.xmin == b.xmin) ? (a.lopen && b.lopen) : a.lopen;
    // Same logic
    bool ropen = (a.xmax == b.xmax) ? (a.ropen && b.ropen) : a.ropen;

    // Now check the vertical axis.
    if ((a.ymin < b.ymin && b.ymin < a.ymax) && (a.ymin < b.ymax && b.ymax < a.ymax)) {
      // b is inside a

      bool topen = (a.ymin == b.ymin) ? (a.topen && b.topen) : a.topen;
      bool bopen = (a.ymax == b.ymax) ? (a.bopen && b.bopen) : a.bopen;

      // Return a
      return BoundingBox{a.xmin, a.xmax, a.ymin, a.ymax, lopen, ropen, topen, bopen};
    }
  }
  if ((b.xmin <= a.xmin && a.xmin <= b.xmax) &&
      (b.xmin <= a.xmax && a.xmax <= b.xmax)) {
    // a is inside b in the horizontal axis

    // Left is open if the left align and both of them are open.
    // Els e just pick a (the outer one)
    bool lopen = (b.xmin == a.xmin) ? (b.lopen && a.lopen) : b.lopen;
    // Same logic
    bool ropen = (b.xmax == a.xmax) ? (b.ropen && a.ropen) : b.ropen;

    // Now check the vertical axis.
    if ((b.ymin < a.ymin && a.ymin < b.ymax) && (b.ymin < a.ymax && a.ymax < b.ymax)) {
      // b is inside a

      bool topen = (b.ymin == a.ymin) ? (b.topen && a.topen) : b.topen;
      bool bopen = (b.ymax == a.ymax) ? (b.bopen && a.bopen) : b.bopen;

      // Return a
      return BoundingBox{b.xmin, b.xmax, b.ymin, b.ymax, lopen, ropen, topen, bopen};
    }
  }

  // Just return a union of the boxes.
  auto ret = TopoUnion{};
  ret.insert(a);
  ret.insert(b);
  return ret;
}

Region union_of(const TopoUnion& a, const BoundingBox& b) {
  auto ret = a;
  ret.insert(b);
  return ret;
}

Region union_of(const TopoUnion& a, const TopoUnion& b) {
  // Just add it. Handle the area computation properly.
  auto ret = a;
  ret.merge(b);
  return ret;
}

} // namespace

namespace percemon::topo {

inline bool is_closed(const Region& region) {
  return std::visit(
      overloaded{[](const Empty&) { return true; },
                 [](const Universe&) { return true; },
                 [](const BoundingBox& bbox) {
                   return !(bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen);
                 },
                 [](const TopoUnion& region) {
                   // If any sub region is open, the set is not closed.
                   for (auto&& bbox : region) {
                     if (bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen) {
                       return false;
                     }
                   }
                   return true;
                 }},
      region);
}

inline bool is_open(const Region& region) {
  return std::visit(
      overloaded{[](const Empty&) { return true; },
                 [](const Universe&) { return true; },
                 [](const BoundingBox& bbox) {
                   return bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen;
                 },
                 [](const TopoUnion& region) {
                   // If even one sub region is open, the entire thing should be open.
                   for (auto&& bbox : region) {
                     if (bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen) {
                       return true;
                     }
                   }
                   return false;
                 }},
      region);
}

double area(const Region& region) {
  return std::visit(
      overloaded{[](const Empty&) -> double { return 0.0; },
                 [](const Universe&) -> double {
                   return std::numeric_limits<double>::infinity();
                 },
                 [](const BoundingBox& bbox) -> double {
                   return std::abs((bbox.xmin - bbox.xmax) * (bbox.ymin - bbox.ymax));
                 },
                 [](const TopoUnion& region) -> double {
                   auto area_op = TopoAreaCompute{};
                   return area_op(region);
                 }},
      region);
}

Region interior(const Region& region) {
  return std::visit(
      overloaded{
          [](const Empty& e) -> Region { return e; },
          [](const Universe& u) -> Region { return u; },
          [](const BoundingBox& bbox) -> Region {
            BoundingBox ret{bbox};
            ret.lopen = true;
            ret.ropen = true;
            ret.topen = true;
            ret.bopen = true;
            assert(is_open(ret));
            return ret;
          },
          [](const TopoUnion& region) -> Region {
            TopoUnion ret{};
            for (auto&& bbox : region) {
              ret.insert(BoundingBox{
                  bbox.xmin, bbox.xmax, bbox.ymin, bbox.ymax, true, true, true, true});
            }
            return ret;
          }},
      region);
}

Region closure(const Region& region) {
  return std::visit(
      overloaded{[](const Empty& e) -> Region { return e; },
                 [](const Universe& u) -> Region { return u; },
                 [](const BoundingBox& bbox) -> Region {
                   BoundingBox ret{bbox};
                   ret.lopen = false;
                   ret.ropen = false;
                   ret.topen = false;
                   ret.bopen = false;
                   assert(is_closed(ret));
                   return ret;
                 },
                 [](const TopoUnion& region) -> Region {
                   TopoUnion ret{};
                   for (const auto& bbox : region) {
                     ret.insert(BoundingBox{bbox.xmin,
                                            bbox.xmax,
                                            bbox.ymin,
                                            bbox.ymax,
                                            false,
                                            false,
                                            false,
                                            false});
                   }

                   return ret;
                 }},
      region);
}

Region spatial_intersect(const Region& lhs, const Region& rhs) {
  if (std::holds_alternative<Empty>(lhs) || std::holds_alternative<Empty>(rhs)) {
    // Handle either being Empty
    return Empty{};
  }
  // Handle either begin Universe
  if (std::holds_alternative<Universe>(lhs)) { return rhs; }
  if (std::holds_alternative<Universe>(rhs)) { return lhs; }
  if (std::holds_alternative<BoundingBox>(lhs) &&
      std::holds_alternative<BoundingBox>(rhs)) {
    return intersection_of(std::get<BoundingBox>(lhs), std::get<BoundingBox>(rhs));
  }
  // LHS is TopoUnion
  if (const auto topo_lhs_p = std::get_if<TopoUnion>(&lhs)) {
    if (std::holds_alternative<BoundingBox>(rhs)) {
      return intersection_of(*topo_lhs_p, std::get<BoundingBox>(rhs));
    } else {
      return intersection_of(*topo_lhs_p, std::get<TopoUnion>(rhs));
    }
  } else {
    // Else LHS is a BoundingBox
    // Switch on RHS
    if (std::holds_alternative<BoundingBox>(rhs)) {
      return intersection_of(std::get<BoundingBox>(lhs), std::get<BoundingBox>(rhs));
    } else {
      return intersection_of(std::get<TopoUnion>(rhs), std::get<BoundingBox>(lhs));
    }
  }
}

Region spatial_union(const Region& lhs, const Region& rhs) {
  if (std::holds_alternative<Universe>(lhs) || std::holds_alternative<Universe>(rhs)) {
    // Handle either being Universe
    return Universe{};
  }
  // Handle either begin Empty
  if (std::holds_alternative<Empty>(lhs)) { return rhs; }
  if (std::holds_alternative<Empty>(rhs)) { return lhs; }
  if (std::holds_alternative<BoundingBox>(lhs) &&
      std::holds_alternative<BoundingBox>(rhs)) {
    return union_of(std::get<BoundingBox>(lhs), std::get<BoundingBox>(rhs));
  }
  // LHS is TopoUnion
  if (const auto topo_lhs_p = std::get_if<TopoUnion>(&lhs)) {
    if (std::holds_alternative<BoundingBox>(rhs)) {
      return union_of(*topo_lhs_p, std::get<BoundingBox>(rhs));
    } else {
      return union_of(*topo_lhs_p, std::get<TopoUnion>(rhs));
    }
  } else {
    // Else LHS is a BoundingBox
    // Switch on RHS
    if (std::holds_alternative<BoundingBox>(rhs)) {
      // NOTE: This should be redundant...
      return union_of(std::get<BoundingBox>(lhs), std::get<BoundingBox>(rhs));
    } else {
      return union_of(std::get<TopoUnion>(rhs), std::get<BoundingBox>(lhs));
    }
  }
}

} // namespace percemon::topo
