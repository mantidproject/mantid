# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import math
import os
import tempfile

from mantid.simpleapi import CropWorkspace, LoadEmptyInstrument, LoadNexus, ReflectometryISISCalibration
import systemtesting


class ReflectometryISISCalibrationPOLREFAbsoluteThetaTest(systemtesting.MantidSystemTest):
    _POLREF_CALIBRATION_MAP = (
        "/Users/mial.lewis/repos/POLREF-Data-Reduction/"
        "POLREF_Red/Calibrations/Cycle_18_3/OSMOND_Map_18_3_thetaScan_thVsCl_pixelsRemoved.nx5"
    )
    _SPECULAR_PIXEL = 280.0

    def skipTests(self):
        return not os.path.exists(self._POLREF_CALIBRATION_MAP)

    def runTest(self):
        calibration_map = LoadNexus(Filename=self._POLREF_CALIBRATION_MAP, OutputWorkspace="polref_calibration_map")
        channels, angles = self._read_calibration_map(calibration_map)

        ws = LoadEmptyInstrument(InstrumentName="POLREF", OutputWorkspace="polref_input")
        end_workspace_index = 5 + len(angles) - 1
        ws = CropWorkspace(InputWorkspace=ws, OutputWorkspace="polref_input", StartWorkspaceIndex=5, EndWorkspaceIndex=end_workspace_index)

        self._calibration_file = self._write_absolute_calibration_file(angles)
        specular_index = self._channel_to_detector_index(channels, self._SPECULAR_PIXEL)

        self._input_ws = ws
        self._output_ws = ReflectometryISISCalibration(
            InputWorkspace=ws,
            CalibrationFile=self._calibration_file,
            CalibrationAngleType="Absolute",
            AbsoluteAngleType="Theta",
            CalibrationSpecularPixelIndex=specular_index,
            ExperimentSpecularPixelIndex=specular_index,
            DetectorCorrectionType="RotateAroundSample",
            OutputWorkspace="polref_calibrated",
        )

        self._specular_index = int(round(specular_index))
        self._check_geometry_changed_in_polref_scattering_plane()

    def validate(self):
        return True

    def cleanup(self):
        if hasattr(self, "_calibration_file") and os.path.exists(self._calibration_file):
            os.remove(self._calibration_file)

    @staticmethod
    def _read_calibration_map(calibration_map):
        angles = list(calibration_map.readX(0))
        channels = list(calibration_map.readY(0))
        if len(angles) == len(channels) + 1:
            angles = [(left + right) / 2.0 for left, right in zip(angles[:-1], angles[1:])]
        channel_angles = sorted(zip(channels, angles), key=lambda pair: pair[0])
        channels, angles = zip(*channel_angles)
        return list(channels), list(angles)

    @staticmethod
    def _write_absolute_calibration_file(angles):
        handle = tempfile.NamedTemporaryFile(mode="w", suffix=".dat", delete=False)
        with handle:
            handle.write("detectorid angle\n")
            for index, angle in enumerate(angles):
                handle.write(f"{index} {angle}\n")
        return handle.name

    @staticmethod
    def _channel_to_detector_index(channels, channel):
        for index, value in enumerate(channels):
            if math.isclose(value, channel):
                return float(index)
        raise RuntimeError(f"Channel {channel} not found in POLREF calibration map")

    def _check_geometry_changed_in_polref_scattering_plane(self):
        det_info_in = self._input_ws.detectorInfo()
        det_info_out = self._output_ws.detectorInfo()
        position_in = det_info_in.position(self._specular_index)
        position_out = det_info_out.position(self._specular_index)

        self.assertDelta(position_in.Y(), position_out.Y(), 1e-12)
        self.assertTrue(abs(position_in.X() - position_out.X()) > 1e-8)
        self.assertTrue(abs(position_in.Z() - position_out.Z()) > 1e-8)
