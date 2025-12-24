# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from dataclasses import dataclass


def _format_hkl(value):
    return f"{value:.2f}".rstrip("0").rstrip(".")


@dataclass(frozen=True)
class Peak:
    detector_id: int
    spectrum_no: int
    location: np.ndarray
    peak_index: int
    hkl: tuple[float, float, float]
    tof: float
    dspacing: float
    wavelength: float
    q: float

    @property
    def label(self) -> str:
        return f"({_format_hkl(self.hkl[0])}, {_format_hkl(self.hkl[1])}, {_format_hkl(self.hkl[2])})"

    def location_in_unit(self, unit: str) -> float:
        unit_lower_case = unit.casefold()
        if unit_lower_case == "tof":
            return self.tof
        if unit_lower_case == "dspacing":
            return self.dspacing
        if unit_lower_case == "wavelength":
            return self.wavelength
        if unit_lower_case == "q" or unit_lower_case == "momentumtransfer":
            return self.q
        raise RuntimeError(f"Unknown unit {unit} for peak location")
