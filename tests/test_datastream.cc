#include "percemon/datastream.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace percemon::datastream;

TEST_CASE("BoundingBox basic properties", "[datastream][bbox]") {
  auto bbox = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};

  SECTION("Area computation") {
    REQUIRE(bbox.area() == 10000); // 100 × 100
  }

  SECTION("Width and height") {
    REQUIRE(bbox.width() == 100);
    REQUIRE(bbox.height() == 100);
  }

  SECTION("Center point") {
    auto [cx, cy] = bbox.center();
    REQUIRE(cx == 150);
    REQUIRE(cy == 100);
  }
}

TEST_CASE("Reference point extraction", "[datastream][refpoint]") {
  auto bbox = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};

  SECTION("Center") {
    auto [x, y] = get_reference_point(bbox, RefPointType::Center);
    REQUIRE(x == 150);
    REQUIRE(y == 100);
  }

  SECTION("Left margin") {
    auto [x, y] = get_reference_point(bbox, RefPointType::LeftMargin);
    REQUIRE(x == 100); // Left edge
    REQUIRE(y == 100); // Vertical center
  }

  SECTION("Right margin") {
    auto [x, y] = get_reference_point(bbox, RefPointType::RightMargin);
    REQUIRE(x == 200); // Right edge
    REQUIRE(y == 100); // Vertical center
  }

  SECTION("Top margin") {
    auto [x, y] = get_reference_point(bbox, RefPointType::TopMargin);
    REQUIRE(x == 150); // Horizontal center
    REQUIRE(y == 50);  // Top edge
  }

  SECTION("Bottom margin") {
    auto [x, y] = get_reference_point(bbox, RefPointType::BottomMargin);
    REQUIRE(x == 150); // Horizontal center
    REQUIRE(y == 150); // Bottom edge
  }
}

TEST_CASE("Euclidean distance", "[datastream][distance]") {
  auto bbox1 = BoundingBox{.xmin = 0, .xmax = 10, .ymin = 0, .ymax = 10};
  auto bbox2 = BoundingBox{.xmin = 30, .xmax = 40, .ymin = 0, .ymax = 10};

  auto dist = euclidean_distance(bbox1, RefPointType::Center, bbox2, RefPointType::Center);

  // Distance between (5,5) and (35,5) = 30
  REQUIRE(dist == 30.0);
}

TEST_CASE("Euclidean distance with different reference points", "[datastream][distance]") {
  auto bbox1 = BoundingBox{.xmin = 0, .xmax = 10, .ymin = 0, .ymax = 10};
  auto bbox2 = BoundingBox{.xmin = 10, .xmax = 20, .ymin = 0, .ymax = 10};

  SECTION("Right margin of bbox1 to left margin of bbox2") {
    auto dist =
        euclidean_distance(bbox1, RefPointType::RightMargin, bbox2, RefPointType::LeftMargin);
    // (10, 5) to (10, 5) = 0
    REQUIRE(dist == 0.0);
  }

  SECTION("Center to center") {
    auto dist = euclidean_distance(bbox1, RefPointType::Center, bbox2, RefPointType::Center);
    // (5, 5) to (15, 5) = 10
    REQUIRE(dist == 10.0);
  }
}

TEST_CASE("Frame structure", "[datastream][frame]") {
  auto frame =
      Frame{.timestamp = 1.5, .frame_num = 45, .size_x = 1920, .size_y = 1080, .objects = {}};

  SECTION("Universe bounding box") {
    auto universe = frame.universe_bbox();
    REQUIRE(universe.xmin == 0);
    REQUIRE(universe.xmax == 1920);
    REQUIRE(universe.ymin == 0);
    REQUIRE(universe.ymax == 1080);
  }

  SECTION("Universe area") {
    auto universe = frame.universe_bbox();
    REQUIRE(universe.area() == 1920 * 1080);
  }
}

TEST_CASE("Object structure", "[datastream][object]") {
  auto bbox = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
  auto obj  = Object{.object_class = 1, .probability = 0.95, .bbox = bbox};

  REQUIRE(obj.object_class == 1);
  REQUIRE(obj.probability == 0.95);
  REQUIRE(obj.bbox.area() == 10000);
}

TEST_CASE("Frame with multiple objects", "[datastream][frame][object]") {
  auto frame = Frame{
      .timestamp = 0.0,
      .frame_num = 0,
      .size_x    = 1920,
      .size_y    = 1080,
      .objects   = {
          {"car_1",
             Object{
                 .object_class = 1,
                 .probability  = 0.95,
                 .bbox         = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150}}},
          {"pedestrian_1",
             Object{
                 .object_class = 2,
                 .probability  = 0.85,
                 .bbox         = BoundingBox{.xmin = 500, .xmax = 550, .ymin = 300, .ymax = 600}}}}};

  REQUIRE(frame.objects.size() == 2);
  REQUIRE(frame.objects.count("car_1") == 1);
  REQUIRE(frame.objects.count("pedestrian_1") == 1);
  REQUIRE(frame.objects.at("car_1").probability == 0.95);
  REQUIRE(frame.objects.at("pedestrian_1").object_class == 2);
}

TEST_CASE("BoundingBox comparison", "[datastream][bbox][comparison]") {
  auto bbox1 = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
  auto bbox2 = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
  auto bbox3 = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 151};

  REQUIRE(bbox1 == bbox2);
  REQUIRE(bbox1 != bbox3);
  REQUIRE(bbox1 < bbox3);
}

TEST_CASE("Object comparison", "[datastream][object][comparison]") {
  auto bbox1 = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
  auto bbox2 = BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};

  auto obj1 = Object{.object_class = 1, .probability = 0.95, .bbox = bbox1};
  auto obj2 = Object{.object_class = 1, .probability = 0.95, .bbox = bbox2};
  auto obj3 = Object{.object_class = 2, .probability = 0.95, .bbox = bbox1};

  REQUIRE(obj1 == obj2);
  REQUIRE(obj1 != obj3);
}

TEST_CASE("Trace (vector of frames)", "[datastream][trace]") {
  auto frame1 =
      Frame{.timestamp = 0.0, .frame_num = 0, .size_x = 1920, .size_y = 1080, .objects = {}};

  auto frame2 =
      Frame{.timestamp = 0.033, .frame_num = 1, .size_x = 1920, .size_y = 1080, .objects = {}};

  Trace trace = {frame1, frame2};

  REQUIRE(trace.size() == 2);
  REQUIRE(trace[0].frame_num == 0);
  REQUIRE(trace[1].frame_num == 1);
  REQUIRE(trace[1].timestamp > trace[0].timestamp);
}
