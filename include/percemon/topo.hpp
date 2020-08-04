/**
 * General topological abstractons
 */

#pragma once

#ifndef __PERCEMON_TOPO_HPP__
#define __PERCEMON_TOPO_HPP__

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <set>
#include <variant>
#include <vector>

#include "percemon/datastream.hpp"

/**
 * A topological space is either a Universe set, an Empty set, a BoundingBox, or a Union
 * of BoundingBoxes.
 *
 * Each of these types must have defined the following functions:
 *
 * 1. Area.
 * 2. Closure
 * 3. Interior.
 */

namespace percemon::topo {
// TODO: Make this more abstract and able to handle general sets.

struct Empty;
struct Universe;
struct BoundingBox;
struct TopoUnion;

struct Empty {};

struct Universe {};

struct BoundingBox {
  // NOTE: The origin in an image is the top left corner.
  double xmin;
  double xmax;
  double ymin;
  double ymax;

  // Are left, right, top, or bottom boundaries open?
  bool lopen = false;
  bool ropen = false;
  bool topen = false;
  bool bopen = false;

  BoundingBox()                   = delete;
  BoundingBox(const BoundingBox&) = default;
  BoundingBox(BoundingBox&&)      = default;
  BoundingBox& operator=(const BoundingBox&) = default;
  BoundingBox& operator=(BoundingBox&&) = default;

  BoundingBox(
      double xmin_,
      double xmax_,
      double ymin_,
      double ymax_,
      bool lopen_ = false,
      bool ropen_ = false,
      bool topen_ = false,
      bool bopen_ = false) :
      xmin{xmin_},
      xmax{xmax_},
      ymin{ymin_},
      ymax{ymax_},
      lopen{lopen_},
      ropen{ropen_},
      topen{topen_},
      bopen{bopen_} {};

  BoundingBox(percemon::datastream::BoundingBox bbox) :
      BoundingBox{static_cast<double>(bbox.xmin),
                  static_cast<double>(bbox.xmax),
                  static_cast<double>(bbox.ymin),
                  static_cast<double>(bbox.ymax)} {};

  BoundingBox& operator=(percemon::datastream::BoundingBox bbox) {
    xmin = bbox.xmin;
    xmax = bbox.xmax;
    ymin = bbox.ymin;
    ymax = bbox.ymax;
    return *this;
  }
};

struct TopoUnion {
  struct BBoxCmp {
    constexpr bool operator()(const BoundingBox& a, const BoundingBox& b) const {
      // TODO: Handling of open and closed sets?
      if (a.xmin < b.xmin) { return true; }
      if (a.xmin == b.xmin) {
        if (a.xmax < b.xmax) { return true; }
        if (a.xmax == b.xmax) {
          if (a.ymin < b.ymin) { return true; }
          if (a.ymin == b.ymin) {
            if (a.ymax < b.ymax) { return true; }
          }
        }
      }
      return false;
    }
  };

  using Set             = std::set<BoundingBox, BBoxCmp>;
  using value_type      = BoundingBox;
  using reference       = BoundingBox&;
  using const_reference = const BoundingBox&;
  using iterator        = Set::iterator;
  using const_iterator  = Set::const_iterator;
  using difference_type = Set::difference_type;
  using size_type       = Set::size_type;

  TopoUnion()                 = default;
  TopoUnion(const TopoUnion&) = default;
  TopoUnion(TopoUnion&&)      = default;
  TopoUnion& operator=(const TopoUnion&) = default;
  TopoUnion& operator=(TopoUnion&&) = default;

  template <typename Iter>
  TopoUnion(Iter first, Iter last) : regions{first, last} {};

  void insert(BoundingBox bbox) { regions.insert(bbox); }
  void merge(const TopoUnion& other) {
    std::merge(
        regions.begin(),
        regions.end(),
        other.begin(),
        other.end(),
        std::inserter(regions, regions.begin()),
        BBoxCmp{});
  }

  iterator begin() { return regions.begin(); }
  [[nodiscard]] const_iterator begin() const { return regions.begin(); }
  iterator end() { return regions.end(); }
  [[nodiscard]] const_iterator end() const { return regions.end(); }

  auto size() { return regions.size(); }

 private:
  Set regions;
};

using Region    = std::variant<Empty, Universe, BoundingBox, TopoUnion>;
using RegionPtr = std::shared_ptr<Region>;

bool is_closed(const Region& region);
bool is_open(const Region& region);
double area(const Region& region);
Region interior(const Region& region);
Region closure(const Region& region);
Region spatial_complement(const Region& region, const BoundingBox& universe);
Region spatial_intersect(const Region& lhs, const Region& rhs);
Region spatial_intersect(const std::vector<Region>&);
Region spatial_union(const Region& lhs, const Region& rhs);
Region spatial_union(const std::vector<Region>&);

Region simplify_region(const Region& region);

} // namespace percemon::topo

#endif /* end of include guard: __PERCEMON_TOPO_HPP__ */
