#pragma once

#include <type_traits>
#ifndef PERCEMON_STQL_HPP
#define PERCEMON_STQL_HPP

#include <cmath>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

/**
 * @file stql.hpp
 * @brief Spatio-Temporal Quality Logic (STQL) AST implementation
 *
 * This file implements the abstract syntax tree for STQL formulas using modern C++20
 * features. STQL is an extension of Timed Quality Temporal Logic (TQTL) that incorporates
 * reasoning about spatial structures in perception data.
 *
 * @see STQL paper: Hekmatnejad et al., "Formalizing and Monitoring Temporal First-Order
 *      Logic Properties of Perception Systems" (2021)
 */

namespace percemon::stql {

// =============================================================================
// Core Concepts
// =============================================================================

/**
 * @brief Concept for any AST node that can be converted to a string.
 *
 * All AST node types must satisfy this concept, providing a to_string() method
 * for debugging and display purposes.
 */
template <typename T>
concept ASTNode = requires(const T& t) {
  { t.to_string() } -> std::convertible_to<std::string>;
};

// =============================================================================
// Variable Types
// =============================================================================

/**
 * @brief Base variable template with CRTP for strong typing.
 *
 * Uses the Curiously Recurring Template Pattern (CRTP) to provide type-safe
 * variable implementations while sharing common functionality.
 */
template <typename Derived>
struct Variable {
  std::string name;

  // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
  explicit Variable(std::string name_) : name(std::move(name_)) {}

  [[nodiscard]] auto to_string() const -> std::string { return name; }

  auto operator<=>(const Variable&) const = default;
};

/**
 * @brief STQL time variable.
 *
 * In STQL syntax: x ∈ V_t (time variables)
 *
 * Represents time variables used in freeze quantifiers and time constraints.
 * Time variables capture timestamps for measuring temporal durations.
 * When frozen using the {x, f}.φ operator, a time variable x stores the
 * timestamp of the current frame, allowing later comparisons like
 * (x - C_TIME) to measure elapsed time.
 *
 * Example:
 * @code
 * auto x = TimeVar{"x"};
 * auto constraint = freeze(x, (x - C_TIME) <= 5.0);  // {x}.((x - C_TIME) ≤ 5.0)
 * // This freezes the current time as 'x', then checks if elapsed time ≤ 5 seconds
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: C_TIME (current time constant), TimeDiff, TimeBoundExpr, FreezeExpr
 */
struct TimeVar : Variable<TimeVar> {
  using Variable::Variable;

  /// Factory for creating the current time constant
  static auto current() -> TimeVar { return TimeVar{"C_TIME"}; }
};

/**
 * @brief STQL current time constant.
 *
 * In STQL syntax: C_TIME (current time reference)
 *
 * Represents the timestamp where the current formula is being evaluated.
 * Used in time constraints like (x - C_TIME) ∼ t to measure time differences
 * from when a time variable x was frozen to the current evaluation time.
 *
 * @see Related: TimeVar, TimeDiff, FreezeExpr
 */
inline const TimeVar C_TIME = TimeVar::current();

/**
 * @brief STQL frame variable.
 *
 * In STQL syntax: f ∈ V_f (frame variables)
 *
 * Represents frame variables used in freeze quantifiers and frame constraints.
 * Frame variables capture frame numbers for measuring frame-based timing.
 * When frozen using the {x, f}.φ operator, a frame variable f stores the
 * frame number, allowing later comparisons like (f - C_FRAME) to measure
 * elapsed frames.
 *
 * Example:
 * @code
 * auto f = FrameVar{"f"};
 * auto constraint = freeze(f, (f - C_FRAME) <= 10);  // {f}.((f - C_FRAME) ≤ 10)
 * // This freezes the current frame as 'f', then checks if elapsed frames ≤ 10
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: C_FRAME (current frame constant), FrameDiff, FrameBoundExpr, FreezeExpr
 */
struct FrameVar : Variable<FrameVar> {
  using Variable::Variable;

  /// Factory for creating the current frame constant
  static auto current() -> FrameVar { return FrameVar{"C_FRAME"}; }
};

/**
 * @brief STQL current frame constant.
 *
 * In STQL syntax: C_FRAME (current frame reference)
 *
 * Represents the frame number where the current formula is being evaluated.
 * Used in frame constraints like (f - C_FRAME) ∼ n to measure frame differences
 * from when a frame variable f was frozen to the current frame.
 *
 * @see Related: FrameVar, FrameDiff, FreezeExpr
 */
inline const FrameVar C_FRAME = FrameVar::current();

/**
 * @brief STQL object ID variable.
 *
 * In STQL syntax: id ∈ V_o (object ID variables)
 *
 * Represents object ID variables used in quantifiers and object-based predicates.
 * Object variables are bound by existential/universal quantifiers (∃, ∀) and refer
 * to detected objects in perception data frames. Each object has properties like
 * class, confidence, and bounding box that can be accessed through STQL operators.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto pedestrian = ObjectVar{"pedestrian"};
 * // "There exists a car and pedestrian"
 * auto formula = exists({car, pedestrian},  condition on car and pedestrian );
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ExistsExpr, ForallExpr, BBoxExpr, ClassFunc, ProbFunc
 */
struct ObjectVar : Variable<ObjectVar> {
  using Variable::Variable;
};

// =============================================================================
// Coordinate Reference Points
// =============================================================================

/**
 * @brief STQL coordinate reference points for bounding boxes.
 *
 * In STQL syntax: CRT ∈ {LM, RM, TM, BM, CT} (coordinate reference points)
 *
 * Defines reference points on bounding boxes for spatial measurements.
 * These points are used in distance functions like ED(id_i, CRT, id_j, CRT)
 * and position functions like Lat(id_i, CRT), Lon(id_i, CRT).
 *
 * Reference points:
 * - LeftMargin (LM): Center of the left edge
 * - RightMargin (RM): Center of the right edge
 * - TopMargin (TM): Center of the top edge
 * - BottomMargin (BM): Center of the bottom edge
 * - Center (CT): Geometric center (centroid) of the bounding box
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto truck = ObjectVar{"truck"};
 * // Measure distance between car's center and truck's left edge
 * auto dist = EuclideanDistFunc{
 *     RefPoint{car, CoordRefPoint::Center},
 *     RefPoint{truck, CoordRefPoint::LeftMargin}
 * };
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: RefPoint, EuclideanDistFunc, LatFunc, LonFunc
 */
enum class CoordRefPoint : uint8_t {
  LeftMargin,   ///< LM: Left edge center
  RightMargin,  ///< RM: Right edge center
  TopMargin,    ///< TM: Top edge center
  BottomMargin, ///< BM: Bottom edge center
  Center        ///< CT: Geometric center (centroid)
};

/**
 * @brief Convert coordinate reference point to string representation.
 */
inline auto to_string(CoordRefPoint crt) -> std::string {
  switch (crt) {
    case CoordRefPoint::LeftMargin: return "LM";
    case CoordRefPoint::RightMargin: return "RM";
    case CoordRefPoint::TopMargin: return "TM";
    case CoordRefPoint::BottomMargin: return "BM";
    case CoordRefPoint::Center: return "CT";
    default: throw std::logic_error("Invalid CoordRefPoint");
  }
}

/**
 * @brief STQL reference point specification for spatial measurements.
 *
 * Combines an object ID with a coordinate reference point (CRT) to specify
 * a precise location on a bounding box for spatial computations.
 *
 * Used in spatial functions like:
 * - ED(id_i, CRT, id_j, CRT): Euclidean distance between reference points
 * - Lat(id_i, CRT): Lateral position of reference point
 * - Lon(id_i, CRT): Longitudinal position of reference point
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto ref = RefPoint{car, CoordRefPoint::Center};
 * // This refers to the center point of the car's bounding box
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: CoordRefPoint, EuclideanDistFunc, LatFunc, LonFunc
 */
struct RefPoint {
  ObjectVar object;
  CoordRefPoint crt;

  RefPoint(ObjectVar obj, CoordRefPoint c) : object(std::move(obj)), crt(c) {}

  [[nodiscard]] auto to_string() const -> std::string {
    return object.to_string() + "." + stql::to_string(crt);
  }
};

// =============================================================================
// Comparison Operators
// =============================================================================

/**
 * @brief Comparison operators for STQL predicates.
 *
 * In STQL syntax: ∼ ∈ {<, ≤, >, ≥, =, ≠}
 *
 * Used in various comparison expressions:
 * - Time bounds: (x - C_TIME) ∼ t
 * - Frame bounds: (f - C_FRAME) ∼ n
 * - Probability: P(id) ∼ r
 * - Distance: ED(id_i, CRT, id_j, CRT) ∼ r
 * - Area: Area(Ω) ∼ r
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 */
enum class CompareOp : uint8_t {
  LessThan,     ///< <
  LessEqual,    ///< ≤
  GreaterThan,  ///< >
  GreaterEqual, ///< ≥
  Equal,        ///< =
  NotEqual      ///< ≠
};

/**
 * @brief Convert comparison operator to string representation.
 */
inline auto to_string(CompareOp op) -> std::string {
  switch (op) {
    case CompareOp::LessThan: return "<";
    case CompareOp::LessEqual: return "<=";
    case CompareOp::GreaterThan: return ">";
    case CompareOp::GreaterEqual: return ">=";
    case CompareOp::Equal: return "==";
    case CompareOp::NotEqual: return "!=";
    default: throw std::logic_error("Invalid CompareOp");
  }
}

/**
 * @brief Negate a comparison operator.
 *
 * Returns the negation of the comparison:
 * - ¬(x < y) ≡ (x ≥ y)
 * - ¬(x ≤ y) ≡ (x > y)
 * - ¬(x = y) ≡ (x ≠ y)
 * etc.
 */
constexpr auto negate(CompareOp op) -> CompareOp {
  switch (op) {
    case CompareOp::LessThan: return CompareOp::GreaterEqual;
    case CompareOp::LessEqual: return CompareOp::GreaterThan;
    case CompareOp::GreaterThan: return CompareOp::LessEqual;
    case CompareOp::GreaterEqual: return CompareOp::LessThan;
    case CompareOp::Equal: return CompareOp::NotEqual;
    case CompareOp::NotEqual: return CompareOp::Equal;
  }
  return op; // Should never reach here
}

/**
 * @brief Flip a comparison operator (swap LHS and RHS).
 *
 * Returns the flipped comparison that allows swapping operands:
 * - (x < y) ≡ (y > x)
 * - (x ≤ y) ≡ (y ≥ x)
 * - (x = y) ≡ (y = x)
 * etc.
 *
 * Note: This is NOT negation, it changes the direction of the comparison.
 */
constexpr auto flip(CompareOp op) -> CompareOp {
  switch (op) {
    case CompareOp::LessThan: return CompareOp::GreaterThan;
    case CompareOp::LessEqual: return CompareOp::GreaterEqual;
    case CompareOp::GreaterThan: return CompareOp::LessThan;
    case CompareOp::GreaterEqual: return CompareOp::LessEqual;
    default: return op; // Equal and NotEqual are symmetric
  }
}

// =============================================================================
// Recursive Type Wrapper
// =============================================================================

/**
 * @brief Wrapper for recursive types in variant.
 *
 * Since std::variant cannot directly contain recursive types, we use Box<T>
 * to wrap recursive members with shared_ptr. This allows expressions to
 * contain other expressions as children.
 *
 * Example:
 * @code
 * struct NotExpr {
 *     Box<Expr> arg;  // Expr can contain NotExpr recursively
 * };
 * @endcode
 */
template <typename T>
struct Box {
  std::shared_ptr<T> ptr;

  Box() : ptr(std::make_shared<T>()) {}
  Box(T value) : ptr(std::make_shared<T>(std::move(value))) {}

  auto operator*() -> T& { return *ptr; }
  auto operator*() const -> const T& { return *ptr; }
  auto operator->() -> T* { return ptr.get(); }
  auto operator->() const -> const T* { return ptr.get(); }
};

// =============================================================================
// Forward Declarations for Expression Types
// =============================================================================

// Boolean and propositional
struct ConstExpr;
struct NotExpr;
struct AndExpr;
struct OrExpr;

// Temporal operators
struct NextExpr;
struct PreviousExpr;
struct AlwaysExpr;
struct HoldsExpr;
struct EventuallyExpr;
struct SometimesExpr;
struct UntilExpr;
struct SinceExpr;
struct ReleaseExpr;
struct BackToExpr;

// Quantifiers
struct ExistsExpr;
struct ForallExpr;
struct FreezeExpr;

// Time and frame constraints
struct TimeDiff;
struct FrameDiff;
struct TimeBoundExpr;
struct FrameBoundExpr;

// Object identity and class
struct ObjectIdCompareExpr;
struct ClassFunc;
struct ClassCompareExpr;

// Probability/confidence
struct ProbFunc;
struct ProbCompareExpr;

// Spatial distance and position
struct EuclideanDistFunc;
struct DistCompareExpr;
struct LatFunc;
struct LonFunc;
struct LatLonCompareExpr;

// Spatial expressions forward declarations
struct EmptySetExpr;
struct UniverseSetExpr;
struct BBoxExpr;
struct SpatialComplementExpr;
struct SpatialUnionExpr;
struct SpatialIntersectExpr;

// Spatial predicates
struct AreaFunc;
struct AreaCompareExpr;
struct SpatialExistsExpr;
struct SpatialForallExpr;

/**
 * @brief Main expression variant type.
 *
 * This variant contains all possible STQL expression types. Using std::variant
 * provides type-safe discriminated unions without deep inheritance hierarchies.
 *
 * The visitor pattern (std::visit) can be used to traverse and operate on expressions.
 */
using Expr = std::variant<
    ConstExpr,
    NotExpr,
    AndExpr,
    OrExpr,
    NextExpr,
    PreviousExpr,
    AlwaysExpr,
    HoldsExpr,
    EventuallyExpr,
    SometimesExpr,
    UntilExpr,
    SinceExpr,
    ReleaseExpr,
    BackToExpr,
    ExistsExpr,
    ForallExpr,
    FreezeExpr,
    TimeBoundExpr,
    FrameBoundExpr,
    ObjectIdCompareExpr,
    ClassCompareExpr,
    ProbCompareExpr,
    DistCompareExpr,
    LatLonCompareExpr,
    AreaCompareExpr,
    SpatialExistsExpr,
    SpatialForallExpr>;

/**
 * @brief Spatial expression variant type.
 *
 * This variant contains all possible spatial expression types (Ω in the grammar).
 * Spatial expressions represent sets of spatial regions (bounding boxes and operations on them).
 */
using SpatialExpr = std::variant<
    EmptySetExpr,
    UniverseSetExpr,
    BBoxExpr,
    SpatialComplementExpr,
    SpatialUnionExpr,
    SpatialIntersectExpr>;

// =============================================================================
// Boolean and Propositional Expressions
// =============================================================================

/**
 * @brief STQL boolean constant.
 *
 * In STQL syntax: ⊤ (top/true) or ⊥ (bottom/false)
 *
 * Represents propositional constants in STQL formulas. Used as base cases
 * in formula evaluation and as building blocks for more complex expressions.
 *
 * Example:
 * @code
 * auto always_true = ConstExpr{true};   // ⊤
 * auto always_false = ConstExpr{false}; // ⊥
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: make_true(), make_false() factory functions
 */
struct ConstExpr {
  bool value;

  explicit ConstExpr(bool v) : value(v) {}

  [[nodiscard]] auto to_string() const -> std::string { return value ? "⊤" : "⊥"; }
};

/**
 * @brief STQL negation operator.
 *
 * In STQL syntax: ¬φ (negation)
 *
 * Represents logical negation in STQL formulas. Evaluates to true if the
 * argument evaluates to false, and vice versa.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto not_phi = NotExpr{phi};  // ¬⊤
 * // Or using the ~ operator: auto not_phi = ~phi;
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: operator~(Expr) for convenient construction
 */
struct NotExpr {
  Box<Expr> arg;

  explicit NotExpr(Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL conjunction operator.
 *
 * In STQL syntax: φ₁ ∧ φ₂ ∧ ... ∧ φₙ (conjunction)
 *
 * Represents logical conjunction in STQL formulas. Evaluates to true if all
 * arguments evaluate to true. This is a derived operator in STQL, defined as
 * φ₁ ∧ φ₂ ≡ ¬(¬φ₁ ∨ ¬φ₂).
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto psi = make_false();
 * auto conj = AndExpr{{phi, psi}};  // φ ∧ ψ
 * // Or using the & operator: auto conj = phi & psi;
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: OrExpr, operator&(Expr, Expr)
 */
struct AndExpr {
  std::vector<Expr> args;

  explicit AndExpr(std::vector<Expr> exprs);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL disjunction operator.
 *
 * In STQL syntax: φ₁ ∨ φ₂ ∨ ... ∨ φₙ (disjunction)
 *
 * Represents logical disjunction in STQL formulas. Evaluates to true if any
 * argument evaluates to true. This is a fundamental operator in STQL.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto psi = make_false();
 * auto disj = OrExpr{{phi, psi}};  // φ ∨ ψ
 * // Or using the | operator: auto disj = phi | psi;
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: AndExpr, operator|(Expr, Expr)
 */
struct OrExpr {
  std::vector<Expr> args;

  explicit OrExpr(std::vector<Expr> exprs);

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Temporal Operators
// =============================================================================

/**
 * @brief STQL next temporal operator.
 *
 * In STQL syntax: ○φ (next)
 *
 * Future-time temporal operator that evaluates φ in the next frame.
 * ○φ is true at frame i if φ is true at frame i+1.
 * Can be extended to ○ⁿφ to look n frames ahead.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto next_phi = NextExpr{phi, 1};     // ○φ (next frame)
 * auto next3_phi = NextExpr{phi, 3};     // ○○○φ (3 frames ahead)
 * // Or using factory: auto next_phi = next(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: PreviousExpr (past dual), EventuallyExpr, AlwaysExpr
 */
struct NextExpr {
  Box<Expr> arg;
  size_t steps; ///< Number of frames to advance (default 1)

  explicit NextExpr(Expr e, size_t n = 1);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL previous temporal operator.
 *
 * In STQL syntax: ◦φ (previous)
 *
 * Past-time temporal operator that evaluates φ in the previous frame.
 * ◦φ is true at frame i if φ is true at frame i-1.
 * Can be extended to ◦ⁿφ to look n frames back.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto prev_phi = PreviousExpr{phi, 1};  // ◦φ (previous frame)
 * auto prev3_phi = PreviousExpr{phi, 3}; // ◦◦◦φ (3 frames back)
 * // Or using factory: auto prev_phi = previous(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: NextExpr (future dual), SometimesExpr, HoldsExpr
 */
struct PreviousExpr {
  Box<Expr> arg;
  size_t steps; ///< Number of frames to go back (default 1)

  explicit PreviousExpr(Expr e, size_t n = 1);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL always (globally) temporal operator.
 *
 * In STQL syntax: □φ (always/globally)
 *
 * Future-time temporal operator. □φ evaluates to true if φ holds in all
 * future frames. This is a derived operator: □φ ≡ false U φ (i.e., φ holds
 * until false, meaning φ always holds).
 *
 * Note: Without interval bounds, this operator requires infinite horizon
 * and is not online-monitorable.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto always_phi = AlwaysExpr{phi};  // □φ
 * // Or using factory: auto always_phi = always(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: EventuallyExpr (dual), HoldsExpr (past version), UntilExpr
 */
struct AlwaysExpr {
  Box<Expr> arg;

  explicit AlwaysExpr(Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL holds (past always) temporal operator.
 *
 * In STQL syntax: ■φ (past always)
 *
 * Past-time temporal operator. ■φ evaluates to true if φ holds in all
 * past frames. This is a derived operator: ■φ ≡ false S φ (i.e., φ holds
 * until false, meaning φ holds holds).
 *
 * Note: Without interval bounds, this operator requires infinite history
 * and is not online-monitorable.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto holds_phi = HoldsExpr{phi};  // ■φ
 * // Or using factory: auto holds_phi = holds(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SometimesExpr (dual), AlwaysExpr (past version), UntilExpr
 */
struct HoldsExpr {
  Box<Expr> arg;

  explicit HoldsExpr(Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL eventually (finally) temporal operator.
 *
 * In STQL syntax: ◇φ (eventually/finally)
 *
 * Future-time temporal operator. ◇φ evaluates to true if φ holds in some
 * future frame. This is a derived operator: ◇φ ≡ true U φ (i.e., true holds
 * until φ, meaning φ eventually holds).
 *
 * Note: Without interval bounds, this operator requires infinite horizon
 * and is not online-monitorable.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto eventually_phi = EventuallyExpr{phi};  // ◇φ
 * // Or using factory: auto eventually_phi = eventually(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: AlwaysExpr (dual), SometimesExpr (past version), UntilExpr
 */
struct EventuallyExpr {
  Box<Expr> arg;

  explicit EventuallyExpr(Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL sometimes (past eventually) temporal operator.
 *
 * In STQL syntax: ♦φ (sometimes/past eventually)
 *
 * Past-time temporal operator. ♦φ evaluates to true if φ holds in some
 * past frame. This is a derived operator: ♦φ ≡ true S φ (i.e., true holds
 * since φ, meaning φ sometimes holds).
 *
 * Note: Without interval bounds, this operator requires infinite history
 * and is not online-monitorable.
 *
 * Example:
 * @code
 * auto phi = make_true();
 * auto sometimes_phi = SometimesExpr{phi};  // ♦φ
 * // Or using factory: auto sometimes_phi = sometimes(phi);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: AlwaysExpr (dual), SometimesExpr (past version), UntilExpr
 */
struct SometimesExpr {
  Box<Expr> arg;

  explicit SometimesExpr(Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL until temporal operator.
 *
 * In STQL syntax: φ₁ U φ₂ (until)
 *
 * Future-time temporal operator. φ₁ U φ₂ is true if φ₂ eventually becomes
 * true, and φ₁ holds continuously until that happens.
 *
 * Semantics: ⋁_{i≤j} (φ₂@j ∧ ⋀_{i≤k≤j} φ₁@k)
 *
 * Example:
 * @code
 * auto safe = make_true();
 * auto goal = make_false();
 * auto until_expr = UntilExpr{safe, goal};  // safe U goal
 * // "Stay safe until the goal is reached"
 * // Or using factory: auto until_expr = until(safe, goal);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SinceExpr (past dual), ReleaseExpr (dual), EventuallyExpr
 */
struct UntilExpr {
  Box<Expr> lhs; ///< Formula that must hold until rhs
  Box<Expr> rhs; ///< Formula that eventually becomes true

  UntilExpr(Expr l, Expr r);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL since temporal operator.
 *
 * In STQL syntax: φ₁ S φ₂ (since)
 *
 * Past-time temporal operator. φ₁ S φ₂ is true if φ₂ was true at some past
 * time, and φ₁ has held continuously since then.
 *
 * Semantics: ⋁_{j≤i} (φ₂@j ∧ ⋀_{j≤k≤i} φ₁@k)
 *
 * Example:
 * @code
 * auto stable = make_true();
 * auto initialized = make_false();
 * auto since_expr = SinceExpr{stable, initialized};  // stable S initialized
 * // "Stable since initialization"
 * // Or using factory: auto since_expr = since(stable, initialized);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: UntilExpr (future dual), BackToExpr (dual), SometimesExpr
 */
struct SinceExpr {
  Box<Expr> lhs; ///< Formula that must hold since rhs
  Box<Expr> rhs; ///< Formula that was true in the past

  SinceExpr(Expr l, Expr r);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL release temporal operator.
 *
 * In STQL syntax: φ₁ R φ₂ (release)
 *
 * Future-time temporal operator, dual of Until. φ₁ R φ₂ is true if φ₂ holds
 * until and including when φ₁ becomes true. Equivalently: ¬(¬φ₁ U ¬φ₂).
 *
 * Example:
 * @code
 * auto trigger = make_false();
 * auto maintained = make_true();
 * auto release_expr = ReleaseExpr{trigger, maintained};  // trigger R maintained
 * // Or using factory: auto release_expr = release(trigger, maintained);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: UntilExpr (dual), BackToExpr (past version)
 */
struct ReleaseExpr {
  Box<Expr> lhs;
  Box<Expr> rhs;

  ReleaseExpr(Expr l, Expr r);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL back-to temporal operator.
 *
 * In STQL syntax: φ₁ B φ₂ (back-to)
 *
 * Past-time temporal operator, dual of Since. φ₁ B φ₂ is true if φ₂ has held
 * back to and including when φ₁ was true. Equivalently: ¬(¬φ₁ S ¬φ₂).
 *
 * Example:
 * @code
 * auto event = make_false();
 * auto condition = make_true();
 * auto backto_expr = BackToExpr{event, condition};  // event B condition
 * // Or using factory: auto backto_expr = backto(event, condition);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SinceExpr (dual), ReleaseExpr (future version)
 */
struct BackToExpr {
  Box<Expr> lhs;
  Box<Expr> rhs;

  BackToExpr(Expr l, Expr r);

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Quantifiers and Freeze
// =============================================================================

/**
 * @brief STQL existential quantifier over object IDs.
 *
 * In STQL syntax: ∃{id₁, id₂, ...}@φ (existential quantifier)
 *
 * Searches over each object in a frame, assigning each object to the specified
 * object variables. Evaluates to true if there exists at least one assignment
 * of objects to variables that satisfies the subformula φ.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto pedestrian = ObjectVar{"pedestrian"};
 * // "There exists a car and pedestrian such that..."
 * auto formula = ExistsExpr{{car, pedestrian},  condition };
 * // Or using factory: auto formula = exists({car, pedestrian}, condition);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ForallExpr (universal), ObjectVar
 */
struct ExistsExpr {
  std::vector<ObjectVar> variables;
  Box<Expr> body;

  ExistsExpr(std::vector<ObjectVar> vars, Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL universal quantifier over object IDs.
 *
 * In STQL syntax: ∀{id₁, id₂, ...}@φ (universal quantifier, derived)
 *
 * Universal quantification over object IDs. Equivalent to ¬∃{id₁, ...}@¬φ.
 * Evaluates to true if all possible assignments of objects to variables
 * satisfy the subformula φ.
 *
 * Example:
 * @code
 * auto vehicle = ObjectVar{"vehicle"};
 * // "For all vehicles, ..."
 * auto formula = ForallExpr{{vehicle}, condition };
 * // Or using factory: auto formula = forall({vehicle}, condition);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ExistsExpr (existential), ObjectVar
 */
struct ForallExpr {
  std::vector<ObjectVar> variables;
  Box<Expr> body;

  ForallExpr(std::vector<ObjectVar> vars, Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL freeze quantifier for pinning time and frame variables.
 *
 * In STQL syntax: {x, f}.φ (freeze quantifier)
 *
 * Freezes the current time and/or frame when the formula φ is evaluated,
 * assigning them to clock variables x and f. This allows measuring duration
 * and frame differences using expressions like (x - C_TIME) and (f - C_FRAME).
 *
 * At least one of time_var or frame_var must be specified.
 *
 * Example:
 * @code
 * auto x = TimeVar{"x"};
 * auto f = FrameVar{"f"};
 * // Freeze both time and frame
 * auto formula1 = FreezeExpr{x, f,  body };
 * // Freeze only time
 * auto formula2 = FreezeExpr{x, std::nullopt,  body };
 * // Or using factory: auto formula2 = freeze(x, body);
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: TimeVar, FrameVar, TimeBoundExpr, FrameBoundExpr
 */
struct FreezeExpr {
  std::optional<TimeVar> time_var;
  std::optional<FrameVar> frame_var;
  Box<Expr> body;

  FreezeExpr(const std::optional<TimeVar>& t, const std::optional<FrameVar>& f, Expr e);
  FreezeExpr(const std::optional<FrameVar>& f, Expr e);
  FreezeExpr(const std::optional<TimeVar>& t, Expr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Time and Frame Constraints
// =============================================================================

/**
 * @brief Time difference for constraint expressions.
 *
 * In STQL syntax: x - y (time difference)
 *
 * Represents the difference between two time variables, typically used in
 * constraints like (x - C_TIME) ∼ t to measure elapsed time.
 *
 * Example:
 * @code
 * auto x = TimeVar{"x"};
 * auto diff = TimeDiff{x, C_TIME};  // x - C_TIME
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: TimeVar, TimeBoundExpr, FreezeExpr
 */
struct TimeDiff {
  TimeVar lhs;
  TimeVar rhs;

  TimeDiff(TimeVar l, TimeVar r) : lhs(std::move(l)), rhs(std::move(r)) {}

  [[nodiscard]] auto to_string() const -> std::string {
    return lhs.to_string() + " - " + rhs.to_string();
  }
};

/**
 * @brief Frame difference for constraint expressions.
 *
 * In STQL syntax: f₁ - f₂ (frame difference)
 *
 * Represents the difference between two frame variables, typically used in
 * constraints like (f - C_FRAME) ∼ n to measure elapsed frames.
 *
 * Example:
 * @code
 * auto f = FrameVar{"f"};
 * auto diff = FrameDiff{f, C_FRAME};  // f - C_FRAME
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: FrameVar, FrameBoundExpr, FreezeExpr
 */
struct FrameDiff {
  FrameVar lhs;
  FrameVar rhs;

  FrameDiff(FrameVar l, FrameVar r) : lhs(std::move(l)), rhs(std::move(r)) {}

  [[nodiscard]] auto to_string() const -> std::string {
    return lhs.to_string() + " - " + rhs.to_string();
  }
};

/**
 * @brief STQL time bound constraint.
 *
 * In STQL syntax: (x - C_TIME) ∼ t where ∼ ∈ {<, ≤, >, ≥}
 *
 * Constrains the time difference between a frozen time variable and the current
 * time. Used to express temporal durations like "within 5 seconds".
 *
 * Example:
 * @code
 * auto x = TimeVar{"x"};
 * auto diff = TimeDiff{x, C_TIME};
 * // (x - C_TIME) ≤ 5.0
 * auto bound = TimeBoundExpr{diff, CompareOp::LessEqual, 5.0};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: TimeDiff, FreezeExpr, CompareOp
 */
struct TimeBoundExpr {
  TimeDiff diff;
  CompareOp op;
  double value;

  TimeBoundExpr(TimeDiff d, CompareOp o, double v) : diff(std::move(d)), op(o), value(v) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL frame bound constraint.
 *
 * In STQL syntax: (f - C_FRAME) ∼ n where ∼ ∈ {<, ≤, >, ≥}
 *
 * Constrains the frame difference between a frozen frame variable and the current
 * frame. Used to express frame-based durations like "within 10 frames".
 *
 * Example:
 * @code
 * auto f = FrameVar{"f"};
 * auto diff = FrameDiff{f, C_FRAME};
 * // (f - C_FRAME) ≤ 10
 * auto bound = FrameBoundExpr{diff, CompareOp::LessEqual, 10};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: FrameDiff, FreezeExpr, CompareOp
 */
struct FrameBoundExpr {
  FrameDiff diff;
  CompareOp op;
  int64_t value;

  FrameBoundExpr(FrameDiff d, CompareOp o, int64_t v) : diff(std::move(d)), op(o), value(v) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Object Identity and Class
// =============================================================================

/**
 * @brief STQL object ID comparison.
 *
 * In STQL syntax: {id_i = id_j} or {id_i ≠ id_j}
 *
 * Compares object IDs for equality or inequality. Used to check if two object
 * variables refer to the same or different objects.
 *
 * Example:
 * @code
 * auto car1 = ObjectVar{"car1"};
 * auto car2 = ObjectVar{"car2"};
 * // Check if they're different objects
 * auto different = ObjectIdCompareExpr{car1, CompareOp::NotEqual, car2};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ObjectVar, CompareOp
 */
struct ObjectIdCompareExpr {
  ObjectVar lhs;
  CompareOp op; // Only Equal or NotEqual valid
  ObjectVar rhs;

  ObjectIdCompareExpr(ObjectVar l, CompareOp o, ObjectVar r) :
      lhs(std::move(l)), op(o), rhs(std::move(r)) {
    if (op != CompareOp::Equal && op != CompareOp::NotEqual) {
      throw std::invalid_argument("ObjectIdCompareExpr only supports == and !=");
    }
  }

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL class extraction function.
 *
 * In STQL syntax: C(id) (class function)
 *
 * Extracts the class/category of an object identified by id. Used in class
 * comparisons like C(id) = c or C(id) = C(id').
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto class_func = ClassFunc{car};  // C(car)
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ClassCompareExpr, ObjectVar
 */
struct ClassFunc {
  ObjectVar object;

  explicit ClassFunc(ObjectVar obj) : object(std::move(obj)) {}

  [[nodiscard]] auto to_string() const -> std::string { return "C(" + object.to_string() + ")"; }
};

/**
 * @brief STQL class comparison expression.
 *
 * In STQL syntax: C(id) = c or C(id) = C(id')
 *
 * Compares object class against either a class literal (int) or another
 * object's class. Only equality comparisons (= or ≠) are supported.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto vehicle_class = 1;
 * // C(car) == 1 (car is a vehicle)
 * auto compare = ClassCompareExpr{ClassFunc{car}, CompareOp::Equal, vehicle_class};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ClassFunc, ObjectVar, CompareOp
 */
struct ClassCompareExpr {
  ClassFunc lhs;
  CompareOp op;                     // Only Equal or NotEqual valid
  std::variant<int, ClassFunc> rhs; // Compare to class ID or another object's class

  ClassCompareExpr(ClassFunc l, CompareOp o, std::variant<int, ClassFunc> r) :
      lhs(std::move(l)), op(o), rhs(std::move(r)) {
    if (op != CompareOp::Equal && op != CompareOp::NotEqual) {
      throw std::invalid_argument("ClassCompareExpr only supports == and !=");
    }
  }

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Probability/Confidence
// =============================================================================

/**
 * @brief STQL probability/confidence extraction function.
 *
 * In STQL syntax: P(id) (probability function)
 *
 * Extracts the detection confidence/probability of an object identified by id.
 * Used in probability comparisons like P(id) ≥ r or P(id) ≥ r × P(id').
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto prob_func = ProbFunc{car};  // P(car)
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ProbCompareExpr, ObjectVar
 */
struct ProbFunc {
  ObjectVar object;

  explicit ProbFunc(ObjectVar obj) : object(std::move(obj)) {}

  [[nodiscard]] auto to_string() const -> std::string { return "P(" + object.to_string() + ")"; }
};

/**
 * @brief STQL probability comparison expression.
 *
 * In STQL syntax: P(id) ∼ r or P(id) ∼ r × P(id')
 *
 * Compares object confidence against either a constant threshold (double) or
 * a scaled version of another object's confidence.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * // P(car) >= 0.8 (car detection confidence at least 80%)
 * auto compare = ProbCompareExpr{ProbFunc{car}, CompareOp::GreaterEqual, 0.8};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ProbFunc, ObjectVar, CompareOp
 */
struct ProbCompareExpr {
  ProbFunc lhs;
  CompareOp op;
  std::variant<double, ProbFunc> rhs;

  ProbCompareExpr(ProbFunc l, CompareOp o, std::variant<double, ProbFunc> r) :
      lhs(std::move(l)), op(o), rhs(std::move(r)) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Spatial Distance and Position Functions
// =============================================================================

/**
 * @brief STQL Euclidean distance function.
 *
 * In STQL syntax: ED(id_i, CRT, id_j, CRT) ≥ r (Euclidean distance)
 *
 * Computes the Euclidean distance between reference points of two objects'
 * bounding boxes. Used in spatial constraints to enforce minimum/maximum
 * distances between objects.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto pedestrian = ObjectVar{"pedestrian"};
 * auto dist = EuclideanDistFunc{
 *     RefPoint{car, CoordRefPoint::Center},
 *     RefPoint{pedestrian, CoordRefPoint::Center}
 * };  // ED(car.CT, pedestrian.CT)
 * // Then compare: dist >= 5.0 ensures at least 5 units distance
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: RefPoint, CoordRefPoint, DistCompareExpr
 */
struct EuclideanDistFunc {
  RefPoint from;
  RefPoint to;

  EuclideanDistFunc(RefPoint f, RefPoint t) : from(std::move(f)), to(std::move(t)) {}

  [[nodiscard]] auto to_string() const -> std::string {
    return "ED(" + from.to_string() + ", " + to.to_string() + ")";
  }
};

/**
 * @brief STQL distance comparison expression.
 *
 * In STQL syntax: ED(id_i, CRT, id_j, CRT) ∼ r
 *
 * Compares Euclidean distance against a threshold value.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto truck = ObjectVar{"truck"};
 * auto dist_func = EuclideanDistFunc{
 *     RefPoint{car, CoordRefPoint::Center},
 *     RefPoint{truck, CoordRefPoint::Center}
 * };
 * // ED(car.CT, truck.CT) >= 10.0
 * auto compare = DistCompareExpr{dist_func, CompareOp::GreaterEqual, 10.0};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: EuclideanDistFunc, CompareOp
 *
 * @note This function doesn't actually make sense for CRTs that aren't centroids. If
 * you think otherwise, contributions are welcome.
 */
struct DistCompareExpr {
  EuclideanDistFunc lhs;
  CompareOp op;
  double rhs;

  DistCompareExpr(EuclideanDistFunc l, CompareOp o, double r) : lhs(std::move(l)), op(o), rhs(r) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL lateral position function.
 *
 * In STQL syntax: Lat(id_i, CRT) (lateral position)
 *
 * Computes the lateral (perpendicular to longitudinal axis) distance
 * of the specified reference point from the lateral axis.
 *
 * Used in positional constraints like Lat(car, LM) > 0 for lane positioning.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto lat_pos = LatFunc{RefPoint{car, CoordRefPoint::Center}};  // Lat(car.CT)
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: LonFunc, RefPoint, LatLonCompareExpr
 */
struct LatFunc {
  RefPoint point;

  explicit LatFunc(RefPoint p) : point(std::move(p)) {}

  [[nodiscard]] auto to_string() const -> std::string { return "Lat(" + point.to_string() + ")"; }
};

/**
 * @brief STQL longitudinal position function.
 *
 * In STQL syntax: Lon(id_i, CRT) (longitudinal position)
 *
 * Computes the longitudinal (along the main axis) distance
 * of the specified reference point from the longitudinal axis.
 *
 * Used in positional constraints like Lon(vehicle, CT) < 100 for range limits.
 *
 * Example:
 * @code
 * auto vehicle = ObjectVar{"vehicle"};
 * auto lon_pos = LonFunc{RefPoint{vehicle, CoordRefPoint::Center}};  // Lon(vehicle.CT)
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: LatFunc, RefPoint, LatLonCompareExpr
 */
struct LonFunc {
  RefPoint point;

  explicit LonFunc(RefPoint p) : point(std::move(p)) {}

  [[nodiscard]] auto to_string() const -> std::string { return "Lon(" + point.to_string() + ")"; }
};

/**
 * @brief STQL lateral/longitudinal position comparison expression.
 *
 * In STQL syntax: Θ ∼ r or Θ ∼ r × Θ where Θ ∈ {Lat(id, CRT), Lon(id, CRT)}
 *
 * Compares lateral or longitudinal position against either a constant or
 * a scaled version of another position measurement.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto lat_func = LatFunc{RefPoint{car, CoordRefPoint::LeftMargin}};
 * // Lat(car.LM) > 2.0 (car's left edge is more than 2 units laterally)
 * auto compare = LatLonCompareExpr{lat_func, CompareOp::GreaterThan, 2.0};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: LatFunc, LonFunc, CompareOp
 */
struct LatLonCompareExpr {
  std::variant<LatFunc, LonFunc> lhs;
  CompareOp op;
  std::variant<double, LatFunc, LonFunc> rhs;

  LatLonCompareExpr(
      std::variant<LatFunc, LonFunc> l,
      CompareOp o,
      std::variant<double, LatFunc, LonFunc> r) :
      lhs(std::move(l)), op(o), rhs(std::move(r)) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Spatial Expressions
// =============================================================================

/**
 * @brief STQL empty spatial set.
 *
 * In STQL syntax: ∅ (empty spatial set)
 *
 * Represents the empty spatial set in spatial expressions. Used as identity
 * element for spatial union (∅ ⊔ Ω = Ω) and in spatial existence checks
 * (∃∅ evaluates to false).
 *
 * Example:
 * @code
 * auto empty = EmptySetExpr{};  // ∅
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: UniverseSetExpr, SpatialExistsExpr
 */
struct EmptySetExpr {
  [[nodiscard]] auto to_string() const -> std::string { return "∅"; }
};

/**
 * @brief STQL universal spatial set.
 *
 * In STQL syntax: U (universal spatial set)
 *
 * Represents the universal spatial set containing all spatial points. Used as
 * identity element for spatial intersection (U ⊓ Ω = Ω) and in complement
 * operations (Ω̅ = U \ Ω).
 *
 * Example:
 * @code
 * auto universe = UniverseSetExpr{};  // U
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: EmptySetExpr, SpatialComplementExpr
 */
struct UniverseSetExpr {
  [[nodiscard]] auto to_string() const -> std::string { return "U"; }
};

/**
 * @brief STQL bounding box extraction.
 *
 * In STQL syntax: BB(id_i) (bounding box extraction)
 *
 * Extracts the bounding box spatial region of an object identified by id_i.
 * Primary building block for spatial expressions and set operations.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto car_bbox = BBoxExpr{car};  // BB(car)
 * // This extracts the spatial region occupied by the car object
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: ObjectVar, SpatialUnionExpr, SpatialIntersectExpr
 */
struct BBoxExpr {
  ObjectVar object;

  explicit BBoxExpr(ObjectVar obj) : object(std::move(obj)) {}

  [[nodiscard]] auto to_string() const -> std::string { return "BB(" + object.to_string() + ")"; }
};

/**
 * @brief STQL spatial complement operation.
 *
 * In STQL syntax: Ω̅ (spatial complement)
 *
 * Computes the spatial complement of a spatial expression. Ω̅ = U \ Ω
 * (universal set minus Ω).
 *
 * Example:
 * @code
 * auto obstacle = ObjectVar{"obstacle"};
 * auto obstacle_bbox = BBoxExpr{obstacle};
 * auto free_space = SpatialComplementExpr{obstacle_bbox};  // BB̅(obstacle)
 * // Represents all space NOT occupied by the obstacle
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialExpr, UniverseSetExpr
 */
struct SpatialComplementExpr {
  Box<SpatialExpr> arg;

  explicit SpatialComplementExpr(SpatialExpr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL spatial union operation.
 *
 * In STQL syntax: Ω₁ ⊔ Ω₂ ⊔ ... ⊔ Ωₙ (spatial union)
 *
 * Computes the spatial union of multiple spatial expressions. Fundamental
 * spatial set operation combining spatial regions.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto trailer = ObjectVar{"trailer"};
 * auto combined = SpatialUnionExpr{{BBoxExpr{car}, BBoxExpr{trailer}}};
 * // BB(car) ⊔ BB(trailer) - combines car and trailer regions
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialIntersectExpr, BBoxExpr, EmptySetExpr
 */
struct SpatialUnionExpr {
  std::vector<SpatialExpr> args;

  explicit SpatialUnionExpr(std::vector<SpatialExpr> exprs);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL spatial intersection operation.
 *
 * In STQL syntax: Ω₁ ⊓ Ω₂ ⊓ ... ⊓ Ωₙ (spatial intersection, derived)
 *
 * Computes the spatial intersection of multiple spatial expressions.
 * Derived operation via De Morgan's law: Ω₁ ⊓ Ω₂ = (Ω̅₁ ⊔ Ω̅₂)̅
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto lane = ObjectVar{"lane"};
 * auto overlap = SpatialIntersectExpr{{BBoxExpr{car}, BBoxExpr{lane}}};
 * // BB(car) ⊓ BB(lane) - finds overlap between car and lane
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialUnionExpr, SpatialComplementExpr, SpatialExistsExpr
 */
struct SpatialIntersectExpr {
  std::vector<SpatialExpr> args;

  explicit SpatialIntersectExpr(std::vector<SpatialExpr> exprs);

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Spatial Quantifiers and Predicates
// =============================================================================

/**
 * @brief STQL spatial area function.
 *
 * In STQL syntax: Area(Ω) (area function)
 *
 * Computes the area of a spatial expression Ω (spatial set). Used in
 * area-based constraints like Area(Ω) ≥ r or Area(Ω₁) ≥ r × Area(Ω₂).
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto truck = ObjectVar{"truck"};
 * auto combined = SpatialUnionExpr{{BBoxExpr{car}, BBoxExpr{truck}}};
 * auto total_area = AreaFunc{combined};  // Area(BB(car) ⊔ BB(truck))
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialExpr, AreaCompareExpr
 */
struct AreaFunc {
  Box<SpatialExpr> spatial_expr;

  explicit AreaFunc(SpatialExpr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL area comparison expression.
 *
 * In STQL syntax: Area(Ω) ∼ r or Area(Ω₁) ∼ r × Area(Ω₂)
 *
 * Compares spatial area against either a constant threshold or a scaled
 * version of another area measurement.
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto area_func = AreaFunc{BBoxExpr{car}};
 * // Area(BB(car)) >= 50.0 (car bounding box area at least 50 square units)
 * auto compare = AreaCompareExpr{area_func, CompareOp::GreaterEqual, 50.0};
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: AreaFunc, CompareOp
 */
struct AreaCompareExpr {
  AreaFunc lhs;
  CompareOp op;
  std::variant<double, AreaFunc> rhs;

  AreaCompareExpr(AreaFunc l, CompareOp o, std::variant<double, AreaFunc> r) :
      lhs(std::move(l)), op(o), rhs(std::move(r)) {}

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL spatial existence quantifier.
 *
 * In STQL syntax: ∃Ω (spatial existence)
 *
 * Checks if a spatial expression Ω results in a non-empty spatial region.
 * ∃Ω evaluates to true if Ω ≠ ∅ (the spatial set is non-empty).
 *
 * Example:
 * @code
 * auto car = ObjectVar{"car"};
 * auto lane = ObjectVar{"lane"};
 * auto overlap = SpatialIntersectExpr{{BBoxExpr{car}, BBoxExpr{lane}}};
 * auto has_overlap = SpatialExistsExpr{overlap};  // ∃(BB(car) ⊓ BB(lane))
 * // True if car overlaps with lane
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialForallExpr, SpatialExpr, EmptySetExpr
 */
struct SpatialExistsExpr {
  Box<SpatialExpr> arg;

  explicit SpatialExistsExpr(SpatialExpr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

/**
 * @brief STQL spatial universal quantifier.
 *
 * In STQL syntax: ∀Ω (spatial universal, derived)
 *
 * Universal spatial quantification (derived operation). ∀Ω ≡ ¬∃(Ω̅)
 * (negation of spatial existence of complement).
 *
 * Example:
 * @code
 * auto free_space = some spatial expression ;
 * auto all_free = SpatialForallExpr{free_space};  // ∀(free_space)
 * @endcode
 *
 * @see STQL Definition in paper Section 2 (Definition 1)
 * @see Related: SpatialExistsExpr, SpatialComplementExpr
 */
struct SpatialForallExpr {
  Box<SpatialExpr> arg;

  explicit SpatialForallExpr(SpatialExpr e);

  [[nodiscard]] auto to_string() const -> std::string;
};

// =============================================================================
// Constructor Implementations
// =============================================================================
// Note: Constructors are defined here (after all types are complete) to avoid
// circular dependencies with the Expr and SpatialExpr variant types.

// Boolean and propositional expressions
inline NotExpr::NotExpr(Expr e) : arg(std::move(e)) {}

inline AndExpr::AndExpr(std::vector<Expr> exprs) : args(std::move(exprs)) {
  if (args.size() < 2) { throw std::invalid_argument("AndExpr requires at least 2 arguments"); }
}

inline OrExpr::OrExpr(std::vector<Expr> exprs) : args(std::move(exprs)) {
  if (args.size() < 2) { throw std::invalid_argument("OrExpr requires at least 2 arguments"); }
}

// Temporal operators
inline NextExpr::NextExpr(Expr e, size_t n) : arg(std::move(e)), steps(n) {
  if (steps < 1) { throw std::invalid_argument("NextExpr requires at least 1 step"); }
}

inline PreviousExpr::PreviousExpr(Expr e, size_t n) : arg(std::move(e)), steps(n) {
  if (steps < 1) { throw std::invalid_argument("PreviousExpr requires at least 1 step"); }
}

inline AlwaysExpr::AlwaysExpr(Expr e) : arg(std::move(e)) {}

inline HoldsExpr::HoldsExpr(Expr e) : arg(std::move(e)) {}

inline EventuallyExpr::EventuallyExpr(Expr e) : arg(std::move(e)) {}

inline SometimesExpr::SometimesExpr(Expr e) : arg(std::move(e)) {}

inline UntilExpr::UntilExpr(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}

inline SinceExpr::SinceExpr(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}

inline ReleaseExpr::ReleaseExpr(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}

inline BackToExpr::BackToExpr(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}

// Quantifiers
inline ExistsExpr::ExistsExpr(std::vector<ObjectVar> vars, Expr e) :
    variables(std::move(vars)), body(std::move(e)) {
  if (variables.empty()) {
    throw std::invalid_argument("ExistsExpr requires at least one variable");
  }
}

inline ForallExpr::ForallExpr(std::vector<ObjectVar> vars, Expr e) :
    variables(std::move(vars)), body(std::move(e)) {
  if (variables.empty()) {
    throw std::invalid_argument("ForallExpr requires at least one variable");
  }
}

inline FreezeExpr::FreezeExpr(
    const std::optional<TimeVar>& t,
    const std::optional<FrameVar>& f,
    Expr e) :
    time_var(t), frame_var(f), body(std::move(e)) {
  if (!t && !f) {
    throw std::invalid_argument("FreezeExpr requires at least time or frame variable");
  }
}
inline FreezeExpr::FreezeExpr(const std::optional<FrameVar>& f, Expr e) :
    FreezeExpr{std::nullopt, f, std::move(e)} {};

inline FreezeExpr::FreezeExpr(const std::optional<TimeVar>& t, Expr e) :
    FreezeExpr{t, std::nullopt, std::move(e)} {};

// Spatial expressions
inline SpatialComplementExpr::SpatialComplementExpr(SpatialExpr e) : arg(std::move(e)) {}

inline SpatialUnionExpr::SpatialUnionExpr(std::vector<SpatialExpr> exprs) : args(std::move(exprs)) {
  if (args.size() < 2) {
    throw std::invalid_argument("SpatialUnionExpr requires at least 2 arguments");
  }
}

inline SpatialIntersectExpr::SpatialIntersectExpr(std::vector<SpatialExpr> exprs) :
    args(std::move(exprs)) {
  if (args.size() < 2) {
    throw std::invalid_argument("SpatialIntersectExpr requires at least 2 arguments");
  }
}

// Spatial predicates
inline AreaFunc::AreaFunc(SpatialExpr e) : spatial_expr(std::move(e)) {}

inline SpatialExistsExpr::SpatialExistsExpr(SpatialExpr e) : arg(std::move(e)) {}

inline SpatialForallExpr::SpatialForallExpr(SpatialExpr e) : arg(std::move(e)) {}

// =============================================================================
// Factory Functions for Ergonomic Construction
// =============================================================================

// Boolean constant factories
inline auto make_true() -> Expr { return ConstExpr{true}; }
inline auto make_false() -> Expr { return ConstExpr{false}; }

// Temporal operator factories
inline auto next(Expr e, size_t n = 1) -> Expr { return NextExpr{std::move(e), n}; }
inline auto previous(Expr e, size_t n = 1) -> Expr { return PreviousExpr{std::move(e), n}; }
inline auto always(Expr e) -> Expr { return AlwaysExpr{std::move(e)}; }
inline auto eventually(Expr e) -> Expr { return EventuallyExpr{std::move(e)}; }
inline auto until(Expr lhs, Expr rhs) -> Expr { return UntilExpr{std::move(lhs), std::move(rhs)}; }
inline auto since(Expr lhs, Expr rhs) -> Expr { return SinceExpr{std::move(lhs), std::move(rhs)}; }
inline auto release(Expr lhs, Expr rhs) -> Expr {
  return ReleaseExpr{std::move(lhs), std::move(rhs)};
}
inline auto backto(Expr lhs, Expr rhs) -> Expr {
  return BackToExpr{std::move(lhs), std::move(rhs)};
}

// Past-time aliases
inline auto holds(Expr e) -> Expr { return HoldsExpr{std::move(e)}; }
inline auto sometimes(Expr e) -> Expr { return SometimesExpr(std::move(e)); }

// Quantifier factories
inline auto exists(std::vector<ObjectVar> vars, Expr body) -> Expr {
  return ExistsExpr{std::move(vars), std::move(body)};
}
inline auto forall(std::vector<ObjectVar> vars, Expr body) -> Expr {
  return ForallExpr{std::move(vars), std::move(body)};
}

// Freeze factories
inline auto freeze(TimeVar t, Expr e) -> Expr { return FreezeExpr{t, std::nullopt, std::move(e)}; }
inline auto freeze(FrameVar f, Expr e) -> Expr { return FreezeExpr{std::nullopt, f, std::move(e)}; }
inline auto freeze(TimeVar t, FrameVar f, Expr e) -> Expr { return FreezeExpr{t, f, std::move(e)}; }

// Spatial expression factories
inline auto bbox(ObjectVar obj) -> SpatialExpr { return BBoxExpr{std::move(obj)}; }
inline auto empty_set() -> SpatialExpr { return EmptySetExpr{}; }
inline auto universe() -> SpatialExpr { return UniverseSetExpr{}; }
inline auto spatial_complement(SpatialExpr e) -> SpatialExpr {
  return SpatialComplementExpr{std::move(e)};
}
inline auto spatial_union(std::vector<SpatialExpr> args) -> SpatialExpr {
  return SpatialUnionExpr{std::move(args)};
}
inline auto spatial_intersect(std::vector<SpatialExpr> args) -> SpatialExpr {
  return SpatialIntersectExpr{std::move(args)};
}

// Spatial quantifier factories
inline auto spatial_exists(SpatialExpr e) -> Expr { return SpatialExistsExpr{std::move(e)}; }
inline auto spatial_forall(SpatialExpr e) -> Expr { return SpatialForallExpr{std::move(e)}; }

// Perception primitive helper factories
// ============================================================================
// These helpers make it easier to construct perception-based conditions
// for use within quantifiers and temporal operators.

/**
 * @brief Check if an object is of a specific class.
 *
 * In STQL syntax: C(id) = class_id
 *
 * Example:
 * @code
 * auto obj = ObjectVar{"obj"};
 * auto is_car = is_class(obj, 1);  // Check if obj is a car
 * auto has_car = exists({obj}, is_car);  // At least one car exists
 * @endcode
 */
inline auto is_class(const ObjectVar& obj, int class_id) -> Expr {
  return ClassCompareExpr{ClassFunc{obj}, CompareOp::Equal, class_id};
}

/**
 * @brief Check if an object is NOT of a specific class.
 *
 * In STQL syntax: C(id) ≠ class_id
 *
 * Example:
 * @code
 * auto obj = ObjectVar{"obj"};
 * auto not_car = is_not_class(obj, 1);
 * auto has_non_car = exists({obj}, not_car);
 * @endcode
 */
inline auto is_not_class(const ObjectVar& obj, int class_id) -> Expr {
  return ClassCompareExpr{ClassFunc{obj}, CompareOp::NotEqual, class_id};
}

/**
 * @brief Check if an object has high confidence/probability.
 *
 * In STQL syntax: P(id) ≥ threshold
 *
 * Example:
 * @code
 * auto obj = ObjectVar{"obj"};
 * auto high_conf = high_confidence(obj, 0.9);
 * auto has_confident_obj = exists({obj}, high_conf);
 * @endcode
 */
inline auto high_confidence(const ObjectVar& obj, double threshold = 0.8) -> Expr {
  return ProbCompareExpr{ProbFunc{obj}, CompareOp::GreaterEqual, threshold};
}

/**
 * @brief Check if an object has low confidence/probability.
 *
 * In STQL syntax: P(id) < threshold
 *
 * Example:
 * @code
 * auto obj = ObjectVar{"obj"};
 * auto low_conf = low_confidence(obj, 0.5);
 * @endcode
 */
inline auto low_confidence(const ObjectVar& obj, double threshold = 0.5) -> Expr {
  return ProbCompareExpr{ProbFunc{obj}, CompareOp::LessThan, threshold};
}

// =============================================================================
// Operator Overloads for Convenient Formula Construction
// =============================================================================

// Negation operator
inline auto operator~(Expr e) -> Expr { return NotExpr{std::move(e)}; }

// Logical operators (note: these create binary And/Or with 2 args)
inline auto operator&(Expr lhs, Expr rhs) -> Expr {
  return AndExpr{{std::move(lhs), std::move(rhs)}};
}
inline auto operator|(Expr lhs, Expr rhs) -> Expr {
  return OrExpr{{std::move(lhs), std::move(rhs)}};
}

// Time/frame difference operators
inline auto operator-(TimeVar lhs, TimeVar rhs) -> TimeDiff {
  return TimeDiff{std::move(lhs), std::move(rhs)};
}
inline auto operator-(FrameVar lhs, FrameVar rhs) -> FrameDiff {
  return FrameDiff{std::move(lhs), std::move(rhs)};
}

// Comparison expressions
inline auto operator<(const TimeDiff& lhs, const double& rhs) -> Expr {
  return TimeBoundExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const TimeDiff& lhs, const double& rhs) -> Expr {
  return TimeBoundExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const TimeDiff& lhs, const double& rhs) -> Expr {
  return TimeBoundExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const TimeDiff& lhs, const double& rhs) -> Expr {
  return TimeBoundExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const FrameDiff& lhs, const int64_t& rhs) -> Expr {
  return FrameBoundExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const FrameDiff& lhs, const int64_t& rhs) -> Expr {
  return FrameBoundExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const FrameDiff& lhs, const int64_t& rhs) -> Expr {
  return FrameBoundExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const FrameDiff& lhs, const int64_t& rhs) -> Expr {
  return FrameBoundExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const ProbFunc& lhs, const double& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const ProbFunc& lhs, const double& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const ProbFunc& lhs, const double& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const ProbFunc& lhs, const double& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const ProbFunc& lhs, const ProbFunc& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const ProbFunc& lhs, const ProbFunc& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const ProbFunc& lhs, const ProbFunc& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const ProbFunc& lhs, const ProbFunc& rhs) -> Expr {
  return ProbCompareExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const EuclideanDistFunc& lhs, const double& rhs) -> Expr {
  return DistCompareExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const EuclideanDistFunc& lhs, const double& rhs) -> Expr {
  return DistCompareExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const EuclideanDistFunc& lhs, const double& rhs) -> Expr {
  return DistCompareExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const EuclideanDistFunc& lhs, const double& rhs) -> Expr {
  return DistCompareExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const AreaFunc& lhs, const double& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const AreaFunc& lhs, const double& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const AreaFunc& lhs, const double& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const AreaFunc& lhs, const double& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::GreaterEqual, rhs};
}

inline auto operator<(const AreaFunc& lhs, const AreaFunc& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::LessThan, rhs};
}
inline auto operator<=(const AreaFunc& lhs, const AreaFunc& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::LessEqual, rhs};
}
inline auto operator>(const AreaFunc& lhs, const AreaFunc& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::GreaterThan, rhs};
}
inline auto operator>=(const AreaFunc& lhs, const AreaFunc& rhs) -> Expr {
  return AreaCompareExpr{lhs, CompareOp::GreaterEqual, rhs};
}

// =============================================================================
// to_string() Implementations
// =============================================================================

// Note: Implementations for types that need to recursively visit children

inline auto NotExpr::to_string() const -> std::string {
  return "¬(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto AndExpr::to_string() const -> std::string {
  std::string result = "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) result += " ∧ ";
    result += std::visit([](const auto& e) { return e.to_string(); }, args[i]);
  }
  return result + ")";
}

inline auto OrExpr::to_string() const -> std::string {
  std::string result = "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) result += " ∨ ";
    result += std::visit([](const auto& e) { return e.to_string(); }, args[i]);
  }
  return result + ")";
}

inline auto NextExpr::to_string() const -> std::string {
  std::string op = steps == 1 ? "○" : "○^" + std::to_string(steps);
  return op + "(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto PreviousExpr::to_string() const -> std::string {
  std::string op = steps == 1 ? "◦" : "◦^" + std::to_string(steps);
  return op + "(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto AlwaysExpr::to_string() const -> std::string {
  return "□(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto EventuallyExpr::to_string() const -> std::string {
  return "◇(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto HoldsExpr::to_string() const -> std::string {
  return "holds(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto SometimesExpr::to_string() const -> std::string {
  return "sometimes(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto UntilExpr::to_string() const -> std::string {
  return "(" + std::visit([](const auto& e) { return e.to_string(); }, *lhs) + " U " +
         std::visit([](const auto& e) { return e.to_string(); }, *rhs) + ")";
}

inline auto SinceExpr::to_string() const -> std::string {
  return "(" + std::visit([](const auto& e) { return e.to_string(); }, *lhs) + " S " +
         std::visit([](const auto& e) { return e.to_string(); }, *rhs) + ")";
}

inline auto ReleaseExpr::to_string() const -> std::string {
  return "(" + std::visit([](const auto& e) { return e.to_string(); }, *lhs) + " R " +
         std::visit([](const auto& e) { return e.to_string(); }, *rhs) + ")";
}

inline auto BackToExpr::to_string() const -> std::string {
  return "(" + std::visit([](const auto& e) { return e.to_string(); }, *lhs) + " B " +
         std::visit([](const auto& e) { return e.to_string(); }, *rhs) + ")";
}

inline auto ExistsExpr::to_string() const -> std::string {
  std::string result = "∃{";
  for (size_t i = 0; i < variables.size(); ++i) {
    if (i > 0) result += ", ";
    result += variables[i].to_string();
  }
  result += "}@(" + std::visit([](const auto& e) { return e.to_string(); }, *body) + ")";
  return result;
}

inline auto ForallExpr::to_string() const -> std::string {
  std::string result = "∀{";
  for (size_t i = 0; i < variables.size(); ++i) {
    if (i > 0) result += ", ";
    result += variables[i].to_string();
  }
  result += "}@(" + std::visit([](const auto& e) { return e.to_string(); }, *body) + ")";
  return result;
}

inline auto FreezeExpr::to_string() const -> std::string {
  std::string result = "{";
  if (time_var) result += time_var->to_string();
  if (time_var && frame_var) result += ", ";
  if (frame_var) result += frame_var->to_string();
  result += "}.(" + std::visit([](const auto& e) { return e.to_string(); }, *body) + ")";
  return result;
}

inline auto TimeBoundExpr::to_string() const -> std::string {
  return "(" + diff.to_string() + " " + stql::to_string(op) + " " + std::to_string(value) + ")";
}

inline auto FrameBoundExpr::to_string() const -> std::string {
  return "(" + diff.to_string() + " " + stql::to_string(op) + " " + std::to_string(value) + ")";
}

inline auto ObjectIdCompareExpr::to_string() const -> std::string {
  return "{" + lhs.to_string() + " " + stql::to_string(op) + " " + rhs.to_string() + "}";
}

inline auto ClassCompareExpr::to_string() const -> std::string {
  std::string rhs_str = std::visit(
      [](const auto& r) {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, int>) {
          return std::to_string(r);
        } else {
          return r.to_string();
        }
      },
      rhs);
  return lhs.to_string() + " " + stql::to_string(op) + " " + rhs_str;
}

inline auto ProbCompareExpr::to_string() const -> std::string {
  std::string rhs_str = std::visit(
      [](const auto& r) {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, double>) {
          return std::to_string(r);
        } else {
          return r.to_string();
        }
      },
      rhs);
  return lhs.to_string() + " " + stql::to_string(op) + " " + rhs_str;
}

inline auto DistCompareExpr::to_string() const -> std::string {
  return lhs.to_string() + " " + stql::to_string(op) + " " + std::to_string(rhs);
}

inline auto LatLonCompareExpr::to_string() const -> std::string {
  std::string lhs_str = std::visit([](const auto& l) { return l.to_string(); }, lhs);
  std::string rhs_str = std::visit(
      [](const auto& r) {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, double>) {
          return std::to_string(r);
        } else {
          return r.to_string();
        }
      },
      rhs);
  return lhs_str + " " + stql::to_string(op) + " " + rhs_str;
}

inline auto SpatialComplementExpr::to_string() const -> std::string {
  return "¬(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto SpatialUnionExpr::to_string() const -> std::string {
  std::string result = "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) result += " ⊔ ";
    result += std::visit([](const auto& e) { return e.to_string(); }, args[i]);
  }
  return result + ")";
}

inline auto SpatialIntersectExpr::to_string() const -> std::string {
  std::string result = "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) result += " ⊓ ";
    result += std::visit([](const auto& e) { return e.to_string(); }, args[i]);
  }
  return result + ")";
}

inline auto AreaFunc::to_string() const -> std::string {
  return "Area(" + std::visit([](const auto& e) { return e.to_string(); }, *spatial_expr) + ")";
}

inline auto AreaCompareExpr::to_string() const -> std::string {
  std::string rhs_str = std::visit(
      [](const auto& r) {
        if constexpr (std::is_same_v<std::decay_t<decltype(r)>, double>) {
          return std::to_string(r);
        } else {
          return r.to_string();
        }
      },
      rhs);
  return lhs.to_string() + " " + stql::to_string(op) + " " + rhs_str;
}

inline auto SpatialExistsExpr::to_string() const -> std::string {
  return "∃(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

inline auto SpatialForallExpr::to_string() const -> std::string {
  return "∀(" + std::visit([](const auto& e) { return e.to_string(); }, *arg) + ")";
}

} // namespace percemon::stql

#endif // PERCEMON_STQL_HPP
