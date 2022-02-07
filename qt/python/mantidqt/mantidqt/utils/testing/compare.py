# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import numpy as np


class ArraysEqual:
    """Compare arrays for equality in mock.assert_called_with calls.
    """
    def __init__(self, expected):
        self._expected = expected

    def __eq__(self, other):
        return np.all(self._expected == other)

    def __repr__(self):
        """Return a string when the test comparison fails"""
        return f"{self._expected}"
