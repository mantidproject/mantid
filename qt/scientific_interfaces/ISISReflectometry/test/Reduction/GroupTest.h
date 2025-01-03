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

  /** Construction
   */

  void test_constructor_sets_group_name() {
    auto group = Group("Group1", {});
    TS_ASSERT_EQUALS("Group1", group.name());
  }

  void test_is_group() {
    auto group = makeEmptyGroup();
    TS_ASSERT_EQUALS(group.isGroup(), true);
  }

  void test_set_name() {
    auto group = makeEmptyGroup();
    group.setName("new name");
    TS_ASSERT_EQUALS(group.name(), "new name");
  }

  /** State
   */

  void test_set_success_marks_as_complete_and_success() {
    auto group = makeEmptyGroup();
    group.setSuccess();
    TS_ASSERT(group.complete());
    TS_ASSERT(group.success());
  }

  void test_set_error_marks_as_complete() {
    auto group = makeEmptyGroup();
    group.setError("test error");
    TS_ASSERT(group.complete());
    TS_ASSERT(!group.success());
  }

  void test_reset_state_clears_complete_and_success() {
    auto group = makeEmptyGroup();
    group.setSuccess();
    group.resetState();
    TS_ASSERT(!group.complete());
    TS_ASSERT(!group.success());
  }

  void test_reset_state_does_not_clear_child_row_state() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSuccess();
    group.resetState(false);
    TS_ASSERT(group[0]->success());
  }

  void test_reset_state_clears_child_row_state() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSuccess();
    group.resetState(true);
    TS_ASSERT(!group[0]->success());
  }

  /** Processing - these tests check if a group has rows that require
   * processing
   */

  void test_empty_group_does_not_require_processing() {
    auto group = makeEmptyGroup();
    checkDoesNotRequireProcessing(group);
  }

  void test_group_with_unprocessed_row_requires_processing() {
    auto group = makeGroupWithOneRow();
    checkRequiresProcessing(group);
  }

  void test_group_with_started_row_does_not_require_processing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setStarting();
    checkDoesNotRequireProcessing(group);
  }

  void test_group_with_running_row_does_not_require_processing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setRunning();
    checkDoesNotRequireProcessing(group);
  }

  void test_group_with_row_completed_does_not_require_processing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSuccess();
    checkDoesNotRequireProcessing(group);
  }

  void test_group_with_row_error_does_not_require_processing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setError("test error");
    TS_ASSERT(!group.requiresProcessing(false));
  }

  void test_group_with_row_error_requires_reprocessing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setError("test error");
    TS_ASSERT(group.requiresProcessing(true));
  }

  void test_setting_error_sets_message() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setError("test error");
    TS_ASSERT_EQUALS(group[0]->message(), "test error");
  }

  void test_skipped_row_does_not_require_processing() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSkipped(true);
    checkDoesNotRequireProcessing(group);
  }

  void test_resetting_skipped_makes_row_require_processing_again() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSkipped(true);
    group.resetSkipped();
    checkRequiresProcessing(group);
  }

  /** Postprocessing - "hasPostprocessing" indicates whether postprocessing is
   * applicable for a group, and "requiresPostprocessing" indicates whether
   * postprocessing is outstanding i.e. has not already been done. These tests
   * check these functions for a new group (i.e. default state)
   */

  void test_no_postprocessing_if_empty_group() {
    auto group = makeEmptyGroup();
    checkPostprocessingNotApplicable(group);
  }

  void test_no_postprocessing_if_one_row() {
    auto group = makeGroupWithOneRow();
    checkPostprocessingNotApplicable(group);
  }

  void test_no_postprocessing_if_one_valid_and_one_invalid_row() {
    auto group = makeGroupWithOneRow();
    group.appendRow(boost::none);
    checkPostprocessingNotApplicable(group);
  }

  void test_has_postprocessing_if_two_rows() {
    auto group = makeGroupWithTwoRows();
    checkPostprocessingIsApplicable(group);
    checkDoesNotRequirePostprocessing(group);
  }

  void test_has_postprocessing_if_two_valid_rows_and_one_invalid_row() {
    auto group = makeGroupWithTwoRows();
    group.appendRow(boost::none);
    checkPostprocessingIsApplicable(group);
    checkDoesNotRequirePostprocessing(group);
  }

  /** Postprocessing and row state - these tests check the
   * requiresPostprocessing function for groups where some processing of rows
   * has been done
   */

  void test_requires_postprocessing_if_all_rows_complete() {
    auto group = makeGroupWithTwoCompleteRows();
    checkRequiresPostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_some_rows_not_started() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_some_rows_are_starting() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setStarting();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_some_rows_are_running() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setRunning();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_some_rows_have_failed() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setError("test error");
    checkDoesNotRequirePostprocessing(group);
  }

  /** Postprocessing and group state - these tests check the
   * requiresPostprocessing function where postprocessing for the group has
   * already been done
   */

  void test_does_not_require_postprocessing_if_started() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setStarting();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_running() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setRunning();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_complete() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setSuccess();
    checkDoesNotRequirePostprocessing(group);
  }

  void test_does_not_require_postprocessing_if_failed() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setError("test group error");
    TS_ASSERT(!group.requiresPostprocessing(false));
  }

  void test_requires_reprocessing_if_postprocessing_failed() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setError("test group error");
    TS_ASSERT(group.requiresPostprocessing(true));
  }

  void test_does_not_require_postprocessing_if_skipped() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setSkipped(true);
    checkDoesNotRequirePostprocessing(group);
  }

  void test_requires_postprocessing_if_reset_skipped() {
    auto group = makeGroupWithTwoCompleteRows();
    group.setSkipped(true);
    group.resetSkipped();
    checkRequiresPostprocessing(group);
  }

  /** Workspace names
   */

  void test_setting_output_name() {
    auto group = makeEmptyGroup();
    group.setOutputNames({"test name"});
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "test name");
  }

  void test_setting_output_names_throws_if_more_than_one_name() {
    auto group = makeEmptyGroup();
    TS_ASSERT_THROWS(group.setOutputNames({"test name 1", "test name 2"}), const std::runtime_error &);
  }

  void test_setting_output_names_throws_if_empty() {
    auto group = makeEmptyGroup();
    TS_ASSERT_THROWS(group.setOutputNames({}), const std::runtime_error &);
  }

  void test_resetting_output_names() {
    auto group = makeEmptyGroup();
    group.setOutputNames({"test name"});
    group.resetOutputs();
    TS_ASSERT_EQUALS(group.postprocessedWorkspaceName(), "");
  }

  /** Adding rows
   */

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

  /** Removing rows
   */

  void test_remove_row() {
    auto group = makeGroupWithThreeRows();
    group.removeRow(1);
    TS_ASSERT_EQUALS(group.rows().size(), 2);
    TS_ASSERT_EQUALS(group[0]->runNumbers(), std::vector<std::string>{"12345"});
    TS_ASSERT_EQUALS(group[1]->runNumbers(), std::vector<std::string>{"12347"});
  }

  void test_remove_row_resets_group_state() {
    auto group = makeGroupWithThreeRows();
    group.setSuccess();
    group.removeRow(1);
    TS_ASSERT(!group.success());
  }

  /** Updating rows
   */

  void test_update_row() {
    auto group = makeGroupWithThreeRows();
    auto row = makeRow("22345", 1.5);
    group.updateRow(1, row);
    TS_ASSERT_EQUALS(group.rows().size(), 3);
    TS_ASSERT_EQUALS(group[1]->runNumbers(), std::vector<std::string>{"22345"});
  }

  void test_update_row_resets_state() {
    auto group = makeGroupWithThreeRows();
    group.setSuccess();
    auto row = makeRow("22345", 1.5);
    group.updateRow(1, row);
    TS_ASSERT(!group.success());
  }

  void test_update_row_does_not_reset_state_if_row_not_changed() {
    auto group = makeGroupWithThreeRows();
    group.setSuccess();
    auto row = makeRow("12346", 0.2);
    group.updateRow(1, row);
    TS_ASSERT(group.success());
  }

  /** Row statistics
   */

  void test_item_count_for_empty_group() {
    auto group = makeEmptyGroup();
    TS_ASSERT_EQUALS(group.totalItems(), 0);
  }

  void test_item_count_for_group_with_one_row() {
    auto group = makeGroupWithOneRow();
    TS_ASSERT_EQUALS(group.totalItems(), 1);
  }

  void test_item_count_for_group_with_two_rows() {
    auto group = makeGroupWithTwoRows();
    // count includes postprocessing for the group, as well as the two rows
    TS_ASSERT_EQUALS(group.totalItems(), 3);
  }

  void test_completed_item_count_for_empty_group() {
    auto group = makeEmptyGroup();
    TS_ASSERT_EQUALS(group.completedItems(), 0);
  }

  void test_completed_item_count_for_group_with_one_incomplete_row() {
    auto group = makeGroupWithOneRow();
    TS_ASSERT_EQUALS(group.completedItems(), 0);
  }

  void test_completed_item_count_for_group_with_one_complete_row() {
    auto group = makeGroupWithOneRow();
    group.mutableRows()[0]->setSuccess();
    TS_ASSERT_EQUALS(group.completedItems(), 1);
  }

  void test_completed_item_count_for_group_with_two_incomplte_rows() {
    auto group = makeGroupWithTwoRows();
    TS_ASSERT_EQUALS(group.completedItems(), 0);
  }

  void test_completed_item_count_for_group_with_one_complte_row_out_of_two() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    TS_ASSERT_EQUALS(group.completedItems(), 1);
  }

  void test_completed_item_count_for_group_with_two_complted_rows() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setSuccess();
    TS_ASSERT_EQUALS(group.completedItems(), 2);
  }

  void test_completed_item_count_for_completed_group() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setSuccess();
    group.setSuccess();
    TS_ASSERT_EQUALS(group.completedItems(), 3);
  }

  /** Row lookup
   */

  void test_index_of_row_with_theta_exact_match() {
    auto group = makeGroupWithThreeRows();
    auto maybeIndex = group.indexOfRowWithTheta(0.2, 0.01);
    TS_ASSERT(maybeIndex.has_value());
    if (maybeIndex.has_value())
      TS_ASSERT_EQUALS(*maybeIndex, 1);
  }

  void test_index_of_row_with_theta_within_tolerance() {
    auto group = makeGroupWithThreeRows();
    auto maybeIndex = group.indexOfRowWithTheta(0.209, 0.01);
    TS_ASSERT(maybeIndex.has_value());
    if (maybeIndex.has_value())
      TS_ASSERT_EQUALS(*maybeIndex, 1);
  }

  void test_index_of_row_with_theta_outside_tolerance() {
    auto group = makeGroupWithThreeRows();
    auto maybeIndex = group.indexOfRowWithTheta(0.23, 0.01);
    TS_ASSERT(!maybeIndex.has_value());
  }

  void test_find_row_with_output_name() {
    auto group = makeGroupWithThreeRows();
    // mark the row complete as an easy way to check we find the right one
    group.mutableRows()[1]->setOutputNames({"12346_Lam", "12346_Q", "12346_QBin"});
    group.mutableRows()[1]->setSuccess();
    auto maybeRow = group.getItemWithOutputWorkspaceOrNone("12346_Q");
    TS_ASSERT(maybeRow.is_initialized());
    if (maybeRow.is_initialized())
      TS_ASSERT(maybeRow->success());
  }

  void test_find_row_by_output_name_fails() {
    auto group = makeGroupWithThreeRows();
    auto maybeRow = group.getItemWithOutputWorkspaceOrNone("99999");
    TS_ASSERT(!maybeRow.is_initialized());
  }

  void test_set_all_row_parents() {
    auto group = makeGroupWithThreeRows();
    for (auto &row : group.mutableRows()) {
      row->setParent(nullptr);
    }
    for (auto const &row : group.rows()) {
      TS_ASSERT_EQUALS(row->getParent(), nullptr)
    }

    group.setAllRowParents();

    for (auto const &row : group.rows()) {
      TS_ASSERT_EQUALS(row->getParent(), &group)
    }
  }

  void test_update_parent_when_all_rows_complete() {
    auto group = makeGroupWithTwoCompleteRows();
    group.notifyChildStateChanged();
    TS_ASSERT_EQUALS(group.state(), State::ITEM_CHILDREN_SUCCESS);
  }

  void test_update_parent_when_some_rows_complete() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.notifyChildStateChanged();
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
  }

  void test_update_parent_when_no_rows_complete() {
    auto group = makeGroupWithTwoRows();
    group.notifyChildStateChanged();
    TS_ASSERT_EQUALS(group.state(), State::ITEM_NOT_STARTED);
  }

  void test_move_updates_all_row_parents() {
    auto group = makeGroupWithThreeRows();
    auto movedGroup = std::move(group);

    for (auto const &row : movedGroup.rows()) {
      TS_ASSERT_EQUALS(row->getParent(), &movedGroup)
    }
  }

  void test_copy_updates_all_row_parents() {
    auto group = makeGroupWithThreeRows();
    auto groupCopy = group;

    for (auto const &row : group.rows()) {
      TS_ASSERT_EQUALS(row->getParent(), &group)
    }
    for (auto const &row : groupCopy.rows()) {
      TS_ASSERT_EQUALS(row->getParent(), &groupCopy)
    }
  }

private:
  Group makeGroupWithThreeRows() {
    return Group("three_row_group", std::vector<boost::optional<Row>>{makeRow("12345", 0.1), makeRow("12346", 0.2),
                                                                      makeRow("12347", 0.3)});
  }

  Group makeGroupWithTwoCompleteRows() {
    auto group = makeGroupWithTwoRows();
    group.mutableRows()[0]->setSuccess();
    group.mutableRows()[1]->setSuccess();
    return group;
  }

  void checkRequiresProcessing(Group const &group) {
    TS_ASSERT(group.requiresProcessing(false));
    TS_ASSERT(group.requiresProcessing(true));
  }

  void checkDoesNotRequireProcessing(Group const &group) {
    TS_ASSERT(!group.requiresProcessing(false));
    TS_ASSERT(!group.requiresProcessing(true));
  }

  void checkPostprocessingIsApplicable(Group const &group) { TS_ASSERT(group.hasPostprocessing()); }

  void checkPostprocessingNotApplicable(Group const &group) {
    TS_ASSERT(!group.hasPostprocessing());
    TS_ASSERT(!group.requiresPostprocessing(false));
    TS_ASSERT(!group.requiresPostprocessing(true));
  }

  void checkRequiresPostprocessing(Group const &group) {
    TS_ASSERT(group.requiresPostprocessing(false));
    TS_ASSERT(group.requiresPostprocessing(true));
  }

  void checkDoesNotRequirePostprocessing(Group const &group) {
    TS_ASSERT(!group.requiresPostprocessing(false));
    TS_ASSERT(!group.requiresPostprocessing(true));
  }
};
