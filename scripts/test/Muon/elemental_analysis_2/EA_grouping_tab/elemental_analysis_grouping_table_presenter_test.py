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
from qtpy.QtWidgets import QWidget

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
        run = 9999
        self.context.data_context._loaded_data.add_data(run=[run], workspace=grpws)
        loadedData = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loadedData)

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
        self.context.data_context._loaded_data.clear()
        self.assert_model_empty()
        self.assert_view_empty()
        self.create_group_workspace_and_load()
        self.presenter.add_group_to_view(self.context.group_context._groups[0], True)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 2)

    def test_update_model_from_view(self):
        self.context.data_context._loaded_data.clear()
        self.assert_model_empty()
        self.assert_view_empty()
        self.create_group_workspace_and_load()
        for group in self.context.group_context._groups:
            self.presenter.add_group_to_view(group, True)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(len(self.model.groups), 2)

        self.view.remove_last_row()
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 2)

        self.presenter.update_model_from_view()
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 1)

    def test_update_view_from_model(self):
        self.context.data_context._loaded_data.clear()
        self.assert_model_empty()
        self.assert_view_empty()

        self.create_group_workspace_and_load()
        self.presenter.add_group_to_view(self.context.group_context._groups[0], True)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 2)

        self.presenter.update_view_from_model()
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(len(self.model.groups), 2)

    def test_plot_default_case(self):
        self.context.data_context._loaded_data.clear()
        self.assert_model_empty()
        self.assert_view_empty()

        self.create_group_workspace_and_load()
        self.presenter.add_group_to_view(self.context.group_context._groups[0], False)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 2)
        analyse_checkbox = self.view.get_table_item(0, 3)
        self.assertEqual(analyse_checkbox.checkState(), 0)

        self.presenter.plot_default_case()
        self.assertEqual(analyse_checkbox.checkState(), 2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
