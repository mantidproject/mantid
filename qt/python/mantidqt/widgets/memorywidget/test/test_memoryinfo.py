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
from mantidqt.widgets.memorywidget.memoryinfo import get_memory_info


class MemoryInfoTest(unittest.TestCase):

    def test_memoryinfo(self):
        mem_used_percent, mem_used, mem_total = get_memory_info()
        self.assertTrue(isinstance(mem_used_percent, int))
        self.assertTrue(0 <= mem_used_percent <= 100)


if __name__ == "__main__":
    unittest.main()
