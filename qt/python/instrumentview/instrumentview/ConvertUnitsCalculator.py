# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.dataobjects import Workspace2D
from mantid.kernel import UnitConversion, DeltaEModeType


class ConvertUnitsCalculator:
    def __init__(self, ws: Workspace2D) -> None:
        self._l1 = ws.detectorInfo().l1()
        self._spectrum_info = ws.spectrumInfo()

    def convert(self, source_unit: str, target_unit: str, spectrum_no: int, value: float) -> float:
        if source_unit == target_unit:
            return value
        diffraction_constants = self._spectrum_info.diffractometerConstants(int(spectrum_no))
        return UnitConversion.run(source_unit, target_unit, value, self._l1, DeltaEModeType.Elastic, diffraction_constants)
