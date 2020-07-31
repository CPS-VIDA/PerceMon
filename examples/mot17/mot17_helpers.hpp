#pragma once

#ifndef __PERCEMON_MOT17_HELPERS_HPP__
#define __PERCEMON_MOT17_HELPERS_HPP__

#include <string>

namespace mot17 {

enum class Labels {
  Pedestrian          = 1,
  PersonOnVehicle     = 2,
  Car                 = 3,
  Bicycle             = 4,
  Motorbike           = 5,
  NonMotorizedVehicle = 6,
  StaticPerson        = 7,
  Distractor          = 8,
  Occluder            = 9,
  OccluderOnGround    = 10,
  OccluderFull        = 11,
  Reflection          = 12
};

/**
 * A row from the MOT17 CSV evaluation file.
 */
struct ResultsRow {
  size_t frame;
  std::string id;
  double bb_left, bb_top, bb_width, bb_height;
  double confidence;
  int label;
  double visibility;
};
} // namespace mot17

#endif /* end of include guard: __PERCEMON_MOT17_HELPERS_HPP__ */
