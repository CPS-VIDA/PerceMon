# PerceMon - Project Context for Claude Code

## Project Overview

PerceMon (Perception Monitoring) is a C++ library for runtime monitoring of perception systems using **Spatio-Temporal Quality Logic (STQL)**. The library enables both online and offline monitoring of temporal and spatial properties in perception data, particularly for autonomous systems and robotics applications.

## What is STQL?

STQL is a formal logic combining:
- **Temporal Logic**: Past and future temporal operators (Until, Since, Always, Eventually, etc.)
- **Spatial Logic**: Set-based spatial operations (Union, Intersection, Complement)
- **Perception Primitives**: Object detection, tracking, and spatial relationships

### Key STQL Components

1. **Variables**:
   - Time variables (x ∈ V_t): Track timestamps
   - Frame variables (f ∈ V_f): Track frame numbers
   - Object ID variables (id ∈ V_o): Reference detected objects

2. **Temporal Operators**:
   - Future: Next (○), Always (□), Eventually (◇), Until (U), Release (R)
   - Past: Previous (◦), Holds (■), Sometimes (⧇), Since (S), BackTo (B)

3. **Spatial Operators**:
   - Set operations: Union (⊔), Intersection (⊓), Complement (¬)
   - Topological: Interior (I), Closure (C)
   - Spatial temporal: Spatial Next (○_s), Spatial Always (□_s), etc.

4. **Perception Primitives**:
   - BB(id): Bounding box of object
   - C(id): Class/category of object
   - P(id): Detection probability/confidence
   - ED(id1, CRT, id2, CRT): Euclidean distance between reference points
   - Lat(id, CRT), Lon(id, CRT): Lateral/longitudinal position
   - Area(Ω): Area of spatial expression

5. **Quantifiers**:
   - ∃{id₁, ...}@φ: Existential over object IDs
   - ∀{id₁, ...}@φ: Universal over object IDs
   - {x, f}.φ: Freeze quantifier (pin time/frame for duration measurement)

### Example STQL Formula

```
// "Always when a car is detected, it stays in the left lane for at least 5 seconds"
□(∃{car}@(C(car) = "vehicle") → {x}.(□[0,5] ∃(BB(car) ⊓ LeftLane) ∧ (x - C_TIME ≤ 5)))
```

## Codebase Structure

### Directory Layout

```
PerceMon/
├── include/
│   └── percemon/                # Modern C++20 implementation
│       ├── stql.hpp              # Core STQL AST (variant-based)
│       ├── monitoring.hpp        # Monitoring requirements interface
│       ├── datastream.hpp        # Runtime data structures
│       ├── evaluation.hpp        # Formula evaluation semantics
│       ├── online_monitor.hpp    # Streaming monitor
│       └── spatial.hpp           # Spatial region operations
├── csrc/
│   ├── stql_requirements.cc      # Memory requirements computation
│   ├── datastream.cc             # Runtime data implementations
│   ├── spatial.cc                # Spatial operations
│   └── monitoring/
│       ├── evaluation.cc         # Boolean evaluator implementation
│       └── online_monitor.cc     # Online monitor implementation
├── tests/                        # Test suite (Catch2)
└── examples/mot17/               # MOT17 dataset example
```

### Core Architecture

**`stql.hpp` - Core AST Implementation**:
- Type-safe variant-based AST (no deep inheritance)
- 40+ expression types covering all STQL operators
- CRTP-based strongly-typed variables (TimeVar, FrameVar, ObjectVar)
- Box<T> wrapper for recursive variant types
- Comprehensive documentation with STQL syntax notation
- Factory functions and operator overloads for ergonomic API
- Zero runtime overhead via compile-time `std::visit` dispatch

**`monitoring.hpp` - Monitoring Requirements**:
- History/Horizon computation: Analyze formulas to determine memory needs
- Monitorability predicates: `is_online_monitorable()`, `is_past_time_formula()`
- Time-to-frame conversion using FPS

**`datastream.hpp` - Runtime Data**:
- BoundingBox, Object, Frame, Trace types
- RefPointType enum for spatial reference points
- Utility functions: `get_reference_point()`, `euclidean_distance()`

**`evaluation.hpp` - Formula Evaluation**:
- EvaluationContext: Manages buffers and variable bindings
- BooleanEvaluator: Evaluates formulas with boolean semantics
- Complete support for all STQL operators
- Error handling for missing objects and insufficient buffers

**`online_monitor.hpp` - Streaming Monitor**:
- OnlineMonitor: Manages formula evaluation on frame streams
- Automatic buffer management (history/horizon windows)
- Frame-by-frame verdicts and monitorability validation

**`spatial.hpp` - Spatial Operations**:
- Region representations and geometric operations
- Set operations: Union, intersection, complement
- Membership testing for bounding boxes

## Key Design Improvements

1. **No Deep Inheritance**: Eliminated virtual inheritance and `dynamic_pointer_cast`
   - Old: BaseExpr → BaseSpatialExpr → 40+ derived classes
   - New: Single std::variant<...> with flat type list

2. **Compile-time Type Safety**: C++20 concepts replace runtime type checks
   - ASTNode concept ensures all types have to_string()
   - Variant dispatch catches type errors at compile time

3. **Ergonomic API**: Builder pattern via factory functions and operators
   - Old: `std::make_shared<AndExpr>(std::make_shared<NotExpr>(phi), ...)`
   - New: `~phi & next(psi)`

4. **Clear Separation of Concerns**:
   - stql.hpp: Pure syntax tree (immutable data structures)
   - monitoring.hpp: Requirements analysis
   - datastream.hpp: Runtime data structures
   - evaluation.hpp: Semantics and evaluation logic
   - online_monitor.hpp: Stream processing and buffer management

## Known Architectural Limitations

**Temporal Scope Separation**: Formulas must use **either** future-time operators **or** past-time operators, but **not both in the same formula**. This limitation is documented in `csrc/monitoring/evaluation.cc` (lines 153-155).

**Why**: The EvaluationContext maintains history and horizon buffers separately. Mixing temporal scopes would require bidirectional frame traversal, which is not currently implemented.

**Current Behavior**:
- Pure future-time formulas: ✅ Fully supported
- Pure past-time formulas: ✅ Fully supported
- Mixed temporal scopes: ❌ Not supported (undefined behavior)

**Testing**: When writing tests for temporal operators, ensure all formulas use only future-time operators, only past-time operators, or no temporal operators at all.

## Legacy Design

The old `include/percemon/ast.hpp` had issues with virtual inheritance, tight coupling, and runtime type checking. The refactoring to `percemon` is **complete**. All code now uses the modern `percemon` namespace with C++20 design patterns. The legacy `percemon` namespace has been fully removed.

## Development Conventions

### Code Style

- Use `snake_case` for functions and variables
- Use `PascalCase` for types and concepts
- Prefer `auto` for complex types when intent is clear
- Use `[[nodiscard]]` for pure functions returning values
- Use trailing return types (`auto func() -> Type`)
- Mark constructors `explicit` to prevent implicit conversions
- Comprehensive comments referencing STQL paper definitions

### Documentation Standards

**CRITICAL**: Every AST node type MUST include comprehensive documentation comments with:
1. **STQL Syntax Line**: Include formal mathematical notation from paper
2. **Plain English Explanation**: Describe semantics without jargon
3. **Concrete Example**: Show how to construct and use the node
4. **Paper Reference**: Link to specific section/definition
5. **Related Operators**: Cross-reference derived/dual/related operators

See doc/DESIGN.md for detailed documentation template.

## Implementation Status

### ✅ Completed

1. **Core STQL AST** (include/percemon/stql.hpp)
   - All 40+ expression types implemented
   - Variant-based design with Box<T> for recursion
   - Factory functions and operator overloads
   - Comprehensive documentation with STQL syntax references
   - to_string() for all types

2. **Monitoring Requirements Analysis** (include/percemon/monitoring.hpp, csrc/stql_requirements.cc)
   - History and Horizon structs
   - compute_requirements() function with scope-aware constraint handling
   - is_online_monitorable() and is_past_time_formula() helpers
   - Comprehensive visitor for all expression types
   - Time-to-frame conversion using FPS
   - Scope direction tracking (Future vs Past)

3. **Datastream Runtime Data** (include/percemon/datastream.hpp, csrc/datastream.cc)
   - BoundingBox, Object, Frame, Trace types
   - RefPointType enum for spatial reference points
   - Utility functions: get_reference_point(), euclidean_distance()

4. **Formula Evaluation** (include/percemon/evaluation.hpp, csrc/monitoring/evaluation.cc)
   - EvaluationContext for managing evaluation state
   - History and horizon buffer management
   - Frozen variable bindings
   - BooleanEvaluator with complete boolean semantics
   - Error handling for missing objects and insufficient buffers

5. **Online Monitoring** (include/percemon/online_monitor.hpp, csrc/monitoring/online_monitor.cc)
   - OnlineMonitor class for streaming evaluation
   - Automatic buffer management (history/horizon)
   - Frame-by-frame evaluation with verdicts
   - Memory requirement tracking and monitorability checks

6. **Spatial Operations** (include/percemon/spatial.hpp, csrc/spatial.cc)
   - Spatial region representations and operations
   - Union, intersection, complement operations
   - Membership testing for bounding boxes

7. **Comprehensive Test Suite** (tests/test_monitoring.cc)
   - Unit tests for simple formulas
   - Tests for all temporal operators
   - Tests for logical operators
   - Tests for quantifiers
   - Tests for constrained operators with meaningful bounds
   - Tests for nested temporal operators
   - Tests for edge cases and scope direction validation

### 🚧 Extended Features (Future Work)

1. **Bounded Temporal Operators**: Intervals like □[a,b]φ, ◇[a,b]φ
2. **Spatial Temporal Operators**: Spatial versions (○_s, □_s, ◇_s, U_s, S_s)
3. **Advanced Features**: Formula simplification, interval-based optimization, robustness analysis

## Key References

- STQL formal semantics and definitions: `./claude-cache/paper/`
- For implementation patterns and C++20 features: See doc/DESIGN.md
- For testing guidelines and patterns: See doc/TESTING.md
- For memory requirements computation deep dive: See doc/TECHNICAL_DETAILS.md
- Build system: CMake (configured) + Pixi for development environment

## Common Patterns in STQL

1. **Bounded Temporal Operators**:
   ```
   □[a,b] φ    // Always φ holds in frame interval [a,b]
   ◇[a,b] φ    // Eventually φ holds in frame interval [a,b]
   ```

2. **Nested Quantifiers**:
   ```
   ∃{car}@(∃(BB(car) ⊓ Lane1))
   ```

3. **Freeze-then-Compare**:
   ```
   {x}.(Eventually((x - C_TIME > 5.0) ∧ φ))
   ```
