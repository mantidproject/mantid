# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# TODO convert back to NamedTuple with defined types in Python 3


class FloatRange(object):
    start = None  # : float
    end = None  #: float

    def __init__(self, start, end):
        self.start = start
        self.end = end

    def __eq__(self, other):
        # Ensures a range_entry != FloatRange
        if isinstance(other, FloatRange):
            return self.start == other.start and self.end == other.end
        return False
