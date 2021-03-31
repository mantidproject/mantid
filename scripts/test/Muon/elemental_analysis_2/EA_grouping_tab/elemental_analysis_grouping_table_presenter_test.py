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

from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter import EAGroupingTablePresenter
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import EAGroupingTableView
from mantidqt.utils.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


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
        ws_detector1 = '9999; Detector 1'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = '9999; Detector 2'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        ws_detector3 = '9999; Detector 3'
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

    def test_plot_default_case(self):
        self.test_setup()
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 3)
        analyse_checkbox = self.view.get_table_item(0, 3)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertEqual(analyse_checkbox.checkState(), 2)

    def test_remove_selected_rows_in_view_and_model(self):
        self.test_setup()
        for group in self.context.group_context._groups:
            self.presenter.add_group_to_view(group, True)
        self.assertEqual(self.view.num_rows(), 3)
        self.assertEqual(len(self.model.groups), 3)

        groups_remove = ['9999; Detector 1', '9999; Detector 3']
        groups_remaining = ['9999; Detector 2']
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
        group_name3 = '9999; Detector 3'
        self.assertTrue(group_name3 in self.model.group_names)

        self.presenter.remove_last_row_in_view_and_model()
        self.assertFalse(group_name3 in self.model.group_names)

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.get_table_item")
    @mock.patch("qtpy.QtWidgets.QTableWidgetItem.text")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model.EAGroupingTabModel.handle_rebin")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.update_model_from_view")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.notify_data_changed")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.update_view_from_model")
    def test_rebin_with_steps_is_called(self, mock_update_view, mock_notify_data, mock_update_model, mock_handle_rebin,
                                        mock_text, mock_get_table_item):
        mock_text.return_value = "Steps:3"
        mock_get_table_item.return_value = QTableWidgetItem()
        self.presenter.handle_data_change(1, 5)

        # Tests
        mock_get_table_item.assert_has_calls([mock.call(1, 5), mock.call(1, 0)])
        self.assertEqual(mock_update_model.call_count, 1)
        self.assertEqual(mock_notify_data.call_count, 0)
        self.assertEqual(mock_update_view.call_count, 0)
        self.assertEqual(mock_text.call_count, 2)
        mock_handle_rebin.assert_has_calls([mock.call(name="Steps:3", rebin_type="Fixed", rebin_param=3)])

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.get_table_item")
    @mock.patch("qtpy.QtWidgets.QTableWidgetItem.text")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model.EAGroupingTabModel.handle_rebin")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.update_model_from_view")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.notify_data_changed")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.update_view_from_model")
    def test_rebin_with_variable_is_called(self, mock_update_view, mock_notify_data, mock_update_model,
                                           mock_handle_rebin,
                                           mock_text, mock_get_table_item):
        mock_text.return_value = "Bin Boundaries:3,2,10"
        mock_get_table_item.return_value = QTableWidgetItem()
        self.presenter.handle_data_change(4, 5)

        # Tests
        mock_get_table_item.assert_has_calls([mock.call(4, 5), mock.call(4, 0)])
        self.assertEqual(mock_update_model.call_count, 1)
        self.assertEqual(mock_notify_data.call_count, 0)
        self.assertEqual(mock_update_view.call_count, 0)
        self.assertEqual(mock_text.call_count, 2)
        mock_handle_rebin.assert_has_calls([mock.call(name="Bin Boundaries:3,2,10", rebin_type="Variable",
                                                      rebin_param="3,2,10")])

    def test_validate_group_name(self):
        self.test_setup()
        self.assertTrue(self.presenter.validate_group_name("mock_group_name"))
        self.assertFalse(self.presenter.validate_group_name('9999; Detector 1'))
        self.assertFalse(self.presenter.validate_group_name('invalid_name$%'))

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.num_rows")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.get_table_contents")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.remove_last_row")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model."
                "EAGroupingTabModel.remove_group_from_analysis")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model."
                "EAGroupingTabModel.remove_groups_by_name")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.get_selected_group_names")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.notify_data_changed")
    def test_handle_remove_group_button_clicked_with_nothing_selected(self,
                                                                      mock_notify_data_changed,
                                                                      mock_get_selected_group_names,
                                                                      mock_remove_groups_by_name,
                                                                      mock_remove_group_from_analysis,
                                                                      mock_remove_last_row, mock_get_table_contents,
                                                                      mock_num_rows):

        mock_get_selected_group_names.return_value = []
        mock_num_rows.return_value = 1
        mock_get_table_contents.return_value = [["mock_name"]]

        self.presenter.handle_remove_group_button_clicked()

        # Assert Statements
        mock_remove_groups_by_name.assert_has_calls([mock.call(["mock_name"])])
        self.assertEqual(mock_remove_last_row.call_count, 1)
        self.assertEqual(mock_notify_data_changed.call_count, 1)
        mock_remove_group_from_analysis.assert_has_calls([mock.call("mock_name")])

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.remove_selected_groups")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model."
                "EAGroupingTabModel.remove_group_from_analysis")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model."
                "EAGroupingTabModel.remove_groups_by_name")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view."
                "EAGroupingTableView.get_selected_group_names")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter."
                "EAGroupingTablePresenter.notify_data_changed")
    def test_handle_remove_group_button_clicked_with_a_group_selected(self, mock_notify_data_changed,
                                                                      mock_get_selected_group_names,
                                                                      mock_remove_groups_by_name,
                                                                      mock_remove_group_from_analysis,
                                                                      mock_remove_selected_groups):

        mock_get_selected_group_names.return_value = ["mock_name"]

        self.presenter.handle_remove_group_button_clicked()

        # Assert Statements
        mock_remove_groups_by_name.assert_has_calls([mock.call(["mock_name"])])
        self.assertEqual(mock_remove_selected_groups.call_count, 1)
        self.assertEqual(mock_notify_data_changed.call_count, 1)
        mock_remove_group_from_analysis.assert_has_calls([mock.call("mock_name")])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
