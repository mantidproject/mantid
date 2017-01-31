from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger

import numpy as np
from itertools import product
from AbinsModules import FrequencyPowderGenerator, AbinsParameters, AbinsConstants, AbinsTestHelpers


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsFrequencyPowderGeneratorTest because Python is too old.")
    return is_python_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_python)
class AbinsFrequencyPowderGeneratorTest(unittest.TestCase):

    # reduce rebining parameters for this test
    AbinsParameters.bin_width = 1.0
    AbinsParameters.min_wavenumber = 0.0
    AbinsParameters.max_wavenumber = 20.0

    def setUp(self):
        self.simple_freq_generator = FrequencyPowderGenerator()

    def test_construct_freq_combinations(self):

        f_array = np.asarray([1, 2])
        f_coeff = np.arange(2, dtype=AbinsConstants.INT_TYPE)
        p_array = np.asarray([1, 2])
        q_order = AbinsConstants.FUNDAMENTALS

        # wrong previous array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=f_array,
                                                                   fundamentals_coefficients=f_coeff,
                                                                   previous_array=[1, 2], 
                                                                   quantum_order=q_order)

        # wrong FUNDAMENTALS
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=[1, 2],
                                                                   fundamentals_coefficients=f_coeff,
                                                                   previous_array=p_array,
                                                                   quantum_order=q_order)

        # wrong fundamentals coefficients
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=f_array,
                                                                   fundamentals_coefficients=[0, 1],
                                                                   previous_array=p_array,
                                                                   quantum_order=q_order)

        # wrong quantum order event
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=f_array,
                                                                   fundamentals_coefficients=f_coeff,
                                                                   previous_array=p_array,
                                                                   quantum_order=-1)

        # wrong previous coefficients
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=f_array,
                                                                   fundamentals_coefficients=f_coeff,
                                                                   previous_array=p_array,
                                                                   previous_coefficients=[0, 1],
                                                                   quantum_order=AbinsConstants.FIRST_OVERTONE)

        # use case: quantum order event 1 (fundamentals)
        array = np.arange(AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        array_coeff = np.arange(array.size, dtype=AbinsConstants.INT_TYPE)
        correct_coefficients = np.arange(array.size, dtype=AbinsConstants.INT_TYPE)

        generated_array, generated_coefficients = \
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array,
                                                                   fundamentals_coefficients=array_coeff,
                                                                   previous_array=array,
                                                                   quantum_order=AbinsConstants.FUNDAMENTALS)  # n = 1

        # generated_array = [ 1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9.]
        # generated_coefficients =  [0, 1, 2, 3, 4, 5, 6, 7, 8]

        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_coefficients, generated_coefficients))

        # use case: second order quantum event (first overtone)
        array_1 = array
        array_1_coeff = np.arange(array_1.size, dtype=AbinsConstants.INT_TYPE)
        array_1_size = array_1.size
        coefficients_1 = np.arange(array.size, dtype=AbinsConstants.INT_TYPE)

        # array_1 = [ 1.,  2.,  3.,  4.,  5.,  6.,  7.,  8.,  9.]
        # coefficients_1 = [0, 1, 2, 3, 4, 5, 6, 7, 8]

        generated_array_1, generated_coefficients_1 = \
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array_1,
                                                                   fundamentals_coefficients=array_1_coeff,
                                                                   previous_array=array_1,
                                                                   previous_coefficients=coefficients_1,
                                                                   quantum_order=AbinsConstants.FIRST_OVERTONE)  # n = 2

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


if __name__ == '__main__':
    unittest.main()
