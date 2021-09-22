# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid import simpleapi


class HYSPECSuggestTIBTest(unittest.TestCase):
    def test_simple(self):
        result = simpleapi.HYSPECSuggestTIB(5.)
        self.assertAlmostEqual(result[0] * .1, 3951.5, 0)
        self.assertAlmostEqual(result[1] * .1, 4151.5, 0)
        result = simpleapi.HYSPECSuggestTIB(40.)
        self.assertAlmostEqual(result[0] * .1, 1189.8, 0)
        self.assertAlmostEqual(result[1] * .1, 1389.8, 0)


if __name__ == "__main__":
    unittest.main()
