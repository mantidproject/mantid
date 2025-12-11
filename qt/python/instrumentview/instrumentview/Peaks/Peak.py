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
    hkl: tuple[float, float, float]
    tof: float
    dspacing: float
    wavelength: float
    q: float

    @property
    def label(self) -> str:
        return f"({_format_hkl(self.hkl[0])}, {_format_hkl(self.hkl[1])}, {_format_hkl(self.hkl[2])})"
