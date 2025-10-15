#include "percemon/datastream.hpp"
#include <cmath>

namespace percemon::datastream {

auto get_reference_point(const BoundingBox& bbox, RefPointType ref_type)
    -> std::pair<double, double> {
    const double cx = (bbox.xmin + bbox.xmax) / 2.0;
    const double cy = (bbox.ymin + bbox.ymax) / 2.0;

    switch (ref_type) {
        case RefPointType::Center:
            return {cx, cy};
        case RefPointType::LeftMargin:
            return {bbox.xmin, cy};
        case RefPointType::RightMargin:
            return {bbox.xmax, cy};
        case RefPointType::TopMargin:
            return {cx, bbox.ymin};
        case RefPointType::BottomMargin:
            return {cx, bbox.ymax};
    }

    // This should never be reached, but provide a fallback
    return {cx, cy};
}

auto euclidean_distance(
    const BoundingBox& bbox1,
    RefPointType ref1,
    const BoundingBox& bbox2,
    RefPointType ref2) -> double {
    auto [x1, y1] = get_reference_point(bbox1, ref1);
    auto [x2, y2] = get_reference_point(bbox2, ref2);

    const double dx = x2 - x1;
    const double dy = y2 - y1;

    return std::sqrt(dx * dx + dy * dy);
}

} // namespace percemon::datastream
