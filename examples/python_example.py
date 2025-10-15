#!/usr/bin/env python3
"""
PerceMon Python Bindings Example

This example demonstrates the key features of PerceMon:
- Creating STQL formulas
- Analyzing monitoring requirements
- Evaluating formulas on perception data frames
- Online monitoring of temporal properties
"""

import percemon


def example_basic_formula() -> None:
    """Example 1: Basic formula construction"""
    print("=" * 70)
    print("Example 1: Basic Formula Construction")
    print("=" * 70)

    # Create object variables
    car = percemon.ObjectVar("car")
    percemon.ObjectVar("pedestrian")

    # Create simple predicates
    # "There exists a car with class 1"
    is_car = percemon.is_class(car, 1)

    # "There exists a car with high confidence (>= 0.8)"
    is_car & percemon.high_confidence(car, 0.8)

    # "There exists a car"
    exists_car = percemon.exists([car], is_car)

    print(f"Formula: {exists_car}")
    print()


def example_temporal_formula() -> None:
    """Example 2: Temporal formulas"""
    print("=" * 70)
    print("Example 2: Temporal Formulas")
    print("=" * 70)

    car = percemon.ObjectVar("car")

    # "Eventually a car is detected"
    eventually_car = percemon.eventually(percemon.exists([car], percemon.is_class(car, 1)))

    # "Next frame has a car"
    next_car = percemon.next(percemon.exists([car], percemon.is_class(car, 1)))

    # "Car detected in next 3 frames"
    next3_car = percemon.next(percemon.exists([car], percemon.is_class(car, 1)), 3)

    print(f"Eventually car: {eventually_car}")
    print(f"Next car: {next_car}")
    print(f"Next 3 car: {next3_car}")
    print()


def example_requirements() -> None:
    """Example 3: Monitoring requirements analysis"""
    print("=" * 70)
    print("Example 3: Monitoring Requirements Analysis")
    print("=" * 70)

    car = percemon.ObjectVar("car")

    # Past-time formula (only uses past)
    past_formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))

    # Future-time formula (only uses future)
    future_formula = percemon.next(percemon.exists([car], percemon.is_class(car, 1)), 5)

    # Check requirements
    past_reqs = percemon.compute_requirements(past_formula, fps=30.0)
    future_reqs = percemon.compute_requirements(future_formula, fps=30.0)

    print(f"Past-time formula: {past_reqs}")
    print(f"  Is past-time: {percemon.is_past_time_formula(past_formula)}")
    print()

    print(f"Future-time formula: {future_reqs}")
    print(f"  Is past-time: {percemon.is_past_time_formula(future_formula)}")
    print()


def example_data_structures() -> None:
    """Example 4: Working with perception data"""
    print("=" * 70)
    print("Example 4: Working with Perception Data")
    print("=" * 70)

    # Create a bounding box
    bbox = percemon.BoundingBox()
    bbox.xmin = 100.0
    bbox.xmax = 200.0
    bbox.ymin = 50.0
    bbox.ymax = 150.0

    print(f"Bounding box: ({bbox.xmin}, {bbox.ymin}) to ({bbox.xmax}, {bbox.ymax})")
    print(f"  Width: {bbox.width()}")
    print(f"  Height: {bbox.height()}")
    print(f"  Area: {bbox.area()}")
    print(f"  Center: {bbox.center()}")
    print()

    # Create an object
    obj = percemon.Object()
    obj.object_class = 1  # Car class
    obj.probability = 0.95
    obj.bbox = bbox

    print(f"Object: class={obj.object_class}, confidence={obj.probability}")
    print()

    # Create a frame
    frame = percemon.Frame()
    frame.timestamp = 0.0
    frame.frame_num = 0
    frame.size_x = 1920
    frame.size_y = 1080
    frame.objects["car_1"] = obj

    print(f"Frame: {frame.frame_num} @ {frame.timestamp}s ({frame.size_x}x{frame.size_y})")
    print(f"  Objects: {list(frame.objects.keys())}")
    print()


def example_online_monitoring() -> None:
    """Example 6: Online monitoring"""
    print("=" * 70)
    print("Example 6: Online Monitoring")
    print("=" * 70)

    # Create a simple formula (past-time only, so it's monitorable)
    car = percemon.ObjectVar("car")
    formula = percemon.previous(percemon.exists([car], percemon.is_class(car, 1)))

    try:
        # Create online monitor
        monitor = percemon.OnlineMonitor(formula, fps=30.0)

        print(f"Created monitor for formula: {formula}")
        print(f"  Is monitorable: {monitor.is_monitorable()}")
        print(f"  Requirements: {monitor.requirements()}")
        print()

        # Create synthetic frames
        frames = []

        # Frame 0: No car
        frame0 = percemon.Frame()
        frame0.timestamp = 0.0
        frame0.frame_num = 0
        frame0.size_x = 1920
        frame0.size_y = 1080
        frames.append(frame0)

        # Frame 1: Car detected
        frame1 = percemon.Frame()
        frame1.timestamp = 1.0 / 30.0
        frame1.frame_num = 1
        frame1.size_x = 1920
        frame1.size_y = 1080

        bbox = percemon.BoundingBox()
        bbox.xmin = 100.0
        bbox.xmax = 200.0
        bbox.ymin = 50.0
        bbox.ymax = 150.0

        obj = percemon.Object()
        obj.object_class = 1
        obj.probability = 0.9
        obj.bbox = bbox

        frame1.objects["car_1"] = obj
        frames.append(frame1)

        # Evaluate on frames
        for frame in frames:
            verdict = monitor.evaluate(frame)
            status = "✓ SATISFIED" if verdict else "✗ VIOLATED"
            print(f"Frame {frame.frame_num}: {status}")

        print()

    except Exception as e:
        print(f"Error: {e}")
        print()


def main() -> None:
    """Run all examples"""
    print("\n")
    print("╔" + "=" * 68 + "╗")
    print("║" + " " * 10 + "PerceMon Python Bindings Examples" + " " * 24 + "║")
    print("╚" + "=" * 68 + "╝")
    print()

    try:
        example_basic_formula()
        example_temporal_formula()
        example_requirements()
        example_data_structures()
        example_online_monitoring()

        print("=" * 70)
        print("All examples completed successfully!")
        print("=" * 70)

    except Exception as e:
        print(f"\n❌ Error running examples: {e}")
        import traceback

        traceback.print_exc()


if __name__ == "__main__":
    main()
