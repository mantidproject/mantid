# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from mantid import config
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateWorkspace
from testhelpers import create_algorithm

from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCreateTransmission import Prop


class ReflectometryISISCreateTransmissionTest(unittest.TestCase):
    _CONFIG_KEY_FACILITY = "default.facility"
    _CONFIG_KEY_INST = "default.instrument"

    _OUTPUT_WS_NAME = "out_ws"

    _LOAD_ALG = "LoadAndMerge"
    _FLOOD_ALG = "ApplyFloodWorkspace"
    _BACK_SUB_ALG = "ReflectometryBackgroundSubtraction"
    _TRANS_WS_ALG = "CreateTransmissionWorkspaceAuto"

    def setUp(self):
        self._oldFacility = config[self._CONFIG_KEY_FACILITY]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config[self._CONFIG_KEY_INST]
        config[self._CONFIG_KEY_FACILITY] = "ISIS"
        config[self._CONFIG_KEY_INST] = "POLREF"

    def tearDown(self):
        AnalysisDataService.clear()
        config[self._CONFIG_KEY_FACILITY] = self._oldFacility
        config[self._CONFIG_KEY_INST] = self._oldInstrument

    def test_correct_output(self):
        output_ws = self._run_algorithm(self._create_args("INTER13460", back_sub_roi=""))
        self.assertIsNotNone(output_ws)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        expected_history = [self._LOAD_ALG, self._FLOOD_ALG, self._TRANS_WS_ALG]
        self._check_history(output_ws, expected_history)
        self._check_output_data(output_ws, 771, 1.9796095404641e-07, 1.049193056445973e-05)

    def test_correct_output_for_multiple_runs(self):
        output_ws = self._run_algorithm(self._create_args(["INTER13463", "INTER13464"], back_sub_roi=""))
        self.assertIsNotNone(output_ws)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        expected_history = [self._LOAD_ALG, self._FLOOD_ALG, self._TRANS_WS_ALG]
        self._check_history(output_ws, expected_history)
        self._check_output_data(output_ws, 771, 0.018547099002195058, 1.2884763705979813e-05)

    def test_correct_output_for_workspace_groups(self):
        expected_history = [
            self._LOAD_ALG,
            self._FLOOD_ALG,
            self._FLOOD_ALG,
            self._BACK_SUB_ALG,
            self._BACK_SUB_ALG,
            self._TRANS_WS_ALG,
            self._TRANS_WS_ALG,
        ]
        output_grp = self._run_workspace_group_test(expected_history=expected_history)
        self._check_output_data(output_grp[0], 905, -8.846391366193592e-10, -3.216869587706761e-10)
        self._check_output_data(output_grp[1], 905, -4.209521594229452e-10, -3.3676172753835613e-10)

    def test_output_workspace_group_returned_when_run_as_child(self):
        alg = self._setup_algorithm(self._create_args("14966"))
        alg.setChild(True)
        alg.execute()
        output_ws = alg.getProperty(Prop.OUTPUT_WS).value

        self.assertIsNotNone(output_ws)
        self.assertIsInstance(output_ws, WorkspaceGroup)

    def test_flood_correction_skipped_if_not_requested(self):
        expected_history = [self._LOAD_ALG, self._BACK_SUB_ALG, self._BACK_SUB_ALG, self._TRANS_WS_ALG, self._TRANS_WS_ALG]
        self._run_workspace_group_test(perform_flood=False, expected_history=expected_history)

    def test_background_subtraction_skipped_if_not_requested(self):
        expected_history = [self._LOAD_ALG, self._FLOOD_ALG, self._FLOOD_ALG, self._TRANS_WS_ALG, self._TRANS_WS_ALG]
        self._run_workspace_group_test(back_sub_roi="", expected_history=expected_history)

    def _create_args(self, input_run, perform_flood=True, back_sub_roi="100-200"):
        args = {
            Prop.INPUT_RUNS: input_run,
            Prop.TRANS_ROI: "4",
            Prop.I0_MON_IDX: "2",
            Prop.MON_WAV_MIN: "2.5",
            Prop.MON_WAV_MAX: "10.0",
            Prop.OUTPUT_WS: self._OUTPUT_WS_NAME,
        }

        if perform_flood:
            args[Prop.FLOOD_WS] = self._create_flood_ws()

        if back_sub_roi:
            args[Prop.BACK_SUB_ROI] = back_sub_roi

        return args

    @staticmethod
    def _create_flood_ws():
        """Creates a MatrixWorkspace with a single bin of data. The workspace has 256 spectra with values from
        0.0 to 2.56 in steps of ~0.01"""
        flood_ws = CreateWorkspace(DataX=[0.0, 1.0], DataY=np.linspace(0.0, 2.56, 256), NSpec=256, UnitX="TOF")
        return flood_ws

    @staticmethod
    def _setup_algorithm(args):
        alg = create_algorithm("ReflectometryISISCreateTransmission", **args)
        alg.setRethrows(True)
        return alg

    def _run_algorithm(self, args):
        alg = self._setup_algorithm(args)
        alg.execute()
        return AnalysisDataService.retrieve(self._OUTPUT_WS_NAME)

    def _check_history(self, ws, expected):
        """Check if the expected algorithm names are found in the workspace history"""
        history = ws.getHistory()
        self.assertFalse(history.empty())
        child_alg_histories = history.getAlgorithmHistory(history.size() - 1).getChildHistories()
        self.assertEqual([alg.name() for alg in child_alg_histories], expected)

    def _run_workspace_group_test(self, expected_history, perform_flood=True, back_sub_roi="100-200"):
        # Omitting the instrument prefix from the run number should use the default instrument
        output_grp = self._run_algorithm(self._create_args("14966", perform_flood, back_sub_roi))
        self.assertIsInstance(output_grp, WorkspaceGroup)
        self.assertEqual(output_grp.size(), 2)
        self._check_history(output_grp[0], expected_history)
        return output_grp

    def _check_output_data(self, ws, expected_num_bins, expected_first_y, expected_last_y):
        self.assertEqual(ws.getNumberHistograms(), 1)
        data_y = ws.readY(0)
        self.assertEqual(data_y.size, expected_num_bins)
        self.assertAlmostEqual(data_y[0], expected_first_y, places=12)
        self.assertAlmostEqual(data_y[-1], expected_last_y, places=12)


if __name__ == "__main__":
    unittest.main()
