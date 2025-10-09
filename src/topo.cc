#include "percemon/topo.hpp"
#include "percemon/exception.hh"

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

Region complement_of(BoundingBox bbox, const BoundingBox& universe) {
  // Check if box is equal to or "larger than" universe.
  if (bbox.xmin <= universe.xmin && bbox.xmax >= universe.xmax &&
      bbox.ymin <= universe.ymin && bbox.ymax >= universe.ymax) {
    return Empty{};
  }
  // Check if box is out of bounds
  {
    auto reg = intersection_of(bbox, universe);
    if (std::holds_alternative<Empty>(reg)) { return Universe{}; }
    bbox = std::get<BoundingBox>(reg);
  }
  // Holds Top, Bottom, Left, and Right fragments of complement
  std::vector<BoundingBox> fragments;

  // Get left fragment
  if (bbox.xmin > universe.xmin || (bbox.xmin == universe.xmin && bbox.lopen)) {
    fragments.emplace_back(
        BoundingBox{
            universe.xmin,
            bbox.xmin,
            bbox.ymin,
            bbox.ymax,
            false,
            !bbox.lopen,
            bbox.topen,
            bbox.bopen});
  }
  // Get right fragment
  if (bbox.xmax < universe.xmax || (bbox.xmax == universe.xmax && bbox.ropen)) {
    fragments.emplace_back(
        BoundingBox{
            bbox.xmax,
            universe.xmax,
            bbox.ymin,
            bbox.ymax,
            !bbox.ropen,
            false,
            bbox.topen,
            bbox.bopen});
  }
  // Get top fragment
  if (bbox.ymin > universe.ymin || (bbox.ymin == universe.ymin && bbox.topen)) {
    fragments.emplace_back(
        BoundingBox{
            universe.xmin,
            universe.xmax,
            universe.ymin,
            bbox.ymin,
            false,
            false,
            false,
            !bbox.topen});
  }
  // Get bottom fragment
  if (bbox.ymax < universe.ymax || (bbox.ymax == universe.ymax && bbox.bopen)) {
    fragments.emplace_back(
        BoundingBox{
            universe.xmin,
            universe.xmax,
            bbox.ymax,
            universe.ymax,
            false,
            false,
            !bbox.bopen,
            false});
  }

  return TopoUnion{std::begin(fragments), std::end(fragments)};
}

Region complement_of(const TopoUnion& region, const BoundingBox& universe) {
  auto ret = TopoUnion{};
  for (auto&& box : region) {
    auto comp_box = complement_of(box, universe);
    if (std::holds_alternative<Universe>(comp_box)) {
      return Universe{}; // Since TopoUnion is the union of regions.
    } else if (std::holds_alternative<Empty>(comp_box)) {
      // Since this implies that the TopoUnion was Universe to begin with as atleast 1
      // bounding box in the union was Universe.
      // Therefore complement must be completely empty.
      // TODO: verify.
      return Empty{};
    }
    ret.merge(std::get<TopoUnion>(comp_box));
  }
  return ret;
}

/**
 * Helper class to convert TopoUnion to a set of disjoint rectangles.
 *
 * Adapted from:
 * https://codercareer.blogspot.com/2011/12/no-27-area-of-rectangles.html
 */
struct TopoSimplify {
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
    constexpr bool operator()(const BoundingBox& a, const BoundingBox& b) const {
      return a.xmin < b.xmin;
    }
  };

  Region operator()(const Empty& e) { return e; }
  Region operator()(const Universe& e) { return e; }
  Region operator()(const BoundingBox& bbox) { return bbox; }

  std::set<double> get_all_xs(const std::set<BoundingBox, BoundOrder>& rects) {
    auto ret = std::set<double>{};
    for (auto&& r : rects) {
      ret.insert(r.xmin);
      ret.insert(r.xmax);
    }
    return ret;
  }

  template <typename RectIter>
  std::vector<Interval>
  get_y_ranges(RectIter start, RectIter end, const Interval& x_range) {
    static_assert(std::is_same_v<BoundingBox, std::decay_t<decltype(*start)>>);

    auto ret = std::vector<Interval>{};

    auto& it = start;
    for (; it != end; ++it) {
      if (x_range.low < it->xmax && x_range.high > it->xmin) {
        // Current rectangle is within the range.
        auto y_range = Interval{it->ymin, it->ymax};
        if (ret.empty()) {
          // Add it if there is nothing in the ranges yet.
          ret.push_back(y_range);
        } else { // Else, try to accumulate the ranges.
          auto tmp = std::vector<Interval>{};
          for (auto&& rg : ret) {
            if (y_range.is_overlapping(rg)) {
              y_range.merge(rg);
            } else {
              tmp.push_back(rg);
            }
          }
          tmp.push_back(y_range);
          ret = tmp;
        }
      }
    }

    return ret;
  }

  Region operator()(const TopoUnion& region) {
    std::set<BoundingBox, BoundOrder> rects{std::begin(region), std::end(region)};
    std::set<double> x_margins = get_all_xs(rects);

    std::vector<BoundingBox> ret;

    auto iter = rects.cbegin();
    for (auto xpair : iter::sliding_window(x_margins, 2)) {
      double x1 = xpair.at(0), x2 = xpair.at(1);
      Interval x_int{x1, x2};
      while (iter->xmax < x1) ++iter; // This step is like interval scheduling.
      // Hold intervals of Y that correspond to the current X interval.
      std::vector<Interval> y_intervals = get_y_ranges(iter, rects.cend(), x_int);
      // Merge the corresponding ranges to BoundingBoxes
      // TODO: Open Closed Bounds?
      for (auto&& y_int : y_intervals) {
        ret.emplace_back(BoundingBox{x_int.low, x_int.high, y_int.low, y_int.high});
      }
    }

    return TopoUnion{ret.begin(), ret.end()};
  }
};

} // namespace

namespace percemon::topo {

inline bool is_closed(const Region& region) {
  return std::visit(
      overloaded{
          [](const Empty&) { return true; },
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
      overloaded{
          [](const Empty&) { return true; },
          [](const Universe&) { return true; },
          [](const BoundingBox& bbox) {
            return bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen;
          },
          [](const TopoUnion& region) {
            // If even one sub region is open, the entire thing should be open.
            for (auto&& bbox : region) {
              if (bbox.lopen || bbox.ropen || bbox.topen || bbox.bopen) { return true; }
            }
            return false;
          }},
      region);
}

double area(const Region& region) {
  auto reg = simplify_region(region);
  return std::visit(
      overloaded{
          [](const Empty&) -> double { return 0.0; },
          [](const Universe&) -> double {
            return std::numeric_limits<double>::infinity();
          },
          [](const BoundingBox& bbox) -> double {
            return std::abs((bbox.xmin - bbox.xmax) * (bbox.ymin - bbox.ymax));
          },
          [](const TopoUnion& topo_un) -> double {
            double region_area = 0;
            for (auto&& bbox : topo_un) {
              region_area +=
                  std::abs((bbox.xmin - bbox.xmax) * (bbox.ymin - bbox.ymax));
            }
            return region_area;
          }},
      reg);
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
              ret.insert(
                  BoundingBox{
                      bbox.xmin,
                      bbox.xmax,
                      bbox.ymin,
                      bbox.ymax,
                      true,
                      true,
                      true,
                      true});
            }
            return ret;
          }},
      region);
}

Region closure(const Region& region) {
  return std::visit(
      overloaded{
          [](const Empty& e) -> Region { return e; },
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
              ret.insert(
                  BoundingBox{
                      bbox.xmin,
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

Region spatial_complement(const Region& region, const BoundingBox& universe) {
  return std::visit(
      utils::overloaded{
          [](const Empty&) -> Region { return Universe{}; },
          [](const Universe&) -> Region { return Empty{}; },
          [&](const BoundingBox& bb) -> Region { return complement_of(bb, universe); },
          [&](const TopoUnion& t) -> Region {
            return complement_of(t, universe);
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

Region spatial_intersect(const std::vector<Region>& regions) {
  Region ret = Empty{};
  for (auto&& reg : regions) { ret = spatial_intersect(ret, reg); }
  return ret;
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

Region spatial_union(const std::vector<Region>& regions) {
  Region ret = Empty{};
  for (auto&& reg : regions) { ret = spatial_union(ret, reg); }
  return ret;
}

Region simplify_region(const Region& region) {
  auto comp = TopoSimplify{};
  return std::visit(comp, region);
}

} // namespace percemon::topo
