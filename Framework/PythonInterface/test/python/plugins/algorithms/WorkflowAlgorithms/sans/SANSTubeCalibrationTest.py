# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np

from mantid.api import AnalysisDataService
from mantid.simpleapi import LoadEmptyInstrument
from mantid.kernel import FloatTimeSeriesProperty
from testhelpers import create_algorithm


class SANSTubeCalibrationTest(unittest.TestCase):
    _STRIP_POSITIONS_PARAM = "StripPositions"
    _DATA_FILES_PARAM = "DataFiles"
    _START_PIXEL_PARAM = "StartingPixel"
    _END_PIXEL_PARAM = "EndingPixel"
    _SKIP_TUBES_PARAM = "SkipTubesOnEdgeFindingError"
    _IS_REAR_PARAM = "RearDetector"
    _ENCODER_BEAM_CENTRE_PARAM = "EncoderAtBeamCentre"
    _SIDE_OFFSET_PARAM = "SideOffset"

    def tearDown(self):
        AnalysisDataService.clear()

    def test_not_enough_data_files_for_strip_positions_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {self._STRIP_POSITIONS_PARAM: [920, 755, 590, 425, 260, 95, 5], self._DATA_FILES_PARAM: [data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("There must be a measurement for each strip position.", alg)

    def test_not_enough_strip_positions_for_data_files_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {self._STRIP_POSITIONS_PARAM: [920], self._DATA_FILES_PARAM: [data_ws_name, data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("There must be a strip position for each measurement.", alg)

    def test_duplicate_strip_positions_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        args = {self._STRIP_POSITIONS_PARAM: [920, 920, 475], self._DATA_FILES_PARAM: [data_ws_name, data_ws_name, data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("Duplicate strip positions are not permitted.", alg)

    def test_end_pixel_not_greater_than_start_pixel_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        expected_error = "The ending pixel must have a greater index than the starting pixel."

        args = {
            self._STRIP_POSITIONS_PARAM: [920],
            self._DATA_FILES_PARAM: [data_ws_name],
            self._START_PIXEL_PARAM: 10,
            self._END_PIXEL_PARAM: 5,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error(expected_error, alg)

        args = {
            self._STRIP_POSITIONS_PARAM: [920],
            self._DATA_FILES_PARAM: [data_ws_name],
            self._START_PIXEL_PARAM: 10,
            self._END_PIXEL_PARAM: 10,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error(expected_error, alg)

    def test_missing_det_Z_log_entry_for_workspace_throws_error(self):
        data_ws_name = "test_SANS_calibration_ws"
        self._create_front_det_input_workspace(data_ws_name)
        args = {self._STRIP_POSITIONS_PARAM: [920], self._DATA_FILES_PARAM: [data_ws_name]}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error('Run log does not contain an entry for "Front_Det_Z"', alg)

    def test_incorrect_number_of_edges_found_throws_error_when_not_skipping_tubes(self):
        data_ws_name = "test_SANS_calibration_ws"
        self._create_front_det_input_workspace(data_ws_name, include_det_z_log=True)
        # The strip in the second workspace will be in the same location as the first
        second_ws_name = f"{data_ws_name}_2"
        self._create_front_det_input_workspace(second_ws_name, include_det_z_log=True)
        # Set up the calibration to expect two strip edges when only one will be present in the merged data
        args = {
            self._STRIP_POSITIONS_PARAM: [920, 755],
            self._DATA_FILES_PARAM: [data_ws_name, second_ws_name],
            self._SKIP_TUBES_PARAM: False,
        }
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("Cannot calibrate tube 0 - found 2 edges when exactly 4 are required", alg)

    def test_calibration_failure_for_tube_throws_error_when_not_skipping_tubes(self):
        data_ws_name = "test_SANS_calibration_ws"
        self._create_front_det_input_workspace(data_ws_name, include_det_z_log=True)
        args = {self._STRIP_POSITIONS_PARAM: [920], self._DATA_FILES_PARAM: [data_ws_name], self._SKIP_TUBES_PARAM: False}
        alg = self._setup_front_detector_calibration(args)
        self._assert_raises_error("Failure attempting to calibrate tube 0 -", alg)

    def _create_front_det_input_workspace(self, input_ws_name, include_det_z_log=False):
        """
        Create a workspace with a single vertical strip/shadow on the SANS2D front detector
        Counts in the shadow will be 10 and counts outside it will be 750
        """
        ws = LoadEmptyInstrument(InstrumentName="SANS2D", DetectorValue=10, OutputWorkspace=input_ws_name)

        # Define the strip edge positions
        comp_info = ws.componentInfo()
        detector_bank_idx = comp_info.indexOfAny("front-detector")
        first_detector_idx = int(comp_info.detectorsInSubtree(detector_bank_idx)[0])

        strip_right_edge = comp_info.position(first_detector_idx).getX() + 0.1
        strip_left_edge = strip_right_edge + 0.03

        # Set the counts outside the strip
        for ws_idx in range(ws.getNumberHistograms()):
            det_x = ws.getDetector(ws_idx).getPos().getX()
            if det_x > strip_left_edge or det_x < strip_right_edge:
                ws.dataY(ws_idx)[0] = 750

        # Add required log values
        self._add_time_series_log(ws, "proton_charge_by_period", 120)

        if include_det_z_log:
            self._add_time_series_log(ws, "Front_Det_Z", 2381)

    @staticmethod
    def _add_time_series_log(ws, log_name, value):
        log = FloatTimeSeriesProperty(log_name)
        log.addValue(np.datetime64("now"), value)
        ws.run().addProperty(name=log.name, value=log, replace=True)

    def _setup_front_detector_calibration(self, args):
        default_args = {self._SIDE_OFFSET_PARAM: 1.1, self._ENCODER_BEAM_CENTRE_PARAM: 474.2, self._IS_REAR_PARAM: False}
        alg = create_algorithm("SANSTubeCalibration", **default_args, **args)
        alg.setRethrows(True)
        return alg

    def _assert_raises_error(self, error_msg_regex, alg):
        self.assertRaisesRegex(RuntimeError, error_msg_regex, alg.execute)


if __name__ == "__main__":
    unittest.main()
