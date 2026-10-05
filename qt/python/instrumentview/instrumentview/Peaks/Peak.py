# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from dataclasses import dataclass
from typing import Optional


def _format_hkl(value):
    return f"{value:.2f}".rstrip("0").rstrip(".")


@dataclass(frozen=True)
class Peak:
    detector_id: int
    peak_index: int
    hkl: tuple[float, float, float]
    tof: float
    dspacing: float
    wavelength: float
    q: float

    @property
    def label(self) -> str:
        return f"({_format_hkl(self.hkl[0])}, {_format_hkl(self.hkl[1])}, {_format_hkl(self.hkl[2])})"

    @staticmethod
    def can_be_located_in(unit: str) -> bool:
        """Whether peaks have a location in the given unit, see location_in_unit."""
        return unit.casefold() in ("tof", "dspacing", "wavelength", "q", "momentumtransfer")

    def location_in_unit(self, unit: str) -> Optional[float]:
        """Where this peak sits in the given unit, or None if it cannot be placed in it.

        A workspace can be in a unit a peak has no position for, e.g. Energy or a label
        unit, and the instrument view still has to draw everything else, so this is not
        an error.
        """
        unit_lower_case = unit.casefold()
        if unit_lower_case == "tof":
            return self.tof
        if unit_lower_case == "dspacing":
            return self.dspacing
        if unit_lower_case == "wavelength":
            return self.wavelength
        if unit_lower_case == "q" or unit_lower_case == "momentumtransfer":
            return self.q
        return None
