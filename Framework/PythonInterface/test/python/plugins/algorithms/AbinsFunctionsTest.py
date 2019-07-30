# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid import logger

#from mantid.simpleapi import mtd, Abins, Scale, CompareWorkspaces, Load, DeleteWorkspace
from AbinsModules import AbinsParameters
from AbinsModules.FrequencyPowderGenerator import FrequencyPowderGenerator
from AbinsModules.AbinsConstants import INT_TYPE, FLOAT_TYPE
import numpy as np
from numpy.testing import assert_array_almost_equal, assert_array_equal

# Functional tests of Abins internals: tests with DFT data go in AbinsBasicTest

class AbinsFrequencyPowderGeneratorTest(unittest.TestCase):
    def setUp(self):
        self.max_wavenumber = AbinsParameters.max_wavenumber
        AbinsParameters.max_wavenumber = 700.
        np.random.seed(1)
        self.fundamentals = np.array(np.random.random(50), dtype=FLOAT_TYPE)
        self.fundamentals.sort()
        self.fundamentals *= 500

    def tearDown(self):
        AbinsParameters.max_wavenumber = self.max_wavenumber

    def test_frequency_generator(self):
        # construct_freq_combinations takes arrays of frequencies and indices,
        # sums all the frequency combinations and keeps the index combinations,
        # and filters out any entries with too-high combined frequency

        # At order one this should just be a pass-through
        # (it also reindexes the coefficients but we might not keep that)
        fundamentals, fund_coeffs = (
            FrequencyPowderGenerator.construct_freq_combinations(
                previous_array=None, previous_coefficients=None,
                fundamentals_array=self.fundamentals,
                fundamentals_coefficients=np.arange(len(self.fundamentals),
                                                    dtype=INT_TYPE),
                quantum_order=1))

        assert_array_equal(self.fundamentals, fundamentals)
        assert_array_equal(fund_coeffs, np.arange(len(self.fundamentals),
                                                  dtype=INT_TYPE))

        # Calcualate some doubles
        doubles, double_coeffs = (
            FrequencyPowderGenerator.construct_freq_combinations(
                previous_array=fundamentals,
                previous_coefficients=fund_coeffs,
                fundamentals_array=self.fundamentals,
                fundamentals_coefficients=np.arange(len(self.fundamentals),
                                                    dtype=INT_TYPE),
                quantum_order=2))

        # Check the doubles have been screened for max frequency
        self.assertEqual(len(doubles), 2104)
        self.assertLess(max(doubles), AbinsParameters.max_wavenumber)

        # Check doubles are in the right places and the maths is just a sum
        self.assertTrue(np.any(fundamentals[0] * 2 == doubles))
        self.assertTrue(np.any(fundamentals[2] + fundamentals[3] == doubles))
        self.assertEqual((fundamentals[double_coeffs[20,0]]
                          + fundamentals[double_coeffs[20,1]]),
                          doubles[20])

if __name__ == '__main__':
    unittest.main()
