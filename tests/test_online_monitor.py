"""
Test suite for online monitoring.

Tests cover:
- OnlineMonitor construction
- evaluate() method
- is_monitorable() predicate
- requirements() accessor
- Frame-by-frame evaluation workflows
"""

from __future__ import annotations

import percemon


class TestOnlineMonitorConstruction:
    """Tests for OnlineMonitor construction."""

    def test_monitor_construction_past_time(self) -> None:
        """Test OnlineMonitor construction with past-time formula."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        # Should not raise
        monitor = percemon.OnlineMonitor(formula, fps=30.0)
        assert monitor is not None

    def test_monitor_construction_with_fps(self) -> None:
        """Test OnlineMonitor with custom FPS."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula, fps=60.0)
        assert monitor is not None

    def test_monitor_construction_default_fps(self) -> None:
        """Test OnlineMonitor with default FPS."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula)
        assert monitor is not None


class TestOnlineMonitorMonitorability:
    """Tests for is_monitorable() method."""

    def test_past_time_formula_is_monitorable(self) -> None:
        """Test that past-time formula is monitorable."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula)
        assert monitor.is_monitorable() is True

    def test_constant_formula_is_monitorable(self) -> None:
        """Test that constant formula is monitorable."""
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)
        assert monitor.is_monitorable() is True


class TestOnlineMonitorRequirements:
    """Tests for requirements() accessor."""

    def test_requirements_accessor(self) -> None:
        """Test requirements() returns MonitoringRequirements."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula)
        reqs = monitor.requirements()
        assert isinstance(reqs, percemon.MonitoringRequirements)
        assert hasattr(reqs, "history")
        assert hasattr(reqs, "horizon")

    def test_requirements_past_time(self) -> None:
        """Test requirements for past-time formula."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula, fps=30.0)
        reqs = monitor.requirements()
        # Past-time formula should have history > 0, horizon = 0
        assert reqs.history.frames > 0
        assert reqs.horizon.frames == 0

    def test_requirements_fps_30(self) -> None:
        """Test requirements with 30 FPS."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula, fps=30.0)
        reqs = monitor.requirements()
        assert reqs.history.frames > 0

    def test_requirements_fps_60(self) -> None:
        """Test requirements with 60 FPS."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula, fps=60.0)
        reqs = monitor.requirements()
        assert reqs.history.frames > 0


class TestOnlineMonitorEvaluate:
    """Tests for evaluate() method."""

    def test_evaluate_empty_frame(self) -> None:
        """Test evaluate on empty frame."""
        percemon.ObjectVar("car")
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)

        frame = percemon.Frame()
        frame.timestamp = 0.0
        frame.frame_num = 0
        frame.size_x = 1920
        frame.size_y = 1080

        result = monitor.evaluate(frame)
        assert isinstance(result, bool)

    def test_evaluate_returns_bool(self, empty_frame) -> None:
        """Test that evaluate returns boolean."""
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(empty_frame)
        assert isinstance(result, bool)

    def test_evaluate_true_formula(self, empty_frame) -> None:
        """Test evaluate with true formula."""
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(empty_frame)
        assert result is True

    def test_evaluate_false_formula(self, empty_frame) -> None:
        """Test evaluate with false formula."""
        formula = percemon.make_false()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(empty_frame)
        assert result is False


class TestOnlineMonitorSequence:
    """Tests for frame-by-frame evaluation."""

    def test_evaluate_frame_sequence(self, frame_sequence) -> None:
        """Test evaluating a sequence of frames."""
        car = percemon.ObjectVar("car")
        formula = percemon.exists([car], percemon.is_class(car, 1))
        monitor = percemon.OnlineMonitor(formula, fps=30.0)

        results = []
        for frame in frame_sequence:
            result = monitor.evaluate(frame)
            results.append(result)

        assert len(results) == 3

    def test_evaluate_maintains_state(self, frame_sequence) -> None:
        """Test that monitor maintains state across evaluations."""
        car = percemon.ObjectVar("car")
        formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))
        monitor = percemon.OnlineMonitor(formula, fps=30.0)

        # Evaluate all frames
        for frame in frame_sequence:
            result = monitor.evaluate(frame)
            # Just verify it doesn't crash
            assert isinstance(result, bool)

    def test_consecutive_evaluations(self, frame_with_object) -> None:
        """Test consecutive frame evaluations."""
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)

        # Evaluate same frame multiple times
        result1 = monitor.evaluate(frame_with_object)
        result2 = monitor.evaluate(frame_with_object)
        result3 = monitor.evaluate(frame_with_object)

        assert result1 is True
        assert result2 is True
        assert result3 is True


class TestOnlineMonitorWithComplexFormulas:
    """Tests for complex formula monitoring."""

    def test_monitor_quantifier_formula(self, frame_sequence) -> None:
        """Test monitoring formula with quantifiers."""
        car = percemon.ObjectVar("car")
        formula = percemon.exists([car], percemon.is_class(car, 1))
        monitor = percemon.OnlineMonitor(formula)

        for frame in frame_sequence:
            result = monitor.evaluate(frame)
            assert isinstance(result, bool)

    def test_monitor_logical_formula(self, empty_frame) -> None:
        """Test monitoring formula with logical operators."""
        formula = percemon.make_true() & percemon.make_false()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(empty_frame)
        # True & False = False
        assert result is False

    def test_monitor_negation_formula(self, empty_frame) -> None:
        """Test monitoring formula with negation."""
        formula = ~percemon.make_false()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(empty_frame)
        # ~False = True
        assert result is True


class TestOnlineMonitorEdgeCases:
    """Tests for edge cases in online monitoring."""

    def test_evaluate_minimal_frame(self) -> None:
        """Test evaluate with minimal frame."""
        frame = percemon.Frame()
        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(frame)
        assert isinstance(result, bool)

    def test_evaluate_frame_with_many_objects(self) -> None:
        """Test evaluate with frame containing many objects."""
        frame = percemon.Frame()
        frame.timestamp = 0.0
        frame.frame_num = 0
        frame.size_x = 1920
        frame.size_y = 1080

        # Add multiple objects
        for i in range(10):
            obj = percemon.Object()
            obj.object_class = 1 + (i % 3)
            obj.probability = 0.8 + (i % 10) * 0.02
            bbox = percemon.BoundingBox()
            bbox.xmin = float(i * 100)
            bbox.xmax = float((i + 1) * 100)
            bbox.ymin = 0.0
            bbox.ymax = 100.0
            obj.bbox = bbox
            frame.objects[f"obj_{i}"] = obj

        formula = percemon.make_true()
        monitor = percemon.OnlineMonitor(formula)
        result = monitor.evaluate(frame)
        assert isinstance(result, bool)


class TestOnlineMonitorIntegration:
    """Integration tests for online monitoring."""

    def test_end_to_end_monitoring(self) -> None:
        """Test complete end-to-end monitoring workflow."""
        # Create formula
        car = percemon.ObjectVar("car")
        formula = percemon.exists([car], percemon.is_class(car, 1))

        # Check requirements
        reqs = percemon.compute_requirements(formula, fps=30.0)
        assert reqs.history.frames == 0
        assert reqs.horizon.frames == 0

        # Create monitor
        monitor = percemon.OnlineMonitor(formula, fps=30.0)
        assert monitor.is_monitorable() is True

        # Create frames
        frame1 = percemon.Frame()
        frame1.timestamp = 0.0
        frame1.frame_num = 0
        frame1.size_x = 1920
        frame1.size_y = 1080

        frame2 = percemon.Frame()
        frame2.timestamp = 1.0 / 30.0
        frame2.frame_num = 1
        frame2.size_x = 1920
        frame2.size_y = 1080

        # Add object to frame2
        bbox = percemon.BoundingBox()
        bbox.xmin = 100.0
        bbox.xmax = 200.0
        bbox.ymin = 50.0
        bbox.ymax = 150.0

        obj = percemon.Object()
        obj.object_class = 1
        obj.probability = 0.9
        obj.bbox = bbox

        frame2.objects["car_1"] = obj

        # Evaluate
        result1 = monitor.evaluate(frame1)
        result2 = monitor.evaluate(frame2)

        assert isinstance(result1, bool)
        assert isinstance(result2, bool)

    def test_monitoring_with_perception_helpers(self) -> None:
        """Test monitoring using perception helper functions."""
        car = percemon.ObjectVar("car")

        # Formula: exists car with class 1 and high confidence
        formula = percemon.exists([car], percemon.is_class(car, 1) & percemon.high_confidence(car, 0.8))

        monitor = percemon.OnlineMonitor(formula, fps=30.0)

        # Create frame with qualified car
        frame = percemon.Frame()
        frame.timestamp = 0.0
        frame.frame_num = 0
        frame.size_x = 1920
        frame.size_y = 1080

        bbox = percemon.BoundingBox()
        bbox.xmin = 100.0
        bbox.xmax = 200.0
        bbox.ymin = 50.0
        bbox.ymax = 150.0

        car_obj = percemon.Object()
        car_obj.object_class = 1
        car_obj.probability = 0.9  # >= 0.8
        car_obj.bbox = bbox

        frame.objects["car_1"] = car_obj

        # Should be satisfied
        result = monitor.evaluate(frame)
        assert isinstance(result, bool)
