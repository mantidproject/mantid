# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import csv
import math
import unittest

import numpy as np

from mantid import FileFinder
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from mantid.kernel import FloatTimeSeriesProperty, V3D
from testhelpers import (assertRaisesNothing, create_algorithm, WorkspaceCreationHelper)


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data.dat")

    @classmethod
    def setUpClass(cls):
        def _create_calibration_data_dictionary():
            # Create dictionary of pixel spectrum number and position from test data
            pixel_positions = {}
            with open(cls._CALIBRATION_TEST_DATA, 'r') as file:
                pixels = csv.reader(file)
                for pixel in pixels:
                    pixel_info = pixel[0].split()
                    pixel_positions[int(pixel_info[0])] = float(pixel_info[1])
            return pixel_positions
        cls.calibration_data = _create_calibration_data_dictionary()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_calibration_successful(self):
        input_ws_name = 'test_1234'
        # We use workspace ID 4 (or detector ID 13) as the specular detector
        spec_det_idx = 4
        ws = self._create_sample_workspace(input_ws_name, spec_det_idx)
        expected_final_xyz, expected_final_dimensions = self._calculate_expected_final_detector_values(ws, spec_det_idx)

        calibration_ws_name = self._calibration_ws_name(ws)
        output_ws_name = 'test_calibrated'
        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': output_ws_name}
        outputs = [input_ws_name, calibration_ws_name, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        self._check_calibration_table(calibration_ws_name, len(self.calibration_data), expected_final_xyz, expected_final_dimensions)

        # Check the final output workspace
        retrieved_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_detector_positions(retrieved_ws, expected_final_xyz)

    def test_calibration_successful_for_workspace_group(self):
        grp_name = 'test'
        output_grp_name = 'test_calibrated'
        # Create a group of two workspaces. The first workspace uses workspace index 4 as the specular detector.
        # The second workspace uses workspace index 2 as the specular detector.
        spec_det_idx_list = [4, 2]
        input_ws_grp_size = len(spec_det_idx_list)
        ws_grp = self._create_workspace_group(grp_name, spec_det_idx_list, input_ws_grp_size)

        expected_final_xyz = []
        expected_final_dimensions = []
        calibration_ws_names = []
        input_ws_names = []
        output_ws_names = []

        for i in range(ws_grp.getNumberOfEntries()):
            ws = ws_grp[i]
            input_ws_names.append(ws.name())
            output_ws_names.append(f'{output_grp_name}_{i + 1}')
            pos_xyz, dimensions = self._calculate_expected_final_detector_values(ws, spec_det_idx_list[i])
            expected_final_xyz.append(pos_xyz)
            expected_final_dimensions.append(dimensions)
            calibration_ws_names.append(self._calibration_ws_name(ws))

        args = {'InputWorkspace': ws_grp,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': output_grp_name}
        outputs = input_ws_names + [grp_name, output_grp_name] + calibration_ws_names + output_ws_names
        self._assert_run_algorithm_succeeds(args, outputs)

        # Check the final output workspace and calibration table
        retrieved_grp = AnalysisDataService.retrieve(output_grp_name)
        self.assertIsInstance(retrieved_grp, WorkspaceGroup)
        self.assertEqual(retrieved_grp.getNumberOfEntries(), input_ws_grp_size)
        for i in range(retrieved_grp.getNumberOfEntries()):
            self._check_calibration_table(calibration_ws_names[i], len(self.calibration_data), expected_final_xyz[i], expected_final_dimensions[i])
            self._check_detector_positions(retrieved_grp[i], expected_final_xyz[i])

    def test_no_specular_pixel_in_workspace_raises_exception(self):
        input_ws_name = 'test_1234'
        ws = self._create_monitor_only_workspace(input_ws_name)

        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args, "Could not find specular pixel index from the workspace")

    def test_missing_entry_in_calibration_file_for_specular_pixel_raises_exception(self):
        input_ws_name = 'test_1234'
        # We use workspace ID 1 (or detector ID 10) as the specular detector because there is no entry for this in the
        # calibration test data file
        ws = self._create_sample_workspace(input_ws_name, 1)

        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args, "Missing calibration data for specular pixel with workspace index \d+")

    def test_detectors_in_calibration_file_but_not_workspace_raises_exception(self):
        # Use workspace ID 2 (or detector ID 3) for the specular detector as this is the only detector in this workspace
        # that has an entry in the calibration file
        ws = self._create_sample_workspace_with_missing_detectors(2)
        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args, "Could not find detector to calibrate at workspace id \d+")

    def test_exception_raised_if_no_calibration_file_supplied(self):
        input_ws_name = 'test_1234'
        ws = self._create_sample_workspace(input_ws_name, 4)
        args = {'InputWorkspace': ws,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args, "Calibration file path must be provided")

    def _check_detector_positions(self, workspace, expected_positions, pos_type='final'):
        for i in range(0, workspace.getNumberHistograms()):
            det = workspace.getDetector(i)
            np.testing.assert_almost_equal(det.getPos(), expected_positions[det.getID()],
                                           err_msg=f"Unexpected {pos_type} position for detector id {det.getID()} found at {det.getPos()}")

    def _calculate_expected_final_detector_values(self, workspace, spec_det_idx):
        specpixel_y = self._start_xyz[workspace.getDetector(spec_det_idx).getID()][1]
        scanned_specpixel_y = self.calibration_data[spec_det_idx]

        expected_positions = {}
        expected_dimensions = {}
        for i in range(0, workspace.getNumberHistograms()):
            # Find new expected new position
            det_id = workspace.getDetector(i).getID()
            det_start_pos = self._start_xyz[det_id]
            det_calibration_y = self.calibration_data.get(i)
            if det_calibration_y:
                calibrated_y = specpixel_y + (scanned_specpixel_y - det_calibration_y) * 0.001
            else:
                calibrated_y = det_start_pos[1]
            expected_positions[det_id] = [det_start_pos[0], calibrated_y, det_start_pos[2]]

            # Find expected height and width values
            det_idx = workspace.detectorInfo().indexOf(det_id)
            comp_info = workspace.componentInfo()
            box = comp_info.shape(det_idx).getBoundingBox().width()
            scalings = comp_info.scaleFactor(det_idx)
            width = box[0] * scalings[0]
            height = box[1] * scalings[1]
            expected_dimensions[det_id] = (width, height)

        return expected_positions, expected_dimensions

    def _check_calibration_table(self, calib_ws_name, expected_num_entries, expected_final_positions, expected_final_dimensions):
        calib_table = AnalysisDataService.retrieve(calib_ws_name).toDict()
        detector_ids = calib_table['Detector ID']
        self.assertEqual(len(detector_ids), expected_num_entries)

        for i in range(len(detector_ids)):
            det_id = detector_ids[i]
            start_pos = self._start_xyz[det_id]
            expected_pos = V3D(start_pos[0], start_pos[1], start_pos[2])

            self.assertAlmostEqual(calib_table['Detector Position'][i], expected_pos)
            self.assertAlmostEqual(calib_table['Detector Y Coordinate'][i], expected_final_positions[det_id][1])
            self.assertAlmostEqual(calib_table['Detector Height'][i], expected_final_dimensions[det_id][1])
            self.assertAlmostEqual(calib_table['Detector Width'][i], expected_final_dimensions[det_id][0])

    def _create_sample_workspace(self, name, ref_pixel_idx):
        """Creates a workspace with 9 detectors. Only detector IDs 11 to 14 (i.e. at workspace indices 2 - 5) will have calibration data"""
        ws = CreateSampleWorkspace(WorkspaceType='Histogram', NumBanks=1, NumMonitors=0,
                                   BankPixelWidth=3, XMin=200, OutputWorkspace=name)

        self._set_workspace_theta(ws, ref_pixel_idx)

        self._start_xyz = {9: [0,0,5], 10: [0,0.008,5], 11: [0,0.016,5], 12: [0.008,0,5], 13: [0.008,0.008,5], 14: [0.008,0.016,5],
                           15: [0.016,0,5], 16: [0.016,0.008,5], 17: [0.016,0.016,5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _create_workspace_group(self, group_name, ref_pixel_idx_list, number_of_items):
        """Creates a workspace group with the given number of items. A list of reference pixel workspace IDs must be
        provided to specify the reference pixel that should be used for each workspace in the group"""
        child_names = list()
        for index in range(0, number_of_items):
            child_name = f"{group_name}_{str(index+1)}"
            ws = self._create_sample_workspace(child_name, ref_pixel_idx_list[index])
            ws.run().addProperty('run_number', index, True)
            child_names.append(child_name)
        return GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)

    def _create_sample_workspace_with_missing_detectors(self, ref_pixel_idx):
        """Creates a workspace with 3 detectors. Only detector ID 3 (i.e. at workspace indices 2) will have calibration data.
        The calibration data will have entries for detectors that are not present in the workspace"""
        ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(3, 20, False)

        self._set_workspace_theta(ws, ref_pixel_idx)

        self._start_xyz = {1: [0,0,5], 2: [0,0.1,5], 3: [0,0.2,5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _create_monitor_only_workspace(self, name):
        """Creates a workspace containing only 2 monitors."""
        ws = CreateSampleWorkspace(WorkspaceType='Histogram', NumBanks=0, NumMonitors=2,
                                   BankPixelWidth=0, XMin=200, OutputWorkspace=name)

        self._create_theta_property_for_workspace(ws, 0.04583658449656179)

        self._start_xyz = {0: [0,0,2.5], 1: [0,0,7.5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _set_workspace_theta(self, ws, ref_pixel_idx):
        """Set workspace theta to the value required so that ref_pixel_idx will be found as the specular pixel"""
        det_info = ws.detectorInfo()
        detector_index = det_info.indexOf(ws.getDetector(ref_pixel_idx).getID())
        det_theta = (det_info.twoTheta(detector_index) * (180 / math.pi)) / 2
        self._create_theta_property_for_workspace(ws, det_theta)

    def _create_theta_property_for_workspace(self, ws, value):
        """Set workspace theta to the given value"""
        theta = FloatTimeSeriesProperty('Theta')
        theta.addValue(np.datetime64('now'), value)
        ws.run().addProperty(name=theta.name, value=theta, replace=True)

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
        alg = create_algorithm('ReflectometryISISCalibration', **args)
        alg.setRethrows(True)
        return alg

    def _calibration_ws_name(self, input_ws):
        return f"Calib_Table_{str(input_ws.getRunNumber())}"


if __name__ == '__main__':
    unittest.main()
