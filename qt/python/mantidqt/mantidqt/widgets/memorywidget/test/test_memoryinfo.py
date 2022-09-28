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
from mantidqt.widgets.memorywidget.memoryinfo import get_memory_info, get_mantid_memory_info


class MemoryInfoTest(unittest.TestCase):

    def test_memoryinfo(self):
        system_bar = get_memory_info()
        self.assertTrue(isinstance(system_bar.used_percent, int))
        self.assertTrue(0 <= system_bar.used_percent <= 100)
        self.assertTrue(isinstance(system_bar.used_GB, float))
        self.assertTrue(0 <= system_bar.used_GB > 0)
        self.assertTrue(isinstance(system_bar.system_total_GB, float))
        self.assertTrue(0 <= system_bar.system_total_GB > 0)

    def test_mantid_memoryinfo(self):
        mantid_bar = get_mantid_memory_info()
        self.assertTrue(isinstance(mantid_bar.used_percent, int))
        self.assertTrue(0 <= mantid_bar.used_percent <= 100)
        self.assertTrue(isinstance(mantid_bar.used_GB, float))
        self.assertTrue(0 <= mantid_bar.used_GB > 0)
        self.assertTrue(isinstance(mantid_bar.system_total_GB, float))
        self.assertTrue(0 <= mantid_bar.system_total_GB > 0)


if __name__ == "__main__":
    unittest.main()
