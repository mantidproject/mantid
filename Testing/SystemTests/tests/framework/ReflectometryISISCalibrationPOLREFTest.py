# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os

from mantid import FileFinder
from mantid.simpleapi import LoadEmptyInstrument, ReflectometryISISCalibration
import systemtesting


class ReflectometryISISCalibrationPOLREFAbsoluteThetaTest(systemtesting.MantidSystemTest):
    _POLREF_CALIBRATION_MAP = "POLREF_calibration_map.dat"
    _POLREF_DATA_FILE = "POLREF00032130.nxs"
    _SPECULAR_PIXEL = 280.0

    def skipTests(self):
        self._calibration_map_path = FileFinder.getFullPath(self._POLREF_CALIBRATION_MAP)
        if not self._calibration_map_path:
            self._calibration_map_path = os.path.abspath(
                os.path.join(os.path.dirname(__file__), "../../../Data/SystemTest", self._POLREF_CALIBRATION_MAP)
            )
        return not os.path.exists(self._calibration_map_path)

    def runTest(self):

        ws = LoadEmptyInstrument(InstrumentName="POLREF", OutputWorkspace="polref_input")

        self._input_ws = ws
        self._output_ws = ReflectometryISISCalibration(
            InputWorkspace=ws,
            CalibrationFile=self._calibration_map_path,
            CalibrationAngleType="Absolute",
            AbsoluteAngleType="Theta",
            CalibrationSpecularPixelIndex=self._SPECULAR_PIXEL,
            ExperimentSpecularPixelIndex=self._SPECULAR_PIXEL,
            DetectorCorrectionType="RotateAroundSample",
            OutputWorkspace="polref_calibrated",
        )

        self._specular_index = int(round(self._SPECULAR_PIXEL))
        self._check_geometry_changed_in_polref_scattering_plane()

    def validate(self):
        return True

    def _check_geometry_changed_in_polref_scattering_plane(self):
        det_info_in = self._input_ws.detectorInfo()
        det_info_out = self._output_ws.detectorInfo()
        position_in = det_info_in.position(self._specular_index)
        position_out = det_info_out.position(self._specular_index)

        self.assertDelta(position_in.Y(), position_out.Y(), 1e-12)
        self.assertTrue(abs(position_in.X() - position_out.X()) > 1e-8)
        self.assertTrue(abs(position_in.Z() - position_out.Z()) > 1e-8)
