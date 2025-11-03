"""
Test suite for monitoring requirements analysis.

Tests cover:
- History and Horizon structs
- MonitoringRequirements struct
- compute_requirements() function
- is_past_time_formula() predicate
- UNBOUNDED constant
"""

from __future__ import annotations

import percemon


class TestConstants:
    """Tests for monitoring constants."""

    def test_unbounded_constant(self) -> None:
        """Test UNBOUNDED constant exists and is large."""
        assert hasattr(percemon, "UNBOUNDED")
        unbounded = percemon.UNBOUNDED
        # Should be a very large number
        assert unbounded > 1_000_000


class TestHistoryStruct:
    """Tests for History struct."""

    def test_history_construction(self) -> None:
        """Test History construction."""
        hist = percemon.History(5)
        assert hist.frames == 5

    def test_history_is_bounded(self) -> None:
        """Test is_bounded() for bounded history."""
        hist = percemon.History(10)
        assert hist.is_bounded() is True

    def test_history_unbounded(self) -> None:
        """Test is_bounded() for unbounded history."""
        hist = percemon.History(percemon.UNBOUNDED)
        assert hist.is_bounded() is False

    def test_history_to_string(self) -> None:
        """Test History to_string()."""
        hist = percemon.History(5)
        hist_str = hist.to_string()
        assert "History" in hist_str
        assert "5" in hist_str


class TestHorizonStruct:
    """Tests for Horizon struct."""

    def test_horizon_construction(self) -> None:
        """Test Horizon construction."""
        horiz = percemon.Horizon(10)
        assert horiz.frames == 10

    def test_horizon_is_bounded(self) -> None:
        """Test is_bounded() for bounded horizon."""
        horiz = percemon.Horizon(20)
        assert horiz.is_bounded() is True

    def test_horizon_unbounded(self) -> None:
        """Test is_bounded() for unbounded horizon."""
        horiz = percemon.Horizon(percemon.UNBOUNDED)
        assert horiz.is_bounded() is False

    def test_horizon_to_string(self) -> None:
        """Test Horizon to_string()."""
        horiz = percemon.Horizon(10)
        horiz_str = horiz.to_string()
        assert "Horizon" in horiz_str
        assert "10" in horiz_str


class TestMonitoringRequirements:
    """Tests for MonitoringRequirements struct."""

    def test_requirements_construction(self) -> None:
        """Test MonitoringRequirements construction."""
        reqs = percemon.MonitoringRequirements()
        assert hasattr(reqs, "history")
        assert hasattr(reqs, "horizon")


class TestComputeRequirementsBasic:
    """Tests for compute_requirements() with basic formulas."""

    def test_constant_true_requirements(self) -> None:
        """Test requirements for constant true formula."""
        formula = percemon.make_true()
        reqs = percemon.compute_requirements(formula)
        # Constants shouldn't need history or horizon
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

    def test_constant_false_requirements(self) -> None:
        """Test requirements for constant false formula."""
        formula = percemon.make_false()
        reqs = percemon.compute_requirements(formula)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

    def test_negation_requirements(self) -> None:
        """Test requirements for negation."""
        formula = ~percemon.make_true()
        reqs = percemon.compute_requirements(formula)
        # Negation doesn't add requirements
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0


class TestComputeRequirementsLogical:
    """Tests for compute_requirements() with logical operators."""

    def test_and_requirements(self) -> None:
        """Test requirements for AND formula."""
        phi = percemon.make_true()
        psi = percemon.make_false()
        formula = phi & psi
        reqs = percemon.compute_requirements(formula)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

    def test_or_requirements(self) -> None:
        """Test requirements for OR formula."""
        phi = percemon.make_true()
        psi = percemon.make_false()
        formula = phi | psi
        reqs = percemon.compute_requirements(formula)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0


class TestComputeRequirementsTemporal:
    """Tests for compute_requirements() with temporal operators."""

    def test_next_requirements(self) -> None:
        """Test requirements for next operator."""
        formula = percemon.next(percemon.make_true())
        reqs = percemon.compute_requirements(formula)
        # One step ahead
        assert reqs.horizon.frames >= 1

    def test_next_multi_step_requirements(self) -> None:
        """Test requirements for next with multiple steps."""
        formula = percemon.next(percemon.make_true(), 5)
        reqs = percemon.compute_requirements(formula)
        # Five steps ahead
        assert reqs.horizon.frames >= 5

    def test_previous_requirements(self) -> None:
        """Test requirements for previous operator."""
        formula = percemon.previous(percemon.make_true())
        reqs = percemon.compute_requirements(formula)
        # One step back
        assert reqs.history.frames >= 1

    def test_previous_multi_step_requirements(self) -> None:
        """Test requirements for previous with multiple steps."""
        formula = percemon.previous(percemon.make_true(), 3)
        reqs = percemon.compute_requirements(formula)
        # Three steps back
        assert reqs.history.frames >= 3

    def test_always_requirements(self) -> None:
        """Test requirements for always operator."""
        formula = percemon.always(percemon.make_true())
        reqs = percemon.compute_requirements(formula)
        # Unbounded horizon
        assert reqs.horizon.frames == percemon.UNBOUNDED

    def test_eventually_requirements(self) -> None:
        """Test requirements for eventually operator."""
        formula = percemon.eventually(percemon.make_true())
        reqs = percemon.compute_requirements(formula)
        # Unbounded horizon
        assert reqs.horizon.frames == percemon.UNBOUNDED


class TestComputeRequirementsQuantifiers:
    """Tests for compute_requirements() with quantifiers."""

    def test_exists_requirements(self) -> None:
        """Test requirements for exists quantifier."""
        car = percemon.ObjectVar("car")
        formula = percemon.exists([car], percemon.is_class(car, 1))
        reqs = percemon.compute_requirements(formula)
        # Quantifiers inherit requirements from body
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

    def test_forall_requirements(self) -> None:
        """Test requirements for forall quantifier."""
        car = percemon.ObjectVar("car")
        formula = percemon.forall([car], percemon.is_class(car, 1))
        reqs = percemon.compute_requirements(formula)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0


class TestComputeRequirementsWithFPS:
    """Tests for compute_requirements() with FPS parameter."""

    def test_requirements_with_fps_30(self) -> None:
        """Test requirements computation with 30 FPS."""
        formula = percemon.make_true()
        reqs = percemon.compute_requirements(formula, fps=30.0)
        # Basic formula should still have no requirements
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

    def test_requirements_with_fps_1(self) -> None:
        """Test requirements computation with 1 FPS."""
        formula = percemon.make_true()
        reqs = percemon.compute_requirements(formula, fps=1.0)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0


class TestIsPastTimeFormula:
    """Tests for is_past_time_formula() predicate."""

    def test_constant_is_past_time(self) -> None:
        """Test that constants are past-time."""
        formula = percemon.make_true()
        assert percemon.is_past_time_formula(formula) is True

    def test_previous_is_past_time(self) -> None:
        """Test that previous operators create past-time formulas."""
        formula = percemon.previous(percemon.make_true())
        assert percemon.is_past_time_formula(formula) is True

    def test_next_is_not_past_time(self) -> None:
        """Test that next operators are not past-time."""
        formula = percemon.next(percemon.make_true())
        assert percemon.is_past_time_formula(formula) is False

    def test_eventually_is_not_past_time(self) -> None:
        """Test that eventually is not past-time."""
        formula = percemon.eventually(percemon.make_true())
        assert percemon.is_past_time_formula(formula) is False


class TestComplexRequirements:
    """Tests for complex requirement scenarios."""

    def test_quantifier_with_temporal_requirements(self) -> None:
        """Test requirements for quantifier with temporal body."""
        car = percemon.ObjectVar("car")
        body = percemon.next(percemon.is_class(car, 1))
        formula = percemon.exists([car], body)
        reqs = percemon.compute_requirements(formula)
        # Should inherit requirements from body
        assert reqs.horizon.frames >= 1

    def test_default_fps(self) -> None:
        """Test that default FPS is 1.0."""
        formula = percemon.next(percemon.make_true())
        reqs_default = percemon.compute_requirements(formula)
        reqs_explicit = percemon.compute_requirements(formula, fps=1.0)
        # Results should be identical
        assert reqs_default.history.frames == reqs_explicit.history.frames
        assert reqs_default.horizon.frames == reqs_explicit.horizon.frames
