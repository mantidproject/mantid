# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import MemoryStats
import sys

class MemoryStatsTest(unittest.TestCase):

    def test_values_are_all_greater_than_zero(self):
        # Best we can do is test that something is returned
        mem = MemoryStats()

        self.assertTrue(hasattr(mem, "update"))
        self.assertTrue(mem.availMem() > 0.0, "Value should be larger than 0.0")
        self.assertTrue(mem.totalMem() > 0.0, "Value should be larger than 0.0")
        self.assertTrue(mem.residentMem() > 0.0, "Value should be larger than 0.0")
        self.assertTrue(mem.virtualMem() > 0.0, "Value should be larger than 0.0")
        self.assertTrue(mem.getFreeRatio() > 0.0, "Value should be larger than 0.0")
        if sys.platform == 'win32':
            self.assertTrue(mem.reservedMem() > 0.0, "Value should be larger than 0.0")
        else:
            self.assertEqual(mem.reservedMem(),  0.0, "Value should 0.0")

if __name__ == '__main__':
    unittest.main()
