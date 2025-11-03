#pragma once

#ifndef PERCEMON_SPATIAL_HPP
#define PERCEMON_SPATIAL_HPP

#include "percemon/datastream.hpp"
#include <set>
#include <variant>
#include <vector>

/**
 * @file spatial.hpp
 * @brief Topological spatial abstractions for STQL spatial expressions
 *
 * This module provides the computational layer for spatial operations used
 * when evaluating STQL formulas. It implements set-theoretic and topological
 * operations on 2D spatial regions.
 *
 * A spatial Region can be:
 * - Empty: ∅ (empty set)
 * - Universe: U (entire space)
 * - BBox: A single rectangular region (possibly with open boundaries)
 * - Union: A union of multiple bounding boxes
 *
 * These are used to compute the semantics of STQL spatial expressions like:
 * - BB(id): Bounding box of object
 * - Ω₁ ⊔ Ω₂: Spatial union
 * - Ω₁ ⊓ Ω₂: Spatial intersection
 * - Ω̅: Spatial complement
 */

namespace percemon::spatial {

// Forward declarations
struct Empty;
struct Universe;
struct BBox;
struct Union;

// =============================================================================
// Core Region Types
// =============================================================================

/**
 * @brief Empty spatial set (∅).
 *
 * Represents the empty set in spatial expressions.
 * - Area = 0
 * - Identity for union: ∅ ⊔ Ω = Ω
 * - Annihilator for intersection: ∅ ⊓ Ω = ∅
 */
struct Empty {
  auto operator<=>(const Empty&) const = default;

  [[nodiscard]] auto to_string() const -> std::string { return "Empty"; }
};

/**
 * @brief Universal spatial set (U).
 *
 * Represents the entire spatial domain (all points).
 * - Area = ∞
 * - Identity for intersection: U ⊓ Ω = Ω
 * - Annihilator for union: U ⊔ Ω = U
 */
struct Universe {
  auto operator<=>(const Universe&) const = default;

  [[nodiscard]] auto to_string() const -> std::string { return "Universe"; }
};

/**
 * @brief Single rectangular spatial region with open/closed boundaries.
 *
 * Represents a 2D rectangular region that may have open or closed boundaries.
 * Used for precise topological operations (interior, closure).
 *
 * Boundary flags:
 * - lopen = true: left boundary (x = xmin) is open (not included)
 * - ropen = true: right boundary (x = xmax) is open
 * - topen = true: top boundary (y = ymin) is open
 * - bopen = true: bottom boundary (y = ymax) is open
 *
 * Example:
 * @code
 * // Closed box: [100, 200] × [50, 150]
 * auto closed = BBox{100, 200, 50, 150};
 *
 * // Open box: (100, 200) × (50, 150)
 * auto open = BBox{100, 200, 50, 150, true, true, true, true};
 * @endcode
 */
struct BBox {
  double xmin, xmax;
  double ymin, ymax;

  bool lopen = false; ///< Left boundary open?
  bool ropen = false; ///< Right boundary open?
  bool topen = false; ///< Top boundary open?
  bool bopen = false; ///< Bottom boundary open?

  BBox(
      double xmin_,
      double xmax_,
      double ymin_,
      double ymax_,
      bool lopen_ = false,
      bool ropen_ = false,
      bool topen_ = false,
      bool bopen_ = false);

  /// Construct from datastream BoundingBox (always closed)
  explicit BBox(const datastream::BoundingBox& bbox);

  auto operator<=>(const BBox&) const = default;

  /// Compute area (ignores open/closed boundaries)
  [[nodiscard]] auto area() const -> double;

  /// Check if all boundaries are closed
  [[nodiscard]] auto is_closed() const -> bool;

  /// Check if any boundary is open
  [[nodiscard]] auto is_open() const -> bool;

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief Union of multiple bounding boxes.
 *
 * Represents Ω₁ ⊔ Ω₂ ⊔ ... ⊔ Ωₙ as a collection of bounding boxes.
 * Stored as a sorted set to enable efficient operations.
 *
 * Note: Boxes may overlap. Use simplify() to get disjoint representation.
 *
 * Example:
 * @code
 * auto u = Union{};
 * u.insert(BBox{100, 200, 50, 150});
 * u.insert(BBox{180, 280, 100, 200});
 * // Now u represents the union of these two boxes
 * @endcode
 */
struct Union {
  /// Comparator for sorting bounding boxes
  struct BBoxCmp {
    constexpr auto operator()(const BBox& a, const BBox& b) const -> bool {
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

  using Set            = std::set<BBox, BBoxCmp>;
  using iterator       = Set::iterator;
  using const_iterator = Set::const_iterator;

  Union() = default;

  /// Construct from iterator range
  template <typename Iter>
  Union(Iter first, Iter last) : regions{first, last} {}

  /// Construct from initializer list
  Union(std::initializer_list<BBox> boxes) : regions{boxes} {}

  /// Insert a bounding box
  void insert(const BBox& bbox);

  /// Merge another union into this one
  void merge(const Union& other);

  /// Iterators
  auto begin() -> iterator { return regions.begin(); }
  [[nodiscard]] auto begin() const -> const_iterator { return regions.begin(); }
  auto end() -> iterator { return regions.end(); }
  [[nodiscard]] auto end() const -> const_iterator { return regions.end(); }

  /// Number of boxes in union
  [[nodiscard]] auto size() const -> size_t { return regions.size(); }

  /// Check if empty
  [[nodiscard]] auto empty() const -> bool { return regions.empty(); }

  [[nodiscard]] auto to_string() const -> std::string;

 private:
  Set regions;
};

/**
 * @brief Discriminated union of all spatial region types.
 *
 * A Region is one of: Empty, Universe, BBox, or Union.
 * Use std::visit to pattern match on the type.
 *
 * Example:
 * @code
 * Region r = BBox{100, 200, 50, 150};
 * double a = area(r);  // Calls area() visitor
 * @endcode
 */
using Region = std::variant<Empty, Universe, BBox, Union>;

/**
 * @brief Utility function to print arbitrary regions
 */
inline auto to_string(const Region& region) -> std::string {
  return std::visit([](const auto& r) { return r.to_string(); }, region);
}

// =============================================================================
// Topological Operations
// =============================================================================

/**
 * @brief Check if region is closed (all boundaries included).
 *
 * - Empty and Universe are always closed
 * - BBox is closed if all boundaries are closed
 * - Union is closed if all sub-regions are closed
 */
auto is_closed(const Region& region) -> bool;

/**
 * @brief Check if region is open (has at least one open boundary).
 *
 * - Empty and Universe are vacuously open
 * - BBox is open if any boundary is open
 * - Union is open if any sub-region is open
 */
auto is_open(const Region& region) -> bool;

/**
 * @brief Compute area of spatial region.
 *
 * - Empty: 0
 * - Universe: ∞
 * - BBox: (xmax - xmin) × (ymax - ymin)
 * - Union: Sum of areas (may overcount if overlapping - use simplify first)
 *
 * For accurate area of overlapping unions, use: area(simplify(region))
 */
auto area(const Region& region) -> double;

/**
 * @brief Compute topological interior.
 *
 * Interior I(Ω) is the largest open set contained in Ω.
 * For boxes, this opens all boundaries.
 *
 * Example:
 * @code
 * auto closed_box = BBox{0, 10, 0, 10};  // [0,10] × [0,10]
 * auto open_box = interior(closed_box);  // (0,10) × (0,10)
 * @endcode
 */
auto interior(const Region& region) -> Region;

/**
 * @brief Compute topological closure.
 *
 * Closure C(Ω) is the smallest closed set containing Ω.
 * For boxes, this closes all boundaries.
 *
 * Example:
 * @code
 * auto open_box = BBox{0, 10, 0, 10, true, true, true, true};  // (0,10) × (0,10)
 * auto closed_box = closure(open_box);  // [0,10] × [0,10]
 * @endcode
 */
auto closure(const Region& region) -> Region;

// =============================================================================
// Set Operations
// =============================================================================

/**
 * @brief Spatial complement: Ω̅ = U \ Ω.
 *
 * Computes the complement of a region within a universe bounding box.
 * Returns all points in universe that are NOT in the region.
 *
 * @param region The region to complement
 * @param universe The bounding box defining the universe (e.g., frame bounds)
 * @return Complement of region within universe
 *
 * Example:
 * @code
 * auto frame_bbox = BBox{0, 1920, 0, 1080};  // Full frame
 * auto obstacle = BBox{500, 700, 300, 500};  // Obstacle region
 * auto free_space = complement(obstacle, frame_bbox);  // Everything except obstacle
 * @endcode
 */
auto complement(const Region& region, const BBox& universe) -> Region;

/**
 * @brief Spatial intersection: Ω₁ ⊓ Ω₂.
 *
 * Returns points that are in BOTH regions.
 *
 * Properties:
 * - Commutative: intersect(a, b) = intersect(b, a)
 * - Associative: intersect(intersect(a, b), c) = intersect(a, intersect(b, c))
 * - Identity: intersect(a, Universe) = a
 * - Annihilator: intersect(a, Empty) = Empty
 */
auto intersect(const Region& lhs, const Region& rhs) -> Region;

/// Variadic intersection: Ω₁ ⊓ Ω₂ ⊓ ... ⊓ Ωₙ
auto intersect(const std::vector<Region>& regions) -> Region;

/**
 * @brief Spatial union: Ω₁ ⊔ Ω₂.
 *
 * Returns points that are in EITHER region (or both).
 *
 * Properties:
 * - Commutative: union_of(a, b) = union_of(b, a)
 * - Associative: union_of(union_of(a, b), c) = union_of(a, union_of(b, c))
 * - Identity: union_of(a, Empty) = a
 * - Annihilator: union_of(a, Universe) = Universe
 */
auto union_of(const Region& lhs, const Region& rhs) -> Region;

/// Variadic union: Ω₁ ⊔ Ω₂ ⊔ ... ⊔ Ωₙ
auto union_of(const std::vector<Region>& regions) -> Region;

// =============================================================================
// Simplification
// =============================================================================

/**
 * @brief Simplify a region to disjoint bounding boxes.
 *
 * Converts an arbitrary Union of potentially overlapping boxes into
 * an equivalent Union of disjoint (non-overlapping) boxes.
 *
 * This is essential for accurate area computation and visualization.
 *
 * Algorithm: Sweep-line algorithm with interval scheduling.
 *
 * Example:
 * @code
 * // Two overlapping boxes
 * auto u = Union{};
 * u.insert(BBox{0, 10, 0, 10});
 * u.insert(BBox{5, 15, 0, 10});
 *
 * // Simplify to disjoint boxes
 * auto simplified = simplify(u);
 * // Result: [0,5]×[0,10], [5,10]×[0,10], [10,15]×[0,10]
 * // (or similar disjoint decomposition)
 * @endcode
 */
auto simplify(const Region& region) -> Region;

// =============================================================================
// Datastream Conversion Helpers
// =============================================================================

/**
 * @brief Convert a datastream BoundingBox to a spatial BBox.
 *
 * Creates a closed bounding box (all boundaries included).
 */
inline auto from_datastream(const datastream::BoundingBox& bbox) -> BBox {
  return BBox{bbox}; // Uses explicit constructor
}

/**
 * @brief Create a spatial region from an object's bounding box.
 *
 * Extracts the bounding box from an Object and returns it as a spatial Region.
 */
inline auto bbox_of_object(const datastream::Object& obj) -> Region { return BBox{obj.bbox}; }

/**
 * @brief Get universe region for a frame.
 *
 * Returns a BBox covering the entire frame.
 */
inline auto frame_universe(const datastream::Frame& frame) -> BBox {
  return BBox{frame.universe_bbox()};
}

} // namespace percemon::spatial

#endif // PERCEMON_SPATIAL_HPP
