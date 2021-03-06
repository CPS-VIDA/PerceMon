/**
 * This file essentially defines what is representative of a stream of
 * perception data.
 *
 * A perception data stream is a discrete signal consisting of frames, with
 * fixed sampling rate or frames per second. Each frame consists of frame
 * number/time stamp and a map associating IDs with labelled objects.
 *
 * Each labelled object should contain the following:
 *
 * - Class: Type of the object, outputted from some object detection algorithm.
 * - Probability: Probability associated with the class.
 * - A bounding box.
 */

#pragma once

#ifndef __PERCEMON_STREAM_HH__
#define __PERCEMON_STREAM_HH__

#include <map>
#include <string>

namespace percemon::datastream {

// TODO: Euclidean Distance between boxes
// TODO: Lat and Lon distances between boxes.

/**
 * A bounding box data structure that follows the Pascal VOC Bounding box format
 * (x-top left, y-top left,x-bottom right, y-bottom right), where each
 * coordinate is in terms of number of pixels.
 *
 * NOTE: The origin in an image is the top left corner.
 *
 */
struct BoundingBox {
  size_t xmin;
  size_t xmax;
  size_t ymin;
  size_t ymax;
};

struct Object {
  int object_class;
  double probability;
  BoundingBox bbox;
};

struct Frame {
  /**
   * Number of seconds elapsed
   */
  double timestamp; // TODO: should I use std::chrono?
  /**
   * Frame number
   */
  size_t frame_num;

  /**
   * The size of the frame/image in pixels
   */
  // TODO: Should this be doubles?
  size_t size_x, size_y;

  std::map<std::string, Object> objects;
};

} // namespace percemon::datastream

#endif /* end of include guard: __PERCEMON_STREAM_HH__ */
