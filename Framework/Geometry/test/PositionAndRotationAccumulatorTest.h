// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/PositionAndRotationAccumulator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

using Mantid::Geometry::PositionAndRotationAccumulator;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

class PositionAndRotationAccumulatorTest : public CxxTest::TestSuite {
public:
  static PositionAndRotationAccumulatorTest *createSuite() { return new PositionAndRotationAccumulatorTest(); }
  static void destroySuite(PositionAndRotationAccumulatorTest *suite) { delete suite; }

  void test_nothing_accumulated_yields_nothing() {
    PositionAndRotationAccumulator accumulator;
    TS_ASSERT(accumulator.positions().empty());
    TS_ASSERT(accumulator.rotations().empty());
  }

  // --- Positions -----------------------------------------------------------------------------

  void test_setting_one_coordinate_leaves_the_other_two_at_the_current_position() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(3, "y", 9.0, V3D(1.0, 2.0, 3.0));

    const auto positions = accumulator.positions();
    TS_ASSERT_EQUALS(positions.size(), 1);
    TS_ASSERT_EQUALS(positions.at(3), V3D(1.0, 9.0, 3.0));
  }

  void test_the_current_position_seeds_only_the_first_coordinate_set() {
    PositionAndRotationAccumulator accumulator;
    // The second call passes a different "current" position. If it re-seeded, the x set by the
    // first call would be discarded.
    accumulator.setCoordinate(3, "x", 7.0, V3D(1.0, 2.0, 3.0));
    accumulator.setCoordinate(3, "y", 8.0, V3D(100.0, 200.0, 300.0));

    TS_ASSERT_EQUALS(accumulator.positions().at(3), V3D(7.0, 8.0, 3.0));
  }

  void test_all_three_coordinates_may_be_set() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(0, "x", 1.5, V3D(0.0, 0.0, 0.0));
    accumulator.setCoordinate(0, "y", 2.5, V3D(0.0, 0.0, 0.0));
    accumulator.setCoordinate(0, "z", 3.5, V3D(0.0, 0.0, 0.0));

    TS_ASSERT_EQUALS(accumulator.positions().at(0), V3D(1.5, 2.5, 3.5));
  }

  void test_an_unrecognised_coordinate_axis_is_ignored_but_still_seeds() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(1, "w", 42.0, V3D(1.0, 2.0, 3.0));

    TS_ASSERT_EQUALS(accumulator.positions().at(1), V3D(1.0, 2.0, 3.0));
  }

  void test_setPosition_discards_coordinates_accumulated_so_far() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(2, "x", 7.0, V3D(1.0, 2.0, 3.0));
    accumulator.setPosition(2, V3D(-1.0, -2.0, -3.0));

    TS_ASSERT_EQUALS(accumulator.positions().at(2), V3D(-1.0, -2.0, -3.0));
  }

  void test_a_coordinate_set_after_setPosition_does_not_reseed() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setPosition(2, V3D(-1.0, -2.0, -3.0));
    accumulator.setCoordinate(2, "y", 5.0, V3D(100.0, 200.0, 300.0));

    TS_ASSERT_EQUALS(accumulator.positions().at(2), V3D(-1.0, 5.0, -3.0));
  }

  void test_components_are_accumulated_independently() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(1, "x", 1.0, V3D(0.0, 0.0, 0.0));
    accumulator.setCoordinate(4, "x", 4.0, V3D(0.0, 0.0, 0.0));

    const auto positions = accumulator.positions();
    TS_ASSERT_EQUALS(positions.size(), 2);
    TS_ASSERT_EQUALS(positions.at(1), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(positions.at(4), V3D(4.0, 0.0, 0.0));
  }

  // --- Rotations -----------------------------------------------------------------------------

  void test_rotation_angles_compose_X_then_Y_then_Z_whatever_order_they_arrive_in() {
    PositionAndRotationAccumulator inOrder;
    inOrder.setRotationAngle(0, "rotx", 30.0);
    inOrder.setRotationAngle(0, "roty", 40.0);
    inOrder.setRotationAngle(0, "rotz", 50.0);

    PositionAndRotationAccumulator reversed;
    reversed.setRotationAngle(0, "rotz", 50.0);
    reversed.setRotationAngle(0, "roty", 40.0);
    reversed.setRotationAngle(0, "rotx", 30.0);

    const auto expected = Quat(30.0, V3D(1, 0, 0)) * Quat(40.0, V3D(0, 1, 0)) * Quat(50.0, V3D(0, 0, 1));
    TS_ASSERT_EQUALS(inOrder.rotations().at(0), expected);
    TS_ASSERT_EQUALS(reversed.rotations().at(0), expected);
  }

  void test_unset_rotation_axes_default_to_zero_rather_than_to_a_current_rotation() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setRotationAngle(0, "roty", 90.0);

    // Only Y was named, so X and Z contribute nothing at all.
    TS_ASSERT_EQUALS(accumulator.rotations().at(0), Quat(90.0, V3D(0, 1, 0)));
  }

  void test_setRotationAngle_reports_an_unrecognised_axis() {
    PositionAndRotationAccumulator accumulator;
    TS_ASSERT(accumulator.setRotationAngle(0, "rotx", 10.0));
    TS_ASSERT(accumulator.setRotationAngle(0, "roty", 10.0));
    TS_ASSERT(accumulator.setRotationAngle(0, "rotz", 10.0));
    // "rot" itself names no axis and is deliberately dropped.
    TS_ASSERT(!accumulator.setRotationAngle(0, "rot", 10.0));
    TS_ASSERT(!accumulator.setRotationAngle(0, "spin", 10.0));
  }

  void test_the_last_value_for_an_axis_wins() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setRotationAngle(0, "rotx", 10.0);
    accumulator.setRotationAngle(0, "rotx", 25.0);

    TS_ASSERT_EQUALS(accumulator.rotations().at(0), Quat(25.0, V3D(1, 0, 0)));
  }

  // --- The two stores are independent ---------------------------------------------------------

  void test_a_component_that_was_only_moved_yields_no_rotation() {
    // This is why positions and rotations are kept in separate stores: were they combined, this
    // component would come back with an identity rotation, and applying it would overwrite the
    // rotation the component already has.
    PositionAndRotationAccumulator accumulator;
    accumulator.setCoordinate(5, "x", 1.0, V3D(0.0, 0.0, 0.0));

    TS_ASSERT_EQUALS(accumulator.positions().count(5), 1);
    TS_ASSERT_EQUALS(accumulator.rotations().count(5), 0);
  }

  void test_a_component_that_was_only_rotated_yields_no_position() {
    PositionAndRotationAccumulator accumulator;
    accumulator.setRotationAngle(5, "rotx", 1.0);

    TS_ASSERT_EQUALS(accumulator.rotations().count(5), 1);
    TS_ASSERT_EQUALS(accumulator.positions().count(5), 0);
  }

  void test_an_unrecognised_rotation_axis_still_registers_the_component() {
    // setRotationAngle() default-constructs the entry before testing the axis, so a component
    // named with a bad axis comes back carrying a zero rotation. Pinned as current behaviour:
    // the caller warns about the axis, and a zero rotation is what the legacy map produced too.
    PositionAndRotationAccumulator accumulator;
    TS_ASSERT(!accumulator.setRotationAngle(6, "rot", 10.0));

    TS_ASSERT_EQUALS(accumulator.rotations().count(6), 1);
    TS_ASSERT_EQUALS(accumulator.rotations().at(6), Quat());
  }
};
