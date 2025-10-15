#pragma once

#ifndef PERCEMON_DATASTREAM_HPP
#define PERCEMON_DATASTREAM_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/**
 * @file datastream.hpp
 * @brief Runtime data structures for perception datastreams
 *
 * This module defines the concrete data structures representing perception data
 * from sensors/cameras. These are the runtime values that STQL formulas monitor.
 *
 * Key distinction: These are RUNTIME DATA, not AST nodes. They represent actual
 * detected objects, not symbolic expressions.
 */

namespace percemon::datastream {

/**
 * @brief 2D bounding box in image coordinates.
 *
 * Follows Pascal VOC format: (xmin, ymin) is top-left, (xmax, ymax) is bottom-right.
 * Coordinates are in pixels.
 *
 * NOTE: Image origin (0,0) is at the top-left corner.
 *
 * Example:
 * @code
 * auto bbox = BoundingBox{
 *     .xmin = 100.0,
 *     .xmax = 200.0,
 *     .ymin = 50.0,
 *     .ymax = 150.0
 * };
 * double width = bbox.xmax - bbox.xmin;   // 100 pixels
 * double height = bbox.ymax - bbox.ymin;  // 100 pixels
 * @endcode
 */
struct BoundingBox {
  double xmin; ///< Left edge x-coordinate
  double xmax; ///< Right edge x-coordinate
  double ymin; ///< Top edge y-coordinate
  double ymax; ///< Bottom edge y-coordinate

  /// Default comparison for use in containers
  auto operator<=>(const BoundingBox&) const = default;

  /// Compute area of bounding box
  [[nodiscard]] auto area() const -> double { return (xmax - xmin) * (ymax - ymin); }

  /// Compute width of bounding box
  [[nodiscard]] auto width() const -> double { return xmax - xmin; }

  /// Compute height of bounding box
  [[nodiscard]] auto height() const -> double { return ymax - ymin; }

  /// Get center point (x, y)
  [[nodiscard]] auto center() const -> std::pair<double, double> {
    return {(xmin + xmax) / 2.0, (ymin + ymax) / 2.0};
  }
};

/**
 * @brief Reference point on a bounding box.
 *
 * Used for spatial measurements like Euclidean distance, lateral position, etc.
 * Corresponds to CoordRefPoint enum in STQL (LM, RM, TM, BM, CT).
 */
enum class RefPointType : uint8_t {
  LeftMargin,   ///< Center of left edge
  RightMargin,  ///< Center of right edge
  TopMargin,    ///< Center of top edge
  BottomMargin, ///< Center of bottom edge
  Center        ///< Geometric center (centroid)
};

/**
 * @brief Get coordinates of a reference point on a bounding box.
 *
 * @param bbox The bounding box
 * @param ref_type Which reference point to extract
 * @return (x, y) coordinates of the reference point
 *
 * Example:
 * @code
 * auto bbox = BoundingBox{100, 200, 50, 150};
 * auto [x, y] = get_reference_point(bbox, RefPointType::Center);
 * // x = 150, y = 100 (center of box)
 * @endcode
 */
auto get_reference_point(const BoundingBox& bbox, RefPointType ref_type)
    -> std::pair<double, double>;

/**
 * @brief Compute Euclidean distance between two reference points.
 *
 * Used to implement the STQL ED(id_i, CRT, id_j, CRT) function.
 *
 * @param bbox1 First bounding box
 * @param ref1 Reference point type on first box
 * @param bbox2 Second bounding box
 * @param ref2 Reference point type on second box
 * @return Euclidean distance between the two reference points
 *
 * Example:
 * @code
 * auto car_bbox = BoundingBox{...};
 * auto truck_bbox = BoundingBox{...};
 * double dist = euclidean_distance(
 *     car_bbox, RefPointType::Center,
 *     truck_bbox, RefPointType::Center
 * );
 * @endcode
 */
auto euclidean_distance(
    const BoundingBox& bbox1,
    RefPointType ref1,
    const BoundingBox& bbox2,
    RefPointType ref2) -> double;

/**
 * @brief Detected object in a perception frame.
 *
 * Represents a single detected object with its class, confidence, and spatial extent.
 * This is what object detection algorithms output.
 *
 * Example:
 * @code
 * auto car = Object{
 *     .object_class = 1,        // Class ID for "car"
 *     .probability = 0.95,      // 95% confidence
 *     .bbox = BoundingBox{...}  // Spatial location
 * };
 * @endcode
 */
struct Object {
  int object_class;   ///< Object class/category ID
  double probability; ///< Detection confidence [0.0, 1.0]
  BoundingBox bbox;   ///< Spatial extent (bounding box)

  auto operator<=>(const Object&) const = default;
};

/**
 * @brief Single frame of perception data.
 *
 * A frame represents one snapshot in time from a perception system (e.g., camera).
 * Contains timing information and all detected objects in that frame.
 *
 * Example:
 * @code
 * auto frame = Frame{
 *     .timestamp = 1.5,      // 1.5 seconds elapsed
 *     .frame_num = 45,       // Frame #45 (at 30 FPS)
 *     .size_x = 1920,        // 1920x1080 resolution
 *     .size_y = 1080,
 *     .objects = {
 *         {"car_1", Object{...}},
 *         {"pedestrian_1", Object{...}}
 *     }
 * };
 * @endcode
 */
struct Frame {
  double timestamp;  ///< Time in seconds since start
  int64_t frame_num; ///< Frame number (0-indexed)
  size_t size_x;     ///< Frame width in pixels
  size_t size_y;     ///< Frame height in pixels

  /// Map from object ID to detected object
  /// Object IDs are strings (e.g., "car_1", "pedestrian_5")
  std::map<std::string, Object> objects;

  /// Get universe bounding box (entire frame)
  [[nodiscard]] auto universe_bbox() const -> BoundingBox {
    return BoundingBox{
        .xmin = 0.0,
        .xmax = static_cast<double>(size_x),
        .ymin = 0.0,
        .ymax = static_cast<double>(size_y)};
  }
};

/**
 * @brief A sequence of perception frames (trace).
 *
 * Represents a complete perception datastream over time.
 * Frames are ordered chronologically.
 *
 * Example:
 * @code
 * auto trace = Trace{
 *     Frame{...},  // Frame 0
 *     Frame{...},  // Frame 1
 *     Frame{...}   // Frame 2
 * };
 * @endcode
 */
using Trace = std::vector<Frame>;

} // namespace percemon::datastream

#endif // PERCEMON_DATASTREAM_HPP
