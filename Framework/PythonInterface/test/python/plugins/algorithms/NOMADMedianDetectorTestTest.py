# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import AlgorithmManager
from mantid.simpleapi import NOMADMedianDetectorTest  # noqa F401

# standard imports
import unittest


class NOMADMedianDetectorTestTest(unittest.TestCase):

    def test_initalize(self):
        alg = AlgorithmManager.create('NOMADMedianDetectorTest')
        alg.initialize()
        self.assertTrue(alg.isInitialized())


if __name__ == '__main__':
    unittest.main()
