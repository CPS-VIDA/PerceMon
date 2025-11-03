"""
Pytest configuration and shared fixtures for PerceMon Python bindings tests.

This module provides reusable fixtures and helper functions for testing the
PerceMon Python bindings.
"""

from __future__ import annotations

import percemon
import pytest

# =============================================================================
# Fixtures: Variables and Constants
# =============================================================================


@pytest.fixture
def time_var() -> percemon.TimeVar:
    """Create a test TimeVar."""
    return percemon.TimeVar("x")


@pytest.fixture
def frame_var() -> percemon.FrameVar:
    """Create a test FrameVar."""
    return percemon.FrameVar("f")


@pytest.fixture
def object_var() -> percemon.ObjectVar:
    """Create a test ObjectVar."""
    return percemon.ObjectVar("obj")


@pytest.fixture
def car_var() -> percemon.ObjectVar:
    """Create a car ObjectVar."""
    return percemon.ObjectVar("car")


@pytest.fixture
def pedestrian_var() -> percemon.ObjectVar:
    """Create a pedestrian ObjectVar."""
    return percemon.ObjectVar("pedestrian")


# =============================================================================
# Fixtures: Basic Formulas
# =============================================================================


@pytest.fixture
def true_formula() -> percemon.ConstExpr:
    """Create a boolean true formula."""
    return percemon.make_true()


@pytest.fixture
def false_formula() -> percemon.ConstExpr:
    """Create a boolean false formula."""
    return percemon.make_false()


@pytest.fixture
def simple_exists_formula(car_var: percemon.ObjectVar) -> percemon.Expr:
    """Create a simple exists formula."""
    return percemon.exists([car_var], percemon.is_class(car_var, 1))


@pytest.fixture
def high_confidence_formula(car_var: percemon.ObjectVar) -> percemon.Expr:
    """Create a formula checking high confidence."""
    return percemon.high_confidence(car_var, 0.8)


# =============================================================================
# Fixtures: Data Structures
# =============================================================================


@pytest.fixture
def sample_bounding_box() -> percemon.BoundingBox:
    """Create a sample bounding box."""
    bbox = percemon.BoundingBox()
    bbox.xmin = 100.0
    bbox.xmax = 200.0
    bbox.ymin = 50.0
    bbox.ymax = 150.0
    return bbox


@pytest.fixture
def sample_object(sample_bounding_box: percemon.BoundingBox) -> percemon.Object:
    """Create a sample object."""
    obj = percemon.Object()
    obj.object_class = 1  # Car
    obj.probability = 0.95
    obj.bbox = sample_bounding_box
    return obj


@pytest.fixture
def empty_frame():
    """Create an empty frame."""
    frame = percemon.Frame()
    frame.timestamp = 0.0
    frame.frame_num = 0
    frame.size_x = 1920
    frame.size_y = 1080
    return frame


@pytest.fixture
def frame_with_object(empty_frame, sample_object):
    """Create a frame with an object."""
    frame = empty_frame
    frame.objects["car_1"] = sample_object
    return frame


@pytest.fixture
def frame_sequence():
    """Create a sequence of frames with varying content."""
    frames = []

    # Frame 0: Empty
    frame0 = percemon.Frame()
    frame0.timestamp = 0.0
    frame0.frame_num = 0
    frame0.size_x = 1920
    frame0.size_y = 1080
    frames.append(frame0)

    # Frame 1: With car
    frame1 = percemon.Frame()
    frame1.timestamp = 1.0 / 30.0
    frame1.frame_num = 1
    frame1.size_x = 1920
    frame1.size_y = 1080

    bbox1 = percemon.BoundingBox()
    bbox1.xmin = 100.0
    bbox1.xmax = 200.0
    bbox1.ymin = 50.0
    bbox1.ymax = 150.0

    car1 = percemon.Object()
    car1.object_class = 1
    car1.probability = 0.9
    car1.bbox = bbox1

    frame1.objects["car_1"] = car1
    frames.append(frame1)

    # Frame 2: With car and pedestrian
    frame2 = percemon.Frame()
    frame2.timestamp = 2.0 / 30.0
    frame2.frame_num = 2
    frame2.size_x = 1920
    frame2.size_y = 1080

    bbox2 = percemon.BoundingBox()
    bbox2.xmin = 300.0
    bbox2.xmax = 400.0
    bbox2.ymin = 100.0
    bbox2.ymax = 250.0

    car2 = percemon.Object()
    car2.object_class = 1
    car2.probability = 0.85
    car2.bbox = bbox2

    ped2 = percemon.Object()
    ped2.object_class = 2  # Pedestrian
    ped2.probability = 0.92
    ped2.bbox = bbox2

    frame2.objects["car_2"] = car2
    frame2.objects["ped_1"] = ped2
    frames.append(frame2)

    return frames


# =============================================================================
# Helper Functions
# =============================================================================


def assert_formula_to_string_contains(formula, expected_substring) -> None:
    """Assert that formula's to_string() output contains expected substring."""
    formula_str = formula.to_string()
    assert expected_substring in formula_str, f"Expected '{expected_substring}' in '{formula_str}'"


def create_frame_with_objects(frame_num, timestamp, objects_dict):
    """Create a frame with specified objects.

    Args:
        frame_num: Frame number
        timestamp: Frame timestamp in seconds
        objects_dict: Dict mapping object_id -> (class_id, probability, bbox_tuple)
                     bbox_tuple = (xmin, xmax, ymin, ymax)

    Returns:
        Frame with specified objects
    """
    frame = percemon.Frame()
    frame.timestamp = timestamp
    frame.frame_num = frame_num
    frame.size_x = 1920
    frame.size_y = 1080

    for obj_id, (class_id, prob, (xmin, xmax, ymin, ymax)) in objects_dict.items():
        bbox = percemon.BoundingBox()
        bbox.xmin = xmin
        bbox.xmax = xmax
        bbox.ymin = ymin
        bbox.ymax = ymax

        obj = percemon.Object()
        obj.object_class = class_id
        obj.probability = prob
        obj.bbox = bbox

        frame.objects[obj_id] = obj

    return frame
