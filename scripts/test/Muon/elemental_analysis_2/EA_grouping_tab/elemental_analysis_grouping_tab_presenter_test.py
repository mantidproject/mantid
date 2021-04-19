# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateSampleWorkspace
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantid.api import WorkspaceGroup
from qtpy.QtWidgets import QWidget

from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter import EAGroupingTabPresenter
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_view import EAGroupingTabView
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter import EAGroupingTablePresenter
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import EAGroupingTableView
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class EAGroupingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.obj = QWidget()

        self.loaded_data = MuonLoadData()

        setup_context_for_ea_tests(self)

        self.model = EAGroupingTabModel(context=self.context)

        self.grouping_table_view = EAGroupingTableView()
        self.grouping_table_widget = EAGroupingTablePresenter(self.grouping_table_view, self.model)

        self.grouping_table_view.warning_popup = mock.MagicMock()

        self.view = EAGroupingTabView(self.grouping_table_view)
        self.presenter = EAGroupingTabPresenter(self.view, self.model,
                                                self.grouping_table_widget)

        self.presenter.create_update_thread = mock.MagicMock(return_value=mock.MagicMock())
        self.view.display_warning_box = mock.MagicMock()
        self.grouping_table_view.warning_popup = mock.MagicMock()

    def create_group_workspace_and_load(self):
        grpws = WorkspaceGroup()
        ws_detector1 = '9999; Detector 1'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = '9999; Detector 2'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        run = 9999
        self.context.data_context._loaded_data.add_data(run=[run], workspace=grpws)
        loaded_data = self.context.data_context._loaded_data
        self.context.group_context.reset_group_to_default(loaded_data)

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_load_grouping_inserts_loaded_groups_correctly(self):
        self.context.data_context._loaded_data.clear()
        self.create_group_workspace_and_load()
        self.presenter.handle_new_data_loaded()
        detector3_loadedData = str(self.context.data_context._loaded_data.get_data(run=[9999])["workspace"][0])
        detector3_table = self.grouping_table_view.get_table_item_text(0, 0)

        self.assertEqual(self.grouping_table_view.num_rows(), 2)
        self.assertEqual(detector3_table, detector3_loadedData)

    def test_clear(self):
        self.context.data_context._loaded_data.clear()
        self.create_group_workspace_and_load()
        self.presenter.handle_new_data_loaded()

        self.assertEqual(self.grouping_table_view.num_rows(), 2)

        self.presenter.on_clear_requested()

        self.assertEqual(self.grouping_table_view.num_rows(), 0)

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model.EAGroupingTabModel.is_data_loaded")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter.EAGroupingTabPresenter."
                "update_view_from_model")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter.EAGroupingTabPresenter."
                "update_description_text")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter.EAGroupingTabPresenter."
                "plot_default_groups")
    def test_handle_data_loaded_when_data_loaded(self, mock_plot_default_groups, mock_update_text, mock_update_view,
                                                 mock_is_data_loaded):
        mock_is_data_loaded.return_value = True

        self.presenter.handle_new_data_loaded()

        # Assert statements
        mock_update_text.assert_called_once()
        mock_update_view.assert_called_once()
        mock_plot_default_groups.assert_called_once()

    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model.EAGroupingTabModel.is_data_loaded")
    @mock.patch("Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter.EAGroupingTabPresenter."
                "on_clear_requested")
    def test_handle_data_loaded_when_data_not_loaded(self, mock_on_clear_requested, mock_is_data_loaded):
        mock_is_data_loaded.return_value = False

        self.presenter.handle_new_data_loaded()

        # Assert statements
        mock_on_clear_requested.assert_called_once()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
