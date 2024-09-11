# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter, RowValid
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import (
    GroupingTableView,
    inverse_group_table_columns,
)
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqt.utils.observer_pattern import Observer
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests

maximum_number_of_groups = 20


def group_name():
    name = []
    for i in range(maximum_number_of_groups + 1):
        name.append("group_" + str(i))
    return name


@start_qapplication
class GroupingTablePresenterTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.model = GroupingTabModel(context=self.context)
        self.view = GroupingTableView(parent=self.obj)
        self.presenter = GroupingTablePresenter(self.view, self.model)

        self.view.enter_group_name = mock.Mock(side_effect=group_name())
        self.view.warning_popup = mock.Mock()
        self.gui_variable_observer.update = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.group_names), 0)
        self.assertEqual(len(self.model.groups), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def add_three_groups_to_table(self):
        group1 = MuonGroup(group_name="my_group_0", detector_ids=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_ids=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_ids=[3])
        self.presenter.add_group(group1)
        self.presenter.add_group(group2)
        self.presenter.add_group(group3)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Initialization
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_table_has_five_columns_when_initialized(self):
        self.assertEqual(self.view.num_cols(), 5)

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Adding and removing groups
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_add_group_button_adds_group(self):
        self.presenter.handle_add_group_button_clicked()
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 1)

    def test_that_add_three_groups_function_adds_three_groups(self):
        self.add_three_groups_to_table()
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(self.view.get_table_item_text(0, 0), "my_group_0")
        self.assertEqual(self.view.get_table_item_text(1, 0), "my_group_1")
        self.assertEqual(self.view.get_table_item_text(2, 0), "my_group_2")

    def test_that_remove_group_button_removes_group(self):
        self.add_three_groups_to_table()
        self.presenter.handle_remove_group_button_clicked()
        self.assertEqual(self.view.num_rows(), 2)

    def test_that_add_group_button_adds_group_to_end_of_table(self):
        self.add_three_groups_to_table()

        self.presenter.add_group(MuonGroup(group_name="new", detector_ids=[4]))

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "new")

    def test_that_remove_group_button_removes_group_from_end_of_table(self):
        self.add_three_groups_to_table()

        self.presenter.handle_remove_group_button_clicked()

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "my_group_1")

    def test_that_highlighting_rows_and_clicking_remove_group_removes_the_selected_rows(self):
        self.add_three_groups_to_table()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.presenter.handle_remove_group_button_clicked()

        self.assertEqual(self.view.get_table_item_text(0, 0), "my_group_1")
        self.assertEqual(self.model.group_names, ["my_group_1"])

    def test_that_cannot_add_more_than_20_rows(self):
        for i in range(maximum_number_of_groups + 1):
            self.presenter.handle_add_group_button_clicked()

        self.assertEqual(self.view.num_rows(), maximum_number_of_groups)
        self.assertEqual(len(self.model.groups), maximum_number_of_groups)

    def test_that_trying_to_add_a_20th_row_gives_warning_message(self):
        for i in range(maximum_number_of_groups + 1):
            self.presenter.handle_add_group_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_remove_group_when_table_is_empty_does_not_throw(self):
        self.presenter.handle_remove_group_button_clicked()
        self.assertEqual(self.view.warning_popup.call_count, 0)

    # ------------------------------------------------------------------------------------------------------------------
    # Testing context menu
    # ------------------------------------------------------------------------------------------------------------------

    def test_context_menu_add_grouping_with_no_rows_selected_adds_group_to_end_of_table(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.contextMenuEvent(0)
        self.view.add_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 2)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(self.view.get_table_item_text(1, 0), "group_1")

    def test_context_menu_add_grouping_with_rows_selected_does_not_add_group(self):
        self.presenter.handle_add_group_button_clicked()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0])

        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.add_group_action.isEnabled())

    def test_context_menu_remove_grouping_with_no_rows_selected_removes_last_row(self):
        self.add_three_groups_to_table()

        self.view.contextMenuEvent(0)
        self.view.remove_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 2)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(self.view.get_table_item_text(0, 0), "my_group_0")
        self.assertEqual(self.view.get_table_item_text(1, 0), "my_group_1")

    def test_context_menu_remove_grouping_removes_selected_rows(self):
        self.add_three_groups_to_table()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.view.contextMenuEvent(0)
        self.view.remove_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 1)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(self.view.get_table_item_text(0, 0), "my_group_1")

    def test_context_menu_remove_grouping_disabled_if_no_groups_in_table(self):
        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.remove_group_action.isEnabled())

    def test_context_menu_add_pair_disabled_unless_two_groups_selected(self):
        self.add_three_groups_to_table()

        for selected_rows in [[], [0], [0, 1, 2]]:
            self.view._get_selected_row_indices = mock.Mock(return_value=selected_rows)
            self.view.contextMenuEvent(0)

            self.assertFalse(self.view.add_pair_action.isEnabled())

    # ------------------------------------------------------------------------------------------------------------------
    # Group name validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_can_change_group_name_to_valid_name_and_update_view_and_model(self):
        self.add_three_groups_to_table()

        self.view.grouping_table.item(0, 0).setText("new_name")

        self.assertEqual(self.view.get_table_item_text(0, 0), "new_name")
        self.assertIn("new_name", self.model.group_names)

    def test_that_if_invalid_name_given_warning_message_is_shown(self):
        self.add_three_groups_to_table()
        invalid_names = ["", "@", "name!", "+-"]

        for invalid_name in invalid_names:
            self.view.grouping_table.item(0, 0).setText(invalid_name)

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), "my_group_0")
            self.assertIn("my_group_0", self.model.group_names)

    def test_that_group_names_with_numbers_and_letters_and_underscores_are_valid(self):
        self.add_three_groups_to_table()

        valid_names = ["fwd", "fwd_1", "1234", "FWD0001", "_fwd"]

        for valid_name in valid_names:
            self.view.grouping_table.item(0, 0).setText(valid_name)

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), valid_name)
            self.assertIn(valid_name, self.model.group_names)

    def test_that_warning_shown_if_duplicated_group_name_used(self):
        self.add_three_groups_to_table()

        self.view.enter_group_name = mock.Mock(return_value="my_group_1")
        self.presenter.handle_add_group_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    # ------------------------------------------------------------------------------------------------------------------
    # detector ID validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_if_not_entering_numbers_into_detector_IDs_the_changes_are_rejected(self):
        self.add_three_groups_to_table()

        invalid_id_lists = ["fwd", "a", "A", "!", "_", "(1)", "11a22", "0"]

        call_count = 0
        for invalid_ids in invalid_id_lists:
            call_count += 1

            self.view.grouping_table.setCurrentCell(0, 3)
            self.view.grouping_table.item(0, 3).setText(invalid_ids)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

            self.assertEqual(self.view.get_table_item_text(0, 3), "1")
            self.assertEqual(self.model._context.group_pair_context["my_group_0"].detectors, [1])

    def test_that_displayed_values_are_simplified_to_least_verbose_form(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.grouping_table.setCurrentCell(0, 3)
        self.view.grouping_table.item(0, 3).setText("20-25,10,5,4,3,2,1")

        self.assertEqual(self.view.get_table_item_text(0, 3), "1-5,10,20-25")
        self.assertEqual(self.model._context.group_pair_context["group_0"].detectors, [1, 2, 3, 4, 5, 10, 20, 21, 22, 23, 24, 25])

    def test_that_if_detector_list_changed_that_number_of_detectors_updates(self):
        self.presenter.handle_add_group_button_clicked()
        self.assertEqual(self.view.get_table_item_text(0, inverse_group_table_columns["number_of_detectors"]), "1")

        self.view.grouping_table.setCurrentCell(0, inverse_group_table_columns["detector_ids"])
        self.view.grouping_table.item(0, inverse_group_table_columns["detector_ids"]).setText("1-10")

        self.assertEqual(self.view.get_table_item_text(0, inverse_group_table_columns["number_of_detectors"]), "10")

    def test_that_detector_numbers_cannot_be_edited(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.grouping_table.setCurrentCell(0, inverse_group_table_columns["detector_ids"])
        self.view.grouping_table.item(0, inverse_group_table_columns["number_of_detectors"]).setText("25")

        self.assertEqual(self.view.get_table_item_text(0, inverse_group_table_columns["number_of_detectors"]), "1")

    def test_modifying_detector_ids_to_non_existent_detector_fails(self):
        self.presenter.handle_add_group_button_clicked()
        self.view.grouping_table.item(0, inverse_group_table_columns["detector_ids"]).setText("1000")

        self.view.warning_popup.assert_called_once_with("Invalid detector list.")

    def test_modifying_detector_ids_to_negative_detectors_fails(self):
        self.presenter.handle_add_group_button_clicked()
        self.view.grouping_table.item(0, inverse_group_table_columns["detector_ids"]).setText("-10-10")

        self.view.warning_popup.assert_called_once_with("Invalid detector list.")

    def test_range_boxes_start_out_disabled(self):
        self.assertFalse(self.view.group_range_min.isEnabled())
        self.assertFalse(self.view.group_range_max.isEnabled())

    def test_enabling_range_min_editing_creates_context_variable(self):
        number = "1.12"
        self.view.group_range_min.setText(number)
        self.view.group_range_use_first_good_data.setChecked(False)

        self.assertEqual(self.gui_context["GroupRangeMin"], float(number))
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {"GroupRangeMin": 1.12})

    def test_disabling_range_min_editing_removes_context_variable(self):
        number = "1.12"
        self.view.group_range_min.setText(number)
        self.view.group_range_use_first_good_data.setChecked(False)

        self.assertEqual(self.gui_context["GroupRangeMin"], float(number))
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {"GroupRangeMin": 1.12})

        self.view.group_range_use_first_good_data.setChecked(True)

        self.assertFalse("GroupRangeMin" in self.gui_context)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

    def test_enabling_range_max_editing_creates_context_variable(self):
        number = "1.12"
        self.view.group_range_max.setText(number)
        self.view.group_range_use_last_data.setChecked(False)

        self.assertEqual(self.gui_context["GroupRangeMax"], float(number))
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {"GroupRangeMax": 1.12})

    def test_disabling_range_max_editing_removes_context_variable(self):
        number = "1.12"
        self.view.group_range_max.setText(number)
        self.view.group_range_use_last_data.setChecked(False)

        self.assertEqual(self.gui_context["GroupRangeMax"], float(number))
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {"GroupRangeMax": 1.12})

        self.view.group_range_use_last_data.setChecked(True)

        self.assertFalse("GroupRangeMax" in self.gui_context)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

    def test_updating_range_min_to_be_greater_than_range_max_displays_warning_and_vice_versa(self):
        original_max = "1.12"
        original_min = "0.1"
        self.view.group_range_max.setText(original_max)
        self.view.group_range_min.setText(original_min)
        self.view.group_range_use_last_data.setChecked(False)
        self.view.group_range_use_first_good_data.setChecked(False)

        self.view.group_range_min.setText("2.0")
        self.view.group_range_min.editingFinished.emit()

        self.assertEqual(self.gui_context["GroupRangeMin"], float(original_min))
        self.view.warning_popup.assert_called_once_with("Minimum of group asymmetry range must be less than maximum")

        self.view.group_range_max.setText("0.05")
        self.view.group_range_max.editingFinished.emit()

        self.assertEqual(self.gui_context["GroupRangeMax"], float(original_max))
        self.view.warning_popup.assert_called_with("Maximum of group asymmetry range must be greater than minimum")
        self.assertEqual(self.view.warning_popup.call_count, 2)

    def test_that_periods_invalid_for_all_runs_return_invalid(self):
        self.presenter._model._context = mock.MagicMock()
        self.presenter._model._context.current_runs = [[84447], [84448], [84449], [84450], [84451]]
        self.presenter._model._context.num_periods = self._fake_num_periods

        valid = self.presenter.validate_periods("1-5")

        self.assertEqual(RowValid.invalid_for_all_runs, valid)

    def test_that_period_valid_for_at_least_one_run_returns_valid(self):
        self.presenter._model._context = mock.MagicMock()
        self.presenter._model._context.current_runs = [[84449], [84450], [84451]]
        self.presenter._model._context.num_periods = self._fake_num_periods

        valid = self.presenter.validate_periods("1-4")

        self.assertEqual(RowValid.valid_for_some_runs, valid)

    def test_that_period_string_containing_not_matching_run_entry_regex_returns_invalid(self):
        valid = self.presenter.validate_periods("Invalid string")

        self.assertEqual(RowValid.invalid_for_all_runs, valid)

    def _fake_num_periods(self, run):
        num_periods_dict = {"[84447]": 4, "[84448]": 4, "[84449]": 4, "[84450]": 2, "[84451]": 1}
        return num_periods_dict[str(run)]

    def test_when_adding_group_to_empty_table_it_is_selected(self):
        self.presenter.add_group(MuonGroup(group_name="group_1", detector_ids=[1, 2, 3, 4]))
        self.presenter.add_group(MuonGroup(group_name="group_2", detector_ids=[1, 2, 3, 4]))

        self.assertEqual(self.model.selected_groups, ["group_1"])

    def test_update_view_from_model_correctly_adds_warnings_for_invalid_periods(self):
        self.presenter._model.validate_periods_list = mock.MagicMock(return_value=RowValid.invalid_for_all_runs)
        self.presenter._view.add_entry_to_table = mock.MagicMock()
        self.presenter.add_group_to_model(MuonGroup(group_name="group_1", detector_ids=[1, 2, 3, 4], periods=[3]))

        self.presenter.update_view_from_model()

        self.presenter._view.add_entry_to_table.assert_called_once_with(
            ["group_1", "3", False, "1-4", "4"], (255, 0, 0), "Warning: group periods invalid for all runs"
        )

    def test_update_view_from_model_correctly_adds_warnings_for_semi_invalid_periods(self):
        self.presenter._model.validate_periods_list = mock.MagicMock(return_value=RowValid.valid_for_some_runs)
        self.presenter._view.add_entry_to_table = mock.MagicMock()
        self.presenter.add_group_to_model(MuonGroup(group_name="group_1", detector_ids=[1, 2, 3, 4], periods=[3]))

        self.presenter.update_view_from_model()

        self.presenter._view.add_entry_to_table.assert_called_once_with(
            ["group_1", "3", False, "1-4", "4"], (255, 255, 0), "Warning: group periods invalid for some runs"
        )

    def test_update_view_from_model_correctly_adds_warnings_for_valid(self):
        self.presenter._model.validate_periods_list = mock.MagicMock(return_value=RowValid.valid_for_all_runs)
        self.presenter._view.add_entry_to_table = mock.MagicMock()
        self.presenter.add_group_to_model(MuonGroup(group_name="group_1", detector_ids=[1, 2, 3, 4], periods=[3]))

        self.presenter.update_view_from_model()

        self.presenter._view.add_entry_to_table.assert_called_once_with(["group_1", "3", False, "1-4", "4"], (255, 255, 255), "")


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
