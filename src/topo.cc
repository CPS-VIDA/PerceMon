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

Region union_of(const BoundingBox& a, const BoundingBox& b) {}
Region union_of(const TopoUnion& a, const BoundingBox& b);
Region union_of(const TopoUnion& a, const TopoUnion& b);

Region simplify(const TopoUnion& a);

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
                   double ret = 0;
                   for (auto&& bbox : region) {
                     ret += std::abs((bbox.xmin - bbox.xmax) * (bbox.ymin - bbox.ymax));
                   }
                   return ret;
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

Region simplify_region(const Region& region) {
  return std::visit(
      overloaded{[](const auto& r) -> Region { return r; },
                 [](const TopoUnion& r) {
                   return simplify(r);
                 }},
      region);
}

} // namespace percemon::topo
