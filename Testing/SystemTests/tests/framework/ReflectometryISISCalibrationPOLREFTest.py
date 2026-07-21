# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from mantid.simpleapi import Load, ReflectometryISISCalibration
import systemtesting


class ReflectometryISISCalibrationPOLREFTest(systemtesting.MantidSystemTest):
    _POLREF_CALIBRATION_MAP = "POLREF_calibration_map.dat"
    _POLREF_DATA_FILE = "POLREF00032130"
    _REFERENCE_FILE = "POLREF00032130_pixels_adjusted.nxs"
    _SPECULAR_PIXEL = 280.0
    _OUTPUT_FILE = "polref_calibrated"

    def runTest(self):
        group_ws = Load(Filename=self._POLREF_DATA_FILE, OutputWorkspace=self._POLREF_DATA_FILE)
        output_ws = ReflectometryISISCalibration(
            InputWorkspace=self._POLREF_DATA_FILE,
            CalibrationFile=self._POLREF_CALIBRATION_MAP,
            InstrumentWorkflow="POLREF",
            SpecularPixelSpectrumNo=self._SPECULAR_PIXEL,
            ExperimentAngle=0.95,
            OutputWorkspace=self._OUTPUT_FILE,
        )

        for i in range(len(group_ws)):
            spec_index = self._workspace_index_for_spectrum_number(group_ws[i], int(round(self._SPECULAR_PIXEL)))
            self._check_geometry_changed_in_polref_scattering_plane(spec_index, group_ws[i], output_ws[i])

    def validate(self):
        return self._OUTPUT_FILE, self._REFERENCE_FILE

    def requiredFiles(self):
        return [self._POLREF_DATA_FILE + ".nxs", self._POLREF_CALIBRATION_MAP]

    def _check_geometry_changed_in_polref_scattering_plane(self, spec_index, input, output):
        det_info_in = input.detectorInfo()
        det_info_out = output.detectorInfo()
        position_in = det_info_in.position(spec_index)
        position_out = det_info_out.position(spec_index)

        self.assertDelta(position_in.Y(), position_out.Y(), 1e-12)
        self.assertTrue(abs(position_in.X() - position_out.X()) > 1e-8)
        self.assertTrue(abs(position_in.Z() - position_out.Z()) > 1e-8)

    @staticmethod
    def _workspace_index_for_spectrum_number(ws, spectrum_number):
        for index in range(ws.getNumberHistograms()):
            if ws.getSpectrum(index).getSpectrumNo() == spectrum_number:
                return index
        raise RuntimeError(f"Spectrum number {spectrum_number} not found in workspace")
