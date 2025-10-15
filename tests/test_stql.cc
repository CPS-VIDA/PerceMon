#include <catch2/catch_all.hpp>
#include <string>
#include <variant>

#include "percemon/stql.hpp"

using namespace percemon::stql;

// =============================================================================
// Variable Types
// =============================================================================

TEST_CASE("TimeVar construction and to_string", "[stql][variables]") {
  SECTION("Basic time variable") {
    auto x = TimeVar{"x"};
    REQUIRE(x.to_string() == "x");
  }

  SECTION("Current time constant") {
    auto ct = TimeVar::current();
    REQUIRE(ct.to_string() == "C_TIME");
    REQUIRE(C_TIME.to_string() == "C_TIME");
  }

  SECTION("Time variable comparison") {
    auto x1 = TimeVar{"x"};
    auto x2 = TimeVar{"x"};
    auto y  = TimeVar{"y"};
    REQUIRE(x1 == x2);
    REQUIRE(x1 != y);
  }
}

TEST_CASE("FrameVar construction and to_string", "[stql][variables]") {
  SECTION("Basic frame variable") {
    auto f = FrameVar{"f"};
    REQUIRE(f.to_string() == "f");
  }

  SECTION("Current frame constant") {
    auto cf = FrameVar::current();
    REQUIRE(cf.to_string() == "C_FRAME");
    REQUIRE(C_FRAME.to_string() == "C_FRAME");
  }

  SECTION("Frame variable comparison") {
    auto f1 = FrameVar{"f"};
    auto f2 = FrameVar{"f"};
    auto g  = FrameVar{"g"};
    REQUIRE(f1 == f2);
    REQUIRE(f1 != g);
  }
}

TEST_CASE("ObjectVar construction and to_string", "[stql][variables]") {
  SECTION("Basic object variable") {
    auto car = ObjectVar{"car"};
    REQUIRE(car.to_string() == "car");
  }

  SECTION("Multiple object variables") {
    auto car        = ObjectVar{"car"};
    auto pedestrian = ObjectVar{"pedestrian"};
    REQUIRE(car.to_string() == "car");
    REQUIRE(pedestrian.to_string() == "pedestrian");
  }

  SECTION("Object variable comparison") {
    auto car1  = ObjectVar{"car"};
    auto car2  = ObjectVar{"car"};
    auto truck = ObjectVar{"truck"};
    REQUIRE(car1 == car2);
    REQUIRE(car1 != truck);
  }
}

// =============================================================================
// Coordinate Reference Points
// =============================================================================

TEST_CASE("CoordRefPoint to_string", "[stql][spatial]") {
  REQUIRE(to_string(CoordRefPoint::LeftMargin) == "LM");
  REQUIRE(to_string(CoordRefPoint::RightMargin) == "RM");
  REQUIRE(to_string(CoordRefPoint::TopMargin) == "TM");
  REQUIRE(to_string(CoordRefPoint::BottomMargin) == "BM");
  REQUIRE(to_string(CoordRefPoint::Center) == "CT");
}

TEST_CASE("RefPoint construction and to_string", "[stql][spatial]") {
  auto car        = ObjectVar{"car"};
  auto ref_center = RefPoint{car, CoordRefPoint::Center};
  auto ref_left   = RefPoint{car, CoordRefPoint::LeftMargin};

  REQUIRE(ref_center.to_string() == "car.CT");
  REQUIRE(ref_left.to_string() == "car.LM");
}

// =============================================================================
// Comparison Operators
// =============================================================================

TEST_CASE("CompareOp to_string", "[stql][operators]") {
  REQUIRE(to_string(CompareOp::LessThan) == "<");
  REQUIRE(to_string(CompareOp::LessEqual) == "<=");
  REQUIRE(to_string(CompareOp::GreaterThan) == ">");
  REQUIRE(to_string(CompareOp::GreaterEqual) == ">=");
  REQUIRE(to_string(CompareOp::Equal) == "==");
  REQUIRE(to_string(CompareOp::NotEqual) == "!=");
}

TEST_CASE("CompareOp negate", "[stql][operators]") {
  REQUIRE(negate(CompareOp::LessThan) == CompareOp::GreaterEqual);
  REQUIRE(negate(CompareOp::LessEqual) == CompareOp::GreaterThan);
  REQUIRE(negate(CompareOp::GreaterThan) == CompareOp::LessEqual);
  REQUIRE(negate(CompareOp::GreaterEqual) == CompareOp::LessThan);
  REQUIRE(negate(CompareOp::Equal) == CompareOp::NotEqual);
  REQUIRE(negate(CompareOp::NotEqual) == CompareOp::Equal);
}

TEST_CASE("CompareOp flip", "[stql][operators]") {
  REQUIRE(flip(CompareOp::LessThan) == CompareOp::GreaterThan);
  REQUIRE(flip(CompareOp::LessEqual) == CompareOp::GreaterEqual);
  REQUIRE(flip(CompareOp::GreaterThan) == CompareOp::LessThan);
  REQUIRE(flip(CompareOp::GreaterEqual) == CompareOp::LessEqual);
  REQUIRE(flip(CompareOp::Equal) == CompareOp::Equal);
  REQUIRE(flip(CompareOp::NotEqual) == CompareOp::NotEqual);
}

// =============================================================================
// Boolean and Propositional Expressions
// =============================================================================

TEST_CASE("ConstExpr construction and to_string", "[stql][expressions][boolean]") {
  SECTION("True constant") {
    auto t = ConstExpr{true};
    REQUIRE(t.to_string() == "⊤");
  }

  SECTION("False constant") {
    auto f = ConstExpr{false};
    REQUIRE(f.to_string() == "⊥");
  }

  SECTION("Factory functions") {
    auto t = make_true();
    auto f = make_false();
    REQUIRE(std::holds_alternative<ConstExpr>(t));
    REQUIRE(std::holds_alternative<ConstExpr>(f));
  }
}

TEST_CASE("NotExpr construction and to_string", "[stql][expressions][boolean]") {
  SECTION("Negation of true") {
    auto t     = make_true();
    auto not_t = NotExpr{t};
    REQUIRE(not_t.to_string() == "¬(⊤)");
  }

  SECTION("Negation operator") {
    auto t     = make_true();
    auto not_t = ~t;
    REQUIRE(std::holds_alternative<NotExpr>(not_t));
  }
}

TEST_CASE("AndExpr construction and to_string", "[stql][expressions][boolean]") {
  SECTION("Conjunction of two expressions") {
    auto t        = make_true();
    auto f        = make_false();
    auto and_expr = AndExpr{{t, f}};
    REQUIRE(and_expr.to_string() == "(⊤ ∧ ⊥)");
  }

  SECTION("Conjunction operator") {
    auto t        = make_true();
    auto f        = make_false();
    auto and_expr = t & f;
    REQUIRE(std::holds_alternative<AndExpr>(and_expr));
  }

  SECTION("Multiple arguments") {
    auto t        = make_true();
    auto f        = make_false();
    auto and_expr = AndExpr{{t, f, t}};
    REQUIRE(and_expr.to_string() == "(⊤ ∧ ⊥ ∧ ⊤)");
  }

  SECTION("Throws on single argument") {
    auto t = make_true();
    REQUIRE_THROWS_AS(AndExpr{{t}}, std::invalid_argument);
  }
}

TEST_CASE("OrExpr construction and to_string", "[stql][expressions][boolean]") {
  SECTION("Disjunction of two expressions") {
    auto t       = make_true();
    auto f       = make_false();
    auto or_expr = OrExpr{{t, f}};
    REQUIRE(or_expr.to_string() == "(⊤ ∨ ⊥)");
  }

  SECTION("Disjunction operator") {
    auto t       = make_true();
    auto f       = make_false();
    auto or_expr = t | f;
    REQUIRE(std::holds_alternative<OrExpr>(or_expr));
  }

  SECTION("Multiple arguments") {
    auto t       = make_true();
    auto f       = make_false();
    auto or_expr = OrExpr{{t, f, t}};
    REQUIRE(or_expr.to_string() == "(⊤ ∨ ⊥ ∨ ⊤)");
  }

  SECTION("Throws on single argument") {
    auto t = make_true();
    REQUIRE_THROWS_AS(OrExpr{{t}}, std::invalid_argument);
  }
}

// =============================================================================
// Temporal Operators
// =============================================================================

TEST_CASE("NextExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Next with default step") {
    auto phi      = make_true();
    auto next_phi = NextExpr{phi};
    REQUIRE(next_phi.to_string() == "○(⊤)");
  }

  SECTION("Next with custom steps") {
    auto phi       = make_true();
    auto next3_phi = NextExpr{phi, 3};
    REQUIRE(next3_phi.to_string() == "○^3(⊤)");
  }

  SECTION("Factory function") {
    auto phi      = make_true();
    auto next_phi = next(phi, 5);
    REQUIRE(std::holds_alternative<NextExpr>(next_phi));
  }

  SECTION("Throws on zero steps") {
    auto phi = make_true();
    REQUIRE_THROWS_AS(NextExpr(phi, 0), std::invalid_argument);
  }
}

TEST_CASE("PreviousExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Previous with default step") {
    auto phi      = make_true();
    auto prev_phi = PreviousExpr{phi};
    REQUIRE(prev_phi.to_string() == "◦(⊤)");
  }

  SECTION("Previous with custom steps") {
    auto phi       = make_true();
    auto prev3_phi = PreviousExpr{phi, 3};
    REQUIRE(prev3_phi.to_string() == "◦^3(⊤)");
  }

  SECTION("Factory function") {
    auto phi      = make_true();
    auto prev_phi = previous(phi, 2);
    REQUIRE(std::holds_alternative<PreviousExpr>(prev_phi));
  }

  SECTION("Throws on zero steps") {
    auto phi = make_true();
    REQUIRE_THROWS_AS(PreviousExpr(phi, 0), std::invalid_argument);
  }
}

TEST_CASE("AlwaysExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Always expression") {
    auto phi        = make_true();
    auto always_phi = AlwaysExpr{phi};
    REQUIRE(always_phi.to_string() == "□(⊤)");
  }

  SECTION("Factory function") {
    auto phi        = make_true();
    auto always_phi = always(phi);
    REQUIRE(std::holds_alternative<AlwaysExpr>(always_phi));
  }
}

TEST_CASE("EventuallyExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Eventually expression") {
    auto phi            = make_true();
    auto eventually_phi = EventuallyExpr{phi};
    REQUIRE(eventually_phi.to_string() == "◇(⊤)");
  }

  SECTION("Factory function") {
    auto phi            = make_true();
    auto eventually_phi = eventually(phi);
    REQUIRE(std::holds_alternative<EventuallyExpr>(eventually_phi));
  }
}

TEST_CASE("UntilExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Until expression") {
    auto safe       = make_true();
    auto goal       = make_false();
    auto until_expr = UntilExpr{safe, goal};
    REQUIRE(until_expr.to_string() == "(⊤ U ⊥)");
  }

  SECTION("Factory function") {
    auto safe       = make_true();
    auto goal       = make_false();
    auto until_expr = until(safe, goal);
    REQUIRE(std::holds_alternative<UntilExpr>(until_expr));
  }
}

TEST_CASE("SinceExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Since expression") {
    auto stable     = make_true();
    auto init       = make_false();
    auto since_expr = SinceExpr{stable, init};
    REQUIRE(since_expr.to_string() == "(⊤ S ⊥)");
  }

  SECTION("Factory function") {
    auto stable     = make_true();
    auto init       = make_false();
    auto since_expr = since(stable, init);
    REQUIRE(std::holds_alternative<SinceExpr>(since_expr));
  }
}

TEST_CASE("ReleaseExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("Release expression") {
    auto trigger      = make_false();
    auto maintained   = make_true();
    auto release_expr = ReleaseExpr{trigger, maintained};
    REQUIRE(release_expr.to_string() == "(⊥ R ⊤)");
  }

  SECTION("Factory function") {
    auto trigger      = make_false();
    auto maintained   = make_true();
    auto release_expr = release(trigger, maintained);
    REQUIRE(std::holds_alternative<ReleaseExpr>(release_expr));
  }
}

TEST_CASE("BackToExpr construction and to_string", "[stql][expressions][temporal]") {
  SECTION("BackTo expression") {
    auto event       = make_false();
    auto condition   = make_true();
    auto backto_expr = BackToExpr{event, condition};
    REQUIRE(backto_expr.to_string() == "(⊥ B ⊤)");
  }

  SECTION("Factory function") {
    auto event       = make_false();
    auto condition   = make_true();
    auto backto_expr = backto(event, condition);
    REQUIRE(std::holds_alternative<BackToExpr>(backto_expr));
  }
}

// =============================================================================
// Quantifiers
// =============================================================================

TEST_CASE("ExistsExpr construction and to_string", "[stql][expressions][quantifiers]") {
  SECTION("Single object variable") {
    auto car         = ObjectVar{"car"};
    auto phi         = make_true();
    auto exists_expr = ExistsExpr{{car}, phi};
    REQUIRE(exists_expr.to_string() == "∃{car}@(⊤)");
  }

  SECTION("Multiple object variables") {
    auto car         = ObjectVar{"car"};
    auto pedestrian  = ObjectVar{"pedestrian"};
    auto phi         = make_true();
    auto exists_expr = ExistsExpr{{car, pedestrian}, phi};
    REQUIRE(exists_expr.to_string() == "∃{car, pedestrian}@(⊤)");
  }

  SECTION("Factory function") {
    auto car         = ObjectVar{"car"};
    auto phi         = make_true();
    auto exists_expr = exists({car}, phi);
    REQUIRE(std::holds_alternative<ExistsExpr>(exists_expr));
  }

  SECTION("Throws on empty variables") {
    auto phi = make_true();
    REQUIRE_THROWS_AS(ExistsExpr({}, phi), std::invalid_argument);
  }
}

TEST_CASE("ForallExpr construction and to_string", "[stql][expressions][quantifiers]") {
  SECTION("Single object variable") {
    auto car         = ObjectVar{"car"};
    auto phi         = make_true();
    auto forall_expr = ForallExpr{{car}, phi};
    REQUIRE(forall_expr.to_string() == "∀{car}@(⊤)");
  }

  SECTION("Multiple object variables") {
    auto car         = ObjectVar{"car"};
    auto truck       = ObjectVar{"truck"};
    auto phi         = make_true();
    auto forall_expr = ForallExpr{{car, truck}, phi};
    REQUIRE(forall_expr.to_string() == "∀{car, truck}@(⊤)");
  }

  SECTION("Factory function") {
    auto car         = ObjectVar{"car"};
    auto phi         = make_true();
    auto forall_expr = forall({car}, phi);
    REQUIRE(std::holds_alternative<ForallExpr>(forall_expr));
  }

  SECTION("Throws on empty variables") {
    auto phi = make_true();
    REQUIRE_THROWS_AS(ForallExpr({}, phi), std::invalid_argument);
  }
}

TEST_CASE("FreezeExpr construction and to_string", "[stql][expressions][quantifiers]") {
  SECTION("Freeze time variable only") {
    auto x           = TimeVar{"x"};
    auto phi         = make_true();
    auto freeze_expr = FreezeExpr{x, std::nullopt, phi};
    REQUIRE(freeze_expr.to_string() == "{x}.(⊤)");
  }

  SECTION("Freeze frame variable only") {
    auto f           = FrameVar{"f"};
    auto phi         = make_true();
    auto freeze_expr = FreezeExpr{std::nullopt, f, phi};
    REQUIRE(freeze_expr.to_string() == "{f}.(⊤)");
  }

  SECTION("Freeze both time and frame") {
    auto x           = TimeVar{"x"};
    auto f           = FrameVar{"f"};
    auto phi         = make_true();
    auto freeze_expr = FreezeExpr{x, f, phi};
    REQUIRE(freeze_expr.to_string() == "{x, f}.(⊤)");
  }

  SECTION("Factory functions") {
    auto x   = TimeVar{"x"};
    auto f   = FrameVar{"f"};
    auto phi = make_true();

    auto freeze_time = freeze(x, phi);
    REQUIRE(std::holds_alternative<FreezeExpr>(freeze_time));

    auto freeze_frame = freeze(f, phi);
    REQUIRE(std::holds_alternative<FreezeExpr>(freeze_frame));

    auto freeze_both = freeze(x, f, phi);
    REQUIRE(std::holds_alternative<FreezeExpr>(freeze_both));
  }

  SECTION("Throws on neither time nor frame") {
    auto phi = make_true();
    REQUIRE_THROWS_AS(FreezeExpr(std::nullopt, std::nullopt, phi), std::invalid_argument);
  }
}

// =============================================================================
// Time and Frame Constraints
// =============================================================================

TEST_CASE("TimeDiff construction and to_string", "[stql][constraints]") {
  auto x    = TimeVar{"x"};
  auto diff = TimeDiff{x, C_TIME};
  REQUIRE(diff.to_string() == "x - C_TIME");
}

TEST_CASE("FrameDiff construction and to_string", "[stql][constraints]") {
  auto f    = FrameVar{"f"};
  auto diff = FrameDiff{f, C_FRAME};
  REQUIRE(diff.to_string() == "f - C_FRAME");
}

TEST_CASE("TimeBoundExpr construction and to_string", "[stql][constraints]") {
  auto x     = TimeVar{"x"};
  auto diff  = TimeDiff{x, C_TIME};
  auto bound = TimeBoundExpr{diff, CompareOp::LessEqual, 5.0};
  REQUIRE(bound.to_string() == "(x - C_TIME <= 5.000000)");
}

TEST_CASE("FrameBoundExpr construction and to_string", "[stql][constraints]") {
  auto f     = FrameVar{"f"};
  auto diff  = FrameDiff{f, C_FRAME};
  auto bound = FrameBoundExpr{diff, CompareOp::LessEqual, 10};
  REQUIRE(bound.to_string() == "(f - C_FRAME <= 10)");
}

TEST_CASE("Difference operators", "[stql][constraints]") {
  SECTION("Time difference operator") {
    auto x    = TimeVar{"x"};
    auto diff = x - C_TIME;
    REQUIRE(diff.to_string() == "x - C_TIME");
  }

  SECTION("Frame difference operator") {
    auto f    = FrameVar{"f"};
    auto diff = f - C_FRAME;
    REQUIRE(diff.to_string() == "f - C_FRAME");
  }
}

// =============================================================================
// Object Identity and Class
// =============================================================================

TEST_CASE("ObjectIdCompareExpr construction and to_string", "[stql][expressions][object]") {
  SECTION("Object equality") {
    auto car1    = ObjectVar{"car1"};
    auto car2    = ObjectVar{"car2"};
    auto compare = ObjectIdCompareExpr{car1, CompareOp::Equal, car2};
    REQUIRE(compare.to_string() == "{car1 == car2}");
  }

  SECTION("Object inequality") {
    auto car1    = ObjectVar{"car1"};
    auto car2    = ObjectVar{"car2"};
    auto compare = ObjectIdCompareExpr{car1, CompareOp::NotEqual, car2};
    REQUIRE(compare.to_string() == "{car1 != car2}");
  }

  SECTION("Throws on invalid operator") {
    auto car1 = ObjectVar{"car1"};
    auto car2 = ObjectVar{"car2"};
    REQUIRE_THROWS_AS(ObjectIdCompareExpr(car1, CompareOp::LessThan, car2), std::invalid_argument);
  }
}

TEST_CASE("ClassFunc construction and to_string", "[stql][expressions][object]") {
  auto car        = ObjectVar{"car"};
  auto class_func = ClassFunc{car};
  REQUIRE(class_func.to_string() == "C(car)");
}

TEST_CASE("ClassCompareExpr construction and to_string", "[stql][expressions][object]") {
  SECTION("Class comparison with integer") {
    auto car        = ObjectVar{"car"};
    auto class_func = ClassFunc{car};
    auto compare    = ClassCompareExpr{class_func, CompareOp::Equal, 1};
    REQUIRE(compare.to_string() == "C(car) == 1");
  }

  SECTION("Class comparison with another class") {
    auto car         = ObjectVar{"car"};
    auto truck       = ObjectVar{"truck"};
    auto class_car   = ClassFunc{car};
    auto class_truck = ClassFunc{truck};
    auto compare     = ClassCompareExpr{class_car, CompareOp::Equal, class_truck};
    REQUIRE(compare.to_string() == "C(car) == C(truck)");
  }

  SECTION("Throws on invalid operator") {
    auto car        = ObjectVar{"car"};
    auto class_func = ClassFunc{car};
    REQUIRE_THROWS_AS(ClassCompareExpr(class_func, CompareOp::LessThan, 1), std::invalid_argument);
  }
}

// =============================================================================
// Probability/Confidence
// =============================================================================

TEST_CASE("ProbFunc construction and to_string", "[stql][expressions][probability]") {
  auto car       = ObjectVar{"car"};
  auto prob_func = ProbFunc{car};
  REQUIRE(prob_func.to_string() == "P(car)");
}

TEST_CASE("ProbCompareExpr construction and to_string", "[stql][expressions][probability]") {
  SECTION("Probability comparison with constant") {
    auto car       = ObjectVar{"car"};
    auto prob_func = ProbFunc{car};
    auto compare   = ProbCompareExpr{prob_func, CompareOp::GreaterEqual, 0.8};
    REQUIRE(compare.to_string() == "P(car) >= 0.800000");
  }

  SECTION("Probability comparison with another probability") {
    auto car        = ObjectVar{"car"};
    auto truck      = ObjectVar{"truck"};
    auto prob_car   = ProbFunc{car};
    auto prob_truck = ProbFunc{truck};
    auto compare    = ProbCompareExpr{prob_car, CompareOp::GreaterThan, prob_truck};
    REQUIRE(compare.to_string() == "P(car) > P(truck)");
  }
}

// =============================================================================
// Spatial Distance and Position
// =============================================================================

TEST_CASE("EuclideanDistFunc construction and to_string", "[stql][expressions][spatial]") {
  auto car       = ObjectVar{"car"};
  auto truck     = ObjectVar{"truck"};
  auto ref_car   = RefPoint{car, CoordRefPoint::Center};
  auto ref_truck = RefPoint{truck, CoordRefPoint::Center};
  auto dist      = EuclideanDistFunc{ref_car, ref_truck};
  REQUIRE(dist.to_string() == "ED(car.CT, truck.CT)");
}

TEST_CASE("DistCompareExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car       = ObjectVar{"car"};
  auto truck     = ObjectVar{"truck"};
  auto ref_car   = RefPoint{car, CoordRefPoint::Center};
  auto ref_truck = RefPoint{truck, CoordRefPoint::Center};
  auto dist_func = EuclideanDistFunc{ref_car, ref_truck};
  auto compare   = DistCompareExpr{dist_func, CompareOp::GreaterEqual, 10.0};
  REQUIRE(compare.to_string() == "ED(car.CT, truck.CT) >= 10.000000");
}

TEST_CASE("LatFunc construction and to_string", "[stql][expressions][spatial]") {
  auto car      = ObjectVar{"car"};
  auto ref      = RefPoint{car, CoordRefPoint::Center};
  auto lat_func = LatFunc{ref};
  REQUIRE(lat_func.to_string() == "Lat(car.CT)");
}

TEST_CASE("LonFunc construction and to_string", "[stql][expressions][spatial]") {
  auto car      = ObjectVar{"car"};
  auto ref      = RefPoint{car, CoordRefPoint::Center};
  auto lon_func = LonFunc{ref};
  REQUIRE(lon_func.to_string() == "Lon(car.CT)");
}

TEST_CASE("LatLonCompareExpr construction and to_string", "[stql][expressions][spatial]") {
  SECTION("Lateral comparison with constant") {
    auto car      = ObjectVar{"car"};
    auto ref      = RefPoint{car, CoordRefPoint::LeftMargin};
    auto lat_func = LatFunc{ref};
    auto compare  = LatLonCompareExpr{lat_func, CompareOp::GreaterThan, 2.0};
    REQUIRE(compare.to_string() == "Lat(car.LM) > 2.000000");
  }

  SECTION("Longitudinal comparison with constant") {
    auto car      = ObjectVar{"car"};
    auto ref      = RefPoint{car, CoordRefPoint::Center};
    auto lon_func = LonFunc{ref};
    auto compare  = LatLonCompareExpr{lon_func, CompareOp::LessThan, 100.0};
    REQUIRE(compare.to_string() == "Lon(car.CT) < 100.000000");
  }

  SECTION("Lateral comparison with another lateral") {
    auto car       = ObjectVar{"car"};
    auto truck     = ObjectVar{"truck"};
    auto ref_car   = RefPoint{car, CoordRefPoint::Center};
    auto ref_truck = RefPoint{truck, CoordRefPoint::Center};
    auto lat_car   = LatFunc{ref_car};
    auto lat_truck = LatFunc{ref_truck};
    auto compare   = LatLonCompareExpr{lat_car, CompareOp::Equal, lat_truck};
    REQUIRE(compare.to_string() == "Lat(car.CT) == Lat(truck.CT)");
  }
}

// =============================================================================
// Spatial Expressions
// =============================================================================

TEST_CASE("EmptySetExpr construction and to_string", "[stql][expressions][spatial]") {
  auto empty = EmptySetExpr{};
  REQUIRE(empty.to_string() == "∅");
}

TEST_CASE("UniverseSetExpr construction and to_string", "[stql][expressions][spatial]") {
  auto universe = UniverseSetExpr{};
  REQUIRE(universe.to_string() == "U");
}

TEST_CASE("BBoxExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car      = ObjectVar{"car"};
  auto car_bbox = BBoxExpr{car};
  REQUIRE(car_bbox.to_string() == "BB(car)");

  SECTION("Factory function") {
    auto car_car  = ObjectVar{"car"};
    auto car_bbox = bbox(car);
    REQUIRE(std::holds_alternative<BBoxExpr>(car_bbox));
  }
}

TEST_CASE("SpatialComplementExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car        = ObjectVar{"car"};
  auto bbox       = BBoxExpr{car};
  auto complement = SpatialComplementExpr{bbox};
  REQUIRE(complement.to_string() == "¬(BB(car))");

  SECTION("Factory function") {
    auto bbox       = BBoxExpr{ObjectVar{"car"}};
    auto complement = spatial_complement(bbox);
    REQUIRE(std::holds_alternative<SpatialComplementExpr>(complement));
  }
}

TEST_CASE("SpatialUnionExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car        = ObjectVar{"car"};
  auto truck      = ObjectVar{"truck"};
  auto bbox_car   = BBoxExpr{car};
  auto bbox_truck = BBoxExpr{truck};
  auto union_expr = SpatialUnionExpr{{bbox_car, bbox_truck}};
  REQUIRE(union_expr.to_string() == "(BB(car) ⊔ BB(truck))");

  SECTION("Factory function") {
    auto bbox_car   = BBoxExpr{ObjectVar{"car"}};
    auto bbox_truck = BBoxExpr{ObjectVar{"truck"}};
    auto union_expr = spatial_union({bbox_car, bbox_truck});
    REQUIRE(std::holds_alternative<SpatialUnionExpr>(union_expr));
  }

  SECTION("Throws on single argument") {
    auto bbox = BBoxExpr{ObjectVar{"car"}};
    REQUIRE_THROWS_AS(SpatialUnionExpr{{bbox}}, std::invalid_argument);
  }
}

TEST_CASE("SpatialIntersectExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car            = ObjectVar{"car"};
  auto lane           = ObjectVar{"lane"};
  auto bbox_car       = BBoxExpr{car};
  auto bbox_lane      = BBoxExpr{lane};
  auto intersect_expr = SpatialIntersectExpr{{bbox_car, bbox_lane}};
  REQUIRE(intersect_expr.to_string() == "(BB(car) ⊓ BB(lane))");

  SECTION("Factory function") {
    auto bbox_car       = BBoxExpr{ObjectVar{"car"}};
    auto bbox_lane      = BBoxExpr{ObjectVar{"lane"}};
    auto intersect_expr = spatial_intersect({bbox_car, bbox_lane});
    REQUIRE(std::holds_alternative<SpatialIntersectExpr>(intersect_expr));
  }

  SECTION("Throws on single argument") {
    auto bbox = BBoxExpr{ObjectVar{"car"}};
    REQUIRE_THROWS_AS(SpatialIntersectExpr{{bbox}}, std::invalid_argument);
  }
}

// =============================================================================
// Spatial Predicates
// =============================================================================

TEST_CASE("AreaFunc construction and to_string", "[stql][expressions][spatial]") {
  auto car       = ObjectVar{"car"};
  auto bbox      = BBoxExpr{car};
  auto area_func = AreaFunc{bbox};
  REQUIRE(area_func.to_string() == "Area(BB(car))");
}

TEST_CASE("AreaCompareExpr construction and to_string", "[stql][expressions][spatial]") {
  SECTION("Area comparison with constant") {
    auto car       = ObjectVar{"car"};
    auto bbox      = BBoxExpr{car};
    auto area_func = AreaFunc{bbox};
    auto compare   = AreaCompareExpr{area_func, CompareOp::GreaterEqual, 50.0};
    REQUIRE(compare.to_string() == "Area(BB(car)) >= 50.000000");
  }

  SECTION("Area comparison with another area") {
    auto car        = ObjectVar{"car"};
    auto truck      = ObjectVar{"truck"};
    auto area_car   = AreaFunc{BBoxExpr{car}};
    auto area_truck = AreaFunc{BBoxExpr{truck}};
    auto compare    = AreaCompareExpr{area_car, CompareOp::LessThan, area_truck};
    REQUIRE(compare.to_string() == "Area(BB(car)) < Area(BB(truck))");
  }
}

TEST_CASE("SpatialExistsExpr construction and to_string", "[stql][expressions][spatial]") {
  auto car           = ObjectVar{"car"};
  auto lane          = ObjectVar{"lane"};
  auto bbox_car      = BBoxExpr{car};
  auto bbox_lane     = BBoxExpr{lane};
  auto intersect     = SpatialIntersectExpr{{bbox_car, bbox_lane}};
  auto spexists_expr = SpatialExistsExpr{intersect};
  REQUIRE(spexists_expr.to_string() == "∃((BB(car) ⊓ BB(lane)))");

  SECTION("Factory function") {
    auto intersect =
        SpatialIntersectExpr{{BBoxExpr{ObjectVar{"car"}}, BBoxExpr{ObjectVar{"lane"}}}};
    auto spexists_expr = spatial_exists(intersect);
    REQUIRE(std::holds_alternative<SpatialExistsExpr>(spexists_expr));
  }
}

TEST_CASE("SpatialForallExpr construction and to_string", "[stql][expressions][spatial]") {
  auto free_space    = EmptySetExpr{};
  auto spforall_expr = SpatialForallExpr{free_space};
  REQUIRE(spforall_expr.to_string() == "∀(∅)");

  SECTION("Factory function") {
    auto free_space    = EmptySetExpr{};
    auto spforall_expr = spatial_forall(free_space);
    REQUIRE(std::holds_alternative<SpatialForallExpr>(spforall_expr));
  }
}

// =============================================================================
// Complex Formula Examples
// =============================================================================

TEST_CASE("Complex formula construction", "[stql][integration]") {
  SECTION("Car stays in left lane for 5 seconds") {
    // □(∃{car}@(C(car) = 1) → {x}.(□[0,5] ∃(BB(car) ⊓ LeftLane)))
    auto car       = ObjectVar{"car"};
    auto left_lane = ObjectVar{"left_lane"};
    auto x         = TimeVar{"x"};

    // C(car) = 1 (class check)
    auto class_check = ClassCompareExpr{ClassFunc{car}, CompareOp::Equal, 1};

    // BB(car) ⊓ LeftLane
    auto overlap = SpatialIntersectExpr{{BBoxExpr{car}, BBoxExpr{left_lane}}};

    // ∃(BB(car) ⊓ LeftLane)
    auto spatial_exists = SpatialExistsExpr{overlap};

    // Inner always (simplified without interval bounds for now)
    auto inner_always = AlwaysExpr{spatial_exists};

    // Freeze time
    auto freeze_expr = FreezeExpr{x, std::nullopt, inner_always};

    // Class check implies freeze expression
    auto implication = OrExpr{{NotExpr{class_check}, freeze_expr}};

    // Outer exists
    auto outer_exists = ExistsExpr{{car}, implication};

    // Outer always
    Expr formula = AlwaysExpr{outer_exists};

    // Just verify it constructs without errors
    REQUIRE(std::holds_alternative<AlwaysExpr>(formula));
  }

  SECTION("Next of eventually") {
    auto phi             = make_true();
    auto eventually_phi  = eventually(phi);
    auto next_eventually = next(eventually_phi);
    REQUIRE(std::holds_alternative<NextExpr>(next_eventually));
  }

  SECTION("Conjunction of temporal operators") {
    auto phi         = make_true();
    auto psi         = make_false();
    auto next_phi    = next(phi);
    auto prev_psi    = previous(psi);
    auto conjunction = next_phi & prev_psi;
    REQUIRE(std::holds_alternative<AndExpr>(conjunction));
  }
}
