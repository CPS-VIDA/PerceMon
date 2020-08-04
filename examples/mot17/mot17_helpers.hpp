#pragma once

#ifndef __PERCEMON_MOT17_HELPERS_HPP__
#define __PERCEMON_MOT17_HELPERS_HPP__

#include <string>

#include "percemon/datastream.hpp"
#include <vector>

namespace mot17 {

enum class Labels : int {
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

std::vector<percemon::datastream::Frame> parse_results(
    const std::string& file,
    const double fps,
    const size_t frame_width,
    const size_t frame_height);

} // namespace mot17

#endif /* end of include guard: __PERCEMON_MOT17_HELPERS_HPP__ */
