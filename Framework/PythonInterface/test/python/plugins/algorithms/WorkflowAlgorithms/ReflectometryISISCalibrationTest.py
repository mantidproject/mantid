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
from mantid.kernel import V3D
from testhelpers import assertRaisesNothing, create_algorithm, WorkspaceCreationHelper
from testhelpers.tempfile_wrapper import TemporaryFileHelper


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data.dat")
    _RAD_TO_DEG = 180.0 / math.pi
    _DEG_TO_RAD = math.pi / 180.0

    _DET_ID_LABEL = "detectorid"
    _THETA_LABEL = "theta_offset"
    _COLUMN_NUM_ERROR = "Calibration file should contain two space de-limited columns"
    _COLUMN_LABELS_ERROR = "Incorrect column labels in calibration file"

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

    def setUp(self):
        self.temp_calibration_file = None

    def tearDown(self):
        AnalysisDataService.clear()
        if self.temp_calibration_file:
            del self.temp_calibration_file

    def test_calibration_successful(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)

        output_ws_name = "test_calibrated"
        args = {"InputWorkspace": ws, "CalibrationFile": self._CALIBRATION_TEST_DATA, "OutputWorkspace": output_ws_name}
        outputs = [input_ws_name, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_final_theta_values(ws, output_ws)

    def test_calibration_successful_for_detectors_with_negative_two_theta(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace_with_negative_two_theta(input_ws_name)

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
        self._assert_run_algorithm_raises_exception(args, r"Detector id \d+ from calibration file cannot be found in input workspace")

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

    def test_exception_raised_if_too_many_columns_in_file(self):
        self.temp_calibration_file = TemporaryFileHelper(
            fileContent=f"{self._DET_ID_LABEL} {self._THETA_LABEL} extra_column\n1 0.05 0.03\n", extension=".dat"
        )
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, self._COLUMN_NUM_ERROR)

    def test_exception_raised_if_too_few_columns_in_file(self):
        self.temp_calibration_file = TemporaryFileHelper(fileContent=f"{self._DET_ID_LABEL}\n1\n", extension=".dat")
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, self._COLUMN_NUM_ERROR)

    def test_exception_raised_if_invalid_column_labels_in_file(self):
        self.temp_calibration_file = TemporaryFileHelper(fileContent=f"{self._DET_ID_LABEL} invalid_label\n1 0.05\n", extension=".dat")
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, self._COLUMN_LABELS_ERROR)

    def test_exception_raised_if_no_column_labels_in_file(self):
        self.temp_calibration_file = TemporaryFileHelper(fileContent="1 0.05\n", extension=".dat")
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, self._COLUMN_LABELS_ERROR)

    def test_exception_raised_if_no_data_in_file(self):
        self.temp_calibration_file = TemporaryFileHelper(fileContent=f"{self._DET_ID_LABEL} {self._THETA_LABEL}\n", extension=".dat")
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, "Calibration file provided contains no data")

    def test_exception_raised_if_column_data_types_incorrect(self):
        self.temp_calibration_file = TemporaryFileHelper(
            fileContent=f"{self._DET_ID_LABEL} {self._THETA_LABEL}\n0.05 1\n", extension=".dat"
        )
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": "test_calibrated"}
        self._assert_run_algorithm_raises_exception(args, "Invalid data in calibration file entry")

    def test_calibration_successful_with_columns_reversed_in_file(self):
        det_id = 11
        theta_offset = 0.05
        self.temp_calibration_file = TemporaryFileHelper(
            fileContent=f"{self._THETA_LABEL} {self._DET_ID_LABEL}\n{theta_offset} {det_id}\n", extension=".dat"
        )
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)

        output_ws_name = "test_calibrated"
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": output_ws_name}
        outputs = [input_ws_name, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_final_theta_values(ws, output_ws, calibration_data={det_id: theta_offset})

    def _check_final_theta_values(self, input_ws, output_ws, calibration_data=None):
        if not calibration_data:
            calibration_data = self.calibration_data

        info_in = input_ws.spectrumInfo()
        info_out = output_ws.spectrumInfo()

        for i in range(input_ws.getNumberHistograms()):
            det_id = input_ws.getDetector(i).getID()

            two_theta_in = info_in.signedTwoTheta(i)
            two_theta_out = info_out.signedTwoTheta(i)

            theta_offset = calibration_data.get(det_id)
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
        """Creates a workspace with 11 detectors. The calibration data will have entries for detectors that are not present in the workspace"""
        ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(11, 20, False)
        return ws

    def _create_sample_workspace_with_negative_two_theta(self, name):
        """Creates a workspace with 9 detectors. Only detector IDs 11 to 14 will have calibration data.
        Detector ID 11 is re-positioned so that its initial two theta value is negative.
        """
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=0, BankPixelWidth=3, XMin=200, OutputWorkspace=name)
        det_info = ws.detectorInfo()
        comp_info = ws.componentInfo()
        det_idx = det_info.indexOf(11)
        comp_info.setPosition(det_idx, V3D(0, -0.00436332, 5))
        self.assertTrue(det_info.signedTwoTheta(det_idx) < 0)
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
