# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.Peak import Peak
import unittest


class TestPeak(unittest.TestCase):
    def test_label(self):
        peak = Peak(0, 0, None, (1.233333, 4.0, 36), 0, 0, 0, 0)
        self.assertEqual("(1.23, 4, 36)", peak.label)


if __name__ == "__main__":
    unittest.main()
