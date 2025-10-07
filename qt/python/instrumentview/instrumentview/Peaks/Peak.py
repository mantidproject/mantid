# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


def _format_hkl(value):
    return f"{value:.2f}".rstrip("0").rstrip(".") if "." in f"{value:.2f}" else f"{value:.2f}"


class Peak:
    def __init__(self, detector_id: int, location: np.ndarray, hkl: tuple[float, float, float], tof: float, dspacing: float) -> None:
        self.location = location
        self._hkl = hkl
        self.detector_id = detector_id
        self.tof = tof
        self.dspacing = dspacing

    @property
    def label(self) -> str:
        return f"[{_format_hkl(self._hkl[0])}, {_format_hkl(self._hkl[1])}, {_format_hkl(self._hkl[2])}]"
