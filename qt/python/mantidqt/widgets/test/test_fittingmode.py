# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from mantidqt.widgets.fitscriptgenerator import FittingMode
from testhelpers import assertRaisesNothing


class FittingModeTest(unittest.TestCase):
    def test_that_FittingMode_has_been_exported_to_python_correctly(self):
        assertRaisesNothing(self, self._create_sequential_fitting_mode)
        assertRaisesNothing(self, self._create_simultaneous_fitting_mode)
        assertRaisesNothing(self, self._create_sequential_and_simultaneous_fitting_mode)

    def _create_sequential_fitting_mode(self):
        return FittingMode.SEQUENTIAL

    def _create_simultaneous_fitting_mode(self):
        return FittingMode.SIMULTANEOUS

    def _create_sequential_and_simultaneous_fitting_mode(self):
        return FittingMode.SEQUENTIAL_AND_SIMULTANEOUS


if __name__ == '__main__':
    unittest.main()
