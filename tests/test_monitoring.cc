#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <string>
#include <variant>

#include "percemon/monitoring.hpp"
#include "percemon/stql.hpp"

using namespace percemon::monitoring;
using namespace percemon::stql;

// =============================================================================
// Constants
// =============================================================================

TEST_CASE("UNBOUNDED constant value", "[monitoring][constants]") {
  REQUIRE(UNBOUNDED == std::numeric_limits<int64_t>::max());
}

// =============================================================================
// History Struct
// =============================================================================

TEST_CASE("History construction and members", "[monitoring][history]") {
  SECTION("Bounded history") {
    History hist{5};
    REQUIRE(hist.frames == 5);
  }

  SECTION("Zero history") {
    History hist{0};
    REQUIRE(hist.frames == 0);
  }

  SECTION("Large bounded history") {
    History hist{1000};
    REQUIRE(hist.frames == 1000);
  }

  SECTION("Unbounded history") {
    History hist{UNBOUNDED};
    REQUIRE(hist.frames == UNBOUNDED);
  }
}

TEST_CASE("History::to_string", "[monitoring][history]") {
  SECTION("Bounded history") {
    History hist{5};
    REQUIRE(hist.to_string() == "History{5 frames}");
  }

  SECTION("Zero history") {
    History hist{0};
    REQUIRE(hist.to_string() == "History{0 frames}");
  }

  SECTION("Unbounded history") {
    History hist{UNBOUNDED};
    REQUIRE(hist.to_string() == "History{unbounded}");
  }
}

TEST_CASE("History::is_bounded", "[monitoring][history]") {
  SECTION("Bounded history returns true") {
    History hist{5};
    REQUIRE(hist.is_bounded());
  }

  SECTION("Zero history is bounded") {
    History hist{0};
    REQUIRE(hist.is_bounded());
  }

  SECTION("Unbounded history returns false") {
    History hist{UNBOUNDED};
    REQUIRE_FALSE(hist.is_bounded());
  }

  SECTION("Large bounded history returns true") {
    History hist{10000};
    REQUIRE(hist.is_bounded());
  }
}

// =============================================================================
// Horizon Struct
// =============================================================================

TEST_CASE("Horizon construction and members", "[monitoring][horizon]") {
  SECTION("Bounded horizon") {
    Horizon horiz{10};
    REQUIRE(horiz.frames == 10);
  }

  SECTION("Zero horizon") {
    Horizon horiz{0};
    REQUIRE(horiz.frames == 0);
  }

  SECTION("Large bounded horizon") {
    Horizon horiz{500};
    REQUIRE(horiz.frames == 500);
  }

  SECTION("Unbounded horizon") {
    Horizon horiz{UNBOUNDED};
    REQUIRE(horiz.frames == UNBOUNDED);
  }
}

TEST_CASE("Horizon::to_string", "[monitoring][horizon]") {
  SECTION("Bounded horizon") {
    Horizon horiz{10};
    REQUIRE(horiz.to_string() == "Horizon{10 frames}");
  }

  SECTION("Zero horizon") {
    Horizon horiz{0};
    REQUIRE(horiz.to_string() == "Horizon{0 frames}");
  }

  SECTION("Unbounded horizon") {
    Horizon horiz{UNBOUNDED};
    REQUIRE(horiz.to_string() == "Horizon{unbounded}");
  }
}

TEST_CASE("Horizon::is_bounded", "[monitoring][horizon]") {
  SECTION("Bounded horizon returns true") {
    Horizon horiz{10};
    REQUIRE(horiz.is_bounded());
  }

  SECTION("Zero horizon is bounded") {
    Horizon horiz{0};
    REQUIRE(horiz.is_bounded());
  }

  SECTION("Unbounded horizon returns false") {
    Horizon horiz{UNBOUNDED};
    REQUIRE_FALSE(horiz.is_bounded());
  }

  SECTION("Large bounded horizon returns true") {
    Horizon horiz{5000};
    REQUIRE(horiz.is_bounded());
  }
}

// =============================================================================
// MonitoringRequirements Struct
// =============================================================================

TEST_CASE("MonitoringRequirements construction and members", "[monitoring][requirements]") {
  SECTION("Both bounded") {
    MonitoringRequirements reqs{.history = History{5}, .horizon = Horizon{10}};
    REQUIRE(reqs.history.frames == 5);
    REQUIRE(reqs.horizon.frames == 10);
  }

  SECTION("Unbounded history, bounded horizon") {
    MonitoringRequirements reqs{.history = History{UNBOUNDED}, .horizon = Horizon{10}};
    REQUIRE(reqs.history.frames == UNBOUNDED);
    REQUIRE(reqs.horizon.frames == 10);
  }

  SECTION("Bounded history, unbounded horizon") {
    MonitoringRequirements reqs{.history = History{5}, .horizon = Horizon{UNBOUNDED}};
    REQUIRE(reqs.history.frames == 5);
    REQUIRE(reqs.horizon.frames == UNBOUNDED);
  }

  SECTION("Both unbounded") {
    MonitoringRequirements reqs{.history = History{UNBOUNDED}, .horizon = Horizon{UNBOUNDED}};
    REQUIRE(reqs.history.frames == UNBOUNDED);
    REQUIRE(reqs.horizon.frames == UNBOUNDED);
  }

  SECTION("Zero history and horizon") {
    MonitoringRequirements reqs{.history = History{0}, .horizon = Horizon{0}};
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == 0);
  }
}

TEST_CASE("MonitoringRequirements::to_string", "[monitoring][requirements]") {
  SECTION("Bounded requirements") {
    MonitoringRequirements reqs{.history = History{5}, .horizon = Horizon{10}};
    std::string str = reqs.to_string();
    REQUIRE(str.find("History{5 frames}") != std::string::npos);
    REQUIRE(str.find("Horizon{10 frames}") != std::string::npos);
  }

  SECTION("Unbounded requirements") {
    MonitoringRequirements reqs{.history = History{UNBOUNDED}, .horizon = Horizon{UNBOUNDED}};
    std::string str = reqs.to_string();
    REQUIRE(str.find("History{unbounded}") != std::string::npos);
    REQUIRE(str.find("Horizon{unbounded}") != std::string::npos);
  }

  SECTION("Mixed requirements") {
    MonitoringRequirements reqs{.history = History{3}, .horizon = Horizon{UNBOUNDED}};
    std::string str = reqs.to_string();
    REQUIRE(str.find("History{3 frames}") != std::string::npos);
    REQUIRE(str.find("Horizon{unbounded}") != std::string::npos);
  }
}

// =============================================================================
// compute_requirements Function Tests
// =============================================================================
// Note: These are placeholder tests. The actual implementation of
// compute_requirements() needs to be provided for these tests to pass.

TEST_CASE("compute_requirements - simple formulas", "[monitoring][compute]") {
  SECTION("True constant requires no history or horizon") {
    auto phi  = make_true();
    auto reqs = compute_requirements(phi);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == 0);
  }

  SECTION("False constant requires no history or horizon") {
    auto phi  = make_false();
    auto reqs = compute_requirements(phi);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == 0);
  }
}

TEST_CASE("compute_requirements - next operator", "[monitoring][compute]") {
  SECTION("Next 1 frame") {
    auto phi     = make_true();
    auto formula = next(phi, 1);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == 1);
  }

  SECTION("Next 5 frames") {
    auto phi     = make_true();
    auto formula = next(phi, 5);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == 5);
  }
}

TEST_CASE("compute_requirements - previous operator", "[monitoring][compute]") {
  SECTION("Previous 1 frame") {
    auto phi     = make_true();
    auto formula = previous(phi, 1);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 1);
    REQUIRE(reqs.horizon.frames == 0);
  }

  SECTION("Previous 3 frames") {
    auto phi     = make_true();
    auto formula = previous(phi, 3);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 3);
    REQUIRE(reqs.horizon.frames == 0);
  }
}

TEST_CASE("compute_requirements - always operator", "[monitoring][compute]") {
  SECTION("Always (unbounded)") {
    auto phi     = make_true();
    auto formula = always(phi);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == UNBOUNDED);
  }
}

TEST_CASE("compute_requirements - eventually operator", "[monitoring][compute]") {
  SECTION("Eventually (unbounded)") {
    auto phi     = make_true();
    auto formula = eventually(phi);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 0);
    REQUIRE(reqs.horizon.frames == UNBOUNDED);
  }
}

TEST_CASE("compute_requirements - conjunction and disjunction", "[monitoring][compute]") {
  SECTION("And - max of children") {
    auto phi     = make_true();
    auto next3   = next(phi, 3);
    auto next5   = next(phi, 5);
    auto formula = next3 & next5;
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.horizon.frames == 5); // max(3, 5) = 5
  }

  SECTION("Or - max of children") {
    auto phi     = make_true();
    auto prev2   = previous(phi, 2);
    auto prev4   = previous(phi, 4);
    auto formula = prev2 | prev4;
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 4); // max(2, 4) = 4
  }
}

TEST_CASE("compute_requirements - negation", "[monitoring][compute]") {
  SECTION("Negation - same as child") {
    auto phi      = make_true();
    auto next_phi = next(phi, 3);
    auto formula  = ~next_phi;
    auto reqs     = compute_requirements(formula);
    REQUIRE(reqs.horizon.frames == 3);
  }
}

TEST_CASE("compute_requirements - quantifiers", "[monitoring][compute]") {
  SECTION("Exists - same as body") {
    auto car     = ObjectVar{"car"};
    auto phi     = next(make_true(), 2);
    auto formula = exists({car}, phi);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.horizon.frames == 2);
  }

  SECTION("Forall - same as body") {
    auto car     = ObjectVar{"car"};
    auto phi     = previous(make_true(), 3);
    auto formula = forall({car}, phi);
    auto reqs    = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 3);
  }
}

TEST_CASE("compute_requirements - time-based constraints with FPS", "[monitoring][compute]") {
  SECTION("Time bound in future scope (trivially satisfied)") {
    // Formula: {x}.(eventually((x - C_TIME) <= 5.0))
    // In a future scope, (x - C_TIME) is always decreasing, so this is trivially satisfied
    // Result: horizon = UNBOUNDED
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    auto body       = eventually(time_bound);
    auto formula    = freeze(x, body);

    auto reqs = compute_requirements(formula, 10.0); // 10 FPS
    REQUIRE(reqs.horizon.frames == UNBOUNDED);       // Trivially satisfied in future scope
  }

  SECTION("Frame bound direct in future scope (trivially satisfied)") {
    // Formula: {f}.(eventually((f - C_FRAME) <= 10))
    // In a future scope, (f - C_FRAME) is always decreasing, so this is trivially satisfied
    auto f           = FrameVar{"f"};
    auto frame_diff  = FrameDiff{f, C_FRAME};
    auto frame_bound = FrameBoundExpr{frame_diff, CompareOp::LessEqual, 10};
    auto formula     = freeze(f, eventually(frame_bound));

    auto reqs = compute_requirements(formula);
    REQUIRE(reqs.horizon.frames == UNBOUNDED); // Trivially satisfied in future scope
  }
}

TEST_CASE("compute_requirements - complex formulas", "[monitoring][compute]") {
  SECTION("Mixed past and future operators") {
    auto phi     = make_true();
    auto next5   = next(phi, 5);
    auto prev3   = previous(phi, 3);
    auto formula = next5 & prev3;

    auto reqs = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 3);
    REQUIRE(reqs.horizon.frames == 5);
  }
}

// =============================================================================
// is_past_time_formula Function Tests
// =============================================================================
// Note: These are placeholder tests. The actual implementation of
// is_past_time_formula() needs to be provided for these tests to pass.

TEST_CASE("is_past_time_formula - past formulas", "[monitoring][past]") {
  SECTION("True constant is past-time") {
    auto phi = make_true();
    REQUIRE(is_past_time_formula(phi));
  }

  SECTION("Previous operator is past-time") {
    auto phi     = make_true();
    auto formula = previous(phi, 5);
    REQUIRE(is_past_time_formula(formula));
  }

  SECTION("Since operator is past-time") {
    auto stable  = make_true();
    auto init    = make_false();
    auto formula = since(stable, init);
    REQUIRE(is_past_time_formula(formula));
  }
}

TEST_CASE("is_past_time_formula - future formulas", "[monitoring][past]") {
  SECTION("Next operator is not past-time") {
    auto phi     = make_true();
    auto formula = next(phi, 1);
    REQUIRE_FALSE(is_past_time_formula(formula));
  }

  SECTION("Always operator is not past-time") {
    auto phi     = make_true();
    auto formula = always(phi);
    REQUIRE_FALSE(is_past_time_formula(formula));
  }

  SECTION("Until operator is not past-time") {
    auto safe    = make_true();
    auto goal    = make_false();
    auto formula = until(safe, goal);
    REQUIRE_FALSE(is_past_time_formula(formula));
  }
}

TEST_CASE("is_past_time_formula - mixed formulas", "[monitoring][past]") {
  SECTION("Mixed past and future is not past-time") {
    auto phi      = make_true();
    auto next_phi = next(phi, 1);
    auto prev_phi = previous(phi, 1);
    auto formula  = next_phi & prev_phi;
    REQUIRE_FALSE(is_past_time_formula(formula));
  }
}

// =============================================================================
// Constrained Future-Time Operators
// =============================================================================

TEST_CASE(
    "compute_requirements - constrained future operators (horizon bounding)",
    "[monitoring][compute][constrained]") {
  SECTION("Eventually with meaningful forward constraint") {
    // Formula: {x}.(◇((C_TIME - x) <= 5.0))
    // Eventually creates a future scope
    // Constraint (C_TIME - x) <= 5.0 is meaningful in future scope
    // With fps=10, 5 seconds = 50 frames
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{C_TIME, x}; // C_TIME - x form
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    auto body       = eventually(time_bound);
    auto formula    = freeze(x, body);

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.horizon.frames == 50); // 5 seconds * 10 fps = 50 frames
    REQUIRE(reqs.history.frames == 0);
  }

  SECTION("Always with meaningful forward constraint") {
    // Formula: {x}.(□((C_TIME - x) <= 10.0))
    // Always creates a future scope
    // With fps=5, 10 seconds = 50 frames
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{C_TIME, x};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 10.0};
    auto body       = always(time_bound);
    auto formula    = freeze(x, body);

    auto reqs = compute_requirements(formula, 5.0);
    REQUIRE(reqs.horizon.frames == 50); // 10 seconds * 5 fps = 50 frames
  }

  SECTION("Eventually with frame bound constraint") {
    // Formula: {f}.(◇((C_FRAME - f) <= 20))
    auto f           = FrameVar{"f"};
    auto frame_diff  = FrameDiff{C_FRAME, f};
    auto frame_bound = FrameBoundExpr{frame_diff, CompareOp::LessEqual, 20};
    auto formula     = freeze(f, eventually(frame_bound));

    auto reqs = compute_requirements(formula);
    REQUIRE(reqs.horizon.frames == 20);
    REQUIRE(reqs.history.frames == 0);
  }

  SECTION("Until with meaningful constraint bounds result") {
    // Formula: until(stable, eventually_goal) where goal has constraint
    // until(φ1, ◇((C_TIME - x) <= 3.0)) with fps=10
    // Result: horizon = 30 frames (from the constraint)
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{C_TIME, x};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 3.0};
    auto goal       = eventually(time_bound);
    auto stable     = make_true();
    auto formula    = until(stable, goal);
    auto frz        = freeze(x, formula);

    auto reqs = compute_requirements(frz, 10.0);
    REQUIRE(reqs.horizon.frames == 30); // 3 seconds * 10 fps
  }

  SECTION("Release with constraint bounds") {
    // Formula: release(φ1, φ2) where φ2 has temporal constraint
    // Similar behavior to until for horizon computation
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{C_TIME, x};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 2.0};
    auto phi2       = eventually(time_bound);
    auto phi1       = make_true();
    auto formula    = release(phi1, phi2);
    auto frz        = freeze(x, formula);

    auto reqs = compute_requirements(frz, 5.0);
    REQUIRE(reqs.horizon.frames == 10); // 2 seconds * 5 fps
  }
}

TEST_CASE(
    "compute_requirements - nested future operators add horizons",
    "[monitoring][compute][constrained][nested]") {
  SECTION("Next wrapping constrained Eventually") {
    // Formula: ○(◇((C_TIME - x) <= 5.0))
    // next adds 1 frame, eventually constraint adds 50 frames (with 10 fps)
    // Total horizon = 1 + 50 = 51
    auto x               = TimeVar{"x"};
    auto time_diff       = TimeDiff{C_TIME, x};
    auto time_bound      = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    auto eventually_expr = eventually(time_bound);
    auto formula         = freeze(x, next(eventually_expr, 1));

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.horizon.frames == 51); // 1 (next) + 50 (eventually constraint)
  }

  SECTION("Next wrapping constrained Always") {
    // Formula: next(5, □((C_TIME - x) <= 2.0))
    // next adds 5 frames, always constraint adds 30 frames (with 15 fps)
    // Total horizon = 5 + 30 = 35
    auto x           = TimeVar{"x"};
    auto time_diff   = TimeDiff{C_TIME, x};
    auto time_bound  = TimeBoundExpr{time_diff, CompareOp::LessEqual, 2.0};
    auto always_expr = always(time_bound);
    auto formula     = freeze(x, next(always_expr, 5));

    auto reqs = compute_requirements(formula, 15.0);
    REQUIRE(reqs.horizon.frames == 35); // 5 (next) + 30 (always constraint)
  }

  SECTION("Constrained Eventually inside Next (nested scopes)") {
    // Formula: next(◇((C_TIME - x) <= 3.0), 2)
    // Inner eventually constraint: 30 frames (3.0 * 10 fps)
    // Outer next adds: 2 frames
    // Total: 2 + 30 = 32
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{C_TIME, x};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 3.0};
    auto inner      = freeze(x, eventually(time_bound));
    auto formula    = next(inner, 2);

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.horizon.frames == 32); // 2 (next) + 30 (eventually)
  }

  SECTION("Double nested future operators with constraints") {
    // Formula: next(next(◇((C_TIME - x) <= 1.0), 2), 3)
    // Innermost eventually constraint: 5 frames (1.0 * 5 fps)
    // Middle next adds: 2 frames
    // Outer next adds: 3 frames
    // Total: 3 + 2 + 5 = 10
    auto x               = TimeVar{"x"};
    auto time_diff       = TimeDiff{C_TIME, x};
    auto time_bound      = TimeBoundExpr{time_diff, CompareOp::LessEqual, 1.0};
    auto eventually_expr = eventually(time_bound);
    auto inner           = freeze(x, next(eventually_expr, 2));
    auto formula         = next(inner, 3);

    auto reqs = compute_requirements(formula, 5.0);
    REQUIRE(reqs.horizon.frames == 10); // 3 + 2 + 5
  }
}

// =============================================================================
// Constrained Past-Time Operators
// =============================================================================

TEST_CASE(
    "compute_requirements - constrained past operators (history bounding)",
    "[monitoring][compute][constrained][past]") {
  SECTION("Sometimes with meaningful backward constraint") {
    // Formula: {x}.(⧇((x - C_TIME) <= 5.0))
    // Sometimes creates a past scope (looks backward from current time)
    // Constraint (x - C_TIME) <= 5.0 is meaningful in past scope
    // With fps=10, 5 seconds = 50 frames
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME}; // x - C_TIME form
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    // Note: "Sometimes" is the past dual of "Eventually"
    // For now, we'll use Since which is simpler
    auto body    = since(make_true(), time_bound);
    auto formula = freeze(x, body);

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.history.frames == 50); // 5 seconds * 10 fps = 50 frames
    REQUIRE(reqs.horizon.frames == 0);
  }

  SECTION("Holds with meaningful backward constraint") {
    // Formula: {x}.(■((x - C_TIME) <= 10.0))
    // Holds creates a past scope
    // With fps=5, 10 seconds = 50 frames
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 10.0};
    auto body       = since(make_true(), time_bound);
    auto formula    = freeze(x, body);

    auto reqs = compute_requirements(formula, 5.0);
    REQUIRE(reqs.history.frames == 50); // 10 seconds * 5 fps = 50 frames
  }

  SECTION("Since with meaningful backward constraint") {
    // Formula: {x}.(■((x - C_TIME) <= 8.0) S φ)
    // With fps=2, 8 seconds = 16 frames
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 8.0};
    auto phi        = since(time_bound, make_false());
    auto formula    = freeze(x, phi);

    auto reqs = compute_requirements(formula, 2.0);
    REQUIRE(reqs.history.frames == 16); // 8 seconds * 2 fps
  }

  SECTION("BackTo with frame bound constraint") {
    // Formula: {f}.(φ₁ B ((f - C_FRAME) <= 15))
    auto f           = FrameVar{"f"};
    auto frame_diff  = FrameDiff{f, C_FRAME};
    auto frame_bound = FrameBoundExpr{frame_diff, CompareOp::LessEqual, 15};
    auto formula     = freeze(f, backto(make_true(), frame_bound));

    auto reqs = compute_requirements(formula);
    REQUIRE(reqs.history.frames == 15);
    REQUIRE(reqs.horizon.frames == 0);
  }
}

TEST_CASE(
    "compute_requirements - nested past operators add histories",
    "[monitoring][compute][constrained][nested][past]") {
  SECTION("Previous wrapping constrained Since") {
    // Formula: ◦({x}.(■((x - C_TIME) <= 5.0) S ⊤))
    // previous adds 1 frame history, since constraint adds 50 frames (with 10 fps)
    // Total history = 1 + 50 = 51
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    auto since_expr = since(time_bound, make_true());
    auto formula    = freeze(x, previous(since_expr, 1));

    auto formula_str = std::visit([](const auto& e) { return e.to_string(); }, formula);
    CAPTURE(formula_str);
    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.history.frames == 51); // 1 (previous) + 50 (since constraint)
  }

  SECTION("Previous wrapping constrained BackTo") {
    // Formula: previous(3, {x}.(φ₁ B ((x - C_TIME) <= 2.0)))
    // previous adds 3 frames history, backto constraint adds 30 frames (with 15 fps)
    // Total history = 3 + 30 = 33
    auto x           = TimeVar{"x"};
    auto time_diff   = TimeDiff{x, C_TIME};
    auto time_bound  = TimeBoundExpr{time_diff, CompareOp::LessEqual, 2.0};
    auto backto_expr = backto(make_true(), time_bound);
    auto formula     = freeze(x, previous(backto_expr, 3));

    auto reqs = compute_requirements(formula, 15.0);
    REQUIRE(reqs.history.frames == 33); // 3 (previous) + 30 (backto constraint)
  }

  SECTION("Constrained Since inside Previous (nested scopes)") {
    // Formula: previous(◇({x}.(■((x - C_TIME) <= 3.0) S ⊤)), 2)
    // Inner since constraint: 30 frames (3.0 * 10 fps)
    // Outer previous adds: 2 frames
    // Total: 2 + 30 = 32
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 3.0};
    auto inner      = freeze(x, since(time_bound, make_true()));
    auto formula    = previous(inner, 2);

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.history.frames == 32); // 2 (previous) + 30 (since)
  }

  SECTION("Double nested past operators with constraints") {
    // Formula: previous(previous({x}.(■((x - C_TIME) <= 1.0) S ⊤), 2), 3)
    // Innermost since constraint: 5 frames (1.0 * 5 fps)
    // Middle previous adds: 2 frames
    // Outer previous adds: 3 frames
    // Total: 3 + 2 + 5 = 10
    auto x          = TimeVar{"x"};
    auto time_diff  = TimeDiff{x, C_TIME};
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 1.0};
    auto since_expr = since(time_bound, make_true());
    auto inner      = freeze(x, previous(since_expr, 2));
    auto formula    = previous(inner, 3);

    auto reqs = compute_requirements(formula, 5.0);
    REQUIRE(reqs.history.frames == 10); // 3 + 2 + 5
  }
}

// =============================================================================
// Mixed Future and Past Operators with Constraints
// =============================================================================

TEST_CASE(
    "compute_requirements - mixed temporal operators with constraints",
    "[monitoring][compute][constrained][mixed]") {
  SECTION("Next future operator + Previous past operator with constraints") {
    // Formula: (next(eventually((C_TIME - x) <= 5.0), 2)) & (previous(since((y - C_TIME) <= 3.0,
    // true), 1)) Future part: 2 + 50 (fps=10) = 52 horizon Past part: 1 + 30 = 31 history
    auto x       = TimeVar{"x"};
    auto y       = TimeVar{"y"};
    auto x_diff  = TimeDiff{C_TIME, x};
    auto y_diff  = TimeDiff{y, C_TIME};
    auto x_bound = TimeBoundExpr{x_diff, CompareOp::LessEqual, 5.0};
    auto y_bound = TimeBoundExpr{y_diff, CompareOp::LessEqual, 3.0};

    auto future_part = freeze(x, next(eventually(x_bound), 2));
    auto past_part   = freeze(y, previous(since(y_bound, make_true()), 1));
    auto formula     = future_part & past_part;

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.horizon.frames == 52); // max(52 future) or 2 + 50
    REQUIRE(reqs.history.frames == 31); // max(31 past) or 1 + 30
  }

  SECTION("Unconstrained unbounded operators dominate") {
    // Formula: always(true) & (next(eventually((C_TIME - x) <= 5.0), 2))
    // always(true) → horizon = UNBOUNDED
    // Since AND takes max, result is UNBOUNDED
    auto x       = TimeVar{"x"};
    auto x_diff  = TimeDiff{C_TIME, x};
    auto x_bound = TimeBoundExpr{x_diff, CompareOp::LessEqual, 5.0};

    auto unbounded   = always(make_true());
    auto constrained = freeze(x, next(eventually(x_bound), 2));
    auto formula     = unbounded & constrained;

    auto reqs = compute_requirements(formula, 10.0);
    REQUIRE(reqs.horizon.frames == UNBOUNDED); // always() returns unbounded
  }
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("MonitoringRequirements can be compared", "[monitoring][integration]") {
  SECTION("Equal requirements") {
    MonitoringRequirements reqs1{.history = History{5}, .horizon = Horizon{10}};
    MonitoringRequirements reqs2{.history = History{5}, .horizon = Horizon{10}};
    REQUIRE(reqs1.history.frames == reqs2.history.frames);
    REQUIRE(reqs1.horizon.frames == reqs2.horizon.frames);
  }

  SECTION("Different requirements") {
    MonitoringRequirements reqs1{.history = History{5}, .horizon = Horizon{10}};
    MonitoringRequirements reqs2{.history = History{3}, .horizon = Horizon{10}};
    REQUIRE(reqs1.history.frames != reqs2.history.frames);
    REQUIRE(reqs1.horizon.frames == reqs2.horizon.frames);
  }
}

TEST_CASE("Combining History and Horizon constraints", "[monitoring][integration]") {
  SECTION("Max of multiple histories") {
    History h1{5};
    History h2{10};
    History h3{3};
    int64_t max_history = std::max({h1.frames, h2.frames, h3.frames});
    REQUIRE(max_history == 10);
  }

  SECTION("Max of multiple horizons") {
    Horizon hz1{7};
    Horizon hz2{15};
    Horizon hz3{2};
    int64_t max_horizon = std::max({hz1.frames, hz2.frames, hz3.frames});
    REQUIRE(max_horizon == 15);
  }

  SECTION("Unbounded wins in max") {
    History h1{5};
    History h2{UNBOUNDED};
    History h3{100};
    int64_t max_history = std::max({h1.frames, h2.frames, h3.frames});
    REQUIRE(max_history == UNBOUNDED);
  }
}
