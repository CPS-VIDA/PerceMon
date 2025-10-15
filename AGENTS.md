# PerceMon - Project Context for Claude Code

## Project Overview

PerceMon (Perception Monitoring) is a C++ library for runtime monitoring of
perception systems using **Spatio-Temporal Quality Logic (STQL)**.
The library enables both online and offline monitoring of temporal and spatial
properties in perception data, particularly for autonomous systems and robotics
applications.

## What is STQL?

STQL is a formal logic combining:
- **Temporal Logic**:
  Past and future temporal operators (Until, Since, Always, Eventually, etc.)
- **Spatial Logic**:
  Set-based spatial operations (Union, Intersection, Complement)
- **Perception Primitives**:
  Object detection, tracking, and spatial relationships

### Key STQL Components

1. **Variables**:
   - Time variables (x ∈ V_t):
     Track timestamps
   - Frame variables (f ∈ V_f):
     Track frame numbers
   - Object ID variables (id ∈ V_o):
     Reference detected objects

2. **Temporal Operators**:
   - Future:
     Next (○), Always (□), Eventually (◇), Until (U), Release (R)
   - Past:
     Previous (◦), Holds (■), Sometimes (⧇), Since (S), BackTo (B)

3. **Spatial Operators**:
   - Set operations:
     Union (⊔), Intersection (⊓), Complement (¬)
   - Topological:
     Interior (I), Closure (C)
   - Spatial temporal:
     Spatial Next (○_s), Spatial Always (□_s), etc.

4. **Perception Primitives**:
   - BB(id):
     Bounding box of object
   - C(id):
     Class/category of object
   - P(id):
     Detection probability/confidence
   - ED(id1, CRT, id2, CRT):
     Euclidean distance between reference points
   - Lat(id, CRT), Lon(id, CRT):
     Lateral/longitudinal position
   - Area(Ω):
     Area of spatial expression

5. **Quantifiers**:
   - ∃{id₁, ...}@φ:
     Existential over object IDs
   - ∀{id₁, ...}@φ:
     Universal over object IDs
   - {x, f}.φ:
     Freeze quantifier (pin time/frame for duration measurement)

### Example STQL Formula

```
// "Always when a car is detected, it stays in the left lane for at least 5 seconds"
□(∃{car}@(C(car) = "vehicle") → {x}.(□[0,5] ∃(BB(car) ⊓ LeftLane) ∧ (x - C_TIME ≤ 5)))
```

## Current State of Codebase

### Directory Structure

```
PerceMon/
├── include/
│   └── percemon2/            # Modern C++20 implementation
│       ├── stql.hpp          # Core STQL AST (variant-based, no inheritance)
│       ├── monitoring.hpp    # Monitoring requirements interface
│       ├── datastream.hpp    # Runtime data structures (frames, objects)
│       ├── evaluation.hpp    # Formula evaluation semantics
│       ├── online_monitor.hpp # Streaming monitor
│       └── spatial.hpp       # Spatial region operations
├── csrc/
│   ├── stql_requirements.cc  # Memory requirements computation
│   ├── datastream.cc         # Runtime data implementations
│   ├── spatial.cc            # Spatial operations implementations
│   └── monitoring/
│       ├── evaluation.cc     # Boolean evaluator implementation
│       └── online_monitor.cc # Online monitor implementation
├── tests/                    # Test suite (using Catch2)
│   ├── test_monitoring.cc
│   ├── test_datastream.cc
│   ├── test_spatial.cc
│   └── test_boolean_evaluation.cc
└── examples/mot17/           # MOT17 dataset example
```

### Core Architecture: `include/percemon2/`

The `percemon2` namespace implements the complete STQL monitoring system using modern C++20 features:

**`stql.hpp` - Core AST Implementation**:
- **Type-safe variant-based AST**:
  Uses `std::variant<...>` for `Expr` and `SpatialExpr` instead of deep
  inheritance hierarchies
- **40+ expression types** covering all STQL operators:
  - Boolean/propositional (ConstExpr, NotExpr, AndExpr, OrExpr)
  - Temporal (NextExpr, PreviousExpr, AlwaysExpr, EventuallyExpr, UntilExpr,
    etc.)
  - Quantifiers (ExistsExpr, ForallExpr, FreezeExpr)
  - Time/frame constraints (TimeBoundExpr, FrameBoundExpr)
  - Spatial operations (SpatialUnionExpr, SpatialIntersectExpr, BBoxExpr, etc.)
  - Perception primitives (ClassFunc, ProbFunc, EuclideanDistFunc, etc.)
- **CRTP-based variables**:
  Strong typing for TimeVar, FrameVar, ObjectVar using Curiously Recurring
  Template Pattern
- **Box<T> wrapper**:
  Enables recursive variant types without heap allocation overhead
- **Comprehensive documentation**:
  Every type includes STQL syntax notation, semantics, usage examples, and paper
  references
- **Factory functions**:
  Ergonomic constructors like `make_true()`, `next()`, `exists()`
- **Operator overloads**:
  Natural syntax with `~`, `&`, `|` operators
- **Zero runtime overhead**:
  All variant dispatch resolved at compile-time via `std::visit`

**`monitoring.hpp` - Monitoring Requirements**:
- **History/Horizon computation**:
  Analyze formulas to determine:
  - `History`:
    How many past frames must be retained
  - `Horizon`:
    How many future frames must be buffered
- **Monitoring predicates**:
  - `is_online_monitorable()`:
    Check if formula has bounded horizon
  - `is_past_time_formula()`:
    Check if formula needs no future frames
- **Time-to-frame conversion**:
  Converts time-based constraints to frame counts using FPS

**`datastream.hpp` - Runtime Data Structures**:
- **BoundingBox**: 2D spatial extent with utility methods
- **RefPointType**: Reference points on boxes (corners, edges, center)
- **Object**: Detected object with class, probability, bbox
- **Frame**: Single frame snapshot with timing and object map
- **Trace**: Sequence of perception frames
- **Utility functions**: `get_reference_point()`, `euclidean_distance()`

**`evaluation.hpp` - Formula Evaluation**:
- **EvaluationContext**: Manages buffers and variable bindings during evaluation
- **BooleanEvaluator**: Evaluates formulas on frames with boolean semantics
- **Complete semantics** for all STQL operators (temporal, logical, spatial, quantifiers)
- **Error handling** for missing objects and insufficient buffers

**`online_monitor.hpp` - Streaming Monitor**:
- **OnlineMonitor**: Manages formula evaluation on frame streams
- **Automatic buffer management**: Maintains history/horizon windows
- **Frame-by-frame verdicts**: Boolean satisfaction results
- **Monitorability validation**: Ensures formula has bounded horizon

**`spatial.hpp` - Spatial Operations**:
- **Region representations**: Geometric operations on bounding boxes
- **Set operations**: Union, intersection, complement
- **Membership testing**: Checks if boxes belong to spatial regions

### Key Design Improvements

1. **No Deep Inheritance**:
   Eliminated virtual inheritance and `dynamic_pointer_cast`
   - Old:
     `BaseExpr` → `BaseSpatialExpr` → 40+ derived classes
   - New:
     Single `std::variant<...>` with flat type list

2. **Compile-time Type Safety**:
   C++20 concepts replace runtime type checks
   - `ASTNode` concept ensures all types have `to_string()`
   - Variant dispatch catches type errors at compile time

3. **Ergonomic API**:
   Builder pattern via factory functions and operators
   ```cpp
   // Old (pseudocode):
   auto expr = std::make_shared<AndExpr>(
       std::make_shared<NotExpr>(phi),
       std::make_shared<NextExpr>(psi, 1)
   );

   // New:
   auto expr = ~phi & next(psi);
   ```

4. **Clear Separation of Concerns**:
   - `stql.hpp`:
     Pure syntax tree (immutable data structures)
   - `monitoring.hpp`:
     Requirements analysis (memory and monitorability)
   - `datastream.hpp`:
     Runtime data structures (frames, objects, perception primitives)
   - `evaluation.hpp`:
     Semantics and evaluation logic (boolean evaluator)
   - `online_monitor.hpp`:
     Stream processing and buffer management

### Known Architectural Limitations

**Temporal Scope Separation**:
Currently, formulas must use **either** future-time operators **or** past-time operators, but **not both in the same formula**. This is a known limitation documented in `csrc/monitoring/evaluation.cc` (lines 153-155).

**Why**: The `EvaluationContext` maintains history and horizon buffers separately. Mixing past-time operators (Previous, Holds, Sometimes, Since, BackTo) with future-time operators (Next, Always, Eventually, Until, Release) in the same formula would require the context to track bidirectional shifts through both buffers simultaneously, which is not currently implemented.

**Current Behavior**:
- Pure future-time formulas: ✅ Fully supported
- Pure past-time formulas: ✅ Fully supported
- Mixed temporal scopes: ❌ Not supported (evaluation behavior undefined)

**Future Work**:
Supporting mixed temporal scopes would require restructuring the evaluation context to handle bidirectional frame traversal. This is planned for a future architectural enhancement.

**Testing Implication**:
When writing tests for temporal operators, ensure all formulas use only future-time operators, only past-time operators, or no temporal operators at all. Do not create test cases mixing both scopes.

### Legacy Design (Deprecated)

The old `include/percemon/ast.hpp` design had several issues:
- Heavy use of virtual inheritance and dynamic polymorphism
- Tight coupling between AST nodes
- Runtime type checking via `dynamic_pointer_cast`
- Difficult to construct and traverse programmatically

**Migration Status**:
The refactoring from `percemon` to `percemon2` is **complete**.
All code now uses the modern `percemon2` namespace and C++20 design patterns.
The legacy `percemon` namespace has been fully removed from the codebase.

## Development Conventions

### C++20 Features Used in `percemon2`

1. **Concepts**:
   Used for compile-time type constraints
```cpp
template <typename T>
concept ASTNode = requires(const T& t) {
  { t.to_string() } -> std::convertible_to<std::string>;
};
```

2. **std::variant with std::visit**:
   Core pattern for type-safe AST without inheritance
```cpp
using Expr = std::variant<
    ConstExpr, NotExpr, AndExpr, OrExpr,
    NextExpr, PreviousExpr, ...
>;

// Pattern matching via std::visit
auto result = std::visit([](const auto& e) {
    return e.to_string();
}, expr);
```

3. **CRTP (Curiously Recurring Template Pattern)**:
   For strongly-typed variables with shared functionality
```cpp
template <typename Derived>
struct Variable { ... };

struct TimeVar : Variable<TimeVar> { ... };
```

4. **Three-way Comparison (operator<=>)**:
   Default comparisons for simple types
```cpp
auto operator<=>(const Variable&) const = default;
```

5. **Trailing Return Types**:
   Modern function declaration style (using `auto ...
   -> Type`)
```cpp
[[nodiscard]] auto to_string() const -> std::string;
```

### Code Style

- Use `snake_case` for functions and variables
- Use `PascalCase` for types and concepts
- Prefer `auto` for complex types when intent is clear
- Use `[[nodiscard]]` for pure functions returning values
- Use trailing return types (`auto func() -> Type`)
- Comprehensive comments referencing STQL paper definitions
- Mark constructors `explicit` to prevent implicit conversions

### Implementation Patterns in `percemon2`

1. **Box<T> Wrapper for Recursive Types**:
   Since `std::variant` cannot directly contain itself, use `Box<T>` to wrap
   recursive members:
   ```cpp
   struct NotExpr {
       Box<Expr> arg;  // Expr can contain NotExpr recursively
       explicit NotExpr(Expr e);
   };
   ```

2. **Forward Declarations Before Variant**:
   All expression types must be forward-declared before the `Expr` variant type:
   ```cpp
   // Forward declarations
   struct ConstExpr;
   struct NotExpr;
   // ...

   // Then define the variant
   using Expr = std::variant<ConstExpr, NotExpr, ...>;

   // Finally implement the types
   struct NotExpr { ... };
   ```

3. **Inline Constructors After Complete Types**:
   Constructors that take `Expr` or `SpatialExpr` must be defined after the
   variant is complete:
   ```cpp
   struct NotExpr {
       Box<Expr> arg;
       explicit NotExpr(Expr e);  // Declaration only
   };

   // ... after Expr is defined ...
   inline NotExpr::NotExpr(Expr e) : arg(std::move(e)) {}
   ```

4. **Factory Functions for Ergonomics**:
   Provide free functions for common operations:
   ```cpp
   inline auto make_true() -> Expr { return ConstExpr{true}; }
   inline auto next(Expr e, size_t n = 1) -> Expr {
       return NextExpr{std::move(e), n};
   }
   ```

5. **Operator Overloading**:
   Enable natural formula construction:
   ```cpp
   inline auto operator~(Expr e) -> Expr { return NotExpr{std::move(e)}; }
   inline auto operator&(Expr lhs, Expr rhs) -> Expr {
       return AndExpr{{std::move(lhs), std::move(rhs)}};
   }
   ```

6. **Visitor Pattern for Traversal**:
   Use `std::visit` with lambdas for operations on expressions:
   ```cpp
   auto to_string_visitor = [](const auto& e) { return e.to_string(); };
   std::string result = std::visit(to_string_visitor, expr);
   ```

### Documentation Standards

**CRITICAL**:
Every AST node type MUST include comprehensive documentation comments that allow
users to easily match syntax nodes with the grammar defined in the paper.

**Documentation Template for All Public Types**:

```cpp
/**
 * @brief Brief one-line description
 *
 * In STQL syntax: [formal notation from paper]
 *
 * Detailed explanation of what this operator does, including:
 * - Semantics (what it means in plain English)
 * - When/how it's used in practice
 * - Any special properties (e.g., derived operators, duals, equivalences)
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto formula = operator_name(phi);  // Concrete usage example
 * @endcode
 *
 * @see STQL Definition in paper Section X (Definition Y)
 * @see Related operators: OtherOperator, AnotherOperator
 */
struct OperatorExpr {
    // ... implementation
};
```

**Key Requirements**:
1. **STQL Syntax Line**:
   Must include "In STQL syntax:" with the formal mathematical notation from the
   paper
2. **Plain English Explanation**:
   Describe semantics without jargon
3. **Concrete Example**:
   Show how to construct and use the node in code
4. **Paper Reference**:
   Link to specific section/definition in the STQL paper
5. **Related Operators**:
   Cross-reference derived/dual/related operators

**Example**:

```cpp
/**
 * @brief STQL until temporal operator.
 *
 * In STQL syntax: φ₁ U φ₂ (until)
 *
 * Future-time temporal operator.
 * φ₁ U φ₂ is true if φ₂ eventually becomes true,
 * and φ₁ holds until that happens.
 *
 * Example:
 * @code
 * auto safe = make_true();
 * auto goal_reached = make_false();
 * auto formula = until(safe, goal_reached);  // safe U goal_reached
 * // Means: stay safe until goal is reached
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: Release (dual), Eventually (derived when lhs is true)
 */
struct UntilExpr {
    Box<Expr> lhs;
    Box<Expr> rhs;

    UntilExpr(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}

    [[nodiscard]] std::string to_string() const;
};
```

This documentation style ensures that:
- Users can quickly map code back to the formal STQL grammar
- The implementation is understandable without constantly referring to the paper
- Examples show idiomatic usage patterns
- Relationships between operators are clear

## Testing Philosophy

All new code in `include/percemon2/` should have corresponding tests in
`tests/`:
- Use Catch2 framework
- Test both API usability and correctness
- Include property-based tests where applicable
- Test edge cases (empty formulas, deeply nested structures, etc.)

## Build System

- CMake-based build system (already configured)
- Don't worry about CMake changes during refactoring - maintainer will handle
- Pixi for development environment management

## Key References

For full STQL semantics and formal definitions, refer to the paper in
`./claude-cache/paper/`.

## Common Patterns in STQL

Understanding these patterns helps when designing the API:

1. **Bounded Temporal Operators**:
   Many temporal operators can have interval bounds
   ```
   □[a,b] φ    // Always φ holds in frame interval [a,b]
   ◇[a,b] φ    // Eventually φ holds in frame interval [a,b]
   ```

2. **Nested Quantifiers**:
   Common to nest spatial and object quantifiers
   ```
   ∃{car}@(∃(BB(car) ⊓ Lane1))
   ```

3. **Freeze-then-Compare**:
   Pin time/frame, then compare differences
   ```
   {x}.(Eventually((x - C_TIME > 5.0) ∧ φ))
   ```

## Implementation Status and Next Steps

### ✅ Completed

1. **Core STQL AST** (`include/percemon2/stql.hpp`):
   - All 40+ expression types implemented
   - Variant-based design with Box<T> for recursion
   - Factory functions and operator overloads
   - Comprehensive documentation with STQL syntax references
   - `to_string()` for all types (debugging and display)

2. **Monitoring Requirements Analysis** (`include/percemon2/monitoring.hpp`, `csrc/stql_requirements.cc`):
   - ✅ `History` and `Horizon` structs defined
   - ✅ `MonitoringRequirements` result type
   - ✅ `compute_requirements()` function with scope-aware constraint handling
   - ✅ `is_online_monitorable()` helper predicate
   - ✅ `is_past_time_formula()` helper predicate
   - ✅ Comprehensive visitor for traversing all expression types
   - ✅ Time-to-frame conversion using FPS parameter
   - ✅ Scope direction tracking (Future vs Past) for correct constraint interpretation

3. **Datastream Runtime Data** (`include/percemon2/datastream.hpp`, `csrc/datastream.cc`):
   - ✅ `BoundingBox` struct with utility methods (area, width, height, center)
   - ✅ `RefPointType` enum for spatial reference points (corners, edges, center)
   - ✅ `Object` struct representing detected objects with class, probability, bbox
   - ✅ `Frame` struct with temporal metadata and object map
   - ✅ `Trace` type alias for sequences of frames
   - ✅ Utility functions: `get_reference_point()`, `euclidean_distance()`

4. **Formula Evaluation** (`include/percemon2/evaluation.hpp`, `csrc/monitoring/evaluation.cc`):
   - ✅ `EvaluationContext` template for managing evaluation state
   - ✅ History and horizon buffer management
   - ✅ Frozen variable bindings (time, frame, objects)
   - ✅ `BooleanEvaluator` struct with `evaluate()` method
   - ✅ Complete boolean semantics for all STQL operators
   - ✅ Error handling for missing objects and insufficient buffers

5. **Online Monitoring** (`include/percemon2/online_monitor.hpp`, `csrc/monitoring/online_monitor.cc`):
   - ✅ `OnlineMonitor` class for streaming evaluation
   - ✅ Automatic buffer management (history/horizon)
   - ✅ Frame-by-frame evaluation with verdicts
   - ✅ Memory requirement tracking
   - ✅ Monitorability checks (bounded horizon verification)

6. **Spatial Operations** (`include/percemon2/spatial.hpp`, `csrc/spatial.cc`):
   - ✅ Spatial region representations and operations
   - ✅ Union, intersection, complement operations
   - ✅ Membership testing for bounding boxes

7. **Comprehensive Test Suite** (`tests/test_monitoring.cc`):
   - ✅ Unit tests for simple formulas (constants)
   - ✅ Tests for all temporal operators (Next, Previous, Always, Eventually, Until, Since, Release, BackTo)
   - ✅ Tests for logical operators (AND, OR, NOT)
   - ✅ Tests for quantifiers (Exists, Forall, Freeze)
   - ✅ Tests for constrained operators with meaningful bounds
   - ✅ Tests for nested temporal operators (horizon/history composition)
   - ✅ Tests for mixed future and past operators
   - ✅ Tests for edge cases and scope direction validation

### 🚧 Extended Features (Future Work)

1. **Bounded Temporal Operators**:
   - Bounded intervals for Always, Eventually, Until, Since (e.g., `□[a,b]φ`, `◇[a,b]φ`)
   - Would tighten horizon/history requirements

2. **Spatial Temporal Operators**:
   - Spatial versions of temporal operators (○_s, □_s, ◇_s, U_s, S_s)
   - Combining spatial and temporal dimensions

3. **Advanced Features**:
   - Evaluation/semantics module (separate from AST)
   - Formula simplification and normalization
   - Interval-based monitoring optimization
   - Robustness analysis for STL/MTL integration

## Memory Requirements Computation: Tricky Scenarios

### The Scope Direction Challenge

The most complex aspect of `compute_requirements()` is handling **temporal scope direction**.
This is critical for correct constraint interpretation:

#### Future-Time Operators (Next, Always, Eventually, Until, Release)

**Scope Semantics**: When evaluating future-time operators, C_TIME advances forward.
Frozen variables (captured via `freeze(x, φ)`) represent times in the **past** relative
to the advancing C_TIME.

**Meaningful Constraints**:
- **Only** constraints of form `(C_TIME - x ∼ n)` bound the horizon
- Constraints like `(x - C_TIME ∼ n)` are **trivially satisfied** (x is always ≤ C_TIME)
  and should NOT contribute to horizon

**Example**:
```cpp
// Formula: {x}.(◇((C_TIME - x) ≤ 5.0))
// Meaningful in future scope: limits horizon to 5 seconds
// Expected: horizon = 50 frames (@ 10 fps)

// Formula: {x}.(◇((x - C_TIME) ≤ 5.0))
// Trivially satisfied in future scope (always true since x ≤ C_TIME)
// Expected: horizon = UNBOUNDED (constraint adds nothing)
```

#### Past-Time Operators (Previous, Holds, Sometimes, Since, BackTo)

**Scope Semantics**: When evaluating past-time operators, C_TIME retreats backward.
Frozen variables represent times in the **future** relative to the retreating C_TIME.

**Meaningful Constraints**:
- **Only** constraints of form `(x - C_TIME ∼ n)` bound the history
- Constraints like `(C_TIME - x ∼ n)` are **trivially satisfied** (C_TIME is always ≥ x)
  and should NOT contribute to history

**Example**:
```cpp
// Formula: {x}.(Since((x - C_TIME) ≤ 5.0, φ))
// Meaningful in past scope: limits history to 5 seconds
// Expected: history = 50 frames (@ 10 fps)

// Formula: {x}.(Since((C_TIME - x) ≤ 5.0, φ))
// Trivially satisfied in past scope (always true since C_TIME ≥ x)
// Expected: history = UNBOUNDED (constraint adds nothing)
```

### Nested Temporal Operators: Additive Composition

Requirements from nested temporal operators **add**, not replace:

**Horizon Composition**:
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

**History Composition**:
```cpp
// Formula: previous(previous({x}.(Since((x - C_TIME) ≤ 1.0), true), 2), 3)
// Layer 1 (inner since): history += 5 frames
// Layer 2 (middle previous): history += 2 frames
// Layer 3 (outer previous): history += 3 frames
// Total: 3 + 2 + 5 = 10 frames
```

### Unbounded Operators Remain Unbounded

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

### Constraint Form Extraction

The implementation carefully distinguishes constraint forms:

#### TimeDiff Structure

```cpp
struct TimeDiff {
    std::variant<TimeVar, TimeVar> lhs;  // Could be x or C_TIME
    std::variant<TimeVar, TimeVar> rhs;  // Could be x or C_TIME
};

// Example: (C_TIME - x)
auto diff = TimeDiff{C_TIME, x};  // lhs=C_TIME, rhs=x
```

#### Form Detection

```cpp
// Check lhs and rhs to determine constraint form:
bool lhs_is_c_time = (get<TimeVar>(diff.lhs).name == "C_TIME");
bool rhs_is_c_time = (get<TimeVar>(diff.rhs).name == "C_TIME");

if (lhs_is_c_time && !rhs_is_c_time) {
    // Form: C_TIME - var
    // Meaningful in FUTURE scopes
} else if (!lhs_is_c_time && rhs_is_c_time) {
    // Form: var - C_TIME
    // Meaningful in PAST scopes
}
```

### FPS Conversion

Time-based constraints are converted to frame counts using FPS:

```cpp
// Formula: {x}.(◇((C_TIME - x) ≤ 5.0)) with fps=10.0
// Conversion: frames = ceil(5.0 * 10.0) = 50 frames

// Formula: {x}.(◇((C_TIME - x) ≤ 5.5)) with fps=10.0
// Conversion: frames = ceil(5.5 * 10.0) = 55 frames

// Frame bounds don't need conversion (already in frame units):
// frames = FrameBoundExpr.value (direct value)
```

### AND vs OR in Constraint Composition

When multiple constraints appear together via logical operators:

**AND (Conjunction)**:
- Multiple constraints must ALL be satisfied
- Results in **tighter** (smaller) bound
- Take the **minimum** of constraint bounds

```cpp
// Formula: (time < 5) ∧ (time < 10)
// Both must hold: time < min(5, 10) = 5
// Result: horizon = 50 frames (@ 10 fps, 5 seconds)
```

**OR (Disjunction)**:
- At least ONE constraint must be satisfied
- Results in **looser** (larger) bound
- Take the **maximum** of constraint bounds

```cpp
// Formula: (time < 5) ∨ (time < 10)
// Either can hold: time < max(5, 10) = 10
// Result: horizon = 100 frames (@ 10 fps, 10 seconds)
```

### Implementation File Structure

**`csrc/monitoring/stql_requirements.cc`** contains:

1. **Namespace `percemon2::monitoring::` (anonymous)**:
   - `ScopeDirection` enum (Future, Past)
   - `ConstraintBound` struct (frames, direction)
   - `extract_time_bound_constraint()` helper
   - `extract_frame_bound_constraint()` helper
   - `intersect_bounds()` and `union_bounds()` helpers
   - `compute_requirements_impl()` recursive visitor

2. **Public API** (in `percemon2::monitoring`):
   - `compute_requirements()`
   - `is_online_monitorable()`
   - `is_past_time_formula()`

**Key Design**: Implementation details are in anonymous namespace; only public API is exposed.

### Extending the Monitoring Implementation

If you need to add new features to `compute_requirements()`:

#### Adding Support for New Temporal Operators

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
   - Test with unconstrained subexpressions (should return UNBOUNDED if applicable)
   - Test with constrained subexpressions (should extract bounds)
   - Test nesting with Next/Previous (should add to requirements)

#### Handling Bounded Interval Operators

Future work may include operators with explicit bounds like `□[a,b]φ` or `◇[a,b]φ`:

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

#### Constraint-Based Optimization

For better bounds, could track constraint chains:

```cpp
// Current: treats constraints atomically
// Future: could extract tighter bounds from AND chains
// Example: (time < 100) ∧ (time < 50) ∧ (time < 75)
// Could compute: time < min(100, 50, 75) = 50 automatically
```

### Common Pitfalls to Avoid

1. **Forgetting Scope Direction**:
   - ❌ Extracting constraint bounds without checking if they're meaningful for current scope
   - ✅ Always check `is_meaningful_in_scope()` before using extracted bounds

2. **Unbounded Win**:
   - ❌ Taking max of bounded + unbounded = bounded result
   - ✅ Unbounded should always win in max operations (return UNBOUNDED)

3. **Constraint Form Confusion**:
   - ❌ Treating `(x - C_TIME)` and `(C_TIME - x)` the same in both scopes
   - ✅ Meaning depends on scope direction

4. **FPS Rounding**:
   - ❌ Using integer division for time-to-frame conversion
   - ✅ Use `ceil()` to ensure sufficient frames (conservative approach)

5. **Operator Requirement Semantics**:
   - ❌ Replacing requirements from nested operators
   - ✅ Adding requirements (nested operators compound memory needs)

## Quantifier Semantics in Testing

### Critical Understanding: Meaningful Constraints in Quantifiers

When writing tests for STQL formulas involving quantifiers (especially within temporal operators), the quantifier body **must contain meaningful constraints** on the quantified objects. Using vacuous constants defeats the purpose of testing.

### ❌ Incorrect Pattern (Vacuous)
```cpp
// BAD: Tests only if ANY object exists, not detection of specific type
auto car_id = ObjectVar{"car"};
auto condition = exists({car_id}, make_true());
auto formula = always(condition);  // Trivially satisfied in any non-empty frame
```

This pattern:
- Tests variable binding, not object detection
- Doesn't verify class comparison works
- Provides false confidence in test results

### ✅ Correct Pattern (Meaningful Constraints)
```cpp
// GOOD: Tests if cars specifically are detected
auto car_id = ObjectVar{"car"};
auto is_car = is_class(car_id, CAR);           // C(car_id) = CAR
auto condition = exists({car_id}, is_car);    // Does a car exist?
auto formula = always(condition);              // Do cars exist in all frames?
```

This pattern:
- Tests actual object detection (class comparison)
- Verifies both quantifier and temporal operator work together
- Tests vary across frames (empty vs. car-containing frames)

### Helper Functions for Readable Tests

Use these helpers (defined in `include/percemon2/stql.hpp`) to simplify constraint construction:

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

### Test Data Constants

The following constants are defined in tests:
```cpp
constexpr int CAR    = 1;    // Object class ID for cars
constexpr int PERSON = 2;    // Object class ID for persons
```

### Example: Temporal + Quantifier Integration Test

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

### Why This Matters

1. **Catches Real Bugs**: If `shift_eval_context()` doesn't preserve quantifier bindings correctly, tests fail
2. **Tests Semantic Integration**: Verifies object detection (class comparison) + temporal reasoning work together
3. **Prevents False Positives**: Vacuous formulas pass even with broken implementations
4. **Documents STQL Semantics**: Clear examples of proper formula construction for users

### Testing Guidelines

When writing tests with quantifiers inside temporal operators:
1. **Always** use class checks: `is_class()`, `is_not_class()`
2. **Never** use: `make_true()`, `make_false()`
3. **Ensure** frame sequences vary (empty frames, frames with different object types)
4. **Verify** the quantifier body actually excludes some objects (e.g., distinguish cars from persons)

### Testing Strategy for Future Development

When adding new features, use this test pattern:

```cpp
TEST_CASE("compute_requirements - new operator", "[monitoring][compute][new]") {
  SECTION("New operator without constraints") {
    auto formula = new_operator(make_true());
    auto reqs = compute_requirements(formula);
    // Should reflect operator's default behavior
  }

  SECTION("New operator with meaningful constraint") {
    auto x = TimeVar{"x"};
    auto time_diff = TimeDiff{C_TIME, x};  // Correct form for your operator
    auto time_bound = TimeBoundExpr{time_diff, CompareOp::LessEqual, 5.0};
    auto formula = freeze(x, new_operator(time_bound));

    auto reqs = compute_requirements(formula, 10.0);
    // Should extract bound: ceil(5.0 * 10.0) = 50 frames
  }

  SECTION("Nested new operator") {
    auto inner = freeze(x, new_operator(constraint));
    auto outer = next(inner, 2);

    auto reqs = compute_requirements(outer, 10.0);
    // Should add: outer_contribution + inner_bound
  }
}
```

### Tips for Future Development

1. **When Adding New Operators**:
   - Add forward declaration before `Expr` variant
   - Define struct with complete documentation (see template in docs section)
   - Add to `Expr` or `SpatialExpr` variant type list
   - Implement constructor after variant is complete
   - Add factory function and/or operator overload
   - Implement `to_string()` method
   - Add corresponding tests

2. **When Implementing Visitors**:
   - Use `std::visit` with generic lambdas (`[](const auto& e)`)
   - Handle all expression types (compiler enforces exhaustiveness)
   - Consider creating reusable visitor utilities
   - Example pattern:
     ```cpp
     auto result = std::visit([&](const auto& expr) {
         using T = std::decay_t<decltype(expr)>;
         if constexpr (std::is_same_v<T, NextExpr>) {
             // Handle NextExpr
         } else if constexpr (std::is_same_v<T, AlwaysExpr>) {
             // Handle AlwaysExpr
         } // ... etc
     }, formula);
     ```

3. **Keep AST Immutable**:
   - All fields should be `const` or captured in constructors
   - No setter methods
   - Transformations create new expressions rather than modifying existing ones
   - This enables safe sharing and parallelism

4. **Validation in Constructors**:
   - Validate invariants in constructors (e.g., AndExpr needs ≥2 args)
   - Throw `std::invalid_argument` with clear error messages
   - This ensures invalid ASTs cannot be constructed
