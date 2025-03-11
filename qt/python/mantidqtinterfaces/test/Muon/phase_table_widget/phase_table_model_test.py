# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_model import PhaseTableModel
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup


class PhaseTableModelTest(unittest.TestCase):
    def setUp(self):
        context = setup_context()
        self.model = PhaseTableModel(context)

        forward_group = MuonGroup(group_name="fwd", detector_ids=[1, 3, 5, 7, 9])
        backward_group = MuonGroup(group_name="bwd", detector_ids=[2, 4, 6, 8, 10])
        self.model.group_pair_context.add_group(forward_group)
        self.model.group_pair_context.add_group(backward_group)

    def test_create_parameters_for_cal_muon_phase_returns_correct_parameter_dict(self):
        workspace_name = "input_workspace_name_raw_data"
        self.model.phase_context.options_dict["input_workspace"] = workspace_name

        result = self.model.create_parameters_for_cal_muon_phase_algorithm()

        self.assertEqual(
            result,
            {
                "BackwardSpectra": [2, 4, 6, 8, 10],
                "FirstGoodData": 0.1,
                "ForwardSpectra": [1, 3, 5, 7, 9],
                "InputWorkspace": workspace_name,
                "LastGoodData": 15,
                "DetectorTable": "input_workspace_name; PhaseTable; fwd; bwd",
            },
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_model.MuonWorkspaceWrapper")
    def test_that_phase_table_added_to_ADS_with_correct_name_and_group(self, mock_workspace_wrapper):
        workspace_wrapper = mock.MagicMock()
        mock_workspace_wrapper.return_value = workspace_wrapper

        self.model.add_phase_table_to_ads("MUSR22222_period_1; PhaseTable")

        mock_workspace_wrapper.assert_called_once_with("MUSR22222 MA/MUSR22222_period_1; PhaseTable")
        workspace_wrapper.show.assert_called_once_with()
