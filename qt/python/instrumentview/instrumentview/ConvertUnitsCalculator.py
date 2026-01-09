# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.dataobjects import Workspace2D
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParams


class ConvertUnitsCalculator:
    def __init__(self, ws: Workspace2D) -> None:
        self._detector_info = ws.detectorInfo()
        self._l1 = self._detector_info.l1()
        self._spectrum_info = ws.spectrumInfo()

    def convert(self, source_unit: str, target_unit: str, spectrum_no: int, detector_id: int, value: float) -> float:
        if source_unit == target_unit:
            return value
        diffraction_constants = self._spectrum_info.diffractometerConstants(int(spectrum_no))
        detector_index = self._detector_info.indexOf(int(detector_id))
        diffraction_constants[UnitParams.l2] = self._detector_info.l2(detector_index)
        return UnitConversion.run(source_unit, target_unit, value, self._l1, DeltaEModeType.Elastic, diffraction_constants)
