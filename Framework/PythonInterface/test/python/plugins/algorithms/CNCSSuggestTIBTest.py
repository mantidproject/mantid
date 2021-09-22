# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid import simpleapi
import numpy


class CNCSSuggestTIBTest(unittest.TestCase):
    def test_simple(self):
        result = simpleapi.CNCSSuggestTIB(3.)
        self.assertAlmostEqual(result[0] * 0.1, 4491.5, 0)
        self.assertAlmostEqual(result[1] * 0.1, 4731.5, 0)
        result = simpleapi.CNCSSuggestTIB(1.)
        self.assertAlmostEqual(result[0] * 0.1, 9562.1, 0)
        self.assertAlmostEqual(result[1] * 0.1, 9902.1, 0)
        result = simpleapi.CNCSSuggestTIB(6.)
        self.assertAlmostEqual(result[0] * 0.1, 2983.3, 0)
        self.assertAlmostEqual(result[1] * 0.1, 3323.3, 0)

    def test_someresult(self):
        for en in numpy.arange(1., 30., 0.1):
            result = simpleapi.CNCSSuggestTIB(en)
            self.assertTrue(result[1] - result[0] > 1000.)


if __name__ == "__main__":
    unittest.main()
