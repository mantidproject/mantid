# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple


class FloatRange(NamedTuple):
    start: float
    end: float

    def __eq__(self, other):
        # Ensures a range_entry != FloatRange
        if isinstance(other, FloatRange):
            return self.start == other.start and \
                   self.end == other.end
        return False
