# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock, TestCase

from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group import EAGroup


class EAGroupContextTest(TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._detector_ws_name_1 = "9999; Detector 1"
        cls._detector_ws_name_2 = "9999; Detector 2"
        cls._detector_ws_1 = CreateSampleWorkspace(StoreInADS=False)
        cls._detector_ws_2 = CreateSampleWorkspace(StoreInADS=False)

        cls._group = WorkspaceGroup()
        cls._group.addWorkspace(cls._detector_ws_1)
        cls._group.addWorkspace(cls._detector_ws_2)

        cls._run = 9999

        cls._context = EAGroupContext()
        cls._loaded_data = MuonLoadData()

    def setUp(self) -> None:
        AnalysisDataService.addOrReplace(self._detector_ws_name_1, self._detector_ws_1)
        AnalysisDataService.addOrReplace(self._detector_ws_name_2, self._detector_ws_2)

    def tearDown(self) -> None:
        self._context.clear()
        self.assertEqual(0, len(self._context.groups))

        self._loaded_data.clear()
        self.assertEqual(0, self._loaded_data.num_items())

    def test_add_new_group(self):
        self._loaded_data.add_data(run=[self._run], workspace=self._group)
        empty_group = []

        new_group = self._context.add_new_group(empty_group, self._loaded_data)

        self.assertEqual(2, len(new_group))

    def test_reset_group_to_default(self):
        self._loaded_data.add_data(run=[self._run], workspace=self._group)

        self._context.reset_group_to_default(self._loaded_data)

        self.assertEqual(2, len(self._context.groups))
        self.assertTrue(self._detector_ws_name_1 in self._context.group_names)
        self.assertTrue(self._detector_ws_name_2 in self._context.group_names)

    def test_remove_group(self):
        self._loaded_data.add_data(run=[self._run], workspace=self._group)
        self._context.reset_group_to_default(self._loaded_data)

        self._context.remove_group(self._detector_ws_name_1)

        self.assertEqual(1, len(self._context.groups))
        self.assertFalse(self._detector_ws_name_1 in self._context.group_names)

    def test_clear(self):
        self._loaded_data.add_data(run=[self._run], workspace=self._group)
        self._context.reset_group_to_default(self._loaded_data)

        self._context.clear()

        self.assertEqual(0, len(self._context.groups))

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_rebinned_workspace_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup(self._detector_ws_name_1, "detector 1", "9999")
        self._context.add_group(mock_group)

        self._context.remove_workspace_from_group("9999; Detector 1_EA_Rebinned_Fixed")

        mock_remove_rebinned.assert_called_once()
        mock_remove_peak.assert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_peak_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup(self._detector_ws_name_1, "detector 1", "9999")
        self._context.add_group(mock_group)

        self._context.remove_workspace_from_group("9999; Detector 1_EA_peak_table")

        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_called_once()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_matches_table_from_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup(self._detector_ws_name_1, "detector 1", "9999")
        self._context.add_group(mock_group)

        self._context.remove_workspace_from_group("9999; Detector 1_EA_matches")

        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_called_once()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_rebinned_workspace")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_peak_table")
    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.EAGroup.remove_matches_group")
    def test_remove_workspace_from_group_when_not_in_group(self, mock_remove_matches, mock_remove_peak, mock_remove_rebinned):
        mock_group = EAGroup(self._detector_ws_name_1, "detector 1", "9999")
        self._context.add_group(mock_group)

        self._context.remove_workspace_from_group("mock_workspace")

        mock_remove_rebinned.assert_not_called()
        mock_remove_peak.aassert_not_called()
        mock_remove_matches.assert_not_called()

    @mock.patch("mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group.remove_ws_if_present")
    def test_error_raised_when_deleting_EAGroup(self, mock_remove_ws):
        self._loaded_data.add_data(run=[self._run], workspace=self._group)
        self._context.reset_group_to_default(self._loaded_data)
        mock_remove_ws.side_effect = ValueError("mock error")
        error_notifier_mock = mock.Mock()
        self._context[self._detector_ws_name_1].error_notifier = error_notifier_mock

        self._context.remove_group(self._detector_ws_name_1)

        self.assertFalse(self._detector_ws_name_1 in self._context.group_names)
        mock_remove_ws.assert_called_once_with(self._detector_ws_name_1)
        error_notifier_mock.notify_subscribers.assert_called_once_with(
            f"Unexpected error occurred when deleting group {self._detector_ws_name_1}: mock error"
        )


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
