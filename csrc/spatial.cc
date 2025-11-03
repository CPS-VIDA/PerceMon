#include "percemon/spatial.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <utility>
#include <vector>

namespace ranges = std::ranges;

namespace percemon::spatial {

// =============================================================================
// BBox Implementation
// =============================================================================

BBox::BBox(
    double xmin_,
    double xmax_,
    double ymin_,
    double ymax_,
    bool lopen_,
    bool ropen_,
    bool topen_,
    bool bopen_) :
    xmin(xmin_),
    xmax(xmax_),
    ymin(ymin_),
    ymax(ymax_),
    lopen(lopen_),
    ropen(ropen_),
    topen(topen_),
    bopen(bopen_) {}

BBox::BBox(const datastream::BoundingBox& bbox) :
    xmin(bbox.xmin), xmax(bbox.xmax), ymin(bbox.ymin), ymax(bbox.ymax) {}

[[nodiscard]] auto BBox::to_string() const -> std::string {
  std::string left   = lopen ? std::format("({}", xmin) : std::format("[{}", xmin);
  std::string right  = ropen ? std::format("{})", xmax) : std::format("{}]", xmax);
  std::string top    = topen ? std::format("({}", ymin) : std::format("[{}", ymin);
  std::string bottom = bopen ? std::format("{})", ymax) : std::format("{}]", ymax);
  return std::format("<{}, {} x {}, {}>", left, right, top, bottom);
}

[[nodiscard]] auto BBox::area() const -> double { return (xmax - xmin) * (ymax - ymin); }

[[nodiscard]] auto BBox::is_closed() const -> bool { return !(lopen || ropen || topen || bopen); }

[[nodiscard]] auto BBox::is_open() const -> bool { return lopen || ropen || topen || bopen; }

// =============================================================================
// Union Implementation
// =============================================================================

// constexpr auto Union::BBoxCmp::operator()(const BBox& a, const BBox& b) const -> bool {
//   return a.xmin < b.xmin;
// }

[[nodiscard]] auto Union::to_string() const -> std::string {
  std::string ret   = "[";
  const size_t last = this->size() - 1;
  size_t idx        = 0;
  for (const auto& box : this->regions) { ret.append(box.to_string() + (idx == last ? "" : ", ")); }
  ret += "]";
  return ret;
}

void Union::insert(const BBox& bbox) { regions.insert(bbox); }

void Union::merge(const Union& other) {
  for (const auto& bbox : other.regions) { regions.insert(bbox); }
}

// =============================================================================
// Helper Functions for Spatial Operations
// =============================================================================

namespace {

// Compute intersection of two bounding boxes
auto intersection_of(const BBox& a, const BBox& b) -> Region {
  // For the new BBOx, we compute the new bounds by taking the max of {a,b}.mins and min
  // of {a,b}.maxs. Depending on who wins, we will be determining the ended ness.
  // Moreover, if the values are equal, we need to take into account for the ended ness.
  const auto max_op = [](double i, bool iopen, double j, bool jopen) {
    if (i > j) { return std::pair{i, iopen}; }
    if (i < j) { return std::pair{j, jopen}; }
    // This is the annoying case. Since they are equal, the interval is closed iff both
    // are closed, i.e., if either of them are open, the max must be open.
    return std::pair{i, iopen || jopen};
  };
  const auto min_op = [](double i, bool iopen, double j, bool jopen) {
    if (i < j) { return std::pair{i, iopen}; }
    if (i > j) { return std::pair{j, jopen}; }
    // This is the annoying case. Since they are equal, the interval is closed iff both
    // are closed, i.e., if either of them are open, the min must be open.
    return std::pair{i, iopen || jopen};
  };

  auto [xmin, lopen] = max_op(a.xmin, a.lopen, b.xmin, b.lopen);
  auto [xmax, ropen] = min_op(a.xmax, a.ropen, b.xmax, b.ropen);
  auto [ymin, topen] = max_op(a.ymin, a.topen, b.ymin, b.topen);
  auto [ymax, bopen] = min_op(a.ymax, a.bopen, b.ymax, b.bopen);

  // Now, we need to check if the new BBox will be empty because xmax <= xmin || ymax
  // <= ymin
  if (xmin >= xmax) { return Empty{}; }
  if (ymin >= ymax) { return Empty{}; }
  return BBox{xmin, xmax, ymin, ymax, lopen, ropen, topen, bopen};
}

// Compute intersection of Union and BBox
auto intersection_of(const Union& a, const BBox& b) -> Region {
  auto intersect_set = std::vector<BBox>{};
  for (const BBox& bbox : a) {
    auto int_box = intersection_of(bbox, b);
    if (auto* box_p = std::get_if<BBox>(&int_box)) { intersect_set.push_back(*box_p); }
  }
  if (intersect_set.empty()) { return Empty{}; }
  if (intersect_set.size() == 1) { return intersect_set.back(); }
  return Union{intersect_set.begin(), intersect_set.end()};
}

// Compute intersection of two Unions
auto intersection_of(const Union& lhs, const Union& rhs) -> Region {
  auto intersect_set = std::vector<BBox>{};
  for (const BBox& a : lhs) {
    for (const BBox& b : rhs) {
      auto int_box = intersection_of(a, b);
      if (auto* box_p = std::get_if<BBox>(&int_box)) { intersect_set.push_back(*box_p); }
    }
  }
  if (intersect_set.empty()) { return Empty{}; }
  if (intersect_set.size() == 1) { return intersect_set.back(); }
  return Union{intersect_set.begin(), intersect_set.end()};
}

// Compute union of two bounding boxes
auto union_of(const BBox& a, const BBox& b) -> Region {
  // Check if one box is inside the other
  if ((a.xmin <= b.xmin && b.xmax <= a.xmax) && (a.ymin <= b.ymin && b.ymax <= a.ymax)) {
    // b is inside a
    bool lopen = (a.xmin == b.xmin) ? (a.lopen && b.lopen) : a.lopen;
    bool ropen = (a.xmax == b.xmax) ? (a.ropen && b.ropen) : a.ropen;
    bool topen = (a.ymin == b.ymin) ? (a.topen && b.topen) : a.topen;
    bool bopen = (a.ymax == b.ymax) ? (a.bopen && b.bopen) : a.bopen;
    return BBox{a.xmin, a.xmax, a.ymin, a.ymax, lopen, ropen, topen, bopen};
  }
  if ((b.xmin <= a.xmin && a.xmax <= b.xmax) && (b.ymin <= a.ymin && a.ymax <= b.ymax)) {
    // a is inside b
    bool lopen = (b.xmin == a.xmin) ? (b.lopen && a.lopen) : b.lopen;
    bool ropen = (b.xmax == a.xmax) ? (b.ropen && a.ropen) : b.ropen;
    bool topen = (b.ymin == a.ymin) ? (b.topen && a.topen) : b.topen;
    bool bopen = (b.ymax == a.ymax) ? (b.bopen && a.bopen) : b.bopen;
    return BBox{b.xmin, b.xmax, b.ymin, b.ymax, lopen, ropen, topen, bopen};
  }

  // Return a union of the boxes
  Union ret;
  ret.insert(a);
  ret.insert(b);
  return ret;
}

// Compute union of Union and BBox
auto union_of(const Union& a, const BBox& b) -> Region {
  Union ret = a;
  ret.insert(b);
  return ret;
}

// Compute union of two Unions
auto union_of(const Union& a, const Union& b) -> Region {
  Union ret = a;
  ret.merge(b);
  return ret;
}

// Compute complement of a single BBox
auto complement_of(BBox bbox, const BBox& universe) -> Region {
  // Check if box is equal to or "larger than" universe
  if (bbox.xmin <= universe.xmin && bbox.xmax >= universe.xmax && bbox.ymin <= universe.ymin &&
      bbox.ymax >= universe.ymax) {
    return Empty{};
  }

  // Check if box is out of bounds
  {
    auto reg = intersection_of(bbox, universe);
    if (std::holds_alternative<Empty>(reg)) { return Universe{}; }
    bbox = std::get<BBox>(reg);
  }

  // Holds Top, Bottom, Left, and Right fragments of complement
  std::vector<BBox> fragments;

  // Get left fragment
  if (bbox.xmin > universe.xmin || (bbox.xmin == universe.xmin && bbox.lopen)) {
    fragments.emplace_back(
        universe.xmin, bbox.xmin, bbox.ymin, bbox.ymax, false, !bbox.lopen, bbox.topen, bbox.bopen);
  }

  // Get right fragment
  if (bbox.xmax < universe.xmax || (bbox.xmax == universe.xmax && bbox.ropen)) {
    fragments.emplace_back(
        bbox.xmax, universe.xmax, bbox.ymin, bbox.ymax, !bbox.ropen, false, bbox.topen, bbox.bopen);
  }

  // Get top fragment
  if (bbox.ymin > universe.ymin || (bbox.ymin == universe.ymin && bbox.topen)) {
    fragments.emplace_back(
        universe.xmin, universe.xmax, universe.ymin, bbox.ymin, false, false, false, !bbox.topen);
  }

  // Get bottom fragment
  if (bbox.ymax < universe.ymax || (bbox.ymax == universe.ymax && bbox.bopen)) {
    fragments.emplace_back(
        universe.xmin, universe.xmax, bbox.ymax, universe.ymax, false, false, !bbox.bopen, false);
  }

  return Union{fragments.begin(), fragments.end()};
}

// Compute complement of a Union
auto complement_of(const Union& region, const BBox& universe) -> Region {
  Union ret;
  for (const auto& box : region) {
    auto comp_box = complement_of(box, universe);
    if (std::holds_alternative<Universe>(comp_box)) {
      return Universe{};
    } else if (std::holds_alternative<Empty>(comp_box)) {
      // Do nothing
    } else if (auto* union_p = std::get_if<Union>(&comp_box)) {
      ret.merge(*union_p);
    } else {
      ret.insert(std::get<BBox>(comp_box));
    }
  }
  return ret;
}

// Helper for simplifying regions
struct TopoSimplify {
  struct Interval {
    double low, high;

    Interval(double l, double h) : low((l < h) ? l : h), high((l + h) - low) {}

    [[nodiscard]] constexpr auto is_overlapping(const Interval& other) const -> bool {
      return low <= other.high && other.low <= high;
    }

    void merge(const Interval& other) {
      if (is_overlapping(other)) {
        low  = std::min(low, other.low);
        high = std::max(high, other.high);
      }
    }
  };

  struct BBoxOrder {
    constexpr auto operator()(const BBox& a, const BBox& b) const -> bool {
      return a.xmin < b.xmin;
    }
  };

  auto operator()(const Empty& e) -> Region { return e; }
  auto operator()(const Universe& u) -> Region { return u; }
  auto operator()(const BBox& bbox) -> Region { return bbox; }

  [[nodiscard]] static auto get_all_xs(const std::set<BBox, BBoxOrder>& rects) -> std::set<double> {
    auto ret = std::set<double>{};
    for (const auto& r : rects) {
      ret.insert(r.xmin);
      ret.insert(r.xmax);
    }
    return ret;
  }

  [[nodiscard]] static auto get_y_ranges(
      const std::set<BBox, BBoxOrder>::const_iterator& start,
      const std::set<BBox, BBoxOrder>::const_iterator& end,
      const Interval& x_range) -> std::vector<Interval> {
    auto ret = std::vector<Interval>{};

    for (auto it = start; it != end; ++it) {
      if (x_range.low < it->xmax && x_range.high > it->xmin) {
        auto y_range = Interval{it->ymin, it->ymax};
        if (ret.empty()) {
          ret.push_back(y_range);
        } else {
          auto tmp = std::vector<Interval>{};
          for (auto& rg : ret) {
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

  auto operator()(const Union& region) -> Region {
    std::set<BBox, BBoxOrder> rects{region.begin(), region.end()};
    std::set<double> x_margins = get_all_xs(rects);

    std::vector<BBox> ret;

    auto iter          = rects.cbegin();
    auto x_margins_vec = std::vector<double>{x_margins.begin(), x_margins.end()};

    for (size_t i = 0; i + 1 < x_margins_vec.size(); ++i) {
      double x1 = x_margins_vec[i];
      double x2 = x_margins_vec[i + 1];
      Interval x_int{x1, x2};

      while (iter != rects.cend() && iter->xmax < x1) { ++iter; }

      std::vector<Interval> y_intervals = get_y_ranges(iter, rects.cend(), x_int);
      for (const auto& y_int : y_intervals) {
        ret.emplace_back(x_int.low, x_int.high, y_int.low, y_int.high);
      }
    }

    if (ret.empty()) { return Empty{}; }
    if (ret.size() == 1) { return ret.back(); }
    return Union{ret.begin(), ret.end()};
  }
};

} // namespace

// =============================================================================
// Public API Functions
// =============================================================================

auto is_open(const Region& region) -> bool {
  return std::visit(
      utils::overloaded{
          [](const BBox& box) { return box.is_open(); },
          [](const Union& r) {
            return ranges::any_of(r, [](const auto& bbox) { return bbox.is_open(); });
          },
          [](const auto&) {
            return true;
          }},
      region);
}
auto is_closed(const Region& region) -> bool {
  return std::visit(
      utils::overloaded{
          [](const BBox& box) { return box.is_closed(); },
          [](const Union& r) {
            return ranges::all_of(r, [](const auto& bbox) { return bbox.is_closed(); });
          },
          [](const auto&) {
            return true;
          }},
      region);
}

auto area(const Region& region) -> double {
  auto simplified = simplify(region);
  return std::visit(
      [](const auto& r) -> double {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Empty>) {
          return 0.0;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Universe>) {
          return std::numeric_limits<double>::infinity();
        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, BBox>) {
          return std::abs((r.xmax - r.xmin) * (r.ymax - r.ymin));
        } else { // Union
          double total = 0.0;
          for (const auto& bbox : r) {
            total += std::abs((bbox.xmax - bbox.xmin) * (bbox.ymax - bbox.ymin));
          }
          return total;
        }
      },
      simplified);
}

auto interior(const Region& region) -> Region {
  return std::visit(
      [](const auto& r) -> Region {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, BBox>) {
          return BBox{r.xmin, r.xmax, r.ymin, r.ymax, true, true, true, true};
        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Union>) {
          Union ret;
          for (const auto& bbox : r) {
            ret.insert(BBox{bbox.xmin, bbox.xmax, bbox.ymin, bbox.ymax, true, true, true, true});
          }
          return ret;
        } else { // Universe, Empty
          return r;
        }
      },
      region);
}

auto closure(const Region& region) -> Region {
  return std::visit(
      [](const auto& r) -> Region {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, BBox>) {
          return BBox{r.xmin, r.xmax, r.ymin, r.ymax, false, false, false, false};
        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Union>) {
          Union ret;
          for (const auto& bbox : r) {
            ret.insert(
                BBox{bbox.xmin, bbox.xmax, bbox.ymin, bbox.ymax, false, false, false, false});
          }
          return ret;
        } else {
          // Universe and Empty
          return r;
        }
      },
      region);
}

auto complement(const Region& region, const BBox& universe) -> Region {
  return std::visit(
      [&](const auto& r) -> Region {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Empty>) {
          return Universe{};
        } else if constexpr (std::is_same_v<std::decay_t<decltype(r)>, Universe>) {
          return Empty{};
        } else {
          return complement_of(r, universe);
        }
      },
      region);
}

auto intersect(const Region& lhs, const Region& rhs) -> Region {
  if (std::holds_alternative<Empty>(lhs) || std::holds_alternative<Empty>(rhs)) { return Empty{}; }
  if (std::holds_alternative<Universe>(lhs)) { return rhs; }
  if (std::holds_alternative<Universe>(rhs)) { return lhs; }

  if (std::holds_alternative<BBox>(lhs) && std::holds_alternative<BBox>(rhs)) {
    return intersection_of(std::get<BBox>(lhs), std::get<BBox>(rhs));
  }

  if (const auto* union_lhs = std::get_if<Union>(&lhs)) {
    if (std::holds_alternative<BBox>(rhs)) {
      return intersection_of(*union_lhs, std::get<BBox>(rhs));
    } else {
      return intersection_of(*union_lhs, std::get<Union>(rhs));
    }
  } else {
    const auto* union_rhs = std::get_if<Union>(&rhs);
    if (union_rhs != nullptr) { return intersection_of(*union_rhs, std::get<BBox>(lhs)); }
  }

  return Empty{};
}

auto intersect(const std::vector<Region>& regions) -> Region {
  if (regions.empty()) { return Universe{}; }
  Region ret = regions[0];
  for (size_t i = 1; i < regions.size(); ++i) { ret = intersect(ret, regions[i]); }
  return ret;
}

auto union_of(const Region& lhs, const Region& rhs) -> Region {
  if (std::holds_alternative<Universe>(lhs) || std::holds_alternative<Universe>(rhs)) {
    return Universe{};
  }
  if (std::holds_alternative<Empty>(lhs)) { return rhs; }
  if (std::holds_alternative<Empty>(rhs)) { return lhs; }

  if (std::holds_alternative<BBox>(lhs) && std::holds_alternative<BBox>(rhs)) {
    return union_of(std::get<BBox>(lhs), std::get<BBox>(rhs));
  }

  if (const auto* union_lhs = std::get_if<Union>(&lhs)) {
    if (std::holds_alternative<BBox>(rhs)) {
      return union_of(*union_lhs, std::get<BBox>(rhs));
    } else {
      return union_of(*union_lhs, std::get<Union>(rhs));
    }
  } else {
    const auto* union_rhs = std::get_if<Union>(&rhs);
    if (union_rhs != nullptr) { return union_of(*union_rhs, std::get<BBox>(lhs)); }
  }

  return Empty{};
}

auto union_of(const std::vector<Region>& regions) -> Region {
  if (regions.empty()) { return Empty{}; }
  Region ret = regions[0];
  for (size_t i = 1; i < regions.size(); ++i) { ret = union_of(ret, regions[i]); }
  return ret;
}

auto simplify(const Region& region) -> Region {
  auto visitor = TopoSimplify{};
  return std::visit(visitor, region);
}

} // namespace percemon::spatial
