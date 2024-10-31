# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import testhelpers
from mantid.api import FrameworkManager


class FrameworkManagerTest(unittest.TestCase):
    def test_clear_functions_do_not_throw(self):
        # Test they don't throw for now
        testhelpers.assertRaisesNothing(self, FrameworkManager.clear)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearData)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearAlgorithms)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearInstruments)
        testhelpers.assertRaisesNothing(self, FrameworkManager.clearPropertyManagers)


if __name__ == "__main__":
    unittest.main()
