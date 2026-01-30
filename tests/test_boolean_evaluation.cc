#include "percemon/evaluation.hpp"
#include "percemon/online_monitor.hpp"
#include "percemon/stql.hpp"

#include <catch2/catch_test_macros.hpp>
#include <deque>

using namespace percemon;
using namespace percemon::monitoring;
using namespace percemon::stql;

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

// Labels to int mapping in these tests.
constexpr int CAR    = 1;
constexpr int PERSON = 2;

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
      .object_class = CAR,
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
      .object_class = CAR,
      .probability  = 0.95,
      .bbox         = {.xmin = 100.0, .xmax = 200.0, .ymin = 50.0, .ymax = 150.0}};
  frame.objects["car_001"] = car;

  datastream::Object person{
      .object_class = PERSON,
      .probability  = 0.85,
      .bbox         = {.xmin = 300.0, .xmax = 350.0, .ymin = 0.0, .ymax = 200.0}};
  frame.objects["person_001"] = person;

  return frame;
}

/**
 * Create a frame with a car object of specific probability
 */
auto make_frame_with_car_probability(
    int frame_num      = 0,
    double timestamp   = 0.0,
    double probability = 0.95) -> datastream::Frame {
  datastream::Frame frame = make_empty_frame(frame_num, timestamp);
  datastream::Object car{
      .object_class = CAR,
      .probability  = probability,
      .bbox         = {.xmin = 100.0, .xmax = 200.0, .ymin = 50.0, .ymax = 150.0}};
  frame.objects["car_001"] = car;
  return frame;
}

/**
 * Create a frame with a person object
 */
auto make_frame_with_person(int frame_num = 0, double timestamp = 0.0) -> datastream::Frame {
  datastream::Frame frame = make_empty_frame(frame_num, timestamp);
  datastream::Object person{
      .object_class = PERSON,
      .probability  = 0.85,
      .bbox         = {.xmin = 300.0, .xmax = 350.0, .ymin = 0.0, .ymax = 200.0}};
  frame.objects["person_001"] = person;
  return frame;
}

/**
 * Create a frame with multiple cars
 */
auto make_frame_with_multiple_cars(int frame_num = 0, double timestamp = 0.0, int count = 2)
    -> datastream::Frame {
  datastream::Frame frame = make_empty_frame(frame_num, timestamp);
  for (int i = 0; i < count; ++i) {
    datastream::Object car{
        .object_class = CAR,
        .probability  = 0.95,
        .bbox = {.xmin = 100.0 + i * 50.0, .xmax = 200.0 + i * 50.0, .ymin = 50.0, .ymax = 150.0}};
    frame.objects[std::string("car_") + std::to_string(i + 1)] = car;
  }
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

  // ==========================================================================
  // NextExpr Tests
  // ==========================================================================

  SECTION("NextExpr - with future frame available (constant true)") {
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

  SECTION("NextExpr - condition true on next frame") {
    auto frame1 = make_empty_frame(0, 0.0);
    auto frame2 = make_frame_with_car(1, 1.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame2);

    // Condition: car exists in next frame
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(condition);

    REQUIRE(evaluator.evaluate(formula, frame1, empty_history, horizon));
  }

  SECTION("NextExpr - condition false on next frame") {
    auto frame1 = make_empty_frame(0, 0.0);
    auto frame2 = make_empty_frame(1, 1.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame2);

    // Condition: car exists in next frame (but it doesn't)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame1, empty_history, horizon));
  }

  SECTION("NextExpr - multiple next applications") {
    auto frame1 = make_empty_frame(0, 0.0);
    auto frame2 = make_empty_frame(1, 1.0);
    auto frame3 = make_frame_with_car(2, 2.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame2);
    horizon.push_back(frame3);

    // Condition: car exists
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(next(condition));

    REQUIRE(evaluator.evaluate(formula, frame1, empty_history, horizon));
  }

  // ==========================================================================
  // NextExpr Tests with Varying Steps
  // ==========================================================================

  SECTION("NextExpr - steps=2 evaluates 2 frames ahead") {
    // Create frames: current (no car), +1 (no car), +2 (has car)
    auto frame0 = make_empty_frame(0, 0.0);
    auto frame1 = make_empty_frame(1, 1.0);
    auto frame2 = make_frame_with_car(2, 2.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame1);
    horizon.push_back(frame2);

    // Formula: next(2, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(condition, 2);

    // Should be TRUE: car exists at frame +2
    REQUIRE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("NextExpr - steps=2 fails when condition false at target frame") {
    // Create frames: current (no car), +1 (has car), +2 (no car)
    auto frame0 = make_empty_frame(0, 0.0);
    auto frame1 = make_frame_with_car(1, 1.0);
    auto frame2 = make_empty_frame(2, 2.0);

    std::deque<datastream::Frame> horizon;
    horizon.push_back(frame1);
    horizon.push_back(frame2);

    // Formula: next(2, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(condition, 2);

    // Should be FALSE: car doesn't exist at frame +2
    REQUIRE_FALSE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("NextExpr - steps=5 evaluates 5 frames ahead") {
    // Create frames: current + 5 in horizon
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));
    horizon.push_back(make_frame_with_person(3, 3.0)); // Person at +3
    horizon.push_back(make_empty_frame(4, 4.0));
    horizon.push_back(make_frame_with_car(5, 5.0)); // Car at +5

    // Formula: next(5, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = next(condition, 5);

    // Should be TRUE: car exists at frame +5
    REQUIRE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("NextExpr - steps=3 with person detection") {
    // Create frames with person appearing at +3
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));      // Car at +1
    horizon.push_back(make_frame_with_car(2, 2.0));      // Car at +2
    horizon.push_back(make_frame_with_person(3, 3.0));   // Person at +3

    // Formula: next(3, exists person)
    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = next(condition, 3);

    // Should be TRUE: person exists at frame +3
    REQUIRE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("NextExpr - steps=4 fails when insufficient horizon") {
    // Only 3 frames in horizon, but asking for +4
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));
    horizon.push_back(make_frame_with_car(3, 3.0));

    // Formula: next(4, true)
    auto formula = next(make_true(), 4);

    // Should be FALSE: not enough frames in horizon
    REQUIRE_FALSE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("NextExpr - nested with different steps") {
    // Test: next(2, next(3, condition))
    // This should evaluate at frame +5 (2 + 3)
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));
    horizon.push_back(make_empty_frame(3, 3.0));
    horizon.push_back(make_empty_frame(4, 4.0));
    horizon.push_back(make_frame_with_car(5, 5.0)); // Car at +5

    // Formula: next(2, next(3, exists car))
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto inner     = next(condition, 3);
    auto formula   = next(inner, 2);

    // Should be TRUE: evaluates at frame +2+3=+5 where car exists
    REQUIRE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  // ==========================================================================
  // AlwaysExpr Tests
  // ==========================================================================

  SECTION("AlwaysExpr - all frames satisfy (constant true)") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));

    auto formula = always(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("AlwaysExpr - some frame violates (constant false)") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));

    auto formula = always(make_false());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("AlwaysExpr - car in all future frames") {
    auto frame = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_frame_with_car(2, 2.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = always(condition);

    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("AlwaysExpr - car missing in one future frame") {
    auto frame = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0)); // Car missing here

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = always(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("AlwaysExpr - empty horizon is vacuously true") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> empty_horizon;

    // With empty horizon, always is vacuously true (no frames to check)
    auto formula = always(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  // ==========================================================================
  // EventuallyExpr Tests
  // ==========================================================================

  SECTION("EventuallyExpr - at least one frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_frame_with_car(2, 2.0)); // Car appears here

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = eventually(condition);
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

  SECTION("EventuallyExpr - person appears in last frame only") {
    auto frame = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_frame_with_person(2, 2.0));

    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = eventually(condition);

    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("EventuallyExpr - person never appears") {
    auto frame = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0));

    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = eventually(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  // ==========================================================================
  // UntilExpr Tests
  // ==========================================================================

  SECTION("UntilExpr - rhs becomes true, lhs holds until then") {
    auto frame0 = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_frame_with_person(2, 2.0)); // Person appears: rhs becomes true

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);       // lhs: car is present
    auto goal      = exists({person_id}, is_person); // rhs: person is present

    auto formula = until(safe, goal);
    REQUIRE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("UntilExpr - lhs fails before rhs becomes true") {
    // Until checks: is there a frame where rhs is true? If yes, does lhs hold until that point?
    // Here: frame 0 has car, frame 1 has no car, frame 2 has person
    // So rhs becomes true at frame 2, but lhs (car) fails at frame 1
    auto frame0 = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));       // Car disappears: lhs fails
    horizon.push_back(make_frame_with_person(2, 2.0)); // Person appears: rhs becomes true

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);
    auto goal      = exists({person_id}, is_person);

    auto formula = until(safe, goal);
    // Until semantics: rhs is true at frame 2, but lhs fails at frame 1, so FAIL
    REQUIRE_FALSE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  SECTION("UntilExpr - rhs never becomes true") {
    auto frame0 = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_frame_with_car(2, 2.0)); // Person never appears

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);
    auto goal      = exists({person_id}, is_person);

    auto formula = until(safe, goal);
    // Until semantics: rhs never becomes true, so FAIL (goal never reached)
    REQUIRE_FALSE(evaluator.evaluate(formula, frame0, empty_history, horizon));
  }

  // ==========================================================================
  // ReleaseExpr Tests
  // ==========================================================================

  SECTION("ReleaseExpr - rhs holds on all frames") {
    auto frame = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_frame_with_car(2, 2.0));

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);
    auto safe   = exists({car_id}, is_car);

    auto formula = release(make_false(), safe); // lhs doesn't matter; rhs holds always
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("ReleaseExpr - rhs becomes false, lhs is true at that point") {
    auto frame = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0)); // Car disappears: rhs becomes false

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);

    auto demand = make_true();              // lhs is true
    auto safe   = exists({car_id}, is_car); // rhs fails at frame 2

    auto formula = release(demand, safe);
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }

  SECTION("ReleaseExpr - rhs becomes false, lhs is false at that point") {
    auto frame = make_frame_with_car(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_frame_with_car(1, 1.0));
    horizon.push_back(make_empty_frame(2, 2.0)); // Car disappears: rhs becomes false

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);

    auto demand = make_false();             // lhs is false
    auto safe   = exists({car_id}, is_car); // rhs fails at frame 2

    auto formula = release(demand, safe);
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, empty_history, horizon));
  }
}

// ============================================================================
// Unit Tests: Temporal Operators (Past-Time)
// ============================================================================

TEST_CASE("BooleanEvaluator - Past-Time Temporal Operators", "[evaluation][temporal][past]") {
  BooleanEvaluator evaluator;
  std::deque<datastream::Frame> empty_horizon;

  // ==========================================================================
  // PreviousExpr Tests
  // ==========================================================================

  SECTION("PreviousExpr - with past frame available (constant true)") {
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

  SECTION("PreviousExpr - condition true on previous frame") {
    auto frame = make_empty_frame(1, 1.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(condition);

    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("PreviousExpr - condition false on previous frame") {
    auto frame = make_empty_frame(1, 1.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("PreviousExpr - multiple previous applications") {
    auto frame = make_empty_frame(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(previous(condition));

    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  // ==========================================================================
  // PreviousExpr Tests with Varying Steps
  // ==========================================================================

  SECTION("PreviousExpr - steps=2 evaluates 2 frames back") {
    // Create frames: -2 (has car), -1 (no car), current (no car)
    auto frame_current = make_empty_frame(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));  // Frame at -2
    history.push_back(make_empty_frame(1, 1.0));     // Frame at -1

    // Formula: previous(2, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(condition, 2);

    // Should be TRUE: car exists at frame -2
    REQUIRE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  SECTION("PreviousExpr - steps=2 fails when condition false at target frame") {
    // Create frames: -2 (no car), -1 (has car), current (no car)
    auto frame_current = make_empty_frame(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));     // Frame at -2
    history.push_back(make_frame_with_car(1, 1.0)); // Frame at -1

    // Formula: previous(2, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(condition, 2);

    // Should be FALSE: car doesn't exist at frame -2
    REQUIRE_FALSE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  SECTION("PreviousExpr - steps=5 evaluates 5 frames back") {
    // Create frames: 5 frames of history + current
    auto frame_current = make_empty_frame(5, 5.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));     // Frame at -5 (has car)
    history.push_back(make_empty_frame(1, 1.0));        // Frame at -4
    history.push_back(make_frame_with_person(2, 2.0)); // Frame at -3 (has person)
    history.push_back(make_empty_frame(3, 3.0));        // Frame at -2
    history.push_back(make_empty_frame(4, 4.0));        // Frame at -1

    // Formula: previous(5, exists car)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = previous(condition, 5);

    // Should be TRUE: car exists at frame -5
    REQUIRE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  SECTION("PreviousExpr - steps=3 with person detection") {
    // Create frames with person appearing at -3
    auto frame_current = make_empty_frame(3, 3.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_person(0, 0.0)); // Person at -3
    history.push_back(make_frame_with_car(1, 1.0));    // Car at -2
    history.push_back(make_frame_with_car(2, 2.0));    // Car at -1

    // Formula: previous(3, exists person)
    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = previous(condition, 3);

    // Should be TRUE: person exists at frame -3
    REQUIRE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  SECTION("PreviousExpr - steps=4 fails when insufficient history") {
    // Only 3 frames in history, but asking for -4
    auto frame_current = make_empty_frame(3, 3.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));
    history.push_back(make_frame_with_car(2, 2.0));

    // Formula: previous(4, true)
    auto formula = previous(make_true(), 4);

    // Should be FALSE: not enough frames in history
    REQUIRE_FALSE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  SECTION("PreviousExpr - nested with different steps") {
    // Test: previous(2, previous(3, condition))
    // This should evaluate at frame -5 (2 + 3)
    auto frame_current = make_empty_frame(5, 5.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));  // Car at -5
    history.push_back(make_empty_frame(1, 1.0));     // Frame at -4
    history.push_back(make_empty_frame(2, 2.0));     // Frame at -3
    history.push_back(make_empty_frame(3, 3.0));     // Frame at -2
    history.push_back(make_empty_frame(4, 4.0));     // Frame at -1

    // Formula: previous(2, previous(3, exists car))
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto inner     = previous(condition, 3);
    auto formula   = previous(inner, 2);

    // Should be TRUE: evaluates at frame -2-3=-5 where car exists
    REQUIRE(evaluator.evaluate(formula, frame_current, history, empty_horizon));
  }

  // ==========================================================================
  // Integration Tests: NextExpr/PreviousExpr with Multiple Steps
  // ==========================================================================

  SECTION("NextExpr - steps with quantifier and class comparison") {
    // Verify that object bindings work correctly at distant frames
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(1, 1.0));
    horizon.push_back(make_frame_with_multiple_objects(2, 2.0)); // Has both car and person
    horizon.push_back(make_empty_frame(3, 3.0));

    // Formula: next(2, exists obj such that obj is a car)
    auto obj_id    = ObjectVar{"obj"};
    auto is_car    = is_class(obj_id, CAR);
    auto condition = exists({obj_id}, is_car);
    auto formula   = next(condition, 2);

    // Should be TRUE: car exists at frame +2
    REQUIRE(evaluator.evaluate(formula, frame0, {}, horizon));
  }

  SECTION("PreviousExpr - steps with quantifier and class comparison") {
    // Verify that object bindings work correctly at distant past frames
    auto frame_current = make_empty_frame(3, 3.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_frame_with_multiple_objects(1, 1.0)); // Has both car and person
    history.push_back(make_empty_frame(2, 2.0));

    // Formula: previous(2, exists obj such that obj is a person)
    auto obj_id    = ObjectVar{"obj"};
    auto is_person = is_class(obj_id, PERSON);
    auto condition = exists({obj_id}, is_person);
    auto formula   = previous(condition, 2);

    // Should be TRUE: person exists at frame -2
    REQUIRE(evaluator.evaluate(formula, frame_current, history, {}));
  }

  SECTION("NextExpr - large steps with probability check") {
    // Test with larger step count and probability comparison
    auto frame0 = make_empty_frame(0, 0.0);
    std::deque<datastream::Frame> horizon;
    for (int i = 1; i <= 9; ++i) {
      horizon.push_back(make_empty_frame(i, static_cast<double>(i)));
    }
    horizon.push_back(make_frame_with_car_probability(10, 10.0, 0.99)); // High confidence at +10

    // Formula: next(10, exists car with P(car) >= 0.95)
    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto high_prob = high_confidence(car_id, 0.95);
    auto condition = exists({car_id}, is_car & high_prob);
    auto formula   = next(condition, 10);

    // Should be TRUE: high-confidence car at frame +10
    REQUIRE(evaluator.evaluate(formula, frame0, {}, horizon));
  }

  SECTION("PreviousExpr - large steps with multiple object types") {
    // Test with larger step count going backwards
    auto frame_current = make_empty_frame(10, 10.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_multiple_objects(0, 0.0)); // Both car and person at -10
    for (int i = 1; i < 10; ++i) {
      history.push_back(make_empty_frame(i, static_cast<double>(i)));
    }

    // Formula: previous(10, exists both car and person)
    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};
    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto has_car   = exists({car_id}, is_car);
    auto has_person = exists({person_id}, is_person);
    auto condition  = has_car & has_person;
    auto formula    = previous(condition, 10);

    // Should be TRUE: both objects exist at frame -10
    REQUIRE(evaluator.evaluate(formula, frame_current, history, {}));
  }

  SECTION("NextExpr and PreviousExpr - asymmetric evaluation") {
    // Verify that next(N) and previous(N) evaluate at correct frames
    // Setup: -2, -1, current, +1, +2
    // Only frame -2 and +2 have cars
    auto frame_current = make_empty_frame(2, 2.0);

    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));  // Frame -2: has car
    history.push_back(make_empty_frame(1, 1.0));     // Frame -1: no car

    std::deque<datastream::Frame> horizon;
    horizon.push_back(make_empty_frame(3, 3.0));     // Frame +1: no car
    horizon.push_back(make_frame_with_car(4, 4.0)); // Frame +2: has car

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);

    // next(2, condition) should be TRUE (car at +2)
    auto next_formula = next(condition, 2);
    REQUIRE(evaluator.evaluate(next_formula, frame_current, history, horizon));

    // previous(2, condition) should be TRUE (car at -2)
    auto prev_formula = previous(condition, 2);
    REQUIRE(evaluator.evaluate(prev_formula, frame_current, history, horizon));

    // next(1, condition) should be FALSE (no car at +1)
    auto next_1_formula = next(condition, 1);
    REQUIRE_FALSE(evaluator.evaluate(next_1_formula, frame_current, history, horizon));

    // previous(1, condition) should be FALSE (no car at -1)
    auto prev_1_formula = previous(condition, 1);
    REQUIRE_FALSE(evaluator.evaluate(prev_1_formula, frame_current, history, horizon));
  }

  // ==========================================================================
  // HoldsExpr Tests
  // ==========================================================================

  SECTION("HoldsExpr - all past frames satisfy (constant true)") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto formula = holds(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("HoldsExpr - some past frame violates (constant false)") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto formula = holds(make_false());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("HoldsExpr - car in all past frames") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_frame_with_car(1, 1.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = holds(condition);

    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("HoldsExpr - car missing in one past frame") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0)); // Car missing here
    history.push_back(make_frame_with_car(1, 1.0));

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = holds(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("HoldsExpr - empty history is vacuously true") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> empty_history;

    // With empty history, holds is vacuously true (no past frames to check)
    auto formula = holds(make_true());
    REQUIRE(evaluator.evaluate(formula, frame, empty_history, empty_horizon));
  }

  // ==========================================================================
  // SometimesExpr Tests
  // ==========================================================================

  SECTION("SometimesExpr - at least one past frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_frame_with_car(1, 1.0)); // Car appears here

    auto car_id    = ObjectVar{"car"};
    auto is_car    = is_class(car_id, CAR);
    auto condition = exists({car_id}, is_car);
    auto formula   = sometimes(condition);
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SometimesExpr - no past frame satisfies") {
    auto frame = make_empty_frame();
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto formula = sometimes(make_false());
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SometimesExpr - person appears in first past frame only") {
    auto frame = make_empty_frame(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_person(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = sometimes(condition);

    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SometimesExpr - person never appeared in past") {
    auto frame = make_empty_frame(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_empty_frame(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0));

    auto person_id = ObjectVar{"person"};
    auto is_person = is_class(person_id, PERSON);
    auto condition = exists({person_id}, is_person);
    auto formula   = sometimes(condition);

    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  // ==========================================================================
  // SinceExpr Tests
  // ==========================================================================

  SECTION("SinceExpr - lhs holds since rhs became true") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_person(0, 0.0)); // Person appears: rhs becomes true
    history.push_back(make_frame_with_car(1, 1.0));    // Car is present since then

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);       // lhs: car is present
    auto trigger   = exists({person_id}, is_person); // rhs: person is present

    auto formula = since(safe, trigger);
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SinceExpr - lhs fails after rhs became true") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_person(0, 0.0)); // Person appears: rhs becomes true
    history.push_back(make_empty_frame(1, 1.0));       // Car disappears: lhs fails

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);
    auto trigger   = exists({person_id}, is_person);

    auto formula = since(safe, trigger);
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("SinceExpr - rhs never was true in past") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_frame_with_car(1, 1.0)); // Person never appeared

    auto car_id    = ObjectVar{"car"};
    auto person_id = ObjectVar{"person"};

    auto is_car    = is_class(car_id, CAR);
    auto is_person = is_class(person_id, PERSON);
    auto safe      = exists({car_id}, is_car);
    auto trigger   = exists({person_id}, is_person);

    auto formula = since(safe, trigger);
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  // ==========================================================================
  // BackToExpr Tests
  // ==========================================================================

  SECTION("BackToExpr - rhs holds on all past frames") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_frame_with_car(1, 1.0));

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);
    auto origin = exists({car_id}, is_car);

    auto formula = backto(make_false(), origin); // lhs doesn't matter; rhs holds always
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("BackToExpr - rhs becomes false going back, lhs is true at that point") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0)); // Car disappears: rhs becomes false

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);

    auto requirement = make_true();              // lhs is true
    auto origin      = exists({car_id}, is_car); // rhs fails at frame 1

    auto formula = backto(requirement, origin);
    REQUIRE(evaluator.evaluate(formula, frame, history, empty_horizon));
  }

  SECTION("BackToExpr - rhs becomes false going back, lhs is false at that point") {
    auto frame = make_frame_with_car(2, 2.0);
    std::deque<datastream::Frame> history;
    history.push_back(make_frame_with_car(0, 0.0));
    history.push_back(make_empty_frame(1, 1.0)); // Car disappears: rhs becomes false

    auto car_id = ObjectVar{"car"};
    auto is_car = is_class(car_id, CAR);

    auto requirement = make_false();             // lhs is false
    auto origin      = exists({car_id}, is_car); // rhs fails at frame 1

    auto formula = backto(requirement, origin);
    REQUIRE_FALSE(evaluator.evaluate(formula, frame, history, empty_horizon));
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
    auto is_car  = is_class(id_var, CAR);
    auto formula = exists({id_var}, is_car);

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

    auto id_var  = ObjectVar{"car"};
    auto is_car  = is_class(id_var, CAR);
    auto formula = exists({id_var}, is_car);

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
    auto class_expr = ClassCompareExpr{ClassFunc{id_var}, CompareOp::Equal, CAR};

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

TEST_CASE(
    "OnlineMonitor - Multi-Frame Streaming with Varying Perception Data",
    "[online_monitor][integration]") {
  // Formula: sometimes a car appeared (past-time, naturally bounded)
  auto car_id       = ObjectVar{"car"};
  auto is_car       = is_class(car_id, CAR);
  auto car_detected = exists({car_id}, is_car);
  auto formula      = sometimes(car_detected);

  OnlineMonitor monitor(formula, 1.0);

  // Frame 0: no car
  auto frame0   = make_empty_frame(0, 0.0);
  bool verdict0 = monitor.evaluate(frame0);
  REQUIRE_FALSE(verdict0); // Car hasn't appeared yet

  // Frame 1: still no car
  auto frame1   = make_empty_frame(1, 1.0);
  bool verdict1 = monitor.evaluate(frame1);
  REQUIRE_FALSE(verdict1); // Car still hasn't appeared

  // Frame 2: car appears
  auto frame2   = make_frame_with_car(2, 2.0);
  bool verdict2 = monitor.evaluate(frame2);
  REQUIRE(verdict2); // Now car has appeared
}

TEST_CASE("OnlineMonitor - Eventually Then Always Pattern", "[online_monitor][integration]") {
  // Formula: sometimes a car appeared (test the pattern)
  auto car_id      = ObjectVar{"car"};
  auto is_car      = is_class(car_id, CAR);
  auto car_present = exists({car_id}, is_car);

  // Use a past-time formula that can be monitored: sometimes car appeared
  auto formula = sometimes(car_present);
  OnlineMonitor monitor(formula, 1.0);

  // Build sequence: empty, empty, car appears, car persists
  std::vector<datastream::Frame> frames;
  frames.push_back(make_empty_frame(0, 0.0));
  frames.push_back(make_empty_frame(1, 1.0));
  frames.push_back(make_frame_with_car(2, 2.0));
  frames.push_back(make_frame_with_car(3, 3.0));

  // Evaluate each frame
  bool verdict0 = monitor.evaluate(frames[0]);
  REQUIRE_FALSE(verdict0); // No car yet

  bool verdict1 = monitor.evaluate(frames[1]);
  REQUIRE_FALSE(verdict1); // Still no car

  bool verdict2 = monitor.evaluate(frames[2]);
  REQUIRE(verdict2); // Car appeared, so sometimes(car) is true

  bool verdict3 = monitor.evaluate(frames[3]);
  REQUIRE(verdict3); // Still true (car persists)
}

TEST_CASE("OnlineMonitor - Nested Temporal Operators", "[online_monitor][integration]") {
  // Formula: a person has always been present in the history (past-time nesting)
  auto person_id      = ObjectVar{"person"};
  auto is_person      = is_class(person_id, PERSON);
  auto person_present = exists({person_id}, is_person);
  auto formula        = holds(person_present);

  OnlineMonitor monitor(formula, 1.0);

  // All frames have person
  auto frame0 = make_frame_with_person(0, 0.0);
  auto frame1 = make_frame_with_person(1, 1.0);
  auto frame2 = make_frame_with_person(2, 2.0);

  bool verdict0 = monitor.evaluate(frame0);
  REQUIRE(verdict0); // Person in frame 0

  bool verdict1 = monitor.evaluate(frame1);
  REQUIRE(verdict1); // Person in both frame 0 and 1

  bool verdict2 = monitor.evaluate(frame2);
  REQUIRE(verdict2); // Person in all frames
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

TEST_CASE(
    "BooleanEvaluator - Property: Idempotence of Eventually",
    "[evaluation][properties][temporal]") {
  // ◇◇φ ≡ ◇φ (eventually eventually equals eventually)
  BooleanEvaluator evaluator;

  auto car_id    = ObjectVar{"car"};
  auto is_car    = is_class(car_id, CAR);
  auto condition = exists({car_id}, is_car);

  auto eventually_once  = eventually(condition);
  auto eventually_twice = eventually(eventually(condition));

  std::deque<datastream::Frame> horizon;
  horizon.push_back(make_empty_frame(1, 1.0));
  horizon.push_back(make_frame_with_car(2, 2.0));

  auto frame = make_empty_frame(0, 0.0);
  std::deque<datastream::Frame> empty_history;

  bool result_once  = evaluator.evaluate(eventually_once, frame, empty_history, horizon);
  bool result_twice = evaluator.evaluate(eventually_twice, frame, empty_history, horizon);

  REQUIRE(result_once == result_twice);
}

TEST_CASE(
    "BooleanEvaluator - Property: Idempotence of Always",
    "[evaluation][properties][temporal]") {
  // □□φ ≡ □φ (always always equals always)
  BooleanEvaluator evaluator;

  auto car_id    = ObjectVar{"car"};
  auto is_car    = is_class(car_id, CAR);
  auto condition = exists({car_id}, is_car);

  auto always_once  = always(condition);
  auto always_twice = always(always(condition));

  std::deque<datastream::Frame> horizon;
  horizon.push_back(make_frame_with_car(1, 1.0));
  horizon.push_back(make_frame_with_car(2, 2.0));

  auto frame = make_frame_with_car(0, 0.0);
  std::deque<datastream::Frame> empty_history;

  bool result_once  = evaluator.evaluate(always_once, frame, empty_history, horizon);
  bool result_twice = evaluator.evaluate(always_twice, frame, empty_history, horizon);

  REQUIRE(result_once == result_twice);
}

TEST_CASE(
    "BooleanEvaluator - Property: Until Semantics with Eventually",
    "[evaluation][properties][temporal]") {
  // until(true, φ) ≡ eventually(φ) (until with true lhs is eventually)
  BooleanEvaluator evaluator;

  auto car_id    = ObjectVar{"car"};
  auto is_car    = is_class(car_id, CAR);
  auto condition = exists({car_id}, is_car);

  auto until_true_lhs = until(make_true(), condition);
  auto eventually_rhs = eventually(condition);

  std::deque<datastream::Frame> horizon;
  horizon.push_back(make_empty_frame(1, 1.0));
  horizon.push_back(make_frame_with_car(2, 2.0));

  auto frame = make_empty_frame(0, 0.0);
  std::deque<datastream::Frame> empty_history;

  bool result_until      = evaluator.evaluate(until_true_lhs, frame, empty_history, horizon);
  bool result_eventually = evaluator.evaluate(eventually_rhs, frame, empty_history, horizon);

  REQUIRE(result_until == result_eventually);
}

TEST_CASE(
    "BooleanEvaluator - Property: Idempotence of Sometimes (Past)",
    "[evaluation][properties][temporal]") {
  // ⧇⧇φ ≡ ⧇φ (sometimes sometimes equals sometimes)
  BooleanEvaluator evaluator;

  auto car_id    = ObjectVar{"car"};
  auto is_car    = is_class(car_id, CAR);
  auto condition = exists({car_id}, is_car);

  auto sometimes_once  = sometimes(condition);
  auto sometimes_twice = sometimes(sometimes(condition));

  std::deque<datastream::Frame> history;
  history.push_back(make_empty_frame(0, 0.0));
  history.push_back(make_frame_with_car(1, 1.0));

  auto frame = make_empty_frame(2, 2.0);
  std::deque<datastream::Frame> empty_horizon;

  bool result_once  = evaluator.evaluate(sometimes_once, frame, history, empty_horizon);
  bool result_twice = evaluator.evaluate(sometimes_twice, frame, history, empty_horizon);

  REQUIRE(result_once == result_twice);
}

TEST_CASE(
    "BooleanEvaluator - Property: Idempotence of Holds (Past)",
    "[evaluation][properties][temporal]") {
  // ■■φ ≡ ■φ (holds holds equals holds)
  BooleanEvaluator evaluator;

  auto car_id    = ObjectVar{"car"};
  auto is_car    = is_class(car_id, CAR);
  auto condition = exists({car_id}, is_car);

  auto holds_once  = holds(condition);
  auto holds_twice = holds(holds(condition));

  std::deque<datastream::Frame> history;
  history.push_back(make_frame_with_car(0, 0.0));
  history.push_back(make_frame_with_car(1, 1.0));

  auto frame = make_frame_with_car(2, 2.0);
  std::deque<datastream::Frame> empty_horizon;

  bool result_once  = evaluator.evaluate(holds_once, frame, history, empty_horizon);
  bool result_twice = evaluator.evaluate(holds_twice, frame, history, empty_horizon);

  REQUIRE(result_once == result_twice);
}
