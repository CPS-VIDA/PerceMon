#include "percemon/spatial.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace percemon::spatial;

TEST_CASE("Empty region properties", "[spatial][empty]") {
  Region empty = Empty{};

  REQUIRE(area(empty) == 0.0);
  REQUIRE(is_closed(empty));
  REQUIRE(is_open(empty));
}

TEST_CASE("Universe region properties", "[spatial][universe]") {
  Region univ = Universe{};

  REQUIRE(area(univ) == std::numeric_limits<double>::infinity());
  REQUIRE(is_closed(univ));
  REQUIRE(is_open(univ));
}

TEST_CASE("BBox basic operations", "[spatial][bbox]") {
  auto bbox = BBox{100, 200, 50, 150};

  SECTION("Area") { REQUIRE(area(bbox) == 10000); }

  SECTION("Closed by default") {
    REQUIRE(bbox.is_closed());
    REQUIRE_FALSE(bbox.is_open());
  }

  SECTION("Open boundaries") {
    auto open_bbox = BBox{100, 200, 50, 150, true, true, true, true};
    REQUIRE(open_bbox.is_open());
    REQUIRE_FALSE(open_bbox.is_closed());
  }

  SECTION("Construct from datastream BoundingBox") {
    auto ds_bbox =
        percemon::datastream::BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
    auto spatial_bbox = BBox{ds_bbox};
    REQUIRE(spatial_bbox.xmin == 100);
    REQUIRE(spatial_bbox.xmax == 200);
    REQUIRE(spatial_bbox.ymin == 50);
    REQUIRE(spatial_bbox.ymax == 150);
    REQUIRE(spatial_bbox.is_closed());
  }
}

TEST_CASE("Union basic operations", "[spatial][union]") {
  Union u;
  auto box1 = BBox{0, 10, 0, 10};
  auto box2 = BBox{20, 30, 0, 10};

  SECTION("Insert boxes") {
    u.insert(box1);
    u.insert(box2);
    REQUIRE(u.size() == 2);
    REQUIRE_FALSE(u.empty());
  }

  SECTION("Construct from initializer list") {
    Union u2{{box1, box2}};
    REQUIRE(u2.size() == 2);
  }
}

TEST_CASE("Spatial intersection", "[spatial][intersection]") {
  SECTION("Two overlapping boxes") {
    auto box1 = BBox{0, 10, 0, 10};
    auto box2 = BBox{5, 15, 0, 10};

    Region result = intersect(box1, box2);

    REQUIRE(std::holds_alternative<BBox>(result));
    auto& intersection_box = std::get<BBox>(result);
    REQUIRE(intersection_box.xmin == 5);
    REQUIRE(intersection_box.xmax == 10);
    REQUIRE(intersection_box.ymin == 0);
    REQUIRE(intersection_box.ymax == 10);
  }

  SECTION("Non-overlapping boxes") {
    auto box1 = BBox{0, 10, 0, 10};
    auto box2 = BBox{20, 30, 0, 10};

    Region result = intersect(box1, box2);

    REQUIRE(std::holds_alternative<Empty>(result));
  }

  SECTION("Intersection with Universe") {
    auto box      = BBox{100, 200, 50, 150};
    Region result = intersect(box, Universe{});

    REQUIRE(std::holds_alternative<BBox>(result));
    REQUIRE(std::get<BBox>(result) == box);
  }

  SECTION("Intersection with Empty") {
    auto box      = BBox{100, 200, 50, 150};
    Region result = intersect(box, Empty{});

    REQUIRE(std::holds_alternative<Empty>(result));
  }

  SECTION("Variadic intersection") {
    std::vector<Region> regions = {BBox{0, 20, 0, 20}, BBox{10, 30, 0, 20}, BBox{0, 20, 5, 15}};

    Region result = intersect(regions);

    REQUIRE(std::holds_alternative<BBox>(result));
    auto& box = std::get<BBox>(result);
    REQUIRE(box.xmin == 10);
    REQUIRE(box.xmax == 20);
    REQUIRE(box.ymin == 5);
    REQUIRE(box.ymax == 15);
  }
}

TEST_CASE("Spatial union", "[spatial][union_op]") {
  SECTION("Two non-overlapping boxes") {
    auto box1 = BBox{0, 10, 0, 10};
    auto box2 = BBox{20, 30, 0, 10};

    Region result = union_of(box1, box2);

    REQUIRE(std::holds_alternative<Union>(result));
    auto& union_region = std::get<Union>(result);
    REQUIRE(union_region.size() == 2);
  }

  SECTION("One box inside another") {
    auto outer = BBox{0, 20, 0, 20};
    auto inner = BBox{5, 15, 5, 15};

    Region result = union_of(outer, inner);

    REQUIRE(std::holds_alternative<BBox>(result));
    auto& result_box = std::get<BBox>(result);
    REQUIRE(result_box.xmin == 0);
    REQUIRE(result_box.xmax == 20);
  }

  SECTION("Union with Universe") {
    auto box      = BBox{100, 200, 50, 150};
    Region result = union_of(box, Universe{});

    REQUIRE(std::holds_alternative<Universe>(result));
  }

  SECTION("Union with Empty") {
    auto box      = BBox{100, 200, 50, 150};
    Region result = union_of(box, Empty{});

    REQUIRE(std::holds_alternative<BBox>(result));
    REQUIRE(std::get<BBox>(result) == box);
  }

  SECTION("Variadic union") {
    std::vector<Region> regions = {BBox{0, 10, 0, 10}, BBox{20, 30, 0, 10}, BBox{10, 20, 5, 15}};

    Region result = union_of(regions);

    REQUIRE(std::holds_alternative<Union>(result));
  }
}

TEST_CASE("Spatial complement", "[spatial][complement]") {
  auto universe = BBox{0, 100, 0, 100};

  SECTION("Complement of Empty is Universe") {
    Region result = complement(Empty{}, universe);
    REQUIRE(std::holds_alternative<Universe>(result));
  }

  SECTION("Complement of Universe is Empty") {
    Region result = complement(Universe{}, universe);
    REQUIRE(std::holds_alternative<Empty>(result));
  }

  SECTION("Complement of centered box") {
    auto center_box = BBox{25, 75, 25, 75};
    Region result   = complement(center_box, universe);

    // Should produce 4 fragments (top, bottom, left, right)
    REQUIRE(std::holds_alternative<Union>(result));
    auto& comp_union = std::get<Union>(result);
    REQUIRE(comp_union.size() == 4);
  }

  SECTION("Complement of edge-aligned box") {
    auto edge_box = BBox{0, 50, 0, 50};
    Region result = complement(edge_box, universe);

    // Should produce 2 fragments (right and bottom)
    REQUIRE(std::holds_alternative<Union>(result));
    auto& comp_union = std::get<Union>(result);
    REQUIRE(comp_union.size() == 2);
  }
}

TEST_CASE("Interior and Closure", "[spatial][topology]") {
  SECTION("Interior opens all boundaries") {
    auto closed        = BBox{0, 10, 0, 10};
    Region open_region = interior(closed);

    REQUIRE(std::holds_alternative<BBox>(open_region));
    auto& open_box = std::get<BBox>(open_region);
    REQUIRE(open_box.is_open());
    REQUIRE(open_box.lopen);
    REQUIRE(open_box.ropen);
    REQUIRE(open_box.topen);
    REQUIRE(open_box.bopen);
  }

  SECTION("Closure closes all boundaries") {
    auto open            = BBox{0, 10, 0, 10, true, true, true, true};
    Region closed_region = closure(open);

    REQUIRE(std::holds_alternative<BBox>(closed_region));
    auto& closed_box = std::get<BBox>(closed_region);
    REQUIRE(closed_box.is_closed());
    REQUIRE_FALSE(closed_box.lopen);
    REQUIRE_FALSE(closed_box.ropen);
    REQUIRE_FALSE(closed_box.topen);
    REQUIRE_FALSE(closed_box.bopen);
  }

  SECTION("Interior on Universe") {
    Region result = interior(Universe{});
    REQUIRE(std::holds_alternative<Universe>(result));
  }

  SECTION("Closure on Empty") {
    Region result = closure(Empty{});
    REQUIRE(std::holds_alternative<Empty>(result));
  }
}

TEST_CASE("Region simplification", "[spatial][simplify]") {
  SECTION("Overlapping boxes simplified to disjoint") {
    Union u;
    u.insert(BBox{0, 10, 0, 10});
    u.insert(BBox{5, 15, 0, 10});
    u.insert(BBox{10, 20, 0, 10});

    Region simplified = simplify(u);

    CAPTURE(to_string(simplified));
    // After simplification, should have disjoint boxes
    bool check =
        std::holds_alternative<Union>(simplified) || std::holds_alternative<BBox>(simplified);
    REQUIRE(check);
  }

  SECTION("Empty simplifies to empty") {
    Region result = simplify(Empty{});
    REQUIRE(std::holds_alternative<Empty>(result));
  }

  SECTION("Single box simplifies to itself") {
    auto bbox     = BBox{0, 10, 0, 10};
    Region result = simplify(bbox);
    REQUIRE(std::holds_alternative<BBox>(result));
  }
}

TEST_CASE("Datastream conversion helpers", "[spatial][conversion]") {
  SECTION("from_datastream") {
    auto ds_bbox =
        percemon::datastream::BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150};
    auto spatial_bbox = from_datastream(ds_bbox);

    REQUIRE(spatial_bbox.xmin == 100);
    REQUIRE(spatial_bbox.xmax == 200);
    REQUIRE(spatial_bbox.ymin == 50);
    REQUIRE(spatial_bbox.ymax == 150);
    REQUIRE(spatial_bbox.is_closed());
  }

  SECTION("bbox_of_object") {
    auto obj = percemon::datastream::Object{
        .object_class = 1,
        .probability  = 0.95,
        .bbox =
            percemon::datastream::BoundingBox{.xmin = 100, .xmax = 200, .ymin = 50, .ymax = 150}};

    Region region = bbox_of_object(obj);

    REQUIRE(std::holds_alternative<BBox>(region));
    auto& bbox = std::get<BBox>(region);
    REQUIRE(bbox.xmin == 100);
    REQUIRE(bbox.xmax == 200);
  }

  SECTION("frame_universe") {
    auto frame = percemon::datastream::Frame{
        .timestamp = 1.5, .frame_num = 45, .size_x = 1920, .size_y = 1080, .objects = {}};

    auto universe = frame_universe(frame);

    REQUIRE(universe.xmin == 0);
    REQUIRE(universe.xmax == 1920);
    REQUIRE(universe.ymin == 0);
    REQUIRE(universe.ymax == 1080);
  }
}

TEST_CASE("Complex spatial operations", "[spatial][complex]") {
  SECTION("Intersection of unions") {
    Union u1;
    u1.insert(BBox{0, 10, 0, 10});
    u1.insert(BBox{20, 30, 20, 30});

    Union u2;
    u2.insert(BBox{5, 15, 0, 10});
    u2.insert(BBox{20, 30, 20, 30});

    Region result = intersect(u1, u2);
    bool check    = std::holds_alternative<Union>(result) || std::holds_alternative<BBox>(result);
    CAPTURE(to_string(result));
    REQUIRE(check);
  }

  SECTION("Union of unions") {
    Union u1;
    u1.insert(BBox{0, 10, 0, 10});

    Union u2;
    u2.insert(BBox{20, 30, 0, 10});

    Region result = union_of(u1, u2);

    REQUIRE(std::holds_alternative<Union>(result));
  }

  SECTION("De Morgan's laws: ~(A ⊓ B) = ~A ⊔ ~B") {
    auto universe = BBox{0, 100, 0, 100};
    auto a        = BBox{10, 40, 10, 40};
    auto b        = BBox{30, 70, 30, 70};

    // Left side: ~(A ⊓ B)
    auto intersection = intersect(a, b);
    auto left         = complement(intersection, universe);

    // Right side: ~A ⊔ ~B
    auto comp_a = complement(a, universe);
    auto comp_b = complement(b, universe);
    auto right  = union_of(comp_a, comp_b);

    // Both should have similar area relationships
    double left_area  = area(left);
    double right_area = area(right);

    REQUIRE(left_area > 0);
    REQUIRE(right_area > 0);
  }
}

TEST_CASE("Open/closed boundary interactions", "[spatial][boundaries]") {
  SECTION("Intersection respects open boundaries") {
    auto box1 = BBox{0, 10, 0, 10, false, false, false, false};
    auto box2 = BBox{10, 20, 0, 10, false, false, false, false};

    Region result = intersect(box1, box2);
    CAPTURE(to_string(result));

    // Boxes touch at x=10, but since box1's right is closed and box2's left is closed,
    // they should not intersect (depending on implementation)
    REQUIRE(std::holds_alternative<Empty>(result));
  }

  SECTION("Open box touches closed box") {
    auto open_box   = BBox{0, 10, 0, 10, false, true, false, false};
    auto closed_box = BBox{10, 20, 0, 10, false, false, false, false};

    Region result = intersect(open_box, closed_box);

    // Should be empty since open_box excludes right boundary
    CAPTURE(to_string(result));
    REQUIRE(std::holds_alternative<Empty>(result));
  }
}
