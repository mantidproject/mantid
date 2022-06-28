# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group import EAGroup


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

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_rebinned_workspace_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_Rebinned_Fixed')
        mock_remove_rebinned.assert_called_once()
        mock_remove_peak.assert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_peak_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_peak_table')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_called_once()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_matches_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('9999; Detector 1_EA_matches')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_called_once()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_workspace_from_group_when_not_in_group(self, mock_remove_matches, mock_remove_peak,
                                                           mock_remove_rebinned):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.context.add_group(mock_group)
        self.context.remove_workspace_from_group('mock_workspace')
        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.remove_ws_if_present")
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
