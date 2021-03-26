# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.difference_table_widget.difference_widget_presenter import DifferencePresenter
from Muon.GUI.Common.muon_diff import MuonDiff
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from mantidqt.utils.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests

MAX_NUMBER_OF_DIFFS = 20


def enter_diff_name_side_effect():
    name = []
    for i in range(MAX_NUMBER_OF_DIFFS + 1):
        name.append("diff_" + str(i))
    return name


@start_qapplication
class DifferenceTablePresenterTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.model = GroupingTabModel(context=self.context)
        self.presenter = DifferencePresenter(self.model)

        # Mock user input for getting diff name
        self.presenter.group_view.enter_diff_name = mock.Mock(side_effect=enter_diff_name_side_effect())
        self.presenter.pair_view.enter_diff_name = mock.Mock(side_effect=enter_diff_name_side_effect())

        # Mock warning methods
        self.presenter.group_view.warning_popup = mock.Mock()
        self.presenter.pair_view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(0, len(self.model.diff_names))
        self.assertEqual(0, len(self.model.diffs))

    def assert_view_empty(self):
        self.assertEqual(0, self.presenter.group_view.num_rows())
        self.assertEqual(0, self.presenter.pair_view.num_rows())

    def add_two_groups(self):
        group0 = MuonGroup(group_name="group_0", detector_ids=[1])
        group1 = MuonGroup(group_name="group_1", detector_ids=[2])
        self.model.add_group(group0)
        self.model.add_group(group1)

    def add_two_pairs(self):
        pair0 = MuonPair(pair_name="pair_0", forward_group_name="group_0", backward_group_name="group_1", alpha=1.0)
        pair1 = MuonPair(pair_name="pair_1", forward_group_name="group_1", backward_group_name="group_0", alpha=1.0)
        self.model.add_pair(pair0)
        self.model.add_pair(pair1)

    def add_two_group_diffs(self):
        self.add_two_groups()
        diff0 = MuonDiff('group_diff_0', 'group_0', 'group_1')
        diff1 = MuonDiff('group_diff_1', 'group_1', 'group_0')
        self.presenter.group_widget.add_diff(diff0)
        self.presenter.group_widget.add_diff(diff1)

    def add_two_pair_diffs(self):
        self.add_two_groups()
        self.add_two_pairs()
        diff0 = MuonDiff('pair_diff_0', 'pair_0', 'pair_1', group_or_pair='pair')
        diff1 = MuonDiff('pair_diff_1', 'pair_1', 'pair_0', group_or_pair='pair')
        self.presenter.pair_widget.add_diff(diff0)
        self.presenter.pair_widget.add_diff(diff1)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Initialization
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_table_has_four_columns_when_initialized(self):
        self.assertEqual(4, self.presenter.group_view.num_cols())
        self.assertEqual(4, self.presenter.pair_view.num_cols())

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    def test_header_labels_set_correctly(self):
        self.assertEqual('Group 1', self.presenter.group_view.diff_table.horizontalHeaderItem(2).text())
        self.assertEqual('Group 2', self.presenter.group_view.diff_table.horizontalHeaderItem(3).text())
        self.assertEqual('Pair 1', self.presenter.pair_view.diff_table.horizontalHeaderItem(2).text())
        self.assertEqual('Pair 2', self.presenter.pair_view.diff_table.horizontalHeaderItem(3).text())

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Adding and removing diffs
    # ------------------------------------------------------------------------------------------------------------------

    def test_add_two_group_diffs_function(self):
        self.add_two_group_diffs()

        self.assertEqual(2, self.presenter.group_view.num_rows())
        self.assertEqual("group_diff_0", self.presenter.group_view.get_table_item_text(0, 0))
        self.assertEqual("group_diff_1", self.presenter.group_view.get_table_item_text(1, 0))

    def test_cannot_add_group_diff_without_two_groups(self):
        self.presenter.group_widget.handle_add_diff_button_clicked()  # No groups
        group0 = MuonGroup(group_name="group_0", detector_ids=[1])
        self.model.add_group(group0)
        self.presenter.group_widget.handle_add_diff_button_clicked()  # 1 group
        group1 = MuonGroup(group_name="group_1", detector_ids=[2])
        self.model.add_group(group1)
        self.presenter.group_widget.handle_add_diff_button_clicked()  # 2 groups
        self.assertEqual(2, self.presenter.group_view.warning_popup.call_count)

    def test_add_diff_button_adds_group_diff(self):
        self.add_two_groups()  # Required for a group diff
        self.presenter.group_widget.handle_add_diff_button_clicked()

        self.assertEqual(1, self.presenter.group_view.num_rows())
        self.assertEqual(1, len(self.model.diffs))
        self.assertEqual('group', self.model.diffs[0].group_or_pair)
        self.assertEqual("diff_0",
                         self.presenter.group_view.get_table_item_text(self.presenter.group_view.num_rows() - 1,
                                                                       0))  # Check added to end of table
        self.assertEqual(0, self.presenter.pair_view.num_rows())  # Check no pair diffs

    def test_add_two_pair_diffs_function(self):
        self.add_two_pair_diffs()

        self.assertEqual(2, self.presenter.pair_view.num_rows())
        self.assertEqual("pair_diff_0", self.presenter.pair_view.get_table_item_text(0, 0))
        self.assertEqual("pair_diff_1", self.presenter.pair_view.get_table_item_text(1, 0))

    def test_cannot_add_pair_diff_without_two_pairs(self):
        self.presenter.pair_widget.handle_add_diff_button_clicked()  # No pairs
        pair0 = MuonPair(pair_name="pair_0", forward_group_name="group_1", backward_group_name="group_0", alpha=1.0)
        self.model.add_pair(pair0)
        self.presenter.pair_widget.handle_add_diff_button_clicked()  # 1 pair
        pair1 = MuonPair(pair_name="pair_1", forward_group_name="group_0", backward_group_name="group_1", alpha=1.0)
        self.model.add_pair(pair1)
        self.presenter.pair_widget.handle_add_diff_button_clicked()  # 2 pairs
        self.assertEqual(2, self.presenter.pair_view.warning_popup.call_count)

    def test_add_diff_button_adds_pair_diff(self):
        self.add_two_pairs()  # Required for a pair diff
        self.presenter.pair_widget.handle_add_diff_button_clicked()

        self.assertEqual(1, self.presenter.pair_view.num_rows())
        self.assertEqual(1, len(self.model.diffs))
        self.assertEqual('pair', self.model.diffs[0].group_or_pair)
        self.assertEqual("diff_0",
                         self.presenter.pair_view.get_table_item_text(self.presenter.pair_view.num_rows() - 1,
                                                                      0))  # Check added to end of table
        self.assertEqual(0, self.presenter.group_view.num_rows())  # Check no group diffs

    def test_remove_diff_button(self):
        self.add_two_group_diffs()
        self.presenter.group_widget.handle_remove_diff_button_clicked()

        self.assertEqual(1, self.presenter.group_view.num_rows())
        self.assertEqual("group_diff_0",
                         self.presenter.group_view.get_table_item_text(self.presenter.group_view.num_rows() - 1, 0))

    def test_remove_diff_button_when_diffs_are_selected(self):
        self.add_two_group_diffs()
        self.presenter.group_view.get_selected_diff_names = mock.Mock(return_value=['group_diff_0'])
        self.presenter.group_widget.handle_remove_diff_button_clicked()

        self.assertEqual("group_diff_1",
                         self.presenter.group_view.get_table_item_text(self.presenter.group_view.num_rows() - 1, 0))

    def test_remove_diff_button_when_table_is_empty_does_not_throw(self):
        self.presenter.group_widget.handle_remove_diff_button_clicked()
        self.assertEqual(0, self.presenter.group_view.warning_popup.call_count)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Context menu has "add diff" and "remove diff" functionality
    # ------------------------------------------------------------------------------------------------------------------

    def test_context_menu_adds_diff_with_no_rows_selected(self):
        self.add_two_groups()  # Required for a group diff
        self.presenter.group_view.contextMenuEvent(0)
        self.presenter.group_view.add_diff_action.triggered.emit(True)

        self.assertEqual(1, self.presenter.group_view.num_rows())
        self.assertEqual("diff_0", self.presenter.group_view.get_table_item_text(0, 0))

    def test_context_menu_does_not_allow_add_diff_if_rows_selected(self):
        self.presenter.group_view._get_selected_row_indices = mock.Mock(return_value=[0])
        self.presenter.group_view.contextMenuEvent(0)

        self.assertFalse(self.presenter.group_view.add_diff_action.isEnabled())

    def test_context_menu_remove_diff_with_no_selected_rows(self):
        self.add_two_group_diffs()
        self.presenter.group_view.contextMenuEvent(0)
        self.presenter.group_view.remove_diff_action.triggered.emit(True)

        self.assertEqual(1, len(self.model.diffs))
        self.assertEqual(1, self.presenter.group_view.num_rows())
        self.assertEqual('group_diff_0', self.presenter.group_view.get_table_item_text(0, 0))

    def test_context_menu_removes_selected_diffs(self):
        self.add_two_group_diffs()
        self.presenter.group_view._get_selected_row_indices = mock.Mock(return_value=[0])
        self.presenter.group_view.contextMenuEvent(0)
        self.presenter.group_view.remove_diff_action.triggered.emit(True)

        self.assertEqual(1, len(self.model.diffs))
        self.assertEqual(1, self.presenter.group_view.num_rows())
        self.assertEqual('group_diff_1', self.presenter.group_view.get_table_item_text(0, 0))

    def test_context_menu_cannot_remove_diff_if_no_diffs_in_table(self):
        self.presenter.group_view.contextMenuEvent(0)
        self.assertFalse(self.presenter.group_view.remove_diff_action.isEnabled())

    # ------------------------------------------------------------------------------------------------------------------
    # Diff name validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_warning_using_duplicated_name(self):
        self.add_two_pair_diffs()

        self.presenter.group_view.enter_diff_name = mock.Mock(return_value="group_0")  # Diff name same as group
        self.presenter.group_widget.handle_add_diff_button_clicked()

        self.presenter.pair_view.enter_diff_name = mock.Mock(return_value="pair_0")  # Diff name same as pair
        self.presenter.pair_widget.handle_add_diff_button_clicked()

        self.presenter.pair_view.enter_diff_name = mock.Mock(return_value="pair_diff_0")  # Diff name same as diff
        self.presenter.pair_widget.handle_add_diff_button_clicked()

        self.presenter.pair_view.enter_diff_name = mock.Mock(return_value="new_diff")  # New diff name
        self.presenter.pair_widget.handle_add_diff_button_clicked()

        self.assertEqual(1, self.presenter.group_view.warning_popup.call_count)  # Group name duplicated
        self.assertEqual(2, self.presenter.pair_view.warning_popup.call_count)  # Pair and Diff name duplicated


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
