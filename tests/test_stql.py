"""
Test suite for STQL formula construction and bindings.

Tests cover:
- Enum bindings (CoordRefPoint, CompareOp)
- Variable types (TimeVar, FrameVar, ObjectVar)
- Constants (C_TIME, C_FRAME)
- Expression construction
- Factory functions
- Operator overloads
- Perception helpers
"""

from __future__ import annotations

import percemon


class TestEnums:
    """Tests for enum bindings."""

    def test_coord_ref_point_enum(self) -> None:
        """Test CoordRefPoint enum values."""
        assert hasattr(percemon, "CoordRefPoint")
        assert hasattr(percemon.CoordRefPoint, "LeftMargin")
        assert hasattr(percemon.CoordRefPoint, "RightMargin")
        assert hasattr(percemon.CoordRefPoint, "TopMargin")
        assert hasattr(percemon.CoordRefPoint, "BottomMargin")
        assert hasattr(percemon.CoordRefPoint, "Center")

    def test_compare_op_enum(self) -> None:
        """Test CompareOp enum values."""
        assert hasattr(percemon, "CompareOp")
        assert hasattr(percemon.CompareOp, "LessThan")
        assert hasattr(percemon.CompareOp, "LessEqual")
        assert hasattr(percemon.CompareOp, "GreaterThan")
        assert hasattr(percemon.CompareOp, "GreaterEqual")
        assert hasattr(percemon.CompareOp, "Equal")
        assert hasattr(percemon.CompareOp, "NotEqual")


class TestVariables:
    """Tests for variable types."""

    def test_timevar_construction(self, time_var: percemon.TimeVar) -> None:
        """Test TimeVar construction."""
        assert time_var.name == "x"
        assert str(time_var) == "x"

    def test_framevar_construction(self, frame_var: percemon.FrameVar) -> None:
        """Test FrameVar construction."""
        assert frame_var.name == "f"
        assert str(frame_var) == "f"

    def test_objectvar_construction(self, object_var: percemon.ObjectVar) -> None:
        """Test ObjectVar construction."""
        assert object_var.name == "obj"
        assert str(object_var) == "obj"

    def test_c_time_constant(self) -> None:
        """Test C_TIME constant."""
        c_time = percemon.C_TIME()
        assert str(c_time) == "C_TIME"

    def test_c_frame_constant(self) -> None:
        """Test C_FRAME constant."""
        c_frame = percemon.C_FRAME()
        assert str(c_frame) == "C_FRAME"


class TestDifferences:
    """Tests for difference types."""

    def test_time_diff(self, time_var: percemon.TimeVar) -> None:
        """Test TimeDiff construction."""
        c_time = percemon.C_TIME()
        diff = time_var - c_time
        assert diff.lhs.name == "x"
        assert diff.rhs.name == "C_TIME"

    def test_frame_diff(self, frame_var) -> None:
        """Test FrameDiff construction."""
        c_frame = percemon.C_FRAME()
        diff = frame_var - c_frame
        assert diff.lhs.name == "f"
        assert diff.rhs.name == "C_FRAME"


class TestRefPoint:
    """Tests for reference points."""

    def test_ref_point_construction(self, object_var) -> None:
        """Test RefPoint construction."""
        ref = percemon.RefPoint(object_var, percemon.CoordRefPoint.Center)
        assert ref.crt == percemon.CoordRefPoint.Center


class TestBasicExpressions:
    """Tests for basic logical expressions."""

    def test_make_true(self, true_formula) -> None:
        """Test make_true() factory."""
        assert str(true_formula) == "⊤"

    def test_make_false(self, false_formula) -> None:
        """Test make_false() factory."""
        assert str(false_formula) == "⊥"

    def test_const_expr_true(self) -> None:
        """Test ConstExpr for true."""
        expr = percemon.ConstExpr(True)
        assert expr.value is True

    def test_const_expr_false(self) -> None:
        """Test ConstExpr for false."""
        expr = percemon.ConstExpr(False)
        assert expr.value is False


class TestLogicalOperators:
    """Tests for logical operators."""

    def test_not_operator(self, true_formula) -> None:
        """Test NOT operator (~)."""
        not_true = ~true_formula
        assert "¬" in str(not_true)

    def test_and_operator(self, true_formula, false_formula) -> None:
        """Test AND operator (&)."""
        and_expr = true_formula & false_formula
        assert "∧" in str(and_expr)

    def test_or_operator(self, true_formula, false_formula) -> None:
        """Test OR operator (|)."""
        or_expr = true_formula | false_formula
        assert "∨" in str(or_expr)

    def test_not_expr_class(self, true_formula) -> None:
        """Test NotExpr constructor."""
        not_expr = percemon.NotExpr(true_formula)
        assert "¬" in str(not_expr)

    def test_and_expr_class(self, true_formula, false_formula) -> None:
        """Test AndExpr constructor."""
        and_expr = percemon.AndExpr([true_formula, false_formula])
        assert "∧" in str(and_expr)

    def test_or_expr_class(self, true_formula, false_formula) -> None:
        """Test OrExpr constructor."""
        or_expr = percemon.OrExpr([true_formula, false_formula])
        assert "∨" in str(or_expr)


class TestTemporalOperators:
    """Tests for temporal operators."""

    def test_next_operator(self, true_formula) -> None:
        """Test next() factory."""
        next_expr = percemon.next(true_formula)
        assert "○" in str(next_expr)

    def test_next_with_steps(self, true_formula) -> None:
        """Test next() with step count."""
        next3_expr = percemon.next(true_formula, 3)
        assert "○" in str(next3_expr)

    def test_previous_operator(self, true_formula) -> None:
        """Test previous() factory."""
        prev_expr = percemon.previous(true_formula)
        assert "◦" in str(prev_expr)

    def test_always_operator(self, true_formula) -> None:
        """Test always() factory."""
        always_expr = percemon.always(true_formula)
        assert "□" in str(always_expr)

    def test_eventually_operator(self, true_formula) -> None:
        """Test eventually() factory."""
        eventually_expr = percemon.eventually(true_formula)
        assert "◇" in str(eventually_expr)

    def test_until_operator(self, true_formula, false_formula) -> None:
        """Test until() factory."""
        until_expr = percemon.until(true_formula, false_formula)
        assert "U" in str(until_expr)

    def test_since_operator(self, true_formula, false_formula) -> None:
        """Test since() factory."""
        since_expr = percemon.since(true_formula, false_formula)
        assert "S" in str(since_expr)


class TestQuantifiers:
    """Tests for quantifiers."""

    def test_exists_quantifier(self, car_var, true_formula) -> None:
        """Test exists() factory."""
        exists_expr = percemon.exists([car_var], true_formula)
        assert "∃" in str(exists_expr)
        assert "car" in str(exists_expr)

    def test_forall_quantifier(self, car_var, true_formula) -> None:
        """Test forall() factory."""
        forall_expr = percemon.forall([car_var], true_formula)
        assert "∀" in str(forall_expr)
        assert "car" in str(forall_expr)

    def test_exists_multiple_vars(self, car_var, pedestrian_var, true_formula) -> None:
        """Test exists with multiple variables."""
        exists_expr = percemon.exists([car_var, pedestrian_var], true_formula)
        assert "∃" in str(exists_expr)
        assert "car" in str(exists_expr)
        assert "pedestrian" in str(exists_expr)


class TestFreezeExpressions:
    """Tests for freeze expressions."""

    def test_freeze_time_var(self, time_var, true_formula) -> None:
        """Test freeze with time variable."""
        frozen = percemon.freeze(time_var, true_formula)
        assert "{" in str(frozen)
        assert "x" in str(frozen)

    def test_freeze_frame_var(self, frame_var, true_formula) -> None:
        """Test freeze with frame variable."""
        frozen = percemon.freeze(frame_var, true_formula)
        assert "{" in str(frozen)
        assert "f" in str(frozen)

    def test_freeze_both_vars(self, time_var, frame_var, true_formula) -> None:
        """Test freeze with both time and frame variables."""
        frozen = percemon.freeze(time_var, frame_var, true_formula)
        assert "{" in str(frozen)
        assert "x" in str(frozen)
        assert "f" in str(frozen)


class TestBoundConstraints:
    """Tests for bound expressions."""

    def test_time_bound_expr(self, time_var) -> None:
        """Test TimeBoundExpr construction."""
        c_time = percemon.C_TIME()
        diff = time_var - c_time
        bound = percemon.TimeBoundExpr(diff, percemon.CompareOp.LessEqual, 5.0)
        assert bound.op == percemon.CompareOp.LessEqual
        assert bound.value == 5.0

    def test_frame_bound_expr(self, frame_var) -> None:
        """Test FrameBoundExpr construction."""
        c_frame = percemon.C_FRAME()
        diff = frame_var - c_frame
        bound = percemon.FrameBoundExpr(diff, percemon.CompareOp.LessEqual, 10)
        assert bound.op == percemon.CompareOp.LessEqual
        assert bound.value == 10


class TestPerceptionHelpers:
    """Tests for perception helper functions."""

    def test_is_class(self, car_var) -> None:
        """Test is_class() helper."""
        expr = percemon.is_class(car_var, 1)
        assert "C(" in str(expr)

    def test_is_not_class(self, car_var) -> None:
        """Test is_not_class() helper."""
        expr = percemon.is_not_class(car_var, 1)
        assert "C(" in str(expr)

    def test_high_confidence(self, car_var) -> None:
        """Test high_confidence() helper."""
        expr = percemon.high_confidence(car_var, 0.8)
        assert "P(" in str(expr)

    def test_low_confidence(self, car_var) -> None:
        """Test low_confidence() helper."""
        expr = percemon.low_confidence(car_var, 0.5)
        assert "P(" in str(expr)


class TestSpatialExpressions:
    """Tests for spatial expressions."""

    def test_empty_set(self) -> None:
        """Test EmptySetExpr."""
        empty = percemon.EmptySetExpr()
        assert "∅" in str(empty)

    def test_universe_set(self) -> None:
        """Test UniverseSetExpr."""
        universe = percemon.UniverseSetExpr()
        assert "U" in str(universe)

    def test_bbox_expr(self, car_var) -> None:
        """Test BBoxExpr."""
        bbox_expr = percemon.BBoxExpr(car_var)
        assert "BB(" in str(bbox_expr)
        assert "car" in str(bbox_expr)

    def test_spatial_complement(self, car_var) -> None:
        """Test SpatialComplementExpr."""
        bbox_expr = percemon.BBoxExpr(car_var)
        complement = percemon.SpatialComplementExpr(bbox_expr)
        assert "¬" in str(complement)

    def test_spatial_union(self, car_var, pedestrian_var) -> None:
        """Test SpatialUnionExpr."""
        bbox1 = percemon.BBoxExpr(car_var)
        bbox2 = percemon.BBoxExpr(pedestrian_var)
        union = percemon.SpatialUnionExpr([bbox1, bbox2])
        assert "⊔" in str(union)

    def test_spatial_intersect(self, car_var, pedestrian_var) -> None:
        """Test SpatialIntersectExpr."""
        bbox1 = percemon.BBoxExpr(car_var)
        bbox2 = percemon.BBoxExpr(pedestrian_var)
        intersect = percemon.SpatialIntersectExpr([bbox1, bbox2])
        assert "⊓" in str(intersect)

    def test_spatial_exists(self, car_var) -> None:
        """Test SpatialExistsExpr."""
        bbox = percemon.BBoxExpr(car_var)
        spatial_exists = percemon.SpatialExistsExpr(bbox)
        assert "∃" in str(spatial_exists)

    def test_spatial_forall(self, car_var) -> None:
        """Test SpatialForallExpr."""
        bbox = percemon.BBoxExpr(car_var)
        spatial_forall = percemon.SpatialForallExpr(bbox)
        assert "∀" in str(spatial_forall)


class TestSpatialFactories:
    """Tests for spatial factory functions."""

    def test_empty_set_factory(self) -> None:
        """Test empty_set() factory."""
        empty = percemon.empty_set()
        assert "∅" in str(empty)

    def test_universe_factory(self) -> None:
        """Test universe() factory."""
        univ = percemon.universe()
        assert "U" in str(univ)

    def test_bbox_factory(self, car_var) -> None:
        """Test bbox() factory."""
        bbox = percemon.bbox(car_var)
        assert "BB(" in str(bbox)

    def test_spatial_union_factory(self, car_var, pedestrian_var) -> None:
        """Test spatial_union() factory."""
        bbox1 = percemon.bbox(car_var)
        bbox2 = percemon.bbox(pedestrian_var)
        union = percemon.spatial_union([bbox1, bbox2])
        assert "⊔" in str(union)

    def test_spatial_intersect_factory(self, car_var, pedestrian_var) -> None:
        """Test spatial_intersect() factory."""
        bbox1 = percemon.bbox(car_var)
        bbox2 = percemon.bbox(pedestrian_var)
        intersect = percemon.spatial_intersect([bbox1, bbox2])
        assert "⊓" in str(intersect)

    def test_spatial_complement_factory(self, car_var) -> None:
        """Test spatial_complement() factory."""
        bbox = percemon.bbox(car_var)
        complement = percemon.spatial_complement(bbox)
        assert "¬" in str(complement)

    def test_spatial_exists_factory(self, car_var) -> None:
        """Test spatial_exists() factory."""
        bbox = percemon.bbox(car_var)
        spatial_exists = percemon.spatial_exists(bbox)
        assert "∃" in str(spatial_exists)

    def test_spatial_forall_factory(self, car_var) -> None:
        """Test spatial_forall() factory."""
        bbox = percemon.bbox(car_var)
        spatial_forall = percemon.spatial_forall(bbox)
        assert "∀" in str(spatial_forall)


class TestComplexFormulas:
    """Tests for complex formula construction."""

    def test_complex_temporal_chain(self, car_var) -> None:
        """Test complex nested temporal operators."""
        formula = percemon.eventually(
            percemon.exists([car_var], percemon.is_class(car_var, 1) & percemon.high_confidence(car_var, 0.8))
        )
        assert "∃" in str(formula)
        assert "∧" in str(formula)
        assert "◇" in str(formula)

    def test_formula_with_freeze(self, time_var, car_var) -> None:
        """Test formula with freeze quantifier."""
        formula = percemon.freeze(time_var, percemon.eventually(percemon.exists([car_var], percemon.is_class(car_var, 1))))
        assert "{" in str(formula)
        assert "◇" in str(formula)
        assert "∃" in str(formula)

    def test_formula_with_spatial(self, car_var) -> None:
        """Test formula with spatial quantifier."""
        bbox = percemon.bbox(car_var)
        spatial_formula = percemon.spatial_exists(bbox)
        formula = percemon.exists([car_var], spatial_formula)
        assert "∃" in str(formula)
        assert "BB(" in str(formula)
