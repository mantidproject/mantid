// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/Reduction/Group.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  ReductionWorkspaces workspaceNames() const { return ReductionWorkspaces({}, TransmissionRunPair{}); }

  void test_constructor_sets_group_name() {
    auto group = Group("Group1", {});
    TS_ASSERT_EQUALS("Group1", group.name());
  }

  void test_append_row() {
    auto group = makeEmptyGroup();
    auto rowToAdd = makeRow("12345", 0.5);
    group.appendRow(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 1);
    TS_ASSERT(group[0].is_initialized());
    TS_ASSERT_EQUALS(group[0].get(), rowToAdd);
  }

  void test_append_empty_row() {
    auto group = makeEmptyGroup();
    group.appendEmptyRow();
    TS_ASSERT_EQUALS(group.rows().size(), 1);
    TS_ASSERT_EQUALS(group[0].is_initialized(), false);
  }

  void test_append_uninitialized_row() {
    // manually append an empty (uninitialized) row
    auto group = makeEmptyGroup();
    auto rowToAdd = boost::optional<Row>{boost::none};
    group.appendRow(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 1);
    TS_ASSERT_EQUALS(group[0].is_initialized(), false);
  }

  void test_insert_row_at_position() {
    auto group = makeGroupWithTwoRows();
    auto rowToAdd = makeRow("12345", 0.5);
    auto const index = 1;
    group.insertRow(rowToAdd, index);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT(group[index].is_initialized());
    TS_ASSERT_EQUALS(group[index].get(), rowToAdd);
  }

  void test_insert_row_sorted_by_angle() {
    auto group = makeGroupWithTwoRowsWithDifferentAngles();
    auto rowToAdd = makeRow("22345", 0.5);
    auto const index = 1; // angle 0.5 is between the two existing rows
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT(group[index].is_initialized());
    TS_ASSERT_EQUALS(group[index].get(), rowToAdd);
  }

  void test_insert_row_sorted_by_angle_at_start() {
    auto group = makeGroupWithTwoRowsWithDifferentAngles();
    auto rowToAdd = makeRow("22345", 0.1);
    auto const index = 0; // angle 0.1 is before the current two rows
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT(group[index].is_initialized());
    TS_ASSERT_EQUALS(group[index].get(), rowToAdd);
  }

  void test_insert_row_sorted_by_angle_at_end() {
    auto group = makeGroupWithTwoRowsWithDifferentAngles();
    auto rowToAdd = makeRow("22345", 1.5);
    auto const index = 2; // angle 1.5 is after the current two rows
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT(group[index].is_initialized());
    TS_ASSERT_EQUALS(group[index].get(), rowToAdd);
  }

  void test_insert_row_sorted_by_angle_into_empty_group() {
    auto group = makeEmptyGroup();
    auto rowToAdd = makeRow("22345", 0.5);
    auto const index = 0;
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 1);
    TS_ASSERT(group[index].is_initialized());
    TS_ASSERT_EQUALS(group[index].get(), rowToAdd);
  }

  void test_insert_row_sorted_by_angle_adds_uninitialized_row_at_end() {
    auto group = makeGroupWithTwoRowsWithDifferentAngles();
    auto rowToAdd = boost::optional<Row>{boost::none};
    auto const index = 2; // should be added at the end
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT_EQUALS(group[index].is_initialized(), false);
  }

  void test_insert_uninitialized_row_sorted_by_angle_into_empty_group() {
    auto group = makeEmptyGroup();
    auto rowToAdd = boost::optional<Row>{boost::none};
    auto const index = 0;
    group.insertRowSortedByAngle(rowToAdd);
    TS_ASSERT_EQUALS(group.rows().size(), 1);
    TS_ASSERT_EQUALS(group[index].is_initialized(), false);
  }
};
