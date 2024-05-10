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

from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from mantidqtinterfaces.Muon.GUI.Common.difference_table_widget.difference_widget_presenter import DifferencePresenter
from mantidqtinterfaces.Muon.GUI.Common.muon_diff import MuonDiff
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqt.utils.observer_pattern import Observer
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests

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
        diff0 = MuonDiff("group_diff_0", "group_0", "group_1")
        diff1 = MuonDiff("group_diff_1", "group_1", "group_0")
        self.presenter.group_widget.add_diff(diff0)
        self.presenter.group_widget.add_diff(diff1)

    def add_two_pair_diffs(self):
        self.add_two_groups()
        self.add_two_pairs()
        diff0 = MuonDiff("pair_diff_0", "pair_0", "pair_1", group_or_pair="pair")
        diff1 = MuonDiff("pair_diff_1", "pair_1", "pair_0", group_or_pair="pair")
        self.presenter.pair_widget.add_diff(diff0)
        self.presenter.pair_widget.add_diff(diff1)

    def get_group_1_selector_from_diff(self, row):
        return self.presenter.group_view.diff_table.cellWidget(row, 2)

    def get_group_2_selector_from_diff(self, row):
        return self.presenter.group_view.diff_table.cellWidget(row, 3)

    def get_pair_1_selector_from_diff(self, row):
        return self.presenter.pair_view.diff_table.cellWidget(row, 2)

    def get_pair_2_selector_from_diff(self, row):
        return self.presenter.pair_view.diff_table.cellWidget(row, 3)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : test the functionality around the combo boxes which allow the user to select the two groups or two pairs
    #         that together make the muon difference.
    # ------------------------------------------------------------------------------------------------------------------

    def test_adding_groups_then_group_diff_sets_combo_boxes_to_added_groups(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()

        self.assertEqual("group_0", self.get_group_1_selector_from_diff(0).currentText())
        self.assertEqual("group_1", self.get_group_2_selector_from_diff(0).currentText())

    def test_adding_pairs_then_pair_diff_sets_combo_boxes_to_added_pairs(self):
        self.add_two_pairs()
        self.presenter.pair_widget.handle_add_diff_button_clicked()

        self.assertEqual("pair_0", self.get_pair_1_selector_from_diff(0).currentText())
        self.assertEqual("pair_1", self.get_pair_2_selector_from_diff(0).currentText())

    def test_get_index_of_text_returns_correct_index_if_text_exists(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()
        index = self.presenter.group_view.get_index_of_text(self.get_group_1_selector_from_diff(0), "group_1")

        self.assertEqual(index, 1)

    def test_get_index_of_text_returns_0_if_text_does_not_exists(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()
        index = self.presenter.group_view.get_index_of_text(self.get_group_1_selector_from_diff(0), "not_a_group_1")

        self.assertEqual(index, 0)

    def test_added_groups_appear_in_selector_combo_boxes_in_existing_diffs(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()
        self.presenter.update_view_from_model()

        self.assertEqual(2, self.get_group_1_selector_from_diff(0).count())
        self.assertNotEqual(-1, self.get_group_1_selector_from_diff(0).findText("group_0"))
        self.assertNotEqual(-1, self.get_group_1_selector_from_diff(0).findText("group_1"))

    def test_removing_groups_removes_them_from_selection(self):
        self.add_two_groups()
        # Add an extra group so we can remove one and keep a diff
        group = MuonGroup(group_name="group_2", detector_ids=[2])
        self.model.add_group(group)
        self.presenter.group_widget.handle_add_diff_button_clicked(group_1="group_0", group_2="group_2")

        self.group_context.remove_group("group_1")
        self.presenter.update_view_from_model()

        self.assertEqual(2, self.get_group_1_selector_from_diff(0).count())
        self.assertEqual(2, self.get_group_2_selector_from_diff(0).count())
        self.assertEqual(-1, self.get_group_2_selector_from_diff(0).findText("group_1"))
        self.assertEqual(-1, self.get_group_2_selector_from_diff(0).findText("group_1"))

    def test_changing_group_selection_triggers_cell_changed_method_in_view(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()
        self.presenter.group_widget.handle_add_diff_button_clicked()
        self.presenter.group_view.on_cell_changed = mock.Mock()
        self.get_group_1_selector_from_diff(0).setCurrentIndex(1)

        self.assertEqual(1, self.presenter.group_view.on_cell_changed.call_count)
        self.assertEqual((0, 2), self.presenter.group_view.on_cell_changed.call_args_list[0][0])

    def test_adding_new_group_does_not_change_current_selection(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()

        self.assertEqual(2, self.get_group_1_selector_from_diff(0).count())
        self.assertEqual(2, self.get_group_2_selector_from_diff(0).count())
        self.assertEqual("group_0", self.get_group_1_selector_from_diff(0).currentText())
        self.assertEqual("group_1", self.get_group_2_selector_from_diff(0).currentText())

        group = MuonGroup(group_name="group_2", detector_ids=[2])
        self.model.add_group(group)
        self.presenter.update_view_from_model()

        self.assertEqual(3, self.get_group_1_selector_from_diff(0).count())
        self.assertEqual(3, self.get_group_2_selector_from_diff(0).count())
        self.assertEqual("group_0", self.get_group_1_selector_from_diff(0).currentText())
        self.assertEqual("group_1", self.get_group_2_selector_from_diff(0).currentText())

    def test_changing_group_to_other_group_switches_groups(self):
        self.add_two_groups()
        self.presenter.group_widget.handle_add_diff_button_clicked()

        self.assertEqual(2, self.get_group_1_selector_from_diff(0).count())
        self.assertEqual(2, self.get_group_2_selector_from_diff(0).count())
        self.assertEqual("group_0", self.get_group_1_selector_from_diff(0).currentText())
        self.assertEqual("group_1", self.get_group_2_selector_from_diff(0).currentText())

        self.get_group_1_selector_from_diff(0).setCurrentIndex(1)

        self.assertEqual(2, self.get_group_1_selector_from_diff(0).count())
        self.assertEqual(2, self.get_group_2_selector_from_diff(0).count())
        self.assertEqual("group_1", self.get_group_1_selector_from_diff(0).currentText())
        self.assertEqual("group_0", self.get_group_2_selector_from_diff(0).currentText())


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
