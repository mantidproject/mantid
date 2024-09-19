# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Tuple, List

from mantid.py36compat import dataclass, field
from SANS.sans.state.JsonSerializable import JsonSerializable


WavRange = Tuple[float, float]
WavRangePairs = List[WavRange]


@dataclass()
class WavelengthInterval(metaclass=JsonSerializable):
    wavelength_step: float = 0.0
    user_wavelength_input: str = ""
    _selected_ranges: WavRangePairs = field(default_factory=list)
    _wavelength_full_range: WavRange = (0.0, 0.0)

    @property
    def wavelength_full_range(self) -> WavRange:
        return self._wavelength_full_range

    @wavelength_full_range.setter
    def wavelength_full_range(self, val):
        if self.wavelength_full_range in self._selected_ranges:
            self._selected_ranges.remove(self.wavelength_full_range)
        if val not in self.selected_ranges:
            self.selected_ranges.insert(0, val)
        self._wavelength_full_range = val

    @property
    def selected_ranges(self) -> WavRangePairs:
        return self._selected_ranges

    @selected_ranges.setter
    def selected_ranges(self, ranges: WavRangePairs):
        # Strip out any duplicates, to avoid duplicate processing later
        self._selected_ranges = list(set(ranges))
        # Sort, it should be ascending, with the first val being the full range to match the "expected order"
        # e.g. (2, 14), (2, 4), (4, 8), (8, 14). The minus on the second val gets the max val first
        self._selected_ranges = sorted(self._selected_ranges, key=lambda x: (x[0], -x[1]))

    @property
    def wavelength_min(self):
        raise AttributeError("Still using old attribute")

    @property
    def wavelength_max(self):
        raise AttributeError("Still using old attribute")
