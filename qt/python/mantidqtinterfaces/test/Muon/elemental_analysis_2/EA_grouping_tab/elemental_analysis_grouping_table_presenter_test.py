# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget, QTableWidgetItem

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter import (
    EAGroupingTablePresenter,
    REBIN_FIXED_OPTION,
    REBIN_VARIABLE_OPTION,
)
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import (
    EAGroupingTableView,
    INVERSE_GROUP_TABLE_COLUMNS,
)
from mantidqt.utils.observer_pattern import Observer
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class GroupingTablePresenterTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_ea_tests(self)

        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.model = EAGroupingTabModel(context=self.context)
        self.view = EAGroupingTableView(parent=self.obj)
        self.presenter = EAGroupingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.gui_variable_observer.update = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.groups), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def create_group_workspace_and_load(self):
        grpws = WorkspaceGroup()
        ws_detector1 = "9999; Detector 1"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = "9999; Detector 2"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        ws_detector3 = "9999; Detector 3"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector3))
        run = 9999
        self.context.data_context._loaded_data.add_data(run=[run], workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)

    def test_setup(self):
        self.context.data_context._loaded_data.clear()
        self.assert_model_empty()
        self.assert_view_empty()
        self.create_group_workspace_and_load()
        self.assertEqual(len(self.context.group_context.group_names), 3)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_table_has_six_columns_when_initialized(self):
        self.assertEqual(self.view.num_cols(), 6)

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    def test_add_group_to_view(self):
        self.test_setup()
        self.presenter.add_group_to_view(self.context.group_context._groups[0], True)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 3)

    def test_update_model_from_view(self):
        self.test_setup()
        for group in self.context.group_context._groups:
            self.presenter.add_group_to_view(group, True)
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)

        self.view.remove_last_row()
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(len(self.model.groups), 3)

        self.presenter.update_model_from_view()
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(len(self.model.groups), 2)

    def test_update_view_from_model(self):
        self.test_setup()

        self.presenter.add_group_to_view(self.context.group_context._groups[0], True)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 3)

        self.presenter.update_view_from_model()
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)

    def test_plot_default_case_with_detector_3_present(self):
        grpws = WorkspaceGroup()
        ws_detector1 = "9999; Detector 1"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = "9999; Detector 2"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        ws_detector3 = "9999; Detector 3"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector3))
        ws_detector4 = "9998; Detector 3"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector4))
        run = [9999, 9998]
        self.context.data_context._loaded_data.add_data(run=run, workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[1], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[2], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[3], False)
        self.assertEqual(self.view.num_rows(), 4)
        self.assertEqual(len(self.model.groups), 4)
        analyse_checkbox = self.view.get_table_item(0, 4)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertCountEqual(self.context.group_context.selected_groups, ["9999; Detector 3", "9998; Detector 3"])

    def test_plot_default_case_with_detector_3_not_present(self):
        grpws = WorkspaceGroup()
        ws_detector1 = "9999; Detector 1"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = "9999; Detector 2"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        ws_detector3 = "9999; Detector 4"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector3))
        ws_detector4 = "9998; Detector 1"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector4))
        run = [9999, 9998]
        self.context.data_context._loaded_data.add_data(run=run, workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[1], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[2], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[3], False)
        self.assertEqual(self.view.num_rows(), 4)
        self.assertEqual(len(self.model.groups), 4)
        analyse_checkbox = self.view.get_table_item(0, 4)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertCountEqual(self.context.group_context.selected_groups, ["9999; Detector 1", "9998; Detector 1"])

    def test_plot_default_case_with_detector_1_not_present(self):
        grpws = WorkspaceGroup()
        ws_detector1 = "9999; Detector 4"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = "9998; Detector 2"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        ws_detector3 = "9998; Detector 4"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector3))
        run = [9999, 9998]
        self.context.data_context._loaded_data.add_data(run=run, workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[1], False)
        self.presenter.add_group_to_view(self.context.group_context._groups[2], False)

        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)
        analyse_checkbox = self.view.get_table_item(0, 3)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertCountEqual(self.context.group_context.selected_groups, ["9999; Detector 4", "9998; Detector 4"])

    def test_plot_default_case_with_detector_4_not_present(self):
        grpws = WorkspaceGroup()
        ws_detector2 = "9998; Detector 2"
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        run = [9998]
        self.context.data_context._loaded_data.add_data(run=run, workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)

        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 1)
        analyse_checkbox = self.view.get_table_item(0, 1)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertCountEqual(self.context.group_context.selected_groups, ["9998; Detector 2"])

    def test_remove_selected_rows_in_view_and_model(self):
        self.test_setup()
        for group in self.context.group_context._groups:
            self.presenter.add_group_to_view(group, True)
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)

        groups_remove = ["9999; Detector 1", "9999; Detector 3"]
        groups_remaining = ["9999; Detector 2"]
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])
        self.presenter.remove_selected_rows_in_view_and_model(groups_remove)

        self.assertEqual(self.context.group_context.group_names, groups_remaining)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 1)

    def test_remove_last_row_in_view_and_model(self):
        self.test_setup()
        for group in self.context.group_context._groups:
            self.presenter.add_group_to_view(group, True)
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)
        group_name3 = "9999; Detector 3"
        self.assertTrue(group_name3 in self.model.group_names)

        self.presenter.remove_last_row_in_view_and_model()
        self.assertFalse(group_name3 in self.model.group_names)

    @mock.patch("qtpy.QtWidgets.QTableWidgetItem.text")
    def test_rebin_with_steps_is_called(self, mock_text):
        self.presenter.notify_data_changed = mock.Mock()
        self.presenter.update_view_from_model = mock.Mock()
        self.presenter.update_model_from_view = mock.Mock()
        self.presenter._model.handle_rebin = mock.Mock()
        self.presenter._view.get_table_item = mock.Mock()
        mock_text.return_value = "Steps:3 KeV"
        self.presenter._view.get_table_item.return_value = QTableWidgetItem()
        self.presenter.handle_data_change(1, 5)

        # Tests
        self.presenter._view.get_table_item.assert_has_calls([mock.call(1, 5), mock.call(1, 0)])
        self.assertEqual(self.presenter.notify_data_changed.call_count, 0)
        self.assertEqual(self.presenter.update_view_from_model.call_count, 0)
        self.assertEqual(mock_text.call_count, 2)
        self.presenter._model.handle_rebin.assert_has_calls([mock.call(name="Steps:3 KeV", rebin_type="Fixed", rebin_param=3)])

    @mock.patch("qtpy.QtWidgets.QTableWidgetItem.text")
    def test_rebin_with_variable_is_called(self, mock_text):
        self.presenter.notify_data_changed = mock.Mock()
        self.presenter.update_view_from_model = mock.Mock()
        self.presenter.update_model_from_view = mock.Mock()
        self.presenter._model.handle_rebin = mock.Mock()
        self.presenter._view.get_table_item = mock.Mock()
        mock_text.return_value = "Bin Boundaries:3,2,10"
        self.presenter._view.get_table_item.return_value = QTableWidgetItem()
        self.presenter.handle_data_change(4, 5)

        # Tests
        self.presenter._view.get_table_item.assert_has_calls([mock.call(4, 5), mock.call(4, 0)])
        self.assertEqual(self.presenter.notify_data_changed.call_count, 0)
        self.assertEqual(self.presenter.update_view_from_model.call_count, 0)
        self.assertEqual(mock_text.call_count, 2)
        self.presenter._model.handle_rebin.assert_has_calls(
            [mock.call(name="Bin Boundaries:3,2,10", rebin_type="Variable", rebin_param="3,2,10")]
        )

    def test_validate_group_name(self):
        self.test_setup()
        self.assertTrue(self.presenter.validate_group_name("mock_group_name"))
        self.assertFalse(self.presenter.validate_group_name("9999; Detector 1"))
        self.assertFalse(self.presenter.validate_group_name("invalid_name$%"))

    def test_handle_remove_group_button_clicked_with_nothing_selected(self):
        # setup
        self.presenter._view.get_table_contents = mock.Mock()
        self.presenter._view.num_rows = mock.Mock()
        self.presenter._view.remove_last_row = mock.Mock()
        self.presenter._model.remove_group_from_analysis = mock.Mock()
        self.presenter._model.remove_groups_by_name = mock.Mock()
        self.presenter._view.get_selected_group_names = mock.Mock()
        self.presenter.notify_data_changed = mock.Mock()

        self.presenter._view.get_selected_group_names.return_value = []
        self.presenter._view.num_rows.return_value = 1
        self.presenter._view.get_table_contents.return_value = [["mock_name"]]

        self.presenter.handle_remove_group_button_clicked()

        # Assert Statements
        self.presenter._model.remove_groups_by_name.assert_has_calls([mock.call(["mock_name"])])
        self.assertEqual(self.presenter._view.remove_last_row.call_count, 1)
        self.assertEqual(self.presenter.notify_data_changed.call_count, 1)
        self.presenter._model.remove_group_from_analysis.assert_has_calls([mock.call("mock_name")])

    def test_handle_remove_group_button_clicked_with_a_group_selected(self):
        # setup
        self.presenter._view.remove_selected_groups = mock.Mock()
        self.presenter._model.remove_group_from_analysis = mock.Mock()
        self.presenter._model.remove_groups_by_name = mock.Mock()
        self.presenter._view.get_selected_group_names = mock.Mock()
        self.presenter.notify_data_changed = mock.Mock()

        self.presenter._view.get_selected_group_names.return_value = ["mock_name"]

        self.presenter.handle_remove_group_button_clicked()

        # Assert Statements
        self.presenter._model.remove_groups_by_name.assert_has_calls([mock.call(["mock_name"])])
        self.assertEqual(self.presenter._view.remove_selected_groups.call_count, 1)
        self.assertEqual(self.presenter.notify_data_changed.call_count, 1)
        self.presenter._model.remove_group_from_analysis.assert_has_calls([mock.call("mock_name")])

    def test_handle_data_changed(self):
        # setup
        self.presenter.handle_to_analyse_column_changed = mock.Mock()
        self.presenter.handle_rebin_column_changed = mock.Mock()
        self.presenter.handle_rebin_option_column_changed = mock.Mock()
        self.presenter.handle_update = mock.Mock()
        self.presenter._view.get_table_item = mock.Mock()
        row = 3
        column = 4
        mock_table_item = mock.Mock()
        mock_table_item.text.return_value = "mock_workspace_name"
        self.presenter._view.get_table_item.return_value = mock_table_item
        self.presenter.handle_to_analyse_column_changed.return_value = "mock_value"

        self.presenter.handle_data_change(row, column)

        # Assert statements
        self.presenter.handle_to_analyse_column_changed.assert_called_once_with(column, mock_table_item, "mock_workspace_name")
        self.presenter.handle_rebin_column_changed.assert_called_once_with(column, row, mock_table_item)
        self.presenter.handle_rebin_option_column_changed.assert_called_once_with(column, mock_table_item, "mock_workspace_name")
        self.presenter.handle_update.assert_called_once_with("mock_value")
        self.presenter._view.get_table_item.assert_has_calls(
            [mock.call(row, column), mock.call(row, INVERSE_GROUP_TABLE_COLUMNS["workspace_name"])]
        )

    def test_handle_to_analyse_column_changed_if_column_is_to_analyse(self):
        # setup
        self.presenter.to_analyse_data_checkbox_changed = mock.Mock()
        ws_name = "mock_workspace"
        mock_changed_item = mock.Mock()
        mock_changed_item.checkState.return_value = "mock_state"

        to_analyse_changed = self.presenter.handle_to_analyse_column_changed(
            INVERSE_GROUP_TABLE_COLUMNS["to_analyse"], mock_changed_item, ws_name
        )
        # Assert statements
        # method should return False
        self.assertEqual(to_analyse_changed, True)
        self.presenter.to_analyse_data_checkbox_changed.assert_called_once_with("mock_state", ws_name)
        mock_changed_item.checkState.assert_called_once()

    def test_handle_to_analyse_column_changed_if_column_is_not_to_analyse(self):
        # setup
        self.presenter.to_analyse_data_checkbox_changed = mock.Mock()
        ws_name = "mock_workspace"
        mock_changed_item = mock.Mock()
        mock_changed_item.checkState.return_value = "mock_state"

        to_analyse_changed = self.presenter.handle_to_analyse_column_changed(
            INVERSE_GROUP_TABLE_COLUMNS["workspace_name"], mock_changed_item, ws_name
        )
        # Assert statements
        # method should return True
        self.assertEqual(to_analyse_changed, False)
        self.presenter.to_analyse_data_checkbox_changed.assert_not_called()
        mock_changed_item.checkState.assert_not_called()

    def test_handle_rebin_column_changed_when_rebin_column_is_not_changed(self):
        self.presenter._view.rebin_fixed_chosen = mock.Mock()
        self.presenter._view.rebin_variable_chosen = mock.Mock()
        mock_changed_item = "mock_item"

        self.presenter.handle_rebin_column_changed(INVERSE_GROUP_TABLE_COLUMNS["to_analyse"], 1, mock_changed_item)

        # Assert statements
        self.presenter._view.rebin_variable_chosen.assert_not_called()
        self.presenter._view.rebin_fixed_chosen.assert_not_called()

    def test_handle_rebin_column_changed_when_rebin_fixed_is_chosen(self):
        self.presenter._view.rebin_fixed_chosen = mock.Mock()
        self.presenter._view.rebin_variable_chosen = mock.Mock()
        mock_changed_item = mock.Mock()
        mock_changed_item.text.return_value = REBIN_FIXED_OPTION
        row = 1

        self.presenter.handle_rebin_column_changed(INVERSE_GROUP_TABLE_COLUMNS["rebin"], row, mock_changed_item)

        # Assert statements
        self.presenter._view.rebin_fixed_chosen.assert_called_once_with(row)
        self.presenter._view.rebin_variable_chosen.assert_not_called()
        mock_changed_item.text.assert_called_once()

    def test_handle_rebin_column_changed_when_rebin_variable_is_chosen(self):
        self.presenter._view.rebin_fixed_chosen = mock.Mock()
        self.presenter._view.rebin_variable_chosen = mock.Mock()
        mock_changed_item = mock.Mock()
        mock_changed_item.text.return_value = REBIN_VARIABLE_OPTION
        row = 4

        self.presenter.handle_rebin_column_changed(INVERSE_GROUP_TABLE_COLUMNS["rebin"], row, mock_changed_item)

        # Assert statements
        self.presenter._view.rebin_variable_chosen.assert_called_once_with(row)
        self.presenter._view.rebin_fixed_chosen.assert_not_called()
        mock_changed_item.text.assert_has_calls([mock.call(), mock.call()])

    def test_handle_rebin_option_column_changed_with_wrong_column(self):
        self.presenter._model.handle_rebin = mock.Mock()
        mock_changed_item = mock.Mock()
        ws_name = "mock_name"

        self.presenter.handle_rebin_option_column_changed(INVERSE_GROUP_TABLE_COLUMNS["to_analyse"], mock_changed_item, ws_name)

        # Assert statements
        mock_changed_item.text.assert_not_called()
        self.presenter._view.warning_popup.assert_not_called()
        self.presenter._model.handle_rebin.assert_not_called()

    def test_handle_rebin_option_column_with_a_fixed_step(self):
        self.presenter._model.handle_rebin = mock.Mock()
        mock_changed_item = mock.Mock()
        mock_changed_item.text.return_value = "Steps: 3 KeV"
        ws_name = "mock_name"

        self.presenter.handle_rebin_option_column_changed(INVERSE_GROUP_TABLE_COLUMNS["rebin_options"], mock_changed_item, ws_name)

        # Assert statements
        mock_changed_item.text.assert_called_once()
        self.presenter._view.warning_popup.assert_not_called()
        self.presenter._model.handle_rebin.assert_called_once_with(name=ws_name, rebin_type="Fixed", rebin_param=3.0)

    def test_handle_rebin_option_column_with_a_variable_step(self):
        self.presenter._model.handle_rebin = mock.Mock()
        mock_changed_item = mock.Mock()
        mock_changed_item.text.return_value = "Bin Boundaries: 3,10,8"
        ws_name = "mock_name"

        self.presenter.handle_rebin_option_column_changed(INVERSE_GROUP_TABLE_COLUMNS["rebin_options"], mock_changed_item, ws_name)

        # Assert statements
        mock_changed_item.text.assert_called_once()
        self.presenter._view.warning_popup.assert_not_called()
        self.presenter._model.handle_rebin.assert_called_once_with(name=ws_name, rebin_type="Variable", rebin_param=" 3,10,8")


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
