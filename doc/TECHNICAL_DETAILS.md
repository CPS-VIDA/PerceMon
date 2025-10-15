# PerceMon Memory Requirements Computation - Technical Details

This document provides an in-depth explanation of how PerceMon computes memory
requirements (history/horizon) for STQL formulas.
This is critical for understanding online monitoring feasibility and
constraints.

## Overview

The memory requirements computation analyzes STQL formulas to determine:
- **History**:
  How many past frames must be retained
- **Horizon**:
  How many future frames must be buffered

This analysis is essential for online monitoring because:
1. **Online Monitorability**:
   Formulas with unbounded horizon cannot be monitored online
2. **Memory Management**:
   Bounded requirements enable efficient stream processing
3. **Buffer Allocation**:
   Requirements determine buffer sizes for frame history/horizon

## The Scope Direction Challenge

The most complex aspect of memory requirements is handling **temporal scope
direction**.
This is critical for correct constraint interpretation:

### Future-Time Operators (Next, Always, Eventually, Until, Release)

**Scope Semantics**:
When evaluating future-time operators, C_TIME advances forward.
Frozen variables (captured via `freeze(x, φ)`) represent times in the **past**
relative to the advancing C_TIME.

**Meaningful Constraints**:
- **Only** constraints of form `(C_TIME - x ∼ n)` bound the horizon
- Constraints like `(x - C_TIME ∼ n)` are **trivially satisfied** (x is always ≤
  C_TIME) and should NOT contribute to horizon

**Example**:
```cpp
// Formula: {x}.(◇((C_TIME - x) ≤ 5.0))
// Meaningful in future scope: limits horizon to 5 seconds
// Expected: horizon = 50 frames (@ 10 fps)

// Formula: {x}.(◇((x - C_TIME) ≤ 5.0))
// Trivially satisfied in future scope (always true since x ≤ C_TIME)
// Expected: horizon = UNBOUNDED (constraint adds nothing)
```

### Past-Time Operators (Previous, Holds, Sometimes, Since, BackTo)

**Scope Semantics**:
When evaluating past-time operators, C_TIME retreats backward.
Frozen variables represent times in the **future** relative to the retreating
C_TIME.

**Meaningful Constraints**:
- **Only** constraints of form `(x - C_TIME ∼ n)` bound the history
- Constraints like `(C_TIME - x ∼ n)` are **trivially satisfied** (C_TIME is
  always ≥ x) and should NOT contribute to history

**Example**:
```cpp
// Formula: {x}.(Since((x - C_TIME) ≤ 5.0, φ))
// Meaningful in past scope: limits history to 5 seconds
// Expected: history = 50 frames (@ 10 fps)

// Formula: {x}.(Since((C_TIME - x) ≤ 5.0, φ))
// Trivially satisfied in past scope (always true since C_TIME ≥ x)
// Expected: history = UNBOUNDED (constraint adds nothing)
```

## Nested Temporal Operators: Additive Composition

Requirements from nested temporal operators **add**, not replace:

### Horizon Composition

```cpp
// Formula: next(next(◇((C_TIME - x) ≤ 1.0), 2), 3)
// Layer 1 (inner eventually): horizon += ceil(1.0 * fps) = 5 frames (@ 5 fps)
// Layer 2 (middle next): horizon += 2 frames
// Layer 3 (outer next): horizon += 3 frames
// Total: 3 + 2 + 5 = 10 frames

// Implementation: compute_requirements_impl is called recursively:
// 1. Eventually with constraint returns {0, 5}
// 2. next(eventually, 2) returns {0, 2+5} = {0, 7}
// 3. next(next(...), 3) returns {0, 3+7} = {0, 10}
```

### History Composition

```cpp
// Formula: previous(previous({x}.(Since((x - C_TIME) ≤ 1.0), true), 2), 3)
// Layer 1 (inner since): history += 5 frames
// Layer 2 (middle previous): history += 2 frames
// Layer 3 (outer previous): history += 3 frames
// Total: 3 + 2 + 5 = 10 frames
```

### Why Addition, Not Max?

- **Nested operators create sequential traversals**:
  Each layer advances through frames
- **Requirements are cumulative**:
  Inner operators use horizon, then outer operators add more
- **Example**:
  `next(next(next(φ)))` requires 1+1+1=3 future frames, not max(1,1,1)=1

## Unbounded Operators Remain Unbounded

**Without Meaningful Constraints**, temporal operators require infinite memory:

```cpp
// Formula: always(true)
// No constraints in subexpression
// Result: horizon = UNBOUNDED (not online-monitorable)

// Formula: eventually(true)
// No constraints in subexpression
// Result: horizon = UNBOUNDED

// Formula: until(true, eventually(true))
// No meaningful bounds from subexpressions
// Result: horizon = UNBOUNDED
```

**With Meaningful Constraints**, bounds become finite:

```cpp
// Formula: eventually(time_bound((C_TIME - x) ≤ 5.0))
// Constraint bounds the horizon
// Result: horizon = 50 frames (@ 10 fps)
```

## Constraint Form Extraction

The implementation carefully distinguishes constraint forms:

### TimeDiff Structure

```cpp
struct TimeDiff {
    std::variant<TimeVar, TimeVar> lhs;  // Could be x or C_TIME
    std::variant<TimeVar, TimeVar> rhs;  // Could be x or C_TIME
};

// Example: (C_TIME - x)
auto diff = TimeDiff{C_TIME, x};  // lhs=C_TIME, rhs=x
```

### Form Detection

```cpp
// Check lhs and rhs to determine constraint form:
bool lhs_is_c_time = (get<TimeVar>(diff.lhs).name == "C_TIME");
bool rhs_is_c_time = (get<TimeVar>(diff.rhs).name == "C_TIME");

if (lhs_is_c_time && !rhs_is_c_time) {
    // Form: C_TIME - var
    // Meaningful in FUTURE scopes only
} else if (!lhs_is_c_time && rhs_is_c_time) {
    // Form: var - C_TIME
    // Meaningful in PAST scopes only
}
```

## FPS Conversion

Time-based constraints are converted to frame counts using FPS:

```cpp
// Formula: {x}.(◇((C_TIME - x) ≤ 5.0)) with fps=10.0
// Conversion: frames = ceil(5.0 * 10.0) = 50 frames

// Formula: {x}.(◇((C_TIME - x) ≤ 5.5)) with fps=10.0
// Conversion: frames = ceil(5.5 * 10.0) = 55 frames

// Frame bounds don't need conversion (already in frame units):
// frames = FrameBoundExpr.value (direct value)
```

**Conservative Approach**:
Use `ceil()` to ensure sufficient frames.
Better to have slightly more buffer than risk evaluation failure.

## AND vs OR in Constraint Composition

When multiple constraints appear together via logical operators:

### AND (Conjunction)

- Multiple constraints must ALL be satisfied
- Results in **tighter** (smaller) bound
- Take the **minimum** of constraint bounds

```cpp
// Formula: (time < 5) ∧ (time < 10)
// Both must hold: time < min(5, 10) = 5
// Result: horizon = 50 frames (@ 10 fps, 5 seconds)
```

### OR (Disjunction)

- At least ONE constraint must be satisfied
- Results in **looser** (larger) bound
- Take the **maximum** of constraint bounds

```cpp
// Formula: (time < 5) ∨ (time < 10)
// Either can hold: time < max(5, 10) = 10
// Result: horizon = 100 frames (@ 10 fps, 10 seconds)
```

## Implementation File Structure

**`csrc/monitoring/stql_requirements.cc`** contains:

### Anonymous Namespace (Implementation Details)

- `ScopeDirection` enum (Future, Past)
- `ConstraintBound` struct (frames, direction)
- `extract_time_bound_constraint()` helper
- `extract_frame_bound_constraint()` helper
- `intersect_bounds()` and `union_bounds()` helpers
- `compute_requirements_impl()` recursive visitor

### Public API (in `percemon::monitoring`)

- `compute_requirements(const Expr& formula, double fps = 30.0) ->
  MonitoringRequirements`
- `is_online_monitorable(const Expr& e) -> bool`
- `is_past_time_formula(const Expr& e) -> bool`

**Key Design**:
Implementation details are in anonymous namespace; only public API is exposed.
This enables refactoring without breaking external code.

## Extending the Monitoring Implementation

### Adding Support for New Temporal Operators

1. **Identify the operator type**:
   - Is it future-time (like Next, Always) or past-time (like Previous, Since)?
   - Does it introduce unbounded requirements by default?

2. **Add to the visitor**:
   ```cpp
   else if constexpr (std::is_same_v<T, NewOperatorExpr>) {
       auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, current_scope);
       // Apply operator-specific logic
       if (new_operator_is_unbounded && no_constraints) {
           horiz = UNBOUNDED;  // or hist = UNBOUNDED for past operators
       }
       return {hist, horiz};
   }
   ```

3. **Test thoroughly**:
   - Test with unconstrained subexpressions (should return UNBOUNDED if
     applicable)
   - Test with constrained subexpressions (should extract bounds)
   - Test nesting with Next/Previous (should add to requirements)

### Handling Bounded Interval Operators

Future work may include operators with explicit bounds like `□[a,b]φ` or
`◇[a,b]φ`:

```cpp
// Example: AlwaysExpr with optional interval bounds
struct AlwaysExprBounded {
    Box<Expr> arg;
    std::optional<std::pair<int64_t, int64_t>> frame_interval;  // [a, b]
};

// In visitor:
else if constexpr (std::is_same_v<T, AlwaysExprBounded>) {
    auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Future);

    // If bounded by interval, use interval bound instead of UNBOUNDED
    if (e.frame_interval.has_value()) {
        auto [a, b] = *e.frame_interval;
        horiz = std::max(horiz, b);  // Bounded by interval
    } else {
        horiz = UNBOUNDED;  // No interval, unbounded
    }
    return {hist, horiz};
}
```

### Constraint-Based Optimization

For better bounds, could track constraint chains:

```cpp
// Current: treats constraints atomically
// Future: could extract tighter bounds from AND chains
// Example: (time < 100) ∧ (time < 50) ∧ (time < 75)
// Could compute: time < min(100, 50, 75) = 50 automatically
```

## Common Pitfalls to Avoid

### 1. Forgetting Scope Direction

❌ **Bad**:
Extracting constraint bounds without checking if they're meaningful for current
scope.
✅ **Good**:
Always check if constraint is meaningful before using extracted bounds.

```cpp
// BAD: Always uses C_TIME - x bound
if (constraint.forms_c_time_minus_var()) {
    horizon = extract_bound(constraint);  // WRONG for past scope!
}

// GOOD: Scope-aware
if (scope == ScopeDirection::Future && constraint.forms_c_time_minus_var()) {
    horizon = extract_bound(constraint);  // Only in future scope
}
```

### 2. Unbounded Win

❌ **Bad**:
Taking max of bounded + unbounded = bounded result.
✅ **Good**:
Unbounded should always win in max operations.

```cpp
// BAD: Returns bounded result
auto result = max(100, UNBOUNDED);  // Returns 100

// GOOD: Returns UNBOUNDED
auto result = (horiz_inner == UNBOUNDED || horiz_outer == UNBOUNDED)
    ? UNBOUNDED
    : max(horiz_inner, horiz_outer);
```

### 3. Constraint Form Confusion

❌ **Bad**:
Treating `(x - C_TIME)` and `(C_TIME - x)` the same in both scopes.
✅ **Good**:
Meaning depends on scope direction.

```cpp
// BAD: Same handling in both scopes
if (constraint.forms_time_diff()) {
    horizon = extract_bound(constraint);  // WRONG!
}

// GOOD: Scope-aware
if (scope == ScopeDirection::Future && constraint.forms_c_time_minus_var()) {
    horizon = extract_bound(constraint);
} else if (scope == ScopeDirection::Past && constraint.forms_var_minus_c_time()) {
    history = extract_bound(constraint);
}
```

### 4. FPS Rounding

❌ **Bad**:
Using integer division for time-to-frame conversion.
✅ **Good**:
Use `ceil()` to ensure sufficient frames.

```cpp
// BAD: Loses precision
auto frames = static_cast<size_t>(time_seconds * fps);  // 5.5 * 10 = 55?

// GOOD: Conservative approach
auto frames = static_cast<size_t>(std::ceil(time_seconds * fps));
```

### 5. Operator Requirement Semantics

❌ **Bad**:
Replacing requirements from nested operators.
✅ **Good**:
Adding requirements (nested operators compound memory needs).

```cpp
// BAD: Inner requirement replaced
auto inner = compute_requirements_impl(*e.arg, fps, scope);
return {inner.history, 5};  // Ignores inner.horizon!

// GOOD: Requirements added
auto inner = compute_requirements_impl(*e.arg, fps, scope);
return {inner.history, inner.horizon + 5};  // Accumulates
```

## Advanced Topics

### Bounded Temporal Operators

```cpp
// When implementing □[a,b]φ bounds extraction:
struct AlwaysBoundedExpr {
    Box<Expr> arg;
    int64_t frame_a, frame_b;  // Inclusive bounds
};

// Requirement: horizon ≥ b to evaluate in window
// Combined with subformula requirements
```

### Constraint Chain Optimization

Possible future optimization for AND chains:

```cpp
// Current approach: treats separately
// Future: extract minimum from AND chain
// (time < 100) ∧ (time < 50) → horizon = 50 directly

// Implementation would need:
// 1. Identify AND chains
// 2. Extract all time constraints from chain
// 3. Take minimum for tighter bound
```

### Quantifier Scope

Frozen variables in quantifiers create implicit scope windows:

```cpp
// {x}.(◇((C_TIME - x) ≤ 5.0)) in any temporal scope
// x is pinned at evaluation time
// Constraint then represents duration from that point
```

## Testing Requirements Computation

See doc/TESTING.md for comprehensive testing guidelines.
Key testing patterns:

1. **Test unconstrained operators** → should return UNBOUNDED
2. **Test constrained operators** → should extract meaningful bounds
3. **Test nested operators** → should add requirements correctly
4. **Test scope direction** → should handle future/past correctly
5. **Test edge cases** → deeply nested, mixed operators, empty subformulas

## References

- Core monitoring API:
  `include/percemon/monitoring.hpp`
- Implementation:
  `csrc/stql_requirements.cc`
- Formula evaluation:
  `csrc/monitoring/evaluation.cc`
- Testing patterns:
  `doc/TESTING.md`
- AST reference:
  `AGENTS.md`
