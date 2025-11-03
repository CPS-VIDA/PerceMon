"""
Test suite for runtime data structures.

Tests cover:
- BoundingBox struct and methods
- RefPointType enum
- Object struct
- Frame struct and objects map
- get_reference_point() utility
- euclidean_distance() computation
"""

from __future__ import annotations

import percemon
import pytest


class TestRefPointTypeEnum:
    """Tests for RefPointType enum."""

    def test_ref_point_type_enum(self) -> None:
        """Test RefPointType enum values."""
        assert hasattr(percemon, "RefPointType")
        assert hasattr(percemon.RefPointType, "LeftMargin")
        assert hasattr(percemon.RefPointType, "RightMargin")
        assert hasattr(percemon.RefPointType, "TopMargin")
        assert hasattr(percemon.RefPointType, "BottomMargin")
        assert hasattr(percemon.RefPointType, "Center")


class TestBoundingBox:
    """Tests for BoundingBox struct."""

    def test_bounding_box_construction(self) -> None:
        """Test BoundingBox default construction."""
        bbox = percemon.BoundingBox()
        assert hasattr(bbox, "xmin")
        assert hasattr(bbox, "xmax")
        assert hasattr(bbox, "ymin")
        assert hasattr(bbox, "ymax")

    def test_bounding_box_with_values(self) -> None:
        """Test BoundingBox construction with values."""
        bbox = percemon.BoundingBox(100.0, 200.0, 50.0, 150.0)
        assert bbox.xmin == 100.0
        assert bbox.xmax == 200.0
        assert bbox.ymin == 50.0
        assert bbox.ymax == 150.0

    def test_bounding_box_assignment(self, sample_bounding_box) -> None:
        """Test BoundingBox property assignment."""
        bbox = sample_bounding_box
        assert bbox.xmin == 100.0
        assert bbox.xmax == 200.0
        assert bbox.ymin == 50.0
        assert bbox.ymax == 150.0

    def test_bounding_box_width(self, sample_bounding_box) -> None:
        """Test BoundingBox width calculation."""
        bbox = sample_bounding_box
        assert bbox.width() == 100.0

    def test_bounding_box_height(self, sample_bounding_box) -> None:
        """Test BoundingBox height calculation."""
        bbox = sample_bounding_box
        assert bbox.height() == 100.0

    def test_bounding_box_area(self, sample_bounding_box) -> None:
        """Test BoundingBox area calculation."""
        bbox = sample_bounding_box
        assert bbox.area() == 10000.0

    def test_bounding_box_center(self, sample_bounding_box) -> None:
        """Test BoundingBox center calculation."""
        bbox = sample_bounding_box
        cx, cy = bbox.center()
        assert cx == 150.0  # (100 + 200) / 2
        assert cy == 100.0  # (50 + 150) / 2

    def test_bounding_box_equality(self) -> None:
        """Test BoundingBox equality."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        assert bbox1 == bbox2

    def test_bounding_box_inequality(self) -> None:
        """Test BoundingBox inequality."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(0.0, 100.0, 0.0, 50.0)
        assert bbox1 != bbox2


class TestObject:
    """Tests for Object struct."""

    def test_object_construction(self) -> None:
        """Test Object default construction."""
        obj = percemon.Object()
        assert hasattr(obj, "object_class")
        assert hasattr(obj, "probability")
        assert hasattr(obj, "bbox")

    def test_object_with_values(self, sample_object) -> None:
        """Test Object with assigned values."""
        obj = sample_object
        assert obj.object_class == 1
        assert obj.probability == 0.95
        assert obj.bbox.xmin == 100.0

    def test_object_class_assignment(self, sample_object) -> None:
        """Test Object class assignment."""
        obj = sample_object
        obj.object_class = 2
        assert obj.object_class == 2

    def test_object_probability_assignment(self, sample_object) -> None:
        """Test Object probability assignment."""
        obj = sample_object
        obj.probability = 0.75
        assert obj.probability == 0.75

    def test_object_bbox_assignment(self, sample_object) -> None:
        """Test Object bbox assignment."""
        new_bbox = percemon.BoundingBox(0.0, 50.0, 0.0, 50.0)
        obj = sample_object
        obj.bbox = new_bbox
        assert obj.bbox.xmin == 0.0
        assert obj.bbox.xmax == 50.0

    def test_object_equality(self, sample_bounding_box) -> None:
        """Test Object equality."""
        obj1 = percemon.Object()
        obj1.object_class = 1
        obj1.probability = 0.9
        obj1.bbox = sample_bounding_box

        obj2 = percemon.Object()
        obj2.object_class = 1
        obj2.probability = 0.9
        obj2.bbox = sample_bounding_box

        assert obj1 == obj2


class TestFrame:
    """Tests for Frame struct."""

    def test_frame_construction(self) -> None:
        """Test Frame default construction."""
        frame = percemon.Frame()
        assert hasattr(frame, "timestamp")
        assert hasattr(frame, "frame_num")
        assert hasattr(frame, "size_x")
        assert hasattr(frame, "size_y")
        assert hasattr(frame, "objects")

    def test_frame_properties(self, empty_frame) -> None:
        """Test Frame property assignment."""
        frame = empty_frame
        assert frame.timestamp == 0.0
        assert frame.frame_num == 0
        assert frame.size_x == 1920
        assert frame.size_y == 1080

    def test_frame_objects_empty(self, empty_frame) -> None:
        """Test Frame with empty objects map."""
        frame = empty_frame
        assert len(frame.objects) == 0

    def test_frame_add_object(self, empty_frame, sample_object) -> None:
        """Test adding object to frame."""
        frame = empty_frame
        frame.objects["car_1"] = sample_object
        assert "car_1" in frame.objects
        assert len(frame.objects) == 1

    def test_frame_access_object(self, frame_with_object) -> None:
        """Test accessing object from frame."""
        frame = frame_with_object
        assert "car_1" in frame.objects
        obj = frame.objects["car_1"]
        assert obj.object_class == 1
        assert obj.probability == 0.95

    def test_frame_multiple_objects(self, empty_frame, sample_bounding_box) -> None:
        """Test frame with multiple objects."""
        frame = empty_frame

        car = percemon.Object()
        car.object_class = 1
        car.probability = 0.9
        car.bbox = sample_bounding_box

        ped = percemon.Object()
        ped.object_class = 2
        ped.probability = 0.85
        ped.bbox = sample_bounding_box

        frame.objects["car_1"] = car
        frame.objects["ped_1"] = ped

        assert len(frame.objects) == 2
        assert frame.objects["car_1"].object_class == 1
        assert frame.objects["ped_1"].object_class == 2

    def test_frame_universe_bbox(self) -> None:
        """Test Frame universe_bbox() method."""
        frame = percemon.Frame()
        frame.size_x = 1920
        frame.size_y = 1080

        univ = frame.universe_bbox()
        assert univ.xmin == 0.0
        assert univ.xmax == 1920.0
        assert univ.ymin == 0.0
        assert univ.ymax == 1080.0

    def test_frame_timestamp_update(self, empty_frame) -> None:
        """Test updating frame timestamp."""
        frame = empty_frame
        frame.timestamp = 1.5
        assert frame.timestamp == 1.5

    def test_frame_frame_num_update(self, empty_frame) -> None:
        """Test updating frame number."""
        frame = empty_frame
        frame.frame_num = 42
        assert frame.frame_num == 42


class TestGetReferencePoint:
    """Tests for get_reference_point() utility function."""

    def test_get_reference_point_center(self, sample_bounding_box) -> None:
        """Test get_reference_point for center."""
        x, y = percemon.get_reference_point(sample_bounding_box, percemon.RefPointType.Center)
        assert x == 150.0
        assert y == 100.0

    def test_get_reference_point_left_margin(self, sample_bounding_box) -> None:
        """Test get_reference_point for left margin."""
        x, y = percemon.get_reference_point(sample_bounding_box, percemon.RefPointType.LeftMargin)
        assert x == 100.0
        assert y == 100.0  # Center y

    def test_get_reference_point_right_margin(self, sample_bounding_box) -> None:
        """Test get_reference_point for right margin."""
        x, y = percemon.get_reference_point(sample_bounding_box, percemon.RefPointType.RightMargin)
        assert x == 200.0
        assert y == 100.0

    def test_get_reference_point_top_margin(self, sample_bounding_box) -> None:
        """Test get_reference_point for top margin."""
        x, y = percemon.get_reference_point(sample_bounding_box, percemon.RefPointType.TopMargin)
        assert x == 150.0
        assert y == 50.0

    def test_get_reference_point_bottom_margin(self, sample_bounding_box) -> None:
        """Test get_reference_point for bottom margin."""
        x, y = percemon.get_reference_point(sample_bounding_box, percemon.RefPointType.BottomMargin)
        assert x == 150.0
        assert y == 150.0


class TestEuclideanDistance:
    """Tests for euclidean_distance() utility function."""

    def test_euclidean_distance_same_point(self, sample_bounding_box) -> None:
        """Test euclidean_distance between same points."""
        dist = percemon.euclidean_distance(
            sample_bounding_box, percemon.RefPointType.Center, sample_bounding_box, percemon.RefPointType.Center
        )
        assert dist == pytest.approx(0.0)

    def test_euclidean_distance_different_centers(self) -> None:
        """Test euclidean_distance between different centers."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(100.0, 200.0, 0.0, 100.0)

        dist = percemon.euclidean_distance(bbox1, percemon.RefPointType.Center, bbox2, percemon.RefPointType.Center)
        # Centers are at (50, 50) and (150, 50), distance = 100
        assert dist == pytest.approx(100.0)

    def test_euclidean_distance_vertical(self) -> None:
        """Test euclidean_distance vertically."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(0.0, 100.0, 100.0, 200.0)

        dist = percemon.euclidean_distance(bbox1, percemon.RefPointType.Center, bbox2, percemon.RefPointType.Center)
        # Centers are at (50, 50) and (50, 150), distance = 100
        assert dist == pytest.approx(100.0)

    def test_euclidean_distance_diagonal(self) -> None:
        """Test euclidean_distance diagonally."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(100.0, 200.0, 100.0, 200.0)

        dist = percemon.euclidean_distance(bbox1, percemon.RefPointType.Center, bbox2, percemon.RefPointType.Center)
        # Centers are at (50, 50) and (150, 150)
        # distance = sqrt((100)^2 + (100)^2) = sqrt(20000) ≈ 141.42
        assert dist == pytest.approx(141.42, rel=1e-2)

    def test_euclidean_distance_between_edges(self) -> None:
        """Test euclidean_distance between different reference points."""
        bbox1 = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        bbox2 = percemon.BoundingBox(100.0, 200.0, 0.0, 100.0)

        dist = percemon.euclidean_distance(bbox1, percemon.RefPointType.RightMargin, bbox2, percemon.RefPointType.LeftMargin)
        # Right margin of bbox1 at (100, 50)
        # Left margin of bbox2 at (100, 50)
        # distance = 0
        assert dist == pytest.approx(0.0)


class TestIntegration:
    """Integration tests combining multiple data structures."""

    def test_frame_sequence_workflow(self, frame_sequence) -> None:
        """Test workflow with frame sequence."""
        frames = frame_sequence
        assert len(frames) == 3
        assert len(frames[0].objects) == 0
        assert len(frames[1].objects) == 1
        assert len(frames[2].objects) == 2

    def test_extract_object_from_frame(self, frame_with_object) -> None:
        """Test extracting object from frame."""
        frame = frame_with_object
        car = frame.objects["car_1"]

        # Verify object properties
        assert car.object_class == 1
        assert car.probability == 0.95

        # Verify bounding box
        bbox = car.bbox
        assert bbox.width() == 100.0
        assert bbox.height() == 100.0
        assert bbox.area() == 10000.0

    def test_compute_distance_between_frame_objects(self) -> None:
        """Test computing distance between objects in a frame."""
        frame = percemon.Frame()
        frame.size_x = 1920
        frame.size_y = 1080

        # Add two objects at different locations
        obj1_bbox = percemon.BoundingBox(0.0, 100.0, 0.0, 100.0)
        obj1 = percemon.Object()
        obj1.object_class = 1
        obj1.bbox = obj1_bbox

        obj2_bbox = percemon.BoundingBox(300.0, 400.0, 0.0, 100.0)
        obj2 = percemon.Object()
        obj2.object_class = 2
        obj2.bbox = obj2_bbox

        frame.objects["obj1"] = obj1
        frame.objects["obj2"] = obj2

        # Compute distance
        dist = percemon.euclidean_distance(obj1_bbox, percemon.RefPointType.Center, obj2_bbox, percemon.RefPointType.Center)
        # Centers at (50, 50) and (350, 50), distance = 300
        assert dist == pytest.approx(300.0)
