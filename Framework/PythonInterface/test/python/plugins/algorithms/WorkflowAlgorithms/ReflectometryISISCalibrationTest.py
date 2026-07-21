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
from ReflectometryISISCalibration import ReflectometryISISCalibration
from testhelpers import assertRaisesNothing, create_algorithm, WorkspaceCreationHelper
from testhelpers.tempfile_wrapper import TemporaryFileHelper


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data.dat")
    _RAD_TO_DEG = 180.0 / math.pi
    _DEG_TO_RAD = math.pi / 180.0

    _DET_ID_LABEL = "detectorid"
    _SPECTRUM_NUMBER_LABEL = "spectrumnumber"
    _THETA_LABEL = "theta_offset"
    _ANGLE_LABEL = "angle"
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

    def test_calibration_successful_with_non_contiguous_default_detector_ids(self):
        theta_offsets = {11: 0.05, 13: -0.03}
        calibration_lines = [f"{self._DET_ID_LABEL} {self._THETA_LABEL}\n"]
        calibration_lines.extend(f"{det_id} {theta_offset}\n" for det_id, theta_offset in theta_offsets.items())
        self.temp_calibration_file = TemporaryFileHelper(fileContent="".join(calibration_lines), extension=".dat")
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)

        output_ws_name = "test_calibrated"
        args = {"InputWorkspace": ws, "CalibrationFile": self.temp_calibration_file.getName(), "OutputWorkspace": output_ws_name}
        self._assert_run_algorithm_succeeds(args, [input_ws_name, output_ws_name])

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_final_theta_values(ws, output_ws, calibration_data=theta_offsets)

    def test_polref_workflow_interpolates_fractional_specular_spectrum_numbers(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = [0.05 * index for index in range(ws.getNumberHistograms())]
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        output_ws_name = "test_calibrated"
        specular_spectrum_number = 4.5
        experiment_angle = 0.5
        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": specular_spectrum_number,
            "ExperimentAngle": experiment_angle,
            "OutputWorkspace": output_ws_name,
        }
        self._assert_run_algorithm_succeeds(args, [input_ws_name, output_ws_name])

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_polref_final_theta_values(ws, output_ws, angles, specular_spectrum_number, experiment_angle)

    def test_polref_workflow_inverts_descending_calibration_map_angles(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = [0.50 - 0.05 * index for index in range(ws.getNumberHistograms())]
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        output_ws_name = "test_calibrated"
        specular_spectrum_number = 4
        experiment_angle = 0.5
        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": specular_spectrum_number,
            "ExperimentAngle": experiment_angle,
            "OutputWorkspace": output_ws_name,
        }
        self._assert_run_algorithm_succeeds(args, [input_ws_name, output_ws_name])

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_polref_final_theta_values(ws, output_ws, angles, specular_spectrum_number, experiment_angle)
        self.assertGreater(
            output_ws.spectrumInfo().signedTwoTheta(self._workspace_index_for_spectrum_number(output_ws, specular_spectrum_number + 1)),
            output_ws.spectrumInfo().signedTwoTheta(self._workspace_index_for_spectrum_number(output_ws, specular_spectrum_number)),
        )

    def test_polref_workflow_raises_if_specular_spectrum_number_out_of_calibration_range(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = [0.05 * index for index in range(ws.getNumberHistograms())]
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": len(angles) + 1,
            "ExperimentAngle": 0.5,
            "OutputWorkspace": "test_calibrated",
        }
        self._assert_run_algorithm_raises_exception(args, "SpecularPixelSpectrumNo must be in the range")

    def test_polref_workflow_raises_if_specular_spectrum_number_is_outside_workspace_range(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = [0.05 * index for index in range(ws.getNumberHistograms() + 2)]
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": ws.getNumberHistograms() + 1,
            "ExperimentAngle": 0.5,
            "OutputWorkspace": "test_calibrated",
        }
        self._assert_run_algorithm_raises_exception(args, "SpecularPixelSpectrumNo must be in the range")

    def test_polref_workflow_requires_experiment_angle(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = [0.05 * index for index in range(ws.getNumberHistograms())]
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": 4,
            "OutputWorkspace": "test_calibrated",
        }
        self._assert_run_algorithm_raises_exception(args, "ExperimentAngle must be provided for the POLREF workflow")

    def test_workflow_options_enable_property_only_for_configured_properties(self):
        workflow_options = ReflectometryISISCalibration.WorkflowOptions(
            calibration_angle_type="Absolute",
            detector_correction_type="RotateAroundSample",
            enabled_properties={"SpecularPixelSpectrumNo"},
        )

        self.assertTrue(workflow_options.enable_property("SpecularPixelSpectrumNo"))
        self.assertFalse(workflow_options.enable_property("ExperimentAngle"))

    def test_polref_workflow_raises_if_spectrum_number_is_fractional(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        self.temp_calibration_file = TemporaryFileHelper(
            fileContent=f"{self._SPECTRUM_NUMBER_LABEL} {self._ANGLE_LABEL}\n2.5 0.10\n3 0.15\n", extension=".dat"
        )

        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": 3,
            "ExperimentAngle": 0.5,
            "OutputWorkspace": "test_calibrated",
        }
        self._assert_run_algorithm_raises_exception(args, "spectrum numbers should be integers")

    def test_polref_workflow_interpolates_missing_spectrum_numbers(self):
        input_ws_name = "test_1234"
        ws = self._create_sample_workspace(input_ws_name)
        angles = {2: 0.10, 3: 0.15, 5: 0.25}
        self.temp_calibration_file = TemporaryFileHelper(fileContent=self._absolute_calibration_file_content(angles), extension=".dat")

        output_ws_name = "test_calibrated"
        args = {
            "InputWorkspace": ws,
            "CalibrationFile": self.temp_calibration_file.getName(),
            "InstrumentWorkflow": "POLREF",
            "SpecularPixelSpectrumNo": 4,
            "ExperimentAngle": 0.5,
            "OutputWorkspace": output_ws_name,
        }
        self._assert_run_algorithm_succeeds(args, [input_ws_name, output_ws_name])

        output_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_polref_final_theta_values(ws, output_ws, angles, 4, 0.5)

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

    def _check_polref_final_theta_values(
        self,
        input_ws,
        output_ws,
        absolute_calibration_angles,
        specular_spectrum_number,
        experiment_angle,
    ):
        info_in = input_ws.spectrumInfo()
        info_out = output_ws.spectrumInfo()
        calibration_specular_angle = self._interpolate_calibration_angle(absolute_calibration_angles, specular_spectrum_number)
        experiment_specular_two_theta = 2.0 * experiment_angle
        calibration_spectrum_numbers = self._calibration_spectrum_numbers(absolute_calibration_angles)
        first_calibrated_spectrum_number = calibration_spectrum_numbers[0]
        last_calibrated_spectrum_number = calibration_spectrum_numbers[-1]

        for index in range(input_ws.getNumberHistograms()):
            spectrum_number = input_ws.getSpectrum(index).getSpectrumNo()
            if first_calibrated_spectrum_number <= spectrum_number <= last_calibrated_spectrum_number:
                relative_calibration_angle = (
                    self._interpolate_calibration_angle(absolute_calibration_angles, spectrum_number) - calibration_specular_angle
                )
                expected_two_theta_degrees = experiment_specular_two_theta - 2.0 * relative_calibration_angle
                expected_two_theta = expected_two_theta_degrees * self._DEG_TO_RAD
            else:
                expected_two_theta = info_in.signedTwoTheta(index)
            self.assertAlmostEqual(
                info_out.signedTwoTheta(index), expected_two_theta, msg=f"Unexpected theta value for spectrum {spectrum_number}"
            )

    def _absolute_calibration_file_content(self, angles, reverse=False):
        angle_items = angles.items() if isinstance(angles, dict) else enumerate(angles, start=1)
        if reverse:
            lines = [f"{self._ANGLE_LABEL} {self._SPECTRUM_NUMBER_LABEL}\n"]
            lines.extend(f"{angle} {index}\n" for index, angle in angle_items)
        else:
            lines = [f"{self._SPECTRUM_NUMBER_LABEL} {self._ANGLE_LABEL}\n"]
            lines.extend(f"{index} {angle}\n" for index, angle in angle_items)
        return "".join(lines)

    def _interpolate_calibration_angle(self, values, index):
        if not isinstance(values, dict):
            values = dict(enumerate(values, start=1))
        if index in values:
            return values[index]

        calibration_indexes = sorted(values)
        lower_index = max(calibration_index for calibration_index in calibration_indexes if calibration_index < index)
        upper_index = min(calibration_index for calibration_index in calibration_indexes if calibration_index > index)
        return self._interpolate_between(index, lower_index, values[lower_index], upper_index, values[upper_index])

    @staticmethod
    def _calibration_spectrum_numbers(values):
        if isinstance(values, dict):
            return sorted(values)
        return list(range(1, len(values) + 1))

    @staticmethod
    def _workspace_index_for_spectrum_number(ws, spectrum_number):
        for index in range(ws.getNumberHistograms()):
            if ws.getSpectrum(index).getSpectrumNo() == spectrum_number:
                return index
        raise RuntimeError(f"Spectrum number {spectrum_number} not found in workspace")

    @staticmethod
    def _interpolate_between(index, lower_index, lower_value, upper_index, upper_value):
        if lower_index == upper_index:
            return lower_value
        fraction = (index - lower_index) / (upper_index - lower_index)
        return lower_value + fraction * (upper_value - lower_value)

    def _create_sample_workspace(self, name):
        """Creates a workspace with 9 detectors. Only detector IDs 11 to 14 will have calibration data"""
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=0, BankPixelWidth=3, XMin=200, OutputWorkspace=name)
        return ws

    def _create_workspace_group(self, group_name, num_workspaces):
        """Creates a workspace group with the given number of workspaces."""
        child_names = list()
        for index in range(num_workspaces):
            child_name = f"{group_name}_{str(index + 1)}"
            self._create_sample_workspace(child_name)
            child_names.append(child_name)
        return GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)

    def _create_sample_workspace_with_missing_detectors(self):
        """
        Creates a workspace with 11 detectors. The calibration data will have entries for detectors
        that are not present in the workspace.
        """
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
