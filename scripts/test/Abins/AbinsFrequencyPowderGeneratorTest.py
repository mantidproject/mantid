# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from itertools import product

import abins
import numpy as np
from abins.constants import FIRST_OVERTONE, FUNDAMENTALS, INT_TYPE, FLOAT_TYPE
from numpy.testing import assert_array_equal


class FrequencyPowderGeneratorTest(unittest.TestCase):
    def setUp(self):
        self.simple_freq_generator = abins.FrequencyPowderGenerator
        self.min_wavenumber = abins.parameters.sampling["min_wavenumber"]
        self.max_wavenumber = abins.parameters.sampling["max_wavenumber"]

    def tearDown(self):
        # Restore default Parameters to avoid conflict with other tests
        abins.parameters.sampling["min_wavenumber"] = self.min_wavenumber
        abins.parameters.sampling["max_wavenumber"] = self.max_wavenumber

    def test_construct_freq_combinations(self):
        # reduce rebining parameters for this test
        # abins.parameters.bin_width = 1.0  # Doesn't seem to be used any more?
        bin_width = 1.0
        abins.parameters.sampling["min_wavenumber"] = 0.0
        abins.parameters.sampling["max_wavenumber"] = 20.0

        f_array = np.asarray([1, 2])
        f_coeff = np.arange(2, dtype=INT_TYPE)
        p_array = np.asarray([1, 2])
        q_order = FUNDAMENTALS

        # wrong previous array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(
                fundamentals_array=f_array, fundamentals_coefficients=f_coeff, previous_array=[1, 2], quantum_order=q_order
            )

        # wrong FUNDAMENTALS
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(
                fundamentals_array=[1, 2], fundamentals_coefficients=f_coeff, previous_array=p_array, quantum_order=q_order
            )

        # wrong fundamentals coefficients
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(
                fundamentals_array=f_array, fundamentals_coefficients=[0, 1], previous_array=p_array, quantum_order=q_order
            )

        # wrong quantum order event
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(
                fundamentals_array=f_array, fundamentals_coefficients=f_coeff, previous_array=p_array, quantum_order=-1
            )

        # wrong previous coefficients
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(
                fundamentals_array=f_array,
                fundamentals_coefficients=f_coeff,
                previous_array=p_array,
                previous_coefficients=[0, 1],
                quantum_order=FIRST_OVERTONE,
            )

        # use case: quantum order event 1 (fundamentals)
        array = np.arange(bin_width, 10.0 * bin_width, bin_width)
        array_coeff = np.arange(array.size, dtype=INT_TYPE)
        correct_coefficients = np.arange(array.size, dtype=INT_TYPE)

        generated_array, generated_coefficients = self.simple_freq_generator.construct_freq_combinations(
            fundamentals_array=array, fundamentals_coefficients=array_coeff, previous_array=array, quantum_order=FUNDAMENTALS
        )  # n = 1

        # generated_array = [ 1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9.]
        # generated_coefficients =  [0, 1, 2, 3, 4, 5, 6, 7, 8]

        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_coefficients, generated_coefficients))

        # use case: second order quantum event (first overtone)
        array_1 = array
        array_1_coeff = np.arange(array_1.size, dtype=INT_TYPE)
        array_1_size = array_1.size
        coefficients_1 = np.arange(array.size, dtype=INT_TYPE)

        # array_1 = [ 1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9.]
        # coefficients_1 = [0, 1, 2, 3, 4, 5, 6, 7, 8]

        generated_array_1, generated_coefficients_1 = self.simple_freq_generator.construct_freq_combinations(
            fundamentals_array=array_1,
            fundamentals_coefficients=array_1_coeff,
            previous_array=array_1,
            previous_coefficients=coefficients_1,
            quantum_order=FIRST_OVERTONE,
        )  # n = 2

        correct_array_1 = np.tile(array_1, array_1_size)
        correct_coefficients_1 = np.asarray(list(product(range(array_1_size), range(array_1_size))))

        for i in range(array_1_size):
            for j in range(array_1_size):
                correct_array_1[i * array_1_size + j] += array_1[i]

        # generated_array_1 =
        #   [  2.,   3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.
        #      3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.
        #      4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.,  12.
        #      5.,   6.,   7.,   8.,   9.,  10.,  11.,  12.,  13.
        #      6.,   7.,   8.,   9.,  10.,  11.,  12.,  13.,  14.
        #      7.,   8.,   9.,  10.,  11.,  12.,  13.,  14.,  15.
        #      8.,   9.,  10.,  11.,  12.,  13.,  14.,  15.,  16.
        #      9.,  10.,  11.,  12.,  13.,  14.,  15.,  16.,  17.
        #      10.,  11.,  12. 13.,  14.,  15.,  16.,  17.,  18.]
        #
        # generated_coefficients_1 =
        # [[0, 0], [0, 1], [0, 2], [0, 3], [0, 4], [0, 5], [0, 6], [0, 7], [0, 8],
        #  [1, 0], [1, 1], [1, 2], [1, 3], [1, 4], [1, 5], [1, 6], [1, 7], [1, 8],
        #  [2, 0], [2, 1], [2, 2], [2, 3], [2, 4], [2, 5], [2, 6], [2, 7], [2, 8],
        #  [3, 0], [3, 1], [3, 2], [3, 3], [3, 4], [3, 5], [3, 6], [3, 7], [3, 8],
        #  [4, 0], [4, 1], [4, 2], [4, 3], [4, 4], [4, 5], [4, 6], [4, 7], [4, 8],
        #  [5, 0], [5, 1], [5, 2], [5, 3], [5, 4], [5, 5], [5, 6], [5, 7], [5, 8],
        #  [6, 0], [6, 1], [6, 2], [6, 3], [6, 4], [6, 5], [6, 6], [6, 7], [6, 8],
        #  [7, 0], [7, 1], [7, 2], [7, 3], [7, 4], [7, 5], [7, 6], [7, 7], [7, 8],
        #  [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [8, 5], [8, 6], [8, 7], [8, 8]]

        self.assertEqual(True, np.allclose(correct_array_1, generated_array_1))
        self.assertEqual(True, np.allclose(correct_coefficients_1, generated_coefficients_1))

    def test_frequency_generator_larger_numbers(self):
        # construct_freq_combinations takes arrays of frequencies and indices,
        # sums all the frequency combinations and keeps the index combinations,
        # and filters out any entries with too-high combined frequency

        abins.parameters.sampling["max_wavenumber"] = 700.0
        np.random.seed(1)

        rand_fundamentals = np.array(np.random.random(50), dtype=FLOAT_TYPE)
        rand_fundamentals.sort()
        rand_fundamentals *= 500

        # At order one this should just be a pass-through
        # (it also reindexes the coefficients but we might not keep that)
        fundamentals, fund_coeffs = abins.FrequencyPowderGenerator.construct_freq_combinations(
            previous_array=None,
            previous_coefficients=None,
            fundamentals_array=rand_fundamentals,
            fundamentals_coefficients=np.arange(len(rand_fundamentals), dtype=INT_TYPE),
            quantum_order=1,
        )

        assert_array_equal(rand_fundamentals, fundamentals)
        assert_array_equal(fund_coeffs, np.arange(len(rand_fundamentals), dtype=INT_TYPE))

        # Calcualate some doubles
        doubles, double_coeffs = abins.FrequencyPowderGenerator.construct_freq_combinations(
            previous_array=fundamentals,
            previous_coefficients=fund_coeffs,
            fundamentals_array=rand_fundamentals,
            fundamentals_coefficients=np.arange(len(rand_fundamentals), dtype=INT_TYPE),
            quantum_order=2,
        )

        # Check the doubles have been screened for max frequency
        self.assertEqual(len(doubles), 2104)
        self.assertLess(max(doubles), abins.parameters.sampling["max_wavenumber"])

        # Check doubles are in the right places and the maths is just a sum
        self.assertTrue(np.any(fundamentals[0] * 2 == doubles))
        self.assertTrue(np.any(fundamentals[2] + fundamentals[3] == doubles))
        self.assertEqual((fundamentals[double_coeffs[20, 0]] + fundamentals[double_coeffs[20, 1]]), doubles[20])


if __name__ == "__main__":
    unittest.main()
