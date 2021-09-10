# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from unittest import mock
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, CreateWorkspace
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext, REBINNED_VARIABLE_WS_SUFFIX
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, remove_ws_if_present


class ElementalAnalysisContextTest(unittest.TestCase):
    def setUp(self):
        self.context = ElementalAnalysisContext(data_context=DataContext(), ea_group_context=EAGroupContext(),
                                                muon_gui_context=MuonGuiContext)

    def assert_workspace_equal(self, workspace1, workspace2):
        self.assertEqual(workspace1.getNumberHistograms(), workspace2.getNumberHistograms())
        for i in range(workspace1.getNumberHistograms()):
            self.assertTrue(np.array_equal(workspace1.readX(i), workspace2.readX(i)))
            self.assertTrue(np.array_equal(workspace1.readY(i), workspace2.readY(i)))
            self.assertTrue(np.array_equal(workspace1.readE(i), workspace2.readE(i)))

    def assert_context_empty(self):
        self.assertEqual(len(self.context.data_context.current_runs), 0)
        self.assertEqual(len(self.context.group_context.groups), 0)
        self.assertEqual(len(self.context.data_context._loaded_data.params), 0)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_name(self):
        self.assertEqual(self.context.name, "Elemental Analysis 2")

    @mock.patch('Muon.GUI.ElementalAnalysis2.context.context.retrieve_ws')
    @mock.patch('Muon.GUI.ElementalAnalysis2.context.context.remove_ws_if_present')
    @mock.patch('Muon.GUI.ElementalAnalysis2.context.ea_group_context.EAGroupContext.__getitem__')
    def test_rebin(self, mock_get_item, mock_remove_ws, mock_retrieve_ws):
        mock_get_item.return_value = EAGroup("9999; Detector 1", "detector 1", "9999")
        name = '9999; Detector 1'
        rebinned_name = '9999; Detector 1' + REBINNED_VARIABLE_WS_SUFFIX
        mock_params = "0, 2, 9"

        x_data = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        y_data = [1, 1, 1, 1, 1, 1, 1, 1, 1]

        CreateWorkspace(OutputWorkspace=name, DataX=x_data, DataY=y_data)
        self.context._run_rebin("9999; Detector 1", "Variable", mock_params)

        correct_data = CreateWorkspace(OutputWorkspace="correct_data", DataX=[0, 2, 4, 6, 8, 9], DataY=[2, 2, 2, 2, 1])

        # Assert Statements
        self.assert_workspace_equal(correct_data, retrieve_ws(rebinned_name))
        mock_remove_ws.assert_has_calls([mock.call(rebinned_name)])
        mock_retrieve_ws.assert_has_calls([mock.call("9999")])
        mock_get_item.assert_has_calls([mock.call(name)])

        # clean up
        remove_ws_if_present(name)
        remove_ws_if_present("correct_data")
        remove_ws_if_present(rebinned_name)

    def test_clear_group(self):
        # setup
        self.assert_context_empty()
        self.context.data_context.current_runs.append("mock_run_1")
        self.context.data_context.current_runs.append("mock_run_2")
        self.context.group_context.add_group(EAGroup("9999; Detector 1", "detector 1", "9999"))
        self.context.group_context.add_group(EAGroup("9999; Detector 2", "detector 2", "9999"))
        self.context.data_context._loaded_data.add_data(workspace="mock_workspace_1")
        self.context.data_context._loaded_data.add_data(workspace="mock_workspace_2")

        # check if context is not empty
        with self.assertRaises(AssertionError):
            self.assert_context_empty()

        # call clear function
        self.context.clear_context()

        # check context is empty
        self.assert_context_empty()

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_remove_workspace_with_a_string(self, mock_notify_subscirbers):
        mock_group = EAGroup("mock_workspace", "detector 1", "9999")
        self.context.group_context.add_group(mock_group)
        self.context.group_context.remove_group = mock.Mock()
        # call remove_workspace function
        self.context.remove_workspace("mock_workspace")

        # assert statement
        mock_notify_subscirbers.assert_has_calls([mock.call("mock_workspace"), mock.call("mock_workspace")])
        self.context.group_context.remove_group.assert_called_once_with("mock_workspace")

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_remove_workspace_with_a_workspace(self, mock_notify_subscirbers):
        # setup
        self.context.group_context.remove_group = mock.Mock()
        mock_group = EAGroup("mock_workspace", "detector 1", "9999")
        self.context.group_context.add_group(mock_group)
        mock_ws = CreateWorkspace(OutputWorkspace="mock_workspace", DataX=[0, 2, 4, 6, 8, 9], DataY=[2, 2, 2, 2, 1])

        # call remove_workspace function
        self.context.remove_workspace(mock_ws)

        # assert statement
        mock_notify_subscirbers.assert_has_calls([mock.call("mock_workspace"), mock.call(mock_ws)])
        self.context.group_context.remove_group.assert_called_once_with("mock_workspace")

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_remove_workspace_with_a_rebinned_workspace(self, mock_notify_subscirbers):
        self.context.group_context.remove_workspace_from_group = mock.Mock()
        # call remove_workspace function
        self.context.remove_workspace("rebinned_mock_workspace")

        # assert statement
        mock_notify_subscirbers.assert_has_calls([mock.call("rebinned_mock_workspace")])
        self.context.group_context.remove_workspace_from_group.assert_called_once_with("rebinned_mock_workspace")

    @mock.patch("mantidqt.utils.observer_pattern.GenericObservable.notify_subscribers")
    def test_remove_workspace_with_a_string_and_not_present_in_group(self, mock_notify_subscirbers):
        self.context.group_context.remove_workspace_from_group = mock.Mock()
        # call remove_workspace function
        self.context.remove_workspace("mock_workspace")

        # assert statement
        mock_notify_subscirbers.assert_has_calls([mock.call("mock_workspace"), mock.call("mock_workspace")])
        self.context.group_context.remove_workspace_from_group.assert_called_once_with("mock_workspace")

    def test_update_current_data_with_empty_data_context(self):
        # check if data context is empty
        self.assertEqual(len(self.context.data_context.current_runs), 0)
        self.context.data_context.clear = mock.Mock()

        # call update_current_data
        self.context.update_current_data()

        self.context.data_context.clear.assert_called_once()

    def test_update_current_data_with_empty_group_context(self):
        # check if group context is empty
        self.assertEqual(len(self.context.group_context.groups), 0)
        self.context.group_context.reset_group_to_default = mock.Mock()

        # add run to data context
        self.context.data_context.current_runs.append("mock_run_1")

        # call update_current_data
        self.context.update_current_data()

        # assert statement
        self.context.group_context.reset_group_to_default.assert_called_once_with(
            self.context.data_context._loaded_data)

    def test_update_current_data_with_populated_group_context(self):
        self.context.group_context.add_new_group = mock.Mock()
        # add run to data context and group to group context
        self.context.data_context.current_runs.append("mock_run_1")
        self.context.group_context.groups.append("mock_group")

        # call update_current_data
        self.context.update_current_data()

        # assert statement
        self.context.group_context.add_new_group.assert_called_once_with(self.context.group_context.groups,
                                                                         self.context.data_context._loaded_data)


class DataContextTest(unittest.TestCase):
    def setUp(self):
        self.context = DataContext()

    def assert_data_context_empty(self):
        self.assertEqual(self.context.current_runs, [])
        self.assertEqual(self.context.run_info, [])

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_add_run_object_to_run_info(self):
        self.assert_data_context_empty()
        new_run_object = mock.Mock(run_number=1234)
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)
        self.assertEqual(self.context.run_info[0].run_number, 1234)

    def test_clear_run_info(self):
        self.assert_data_context_empty()
        new_run_object = mock.Mock()
        self.context.run_info_update(new_run_object)
        self.assertEqual(len(self.context.run_info), 1)

        self.context.clear_run_info()
        self.assert_data_context_empty()


class EAGroupContextTest(unittest.TestCase):
    def setUp(self):
        self.context = EAGroupContext()
        self.loadedData = MuonLoadData()

    def create_group_workspace_and_load(self):
        grpws = WorkspaceGroup()
        ws_detector1 = '9999; Detector 1'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector1))
        ws_detector2 = '9999; Detector 2'
        grpws.addWorkspace(CreateSampleWorkspace(OutputWorkspace=ws_detector2))
        run = 9999
        self.loadedData.add_data(run=[run], workspace=grpws)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_add_new_group(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        empty_group = []
        new_group = self.context.add_new_group(empty_group, self.loadedData)
        self.assertEqual(len(new_group), 2)

    def test_remove_group(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)
        group_name1 = '9999; Detector 1'
        self.assertTrue(group_name1 in self.context.group_names)

        self.context.remove_group(group_name1)
        self.assertFalse(group_name1 in self.context.group_names)

    def test_reset_group_to_default(self):
        self.loadedData.clear()
        self.assertEqual(self.loadedData.num_items(), 0)
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)

    def test_clear(self):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)

        self.context.clear()
        self.assertEqual(len(self.context.groups), 0)

    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_rebinned_workspace_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_Rebinned_Fixed')
        mock_remove_rebinned.assert_called_once()
        mock_remove_peak.assert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_peak_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_peak_table')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_called_once()
        mock_remove_matches.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_matches_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_matches')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_called_once()

    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_workspace_from_group_when_not_in_group(self, mock_remove_matches, mock_remove_peak,
                                                           mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('mock_workspace')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.ea_group.remove_ws_if_present")
    def test_error_raised_when_deleting_EAGroup(self, mock_remove_ws):
        self.loadedData.clear()
        self.create_group_workspace_and_load()
        self.context.reset_group_to_default(self.loadedData)
        self.assertEqual(len(self.context.groups), 2)
        group_name1 = '9999; Detector 1'
        self.assertTrue(group_name1 in self.context.group_names)
        mock_remove_ws.side_effect = ValueError("mock error")
        error_notifier_mock = mock.Mock()
        self.context[group_name1].error_notifier = error_notifier_mock

        self.context.remove_group(group_name1)
        self.assertFalse(group_name1 in self.context.group_names)
        mock_remove_ws.assert_called_once_with(group_name1)
        error_notifier_mock.notify_subscribers.assert_called_once_with(f"Unexpected error occurred when"
                                                                       f" deleting group {group_name1}: mock error")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
