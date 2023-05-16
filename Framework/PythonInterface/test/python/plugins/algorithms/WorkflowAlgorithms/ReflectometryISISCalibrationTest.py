# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import csv
import math
import unittest

from mantid import FileFinder
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from testhelpers import assertRaisesNothing, create_algorithm, WorkspaceCreationHelper


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data.dat")
    _RAD_TO_DEG = 180.0 / math.pi
    _DEG_TO_RAD = math.pi / 180.0

    @classmethod
    def setUpClass(cls):
        def _create_calibration_data_dictionary():
            # Create dictionary of detector ID and theta offset from test data
            # This assumes that the columns in the test data will be ordered with detectorid first and theta_offset second
            det_theta_offset = {}
            with open(cls._CALIBRATION_TEST_DATA, "r") as file:
                reader = csv.reader(file)
                for row in reader:
                    if len(row) == 0:
                        continue

                    entries = row[0].split()

                    first_character = entries[0][0].lower()
                    if first_character in ["#", "d"]:
                        # Ignore header lines and column labels
                        continue

                    det_theta_offset[int(entries[0])] = float(entries[1])
            return det_theta_offset

        cls.calibration_data = _create_calibration_data_dictionary()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_calibration_successful(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)

        output_ws_name = "test_calibrated"
        args = {"InputWorkspace": ws, "CalibrationFile": self._CALIBRATION_TEST_DATA, "OutputWorkspace": output_ws_name}
        outputs = [input_ws_name, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_final_theta_values(ws, output_ws)

    def test_calibration_successful_for_workspace_group(self):
        grp_name = "test"
        output_grp_name = "test_calibrated"
        input_ws_grp_size = 2
        ws_grp = self._create_workspace_group(grp_name, input_ws_grp_size)

        input_ws_names = []
        output_ws_names = []
        for i in range(input_ws_grp_size):
            ws = ws_grp[i]
            input_ws_names.append(ws.name())
            output_ws_names.append(f"{output_grp_name}_{i + 1}")

        args = {"InputWorkspace": ws_grp, "CalibrationFile": self._CALIBRATION_TEST_DATA, "OutputWorkspace": output_grp_name}
        outputs = input_ws_names + [grp_name, output_grp_name] + output_ws_names
        self._assert_run_algorithm_succeeds(args, outputs)

        output_grp = AnalysisDataService.retrieve(output_grp_name)
        self.assertIsInstance(output_grp, WorkspaceGroup)
        self.assertEqual(output_grp.getNumberOfEntries(), input_ws_grp_size)
        for i in range(input_ws_grp_size):
            self._check_final_theta_values(ws_grp[i], output_grp[i])

    def test_detectors_in_calibration_file_but_not_workspace_raises_exception(self):
        ws = self._create_sample_workspace_with_missing_detectors()
        args = {"InputWorkspace": ws, "CalibrationFile": self._CALIBRATION_TEST_DATA, "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, "Detector id \d+ from calibration file cannot be found in input workspace")

    def test_exception_raised_if_no_calibration_file_supplied(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, "Calibration file path must be provided")

    def test_exception_raised_if_invalid_calibration_filepath_supplied(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": "invalid/file_path.dat", "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, "Calibration file path cannot be found")

    def _check_final_theta_values(self, input_ws, output_ws):
        info_in = input_ws.spectrumInfo()
        info_out = output_ws.spectrumInfo()

        for i in range(input_ws.getNumberHistograms()):
            det_id = input_ws.getDetector(i).getID()

            two_theta_in = info_in.signedTwoTheta(i)
            two_theta_out = info_out.signedTwoTheta(i)

            theta_offset = self.calibration_data.get(det_id)
            expected_two_theta = ((two_theta_in * self._RAD_TO_DEG) + theta_offset) * self._DEG_TO_RAD if theta_offset else two_theta_in

            self.assertAlmostEqual(two_theta_out, expected_two_theta, msg=f"Unexpected theta value for detector {det_id}")

    def _create_sample_workspace(self, name):
        """Creates a workspace with 9 detectors. Only detector IDs 11 to 14 will have calibration data"""
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=0, BankPixelWidth=3, XMin=200, OutputWorkspace=name)
        return ws

    def _create_workspace_group(self, group_name, num_workspaces):
        """Creates a workspace group with the given number of workspaces."""
        child_names = list()
        for index in range(num_workspaces):
            child_name = f"{group_name}_{str(index+1)}"
            self._create_sample_workspace(child_name)
            child_names.append(child_name)
        return GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)

    def _create_sample_workspace_with_missing_detectors(self):
        """Creates a workspace with 11 detectors. Only detector ID 11 will have calibration data.
        The calibration data will have entries for detectors that are not present in the workspace"""
        ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(11, 20, False)
        return ws

    def _assert_run_algorithm_succeeds(self, args, expected=None):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list."""
        alg = self._setup_algorithm(args)
        assertRaisesNothing(self, alg.execute)
        if expected is not None:
            actual = AnalysisDataService.getObjectNames()
            self.assertEqual(set(expected), set(actual))

    def _assert_run_algorithm_raises_exception(self, args, error_msg_regex):
        """Run the algorithm with the given args and check it raises the expected exception"""
        alg = self._setup_algorithm(args)
        self.assertRaisesRegex(RuntimeError, error_msg_regex, alg.execute)

    def _setup_algorithm(self, args):
        alg = create_algorithm("ReflectometryISISCalibration", **args)
        alg.setRethrows(True)
        return alg


if __name__ == "__main__":
    unittest.main()
