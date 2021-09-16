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

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_widget_presenter import EAGroupingTabPresenter
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_view import EAGroupingTabView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_presenter import EAGroupingTablePresenter
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import EAGroupingTableView
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


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

    def test_handle_data_loaded_when_data_loaded(self):
        self.presenter._model.is_data_loaded = mock.Mock()
        self.presenter._model.is_data_loaded.return_value = True
        self.presenter.update_view_from_model = mock.Mock()
        self.presenter.update_description_text = mock.Mock()
        self.presenter.plot_default_groups = mock.Mock()

        self.presenter.handle_new_data_loaded()

        # Assert statements
        self.presenter.update_view_from_model.assert_called_once()
        self.presenter.update_description_text.assert_called_once()
        self.presenter.plot_default_groups.assert_called_once()

    def test_handle_data_loaded_when_data_not_loaded(self):
        self.presenter._model.is_data_loaded = mock.Mock()
        self.presenter._model.is_data_loaded.return_value = False
        self.presenter.on_clear_requested = mock.Mock()

        self.presenter.handle_new_data_loaded()

        # Assert statements
        self.presenter.on_clear_requested.assert_called_once()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
