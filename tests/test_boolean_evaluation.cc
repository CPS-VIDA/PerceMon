#include "percemon2/evaluation.hpp"
#include "percemon2/online_monitor.hpp"
#include "percemon2/stql.hpp"

#include <catch2/catch_test_macros.hpp>
#include <deque>

using namespace percemon2;
using namespace percemon2::monitoring;
using namespace percemon2::stql;

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

/**
 * Create a simple frame with no objects
 */
auto make_empty_frame(int frame_num = 0, double timestamp = 0.0) -> datastream::Frame {
  return datastream::Frame{
      .timestamp = timestamp,
      .frame_num = frame_num,
      .size_x    = 1920,
      .size_y    = 1080,
      .objects   = {}};
}

/**
 * Create a frame with a single car object
 */
auto make_frame_with_car(int frame_num = 0, double timestamp = 0.0) -> datastream::Frame {
  datastream::Frame frame = make_empty_frame(frame_num, timestamp);
  datastream::Object car{
      .object_class = 1,
      .probability  = 0.95,
      .bbox = {.xmin = 100.0, .xmax = 200.0, .ymin = 50.0, .ymax = 150.0} // xmin, xmax, ymin, ymax
  };
  frame.objects["car_001"] = car;
  return frame;
}

/**
 * Create a frame with multiple objects
 */
auto make_frame_with_multiple_objects(int frame_num = 0, double timestamp = 0.0)
    -> datastream::Frame {
  datastream::Frame frame = make_empty_frame(frame_num, timestamp);

  datastream::Object car{
      .object_class = 1,
      .probability  = 0.95,
      .bbox         = {.xmin = 100.0, .xmax = 200.0, .ymin = 50.0, .ymax = 150.0}};
  frame.objects["car_001"] = car;

  datastream::Object person{
      .object_class = 2,
      .probability  = 0.85,
      .bbox         = {.xmin = 300.0, .xmax = 350.0, .ymin = 0.0, .ymax = 200.0}};
  frame.objects["person_001"] = person;

  return frame;
}

// ============================================================================
// Unit Tests: Propositional Operators
// ============================================================================

TEST_CASE("BooleanEvaluator - Propositional Operators", "[evaluation][boolean]") {
  BooleanEvaluator evaluator;
  auto frame = make_empty_frame();
  std::deque<datastream::Frame> empty_history, empty_horizon;

  SECTION("ConstExpr - true") {
    auto formula = make_true();
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("ConstExpr - false") {
    auto formula = make_false();
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("NotExpr - negation") {
    auto formula = ~make_true();
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));

    formula = ~make_false();
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("AndExpr - both true") {
    auto formula = make_true() & make_true();
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("AndExpr - one false") {
    auto formula = make_true() & make_false();
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("OrExpr - one true") {
    auto formula = make_true() | make_false();
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("OrExpr - both false") {
    auto formula = make_false() | make_false();
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }
}

// ============================================================================
// Unit Tests: Temporal Operators (Future-Time)
// ============================================================================

TEST_CASE("BooleanEvaluator - Future-Time Temporal Operators", "[evaluation][temporal][future]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_history;

  SECTION("NextExpr - with future frame available") {
    auto frame1 = make_empty_frame(0, 0.0);
    auto frame2 = make_frame_with_car(1, 1.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame2);

    auto formula = next(make_true());
    REQUIRE(evaluator.evaluate(formula, frame1, empty_history, horizon));
  }

  SECTION("NextExpr - no future frame") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> empty_horizon;

    auto formula = next(make_true());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("AlwaysExpr - all frames satisfy") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));

    auto formula = always(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("AlwaysExpr - some frame violates") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0)); // This will violate

    auto formula = always(make_false());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("EventuallyExpr - at least one frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_frame_with_car(2, 2.0)); // This satisfies

    auto formula = eventually(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("EventuallyExpr - no frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));

    auto formula = eventually(make_false());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }
}

// ============================================================================
// Unit Tests: Temporal Operators (Past-Time)
// ============================================================================

TEST_CASE("BooleanEvaluator - Past-Time Temporal Operators", "[evaluation][temporal][past]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_horizon;

  SECTION("PreviousExpr - with past frame available") {
    auto frame = make_empty_frame(1, 1.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));

    auto formula = previous(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("PreviousExpr - no past frame") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> empty_history;

    auto formula = previous(make_true());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("HoldsExpr - all past frames satisfy") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto formula = holds(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SometimesExpr - at least one past frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_frame_with_car(1, 1.0));

    auto formula = sometimes(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }
}

// ============================================================================
// Unit Tests: Constraint Operators
// ============================================================================

TEST_CASE("BooleanEvaluator - Constraint Operators", "[evaluation][constraints]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_history, empty_horizon;

  SECTION("TimeBoundExpr - time comparison") {
    auto frame = make_empty_frame(0, 0.0);

    // Create freeze and constraint: {x}.(eventually((C_TIME - x) <= 5))
    // This is tested indirectly through OnlineMonitor
    // For now, test that unbound variable returns false
    auto x         = TimeVar{"x"};
    auto time_diff = TimeBoundExpr{C_TIME - x, CompareOp::LessEqual, 5.0};

    // Without freeze, should fail (unbound variable)
    REQUIRE_THROWS_AS(
        evaluator.evaluate(time_diff, frame, empty_history, empty_horizon), std::logic_error);
  }

  SECTION("FrameBoundExpr - frame comparison") {
    auto frame = make_empty_frame(10, 0.0);

    // Without freeze, should fail (unbound variable)
    auto f          = FrameVar{"f"};
    auto frame_diff = FrameBoundExpr{C_FRAME - f, CompareOp::LessEqual, 5};

    REQUIRE_THROWS_AS(
        evaluator.evaluate(frame_diff, frame, empty_history, empty_horizon), std::logic_error);
  }
}

// ============================================================================
// Unit Tests: Quantifiers
// ============================================================================

TEST_CASE("BooleanEvaluator - Quantifiers", "[evaluation][quantifiers]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_history, empty_horizon;

  SECTION("ExistsExpr - empty frame") {
    auto frame = make_empty_frame();

    auto id_var  = ObjectVar{"car"};
    auto formula = exists({id_var}, make_true());

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("ForallExpr - empty frame") {
    auto frame = make_empty_frame();

    auto id_var  = ObjectVar{"car"};
    auto formula = forall({id_var}, make_true());

    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("ExistsExpr - with objects") {
    auto frame = make_frame_with_multiple_objects();

    auto id_var  = ObjectVar{"id"};
    auto formula = exists({id_var}, make_true());

    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  SECTION("ForallExpr - with objects") {
    auto frame = make_frame_with_multiple_objects();

    auto id_var  = ObjectVar{"id"};
    auto formula = forall({id_var}, make_true());

    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }
}

// ============================================================================
// Unit Tests: Perception Operators
// ============================================================================

TEST_CASE("BooleanEvaluator - Perception Operators", "[evaluation][perception]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_history, empty_horizon;

  SECTION("ClassCompareExpr - unbound object var") {
    auto frame = make_empty_frame();

    auto id_var     = ObjectVar{"car"};
    auto class_expr = ClassCompareExpr{ClassFunc{id_var}, CompareOp::Equal, 1};

    REQUIRE_THROWS_AS(
        evaluator.evaluate(class_expr, frame, empty_history, empty_horizon), std::logic_error);
  }

  SECTION("ProbCompareExpr - unbound object var") {
    auto frame = make_empty_frame();

    auto id_var    = ObjectVar{"car"};
    auto prob_expr = ProbCompareExpr{ProbFunc{id_var}, CompareOp::GreaterEqual, 0.8};

    REQUIRE_THROWS_AS(
        evaluator.evaluate(prob_expr, frame, empty_history, empty_horizon), std::logic_error);
  }
}

// ============================================================================
// Integration Tests: OnlineMonitor
// ============================================================================

TEST_CASE("OnlineMonitor - Basic Operation", "[online_monitor][integration]") {
  // Create a simple monitorable formula
  auto formula = sometimes(make_true());

  REQUIRE_NOTHROW(OnlineMonitor(formula, 1.0));
}

TEST_CASE("OnlineMonitor - Reject Unbounded Formula", "[online_monitor][integration]") {
  // Create an unbounded formula (always without constraint)
  auto formula = always(make_true());

  REQUIRE_THROWS(OnlineMonitor(formula, 1.0));
}

TEST_CASE("OnlineMonitor - Streaming Evaluation", "[online_monitor][integration]") {
  // Simple test: eventually true should become true after any frame
  auto formula = sometimes(make_true());
  OnlineMonitor monitor(formula, 1.0);

  auto frame1 = make_empty_frame(0, 0.0);
  auto frame2 = make_empty_frame(1, 1.0);

  // First frame
  bool verdict1 = monitor.evaluate(frame1);
  REQUIRE(verdict1); // Eventually true with any frame satisfies

  // Second frame
  bool verdict2 = monitor.evaluate(frame2);
  REQUIRE(verdict2);
}

// ============================================================================
// Property-Based Tests
// ============================================================================

TEST_CASE("BooleanEvaluator - Property: Not Not φ ≡ φ", "[evaluation][properties]") {
  BooleanEvaluator evaluator;
  auto frame = make_empty_frame();
  std::deque<datastream::Frame> empty_history, empty_horizon;

  auto formula_true  = make_true();
  auto formula_false = make_false();

  bool result_true = evaluator.evaluate(formula_true, frame, empty_history, empty_horizon);
  bool result_not_not_true =
      evaluator.evaluate(~(~formula_true), frame, empty_history, empty_horizon);

  REQUIRE(result_true == result_not_not_true);

  bool result_false = evaluator.evaluate(formula_false, frame, empty_history, empty_horizon);
  bool result_not_not_false =
      evaluator.evaluate(~(~formula_false), frame, empty_history, empty_horizon);

  REQUIRE(result_false == result_not_not_false);
}

TEST_CASE("BooleanEvaluator - Property: De Morgan's Laws", "[evaluation][properties]") {
  BooleanEvaluator evaluator;
  auto frame = make_empty_frame();
  std::deque<datastream::Frame> empty_history, empty_horizon;

  // ~(φ ∧ ψ) ≡ ~φ ∨ ~ψ
  auto phi = make_true();
  auto psi = make_false();

  auto lhs = ~(phi & psi);
  auto rhs = (~phi) | (~psi);

  bool result_lhs = evaluator.evaluate(lhs, frame, empty_history, empty_horizon);
  bool result_rhs = evaluator.evaluate(rhs, frame, empty_history, empty_horizon);

  REQUIRE(result_lhs == result_rhs);
}
