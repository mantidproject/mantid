# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import unittest
from workbench.plugins.memoryinfo import getMemoryUsed

class MemoryInfoTest(unittest.TestCase):

    def test_MemoryInfo(self):
        mem_used = getMemoryUsed()
        self.assertTrue(isinstance(mem_used, int))
        self.assertTrue(0 <= mem_used <= 100)

if __name__ == "__main__":
    unittest.main()