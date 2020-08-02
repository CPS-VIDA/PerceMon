/**
 * General topological abstractons
 */

#pragma once

#ifndef __PERCEMON_TOPO_HPP__
#define __PERCEMON_TOPO_HPP__

#include <cstddef>
#include <set>

#include "percemon/datastream.hpp"

namespace percemon::topo {
// TODO: Make this more abstract and able to handle general sets.

/**
 * A bounding box data structure that follows the Pascal VOC Bounding box format
 * (x-top left, y-top left,x-bottom right, y-bottom right), where each
 * coordinate is in terms of number of pixels.
 */
struct BoundingBox {
  double xmin;
  double xmax;
  double ymin;
  double ymax;

  // Are left, right, top, or bottom boundaries open?
  bool lopen = false;
  bool ropen = false;
  bool topen = false;
  bool bopen = false;

  BoundingBox(const BoundingBox&) = default;
  BoundingBox(BoundingBox&&)      = default;
  BoundingBox& operator=(const BoundingBox&) = default;
  BoundingBox& operator=(BoundingBox&&) = default;

  BoundingBox(percemon::datastream::BoundingBox bbox) :
      xmin{static_cast<double>(bbox.xmin)},
      xmax{static_cast<double>(bbox.xmax)},
      ymin{static_cast<double>(bbox.ymin)},
      ymax{static_cast<double>(bbox.ymax)} {};

  BoundingBox& operator=(percemon::datastream::BoundingBox bbox) {
    xmin = bbox.xmin;
    xmax = bbox.xmax;
    ymin = bbox.ymin;
    ymax = bbox.ymax;
    return *this;
  }
};

/**
 * Spatial region.
 *
 * Can be used to define unions of other topological spaces
 */
struct SpatialRegion {
  struct BBoxCmp {
    bool operator()(const BoundingBox& a, const BoundingBox& b) {
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
  using Set = std::set<BoundingBox, BBoxCmp>;

  Set regions;
};

} // namespace percemon::topo

#endif /* end of include guard: __PERCEMON_TOPO_HPP__ */
