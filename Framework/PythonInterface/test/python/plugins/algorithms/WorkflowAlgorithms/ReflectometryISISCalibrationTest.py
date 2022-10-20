# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace
from mantid.kernel import FloatTimeSeriesProperty, V3D
from testhelpers import (assertRaisesNothing, create_algorithm, WorkspaceCreationHelper)


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = 'C:/repos/mantid-data/calibration_test_data.dat'
    _CALIBRATION_WS_NAME = 'CalibTable'

    def tearDown(self):
        AnalysisDataService.clear()

    def test_calibration_successful_multiple_component_levels(self):
        input_ws_name = 'test_1234'
        # This theta value should result in detector ID 13 (or workspace ID 4) being treated as the specular detector
        ws = self._create_sample_workspace_multiple_component_levels(input_ws_name, 0.0648225)
        expected_final_xyz = {9: [0,0,5], 10: [0,0.008,5], 11: [0,0.006,5], 12: [0.008,0.007,5], 13: [0.008,0.008,5], 14: [0.008,0.009,5],
                           15: [0.016,0,5], 16: [0.016,0.008,5], 17: [0.016,0.016,5]}

        output_ws_name = 'test_calibrated'
        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': output_ws_name}
        outputs = [input_ws_name, self._CALIBRATION_WS_NAME, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        self._check_calibration_table(4, self._start_xyz, expected_final_xyz, 0.0002, 0.008)

        # Check the final output workspace
        retrieved_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_detector_positions(retrieved_ws, expected_final_xyz)

    def test_calibration_successful_one_component_level(self):
        # This theta value should result in detector ID 5 (or workspace ID 4) being treated as the specular detector
        ws = self._create_sample_workspace_one_component_level(2.286960629950409)
        expected_final_xyz = {1: [0,0,5], 2: [0,0.1,5], 3: [0,0.398,5], 4: [0,0.399,5], 5: [0,0.4,5], 6: [0,0.401,5],
                              7: [0,0.6,5], 8: [0,0.7,5], 9: [0,0.8,5]}

        output_ws_name = 'test_calibrated'
        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': output_ws_name}
        outputs = [self._CALIBRATION_WS_NAME, output_ws_name]
        self._assert_run_algorithm_succeeds(args, outputs)

        self._check_calibration_table(4, self._start_xyz, expected_final_xyz, 0.02, 0.1)

        # Check the final output workspace
        retrieved_ws = AnalysisDataService.retrieve(output_ws_name)
        self._check_detector_positions(retrieved_ws, expected_final_xyz)

    def test_no_specular_pixel_in_workspace_raises_exception(self):
        input_ws_name = 'test_1234'
        ws = self._create_monitor_only_workspace(input_ws_name)

        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args)

    def test_missing_entry_in_calibration_file_for_specular_pixel_raises_exception(self):
        input_ws_name = 'test_1234'
        # This theta value should result in detector ID 10 (or workspace ID 1) being treated as the specular
        # detector, for which there is no entry in the calibration test data file
        ws = self._create_sample_workspace_multiple_component_levels(input_ws_name, 0.04583658449656179)

        args = {'InputWorkspace': ws,
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                'OutputWorkspace': 'test_calibrated'}
        self._assert_run_algorithm_raises_exception(args)

    def _check_detector_positions(self, workspace, expected_positions, pos_type='final'):
        for i in range(0, workspace.getNumberHistograms()):
            detector = workspace.getDetector(i)
            np.testing.assert_almost_equal(detector.getPos(), expected_positions[detector.getID()],
                                           err_msg=f"Unexpected {pos_type} position for detector id {detector.getID()}")

    def _check_calibration_table(self, expected_num_entries, start_positions, expected_final_positions, expected_height, expected_width):
        calib_table = AnalysisDataService.retrieve(self._CALIBRATION_WS_NAME).toDict()
        detector_ids = calib_table['Detector ID']
        self.assertEqual(len(detector_ids), expected_num_entries)

        for i in range(len(detector_ids)):
            det_id = detector_ids[i]
            start_pos = start_positions[det_id]
            expected_pos = V3D(start_pos[0], start_pos[1], start_pos[2])

            self.assertAlmostEqual(calib_table['Detector Position'][i], expected_pos)
            self.assertAlmostEqual(calib_table['Detector Y Coordinate'][i], expected_final_positions[det_id][1])
            self.assertAlmostEqual(calib_table['Detector Height'][i], expected_height)
            self.assertAlmostEqual(calib_table['Detector Width'][i], expected_width)

    def _create_sample_workspace_multiple_component_levels(self, name, theta):
        """Creates a workspace with 9 detectors. Only detector IDs 11 to 14 (i.e. at workspace indices 2 - 5) will have calibration data"""
        ws = CreateSampleWorkspace(WorkspaceType='Histogram', NumBanks=1, NumMonitors=0,
                                   BankPixelWidth=3, XMin=200, OutputWorkspace=name)

        self._set_workspace_theta(ws, theta)

        self._start_xyz = {9: [0,0,5], 10: [0,0.008,5], 11: [0,0.016,5], 12: [0.008,0,5], 13: [0.008,0.008,5], 14: [0.008,0.016,5],
                           15: [0.016,0,5], 16: [0.016,0.008,5], 17: [0.016,0.016,5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _create_sample_workspace_one_component_level(self, theta):
        """Creates a workspace with 9 detectors. Only detector IDs 3 to 6 (i.e. at workspace indices 2 - 5) will have calibration data"""
        ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(9, 20, False)

        self._set_workspace_theta(ws, theta)

        self._start_xyz = {1: [0,0,5], 2: [0,0.1,5], 3: [0,0.2,5], 4: [0,0.3,5], 5: [0,0.4,5], 6: [0,0.5,5],
                           7: [0,0.6,5], 8: [0,0.7,5], 9: [0,0.8,5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _create_monitor_only_workspace(self, name):
        """Creates a workspace containing only 2 monitors."""
        ws = CreateSampleWorkspace(WorkspaceType='Histogram', NumBanks=0, NumMonitors=2,
                                   BankPixelWidth=0, XMin=200, OutputWorkspace=name)

        self._set_workspace_theta(ws, 0.04583658449656179)

        self._start_xyz = {0: [0,0,2.5], 1: [0,0,7.5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _set_workspace_theta(self, ws, theta_value):
        theta = FloatTimeSeriesProperty('Theta')
        theta.addValue(np.datetime64('now'), theta_value)
        ws.run().addProperty(name=theta.name, value=theta, replace=True)

    def _assert_run_algorithm_succeeds(self, args, expected=None):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list."""
        alg = self._setup_algorithm(args)
        assertRaisesNothing(self, alg.execute)
        if expected is not None:
            actual = AnalysisDataService.getObjectNames()
            self.assertEqual(set(expected), set(actual))

    def _assert_run_algorithm_raises_exception(self, args):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list."""
        alg = self._setup_algorithm(args)
        self.assertRaises(RuntimeError, alg.execute)

    def _setup_algorithm(self, args):
        alg = create_algorithm('ReflectometryISISCalibration', **args)
        alg.setRethrows(True)
        return alg


if __name__ == '__main__':
    unittest.main()
