# PerceMon Testing Guide

This document covers testing philosophy, guidelines, and patterns for PerceMon
STQL implementation.

## Testing Philosophy

All new code in `include/percemon/` should have corresponding tests in
`tests/`:
- Use Catch2 framework for all test cases
- Test both API usability and semantic correctness
- Include property-based tests where applicable
- Test edge cases (empty formulas, deeply nested structures, unusual operator
  combinations)
- Ensure tests verify actual behavior, not just that code runs

## Quantifier Semantics in Testing

### Critical Understanding: Meaningful Constraints

When writing tests for STQL formulas involving quantifiers (especially within
temporal operators), the quantifier body **must contain meaningful constraints**
on the quantified objects.
Using vacuous constants defeats the purpose of testing.

### ❌ Incorrect Pattern (Vacuous Constraints)

```cpp
// BAD: Tests only if ANY object exists, not detection of specific type
auto car_id = ObjectVar{"car"};
auto condition = exists({car_id}, make_true());
auto formula = always(condition);  // Trivially satisfied in any non-empty frame
```

This pattern has critical flaws:
- Tests variable binding, not object detection
- Doesn't verify class comparison works
- Provides false confidence in test results
- Masks bugs in constraint evaluation

### ✅ Correct Pattern (Meaningful Constraints)

```cpp
// GOOD: Tests if cars specifically are detected
auto car_id = ObjectVar{"car"};
auto is_car = is_class(car_id, CAR);           // C(car_id) = CAR
auto condition = exists({car_id}, is_car);    // Does a car exist?
auto formula = always(condition);              // Do cars exist in all frames?
```

This pattern properly tests:
- Actual object detection (class comparison)
- Both quantifier and temporal operator working together
- Evaluation varies across frames (empty vs. car-containing frames)
- Real constraint evaluation, not just tree traversal

### Why This Matters

1. **Catches Real Bugs**:
   If `shift_eval_context()` doesn't preserve quantifier bindings correctly,
   tests fail
2. **Tests Semantic Integration**:
   Verifies object detection (class comparison) + temporal reasoning work
   together
3. **Prevents False Positives**:
   Vacuous formulas pass even with broken implementations
4. **Documents STQL Semantics**:
   Clear examples of proper formula construction for users

## Helper Functions for Readable Tests

Use these helpers (defined in `include/percemon/stql.hpp`) to simplify
constraint construction:

```cpp
// Check if object is of specific class (C(id) = class_id)
auto is_car = is_class(car_id, CAR);           // Instead of ClassCompareExpr{...}
auto is_person = is_class(person_id, PERSON);

// Check if object is NOT of specific class (C(id) ≠ class_id)
auto not_car = is_not_class(car_id, CAR);

// Check if object has high/low confidence
auto high_conf = high_confidence(obj_id, 0.9);  // P(id) ≥ 0.9
auto low_conf = low_confidence(obj_id, 0.5);   // P(id) < 0.5
```

These helpers:
- Make test intent clear and concise
- Reduce boilerplate and visual noise
- Ensure consistent constraint construction
- Enable fluent test writing

## Test Data Constants

The following constants are defined in tests:

```cpp
constexpr int CAR    = 1;    // Object class ID for cars
constexpr int PERSON = 2;    // Object class ID for persons
constexpr int BIKE   = 3;    // Object class ID for bikes
```

Use these in tests to create meaningful discriminators between object types.

## Integration Test Example: Temporal + Quantifier

```cpp
TEST_CASE("eventually car appears", "[temporal][quantifier]") {
  BooleanEvaluator evaluator;

  // Create frames: empty, empty, car appears
  auto frame0 = make_empty_frame(0, 0.0);
  auto frame1 = make_empty_frame(1, 1.0);
  auto frame2 = make_frame_with_car(2, 2.0);  // Car has class CAR (= 1)

  std::deque<datastream::Frame> horizon;
  horizon.push_back(frame1);
  horizon.push_back(frame2);

  // Formula: eventually a car appears
  auto car_id = ObjectVar{"car"};
  auto is_car = is_class(car_id, CAR);        // ✅ Meaningful constraint
  auto has_car = exists({car_id}, is_car);
  auto formula = eventually(has_car);

  // Should succeed because car appears in frame 2
  REQUIRE(evaluator.evaluate(formula, frame0, {}, horizon));
}
```

### What This Test Verifies

1. **Quantifier Correctness**:
   `exists` properly binds and evaluates car_id
2. **Constraint Evaluation**:
   `is_class` correctly identifies car objects
3. **Temporal Operator**:
   `eventually` properly traverses horizon
4. **Integration**:
   All components work together correctly

## Testing Guidelines for Quantifiers

When writing tests with quantifiers inside temporal operators:

1. **Always** use class checks:
   `is_class()`, `is_not_class()`
2. **Never** use:
   `make_true()`, `make_false()` in quantifier bodies
3. **Ensure** frame sequences vary:
   - Empty frames (no objects)
   - Frames with specific object types
   - Mixed frames (multiple object types)
4. **Verify** the quantifier body discriminates:
   - Can distinguish cars from persons
   - Constraints actually filter objects
   - Frame variation matters for test result

## Testing Strategy for New Features

When adding new operators or features, use this pattern:

### 1. Unconstrained Test

```cpp
SECTION("New operator without constraints") {
  auto formula = new_operator(make_true());
  auto reqs = compute_requirements(formula);
  // Should reflect operator's default behavior
  // Usually UNBOUNDED for temporal operators
}
```

### 2. Constrained Test

```cpp
SECTION("New operator with meaningful constraint") {
  auto x = TimeVar{"x"};
  auto time_diff = TimeDiff{C_TIME, x};  // Correct form for your operator
  auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
  auto formula = freeze(x, new_operator(time_bound));

  auto reqs = compute_requirements(formula, 10.0);
  // Should extract bound: ceil(5.0 * 10.0) = 50 frames
}
```

### 3. Nested Test

```cpp
SECTION("Nested new operator") {
  auto inner = freeze(x, new_operator(constraint));
  auto outer = next(inner, 2);

  auto reqs = compute_requirements(outer, 10.0);
  // Should add: outer_contribution + inner_bound
}
```

### 4. Edge Cases

```cpp
SECTION("Edge case: empty formula") {
  auto formula = new_operator(make_true());
  // Test behavior with empty subformulas
}

SECTION("Edge case: deeply nested") {
  auto formula = new_operator(new_operator(new_operator(make_true())));
  // Test behavior with deep nesting
}

SECTION("Edge case: with quantifiers") {
  auto x = ObjectVar{"obj"};
  auto is_car = is_class(x, CAR);
  auto formula = new_operator(exists({x}, is_car));
  // Test interaction with quantifiers
}
```

## Test Organization

Organize tests using Catch2 sections:

```cpp
TEST_CASE("operator_name", "[category][operator]") {
  SECTION("basic functionality") {
    // Test core behavior
  }

  SECTION("with temporal nesting") {
    // Test composition with other operators
  }

  SECTION("with quantifiers") {
    // Test integration with quantifiers
  }

  SECTION("edge cases") {
    // Test boundary conditions
  }

  SECTION("error handling") {
    // Test error cases
  }
}
```

Tag conventions:
- `[temporal]`:
  Tests temporal operators
- `[spatial]`:
  Tests spatial operators
- `[quantifier]`:
  Tests quantifiers
- `[monitoring]`:
  Tests monitoring/requirements
- `[evaluation]`:
  Tests formula evaluation
- `[integration]`:
  Tests component integration
- `[edge]`:
  Tests edge cases

## Testing Temporal Scope Separation

Remember the architectural limitation:
formulas must use **either** future-time operators **or** past-time operators,
not both.

### Future-Time Tests

```cpp
TEST_CASE("future time operators", "[temporal][future]") {
  auto formula = next(next(always(make_true()), 1), 2);
  // All operators point forward in time
  auto reqs = compute_requirements(formula);
  // horizon should be bounded
}
```

### Past-Time Tests

```cpp
TEST_CASE("past time operators", "[temporal][past]") {
  auto formula = previous(previous(holds(make_true()), 1), 2);
  // All operators point backward in time
  auto reqs = compute_requirements(formula);
  // history should be bounded
}
```

### ❌ DO NOT Mix Temporal Scopes

```cpp
// NEVER DO THIS - undefined behavior
auto bad_formula = next(previous(make_true()));  // Mixed scopes!
```

## Performance Testing

For computationally intensive operations:

```cpp
TEST_CASE("performance: large formula tree", "[performance]") {
  auto formula = make_true();
  for (int i = 0; i < 100; ++i) {
    formula = next(formula);
  }

  auto start = std::chrono::steady_clock::now();
  auto reqs = compute_requirements(formula);
  auto end = std::chrono::steady_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  REQUIRE(duration.count() < 100);  // Should complete in under 100ms
}
```

## Tips for Effective Testing

1. **Test Behavior, Not Implementation**:
   - Focus on what the formula computes, not how
   - Test the contract, not the mechanism

2. **Use Meaningful Data**:
   - Create frame sequences that vary in meaningful ways
   - Objects should have different classes, confidences, spatial positions

3. **Test Incrementally**:
   - Start with simple formulas
   - Build up to complex compositions
   - Verify each layer works before adding complexity

4. **Document Test Intent**:
   - Use clear test names and section headings
   - Add comments explaining what is being tested and why
   - Include expected vs actual behavior commentary

5. **Test Integration Points**:
   - Quantifiers with temporal operators
   - Spatial operators with quantifiers
   - Constraints with requirement computation
   - Frozen variables with comparisons

## Common Testing Mistakes to Avoid

❌ **Vacuous Formulas**:
Using `make_true()` in quantifier bodies.
✅ **Meaningful Constraints**:
Using `is_class()`, `is_not_class()`, etc.

❌ **Uniform Frames**:
All frames identical.
✅ **Varied Frames**:
Mix of empty, containing cars, containing persons, etc.

❌ **Insufficient Buffers**:
Not providing enough horizon/history for evaluation.
✅ **Proper Buffers**:
Match buffer size to formula requirements.

❌ **Mixed Temporal Scopes**:
Future and past operators in same formula.
✅ **Pure Scopes**:
Use only future-time or only past-time operators.

❌ **Ignoring Edge Cases**:
Only testing happy path.
✅ **Complete Coverage**:
Test edge cases, error conditions, boundary values.

## Running Tests

Using Catch2 framework:

```bash
# Run all tests
ctest

# Run tests matching pattern
ctest -R "temporal"

# Run with verbose output
ctest --verbose

# Run specific test
ctest -R "exactly car appears"
```
