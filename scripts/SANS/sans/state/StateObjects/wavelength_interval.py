# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Tuple, List

from mantid.py36compat import dataclass, field
from sans.state.JsonSerializable import JsonSerializable


@dataclass()
class WavelengthInterval(metaclass=JsonSerializable):
    wavelength_min: float = 0.
    wavelength_max: float = 0.
    wavelength_step: float = 0.

    selected_ranges: List[Tuple[float, float]] = field(default_factory=list)
