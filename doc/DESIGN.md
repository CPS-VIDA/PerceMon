# PerceMon Implementation Design Guide

This document details C++20 patterns, code style conventions, and implementation
practices used in the `percemon` namespace.

## C++20 Features Used

### 1. Concepts

Concepts provide compile-time type constraints without runtime overhead:

```cpp
template <typename T>
concept ASTNode = requires(const T& t) {
  { t.to_string() } -> std::convertible_to<std::string>;
};
```

Benefits:
- Compile-time verification of type requirements
- Better error messages than SFINAE
- Clear API contracts

### 2. std::variant with std::visit

Core pattern for type-safe AST without inheritance:

```cpp
using Expr = std::variant<
    ConstExpr, NotExpr, AndExpr, OrExpr,
    NextExpr, PreviousExpr, // ... all 40+ types
>;

// Pattern matching via std::visit
auto result = std::visit([](const auto& e) {
    return e.to_string();
}, expr);
```

Benefits:
- No virtual function overhead
- Exhaustiveness checking at compile time
- Type safety with no dynamic_cast

### 3. CRTP (Curiously Recurring Template Pattern)

For strongly-typed variables with shared functionality:

```cpp
template <typename Derived>
struct Variable {
    auto operator<=>(const Variable&) const = default;
    // ... shared functionality
};

struct TimeVar : Variable<TimeVar> { ... };
struct FrameVar : Variable<FrameVar> { ... };
```

Benefits:
- Static polymorphism without virtual functions
- Strongly-typed distinctions at compile time
- Zero runtime overhead

### 4. Three-way Comparison (operator<=>)

Default comparisons for simple types:

```cpp
auto operator<=>(const Variable&) const = default;
```

Benefits:
- Generates all comparison operators automatically
- Enables use in containers and algorithms
- No manual operator== and operator< needed

### 5. Trailing Return Types

Modern function declaration style:

```cpp
[[nodiscard]] auto to_string() const -> std::string;
[[nodiscard]] auto next(Expr e, size_t n = 1) -> Expr;
```

Benefits:
- Return type visible at point of use
- Works better with complex template types
- Consistent style across codebase

## Code Style Conventions

### Naming

- **Functions/Variables**:
  `snake_case`
  ```cpp
  auto compute_requirements(const Expr& formula) -> MonitoringRequirements;
  auto frame_count = 0;
  ```

- **Types/Concepts**:
  `PascalCase`
  ```cpp
  struct ConstExpr { ... };
  concept ASTNode { ... };
  ```

- **Constants**:
  `UPPER_SNAKE_CASE`
  ```cpp
  constexpr int CAR = 1;
  constexpr size_t UNBOUNDED = std::numeric_limits<size_t>::max();
  ```

### Type Inference

Prefer `auto` for complex types when intent is clear:

```cpp
// Good: intent clear from RHS
auto evaluator = BooleanEvaluator{};

// Good: explicitly shows type
const double fps = 10.0;

// Avoid: type unclear
auto x = compute_requirements(formula);  // What type?

// Better:
auto reqs = compute_requirements(formula);  // MonitoringRequirements
```

### Attributes

Use `[[nodiscard]]` for pure functions returning values:

```cpp
[[nodiscard]] auto to_string() const -> std::string;
[[nodiscard]] auto is_online_monitorable(const Expr& e) -> bool;
```

Omit for functions that modify state:

```cpp
void add_frame(const Frame& f);  // Modifies this->frames
```

### Constructors

Mark constructors `explicit` to prevent implicit conversions:

```cpp
struct TimeVar {
    std::string name;
    explicit TimeVar(std::string n) : name(std::move(n)) {}
};

// Usage:
// auto tv = TimeVar{"x"};  // OK
// auto tv = "x"s;          // ERROR: prevented by explicit
```

### Comments

Include comprehensive comments referencing STQL paper definitions:

```cpp
/**
 * @brief STQL always temporal operator.
 *
 * In STQL syntax: □φ (always)
 *
 * Future-time temporal operator. □φ is true if φ holds in all future states.
 * Requires bounded horizon for online monitoring.
 *
 * @see STQL Definition in paper Section 2, Definition 1
 * @see Related: Eventually (◇), Release (dual)
 */
struct AlwaysExpr {
    Box<Expr> arg;
    // ...
};
```

## Implementation Patterns in percemon

### 1. Box<T> Wrapper for Recursive Types

Since `std::variant` cannot directly contain itself, use `Box<T>` to wrap
recursive members:

```cpp
struct NotExpr {
    Box<Expr> arg;  // Expr can contain NotExpr recursively
    explicit NotExpr(Expr e);
};

struct AndExpr {
    Box<Expr> lhs;
    Box<Expr> rhs;
    AndExpr(Expr l, Expr r);
};
```

`Box<T>` should be:
- Small wrapper around `std::unique_ptr<T>` or similar
- Provides ergonomic access via operator* and operator->
- Ensures expressions remain lightweight

### 2. Forward Declarations Before Variant

All expression types must be forward-declared before the `Expr` variant type:

```cpp
// Forward declarations
struct ConstExpr;
struct NotExpr;
struct AndExpr;
struct OrExpr;
// ... all 40+ types ...

// Then define the variant
using Expr = std::variant<
    ConstExpr, NotExpr, AndExpr, OrExpr,
    NextExpr, PreviousExpr, // ... etc
>;

// Finally implement the types
struct ConstExpr { ... };
struct NotExpr { ... };
// ... implementations ...
```

**Why**:
Variant needs complete type information.
This pattern ensures all types are declared before variant definition.

### 3. Inline Constructors After Complete Types

Constructors that take `Expr` or `SpatialExpr` must be defined after the variant
is complete:

```cpp
struct NotExpr {
    Box<Expr> arg;
    explicit NotExpr(Expr e);  // Declaration only - incomplete type
};

// ... after Expr is defined ...

inline NotExpr::NotExpr(Expr e) : arg(std::move(e)) {}
```

This pattern avoids compilation errors from incomplete variant types.

### 4. Factory Functions for Ergonomics

Provide free functions for common operations:

```cpp
inline auto make_true() -> Expr {
    return ConstExpr{true};
}

inline auto make_false() -> Expr {
    return ConstExpr{false};
}

inline auto next(Expr e, size_t n = 1) -> Expr {
    return NextExpr{std::move(e), n};
}

inline auto always(Expr e) -> Expr {
    return AlwaysExpr{std::move(e)};
}

inline auto eventually(Expr e) -> Expr {
    return EventuallyExpr{std::move(e)};
}
```

Benefits:
- Shorter, more readable code
- Consistent naming across API
- Easy to extend with new constructors

### 5. Operator Overloading

Enable natural formula construction:

```cpp
inline auto operator~(Expr e) -> Expr {
    return NotExpr{std::move(e)};
}

inline auto operator&(Expr lhs, Expr rhs) -> Expr {
    return AndExpr{std::move(lhs), std::move(rhs)};
}

inline auto operator|(Expr lhs, Expr rhs) -> Expr {
    return OrExpr{std::move(lhs), std::move(rhs)};
}
```

Usage:
```cpp
// Instead of: AndExpr(NotExpr(phi), NextExpr(psi))
auto formula = ~phi & next(psi);
```

### 6. Visitor Pattern for Traversal

Use `std::visit` with lambdas for operations on expressions:

```cpp
auto to_string_visitor = [](const auto& e) {
    return e.to_string();
};
std::string result = std::visit(to_string_visitor, expr);

// Or inline:
std::visit([&](const auto& e) {
    using T = std::decay_t<decltype(e)>;
    if constexpr (std::is_same_v<T, NextExpr>) {
        // Handle NextExpr
    } else if constexpr (std::is_same_v<T, AlwaysExpr>) {
        // Handle AlwaysExpr
    } // ... etc
}, formula);
```

Benefits:
- Exhaustiveness checking at compile time
- No virtual function dispatch
- Type-safe pattern matching

## Documentation Standards

**CRITICAL**:
Every AST node type MUST include comprehensive documentation comments.

### Documentation Template

```cpp
/**
 * @brief Brief one-line description
 *
 * In STQL syntax: [formal notation from paper]
 *
 * [Scope type - Future-time or Past-time]
 *
 * Detailed explanation of what this operator does:
 * - Semantics: What it means in plain English
 * - When/how it's used in practice
 * - Any special properties (derived operators, duals, equivalences)
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto formula = operator_name(phi);  // Concrete usage example
 * // Explanation of what this computes
 * @endcode
 *
 * @see STQL Definition in paper Section X (Definition Y)
 * @see Related operators: OtherOperator, AnotherOperator
 */
struct OperatorExpr {
    // ... implementation
};
```

### Complete Example

```cpp
/**
 * @brief STQL until temporal operator.
 *
 * In STQL syntax: φ₁ U φ₂ (until)
 *
 * Future-time temporal operator.
 *
 * φ₁ U φ₂ is true if φ₂ eventually becomes true,
 * and φ₁ holds at every step until that happens.
 * This operator requires bounded horizon for online monitoring.
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

### Key Requirements for Documentation

1. **STQL Syntax Line**:
   Must include "In STQL syntax:" with formal mathematical notation
2. **Scope Annotation**:
   Clearly state if Future-time or Past-time
3. **Plain English Explanation**:
   Describe semantics without jargon
4. **Usage Example**:
   Show how to construct and use the node in code
5. **Paper Reference**:
   Link to specific section/definition in STQL paper
6. **Related Operators**:
   Cross-reference derived/dual/related operators

### Documentation Ensures

- Users can quickly map code back to formal STQL grammar
- Implementation is understandable without constantly consulting the paper
- Examples show idiomatic usage patterns
- Relationships between operators are clear and discoverable

## Adding New Operators

When extending the AST with new operators, follow this checklist:

1. **Add forward declaration** before `Expr` variant
2. **Define struct** with complete documentation (use template above)
3. **Add to variant type list**:
   Include in `Expr` or `SpatialExpr`
4. **Implement constructor** after variant is complete
5. **Add factory function** and/or operator overload
6. **Implement to_string()** method
7. **Add visitor handling** in all traversal functions
8. **Add corresponding tests** in tests/test_monitoring.cc

Example workflow:

```cpp
// Step 1: Forward declaration
struct NewOperatorExpr;

// Step 2: Add to variant
using Expr = std::variant<
    // ... existing types ...
    NewOperatorExpr,
>;

// Step 3: Define struct with documentation
struct NewOperatorExpr {
    Box<Expr> arg;

    explicit NewOperatorExpr(Expr e) : arg(std::move(e)) {}
    [[nodiscard]] std::string to_string() const;
};

// Step 4: Implement to_string()
inline auto NewOperatorExpr::to_string() const -> std::string {
    return "new_op(" + arg->to_string() + ")";
}

// Step 5: Add factory function
inline auto new_operator(Expr e) -> Expr {
    return NewOperatorExpr{std::move(e)};
}

// Step 6: Handle in visitors
std::visit([](const auto& expr) {
    using T = std::decay_t<decltype(expr)>;
    if constexpr (std::is_same_v<T, NewOperatorExpr>) {
        // Handle NewOperatorExpr
    }
    // ... other cases ...
}, formula);
```

## Build System Notes

- **CMake-based**:
  Configuration in CMakeLists.txt
- **No manual changes during refactoring**:
  Maintainer handles CMake updates
- **Pixi**:
  Development environment management via pixi.toml
- **C++20 requirement**:
  Set in CMakeLists.txt, enforced across codebase

## Performance Considerations

### Zero-Cost Abstractions

The percemon design achieves zero-cost abstractions:

- **std::variant dispatch**:
  Resolved at compile-time via std::visit
- **No virtual functions**:
  No runtime polymorphism overhead
- **CRTP**:
  Static polymorphism replaces virtual dispatch
- **Inline everything**:
  Small utility functions compiled inline
- **Move semantics**:
  No unnecessary copies of expressions

### Memory Efficiency

- **Box<T>**:
  Wraps unique_ptr for efficient recursive types
- **Immutable AST**:
  Expressions can be safely shared
- **Stack allocation**:
  Small types stored directly in variant
- **No separate allocator**:
  Standard allocator sufficient

## Testing the Implementation

For guidelines on testing new implementations, see doc/TESTING.md.

Key testing practices:
- Use Catch2 framework for all tests
- Test both API usability and semantic correctness
- Include edge cases (empty formulas, deeply nested structures)
- Verify integration with existing operators
- Test with meaningful constraints (not vacuous formulas)
