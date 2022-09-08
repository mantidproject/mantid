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
from testhelpers import (assertRaisesNothing, create_algorithm)


class ReflectometryISISCalibrationTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = 'TO_BE_CREATED'
    _CALIBRATION_WS_NAME = 'CalibTable'

    def test_calibration_successful(self):
        input_ws_name = 'test_1234'
        ws = self._create_sample_workspace(input_ws_name)
        expected_final_xyz = {8: [0, 0, 2.5], 9: [0, 0, 7.5], 4: [0, 0.007, 5], 5: [0, 0.008, 5], 6: [0.008, 0.009, 5],
                              7: [0.008, 0.01, 5]}

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

    def _create_sample_workspace(self, name):
        """Creates a workspace with 4 detectors and 2 monitors. The detectors are at workspace ids 2 - 5."""
        ws = CreateSampleWorkspace(WorkspaceType='Histogram', NumBanks=1, NumMonitors=2,
                                   BankPixelWidth=2, XMin=200, OutputWorkspace=name)

        # This value of theta should result in detector ID 5 (or workspace ID 3) being treated as the specular detector
        theta = FloatTimeSeriesProperty('Theta')
        theta.addValue(np.datetime64('now'), 0.04583658449656179)
        ws.run().addProperty(name=theta.name, value=theta, replace=True)

        self._start_xyz = {8: [0,0,2.5], 9: [0,0,7.5], 4: [0,0,5], 5: [0,0.008,5], 6: [0.008,0,5], 7: [0.008,0.008,5]}
        # Check that the detector positions in the test data have been created where we expect
        self._check_detector_positions(ws, self._start_xyz, 'start')

        return ws

    def _assert_run_algorithm_succeeds(self, args, expected=None):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list."""
        alg = create_algorithm('ReflectometryISISCalibration', **args)
        assertRaisesNothing(self, alg.execute)
        if expected is not None:
            actual = AnalysisDataService.getObjectNames()
            self.assertEqual(set(actual), set(expected))


if __name__ == '__main__':
    unittest.main()
