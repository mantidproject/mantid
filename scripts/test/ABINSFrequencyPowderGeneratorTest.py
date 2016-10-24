import unittest
from mantid.simpleapi import *
from os import path
import numpy as np

try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculateDWCrystalTest because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateDWCrystalTest because h5py is unavailable.")
    exit(1)

from AbinsModules import FrequencyPowderGenerator, AbinsConstants, AbinsParameters

class FrequencyPowderGeneratorTest(unittest.TestCase):

    # reduce rebining parameters for this test
    AbinsParameters.bin_width = 1.0
    AbinsParameters.min_wavenumber = 0.0
    AbinsParameters.max_wavenumber = 40.0

    def setUp(self):
        self.simple_freq_generator = FrequencyPowderGenerator()
        self._bins = np.arange(AbinsParameters.acoustic_phonon_threshold, AbinsParameters.max_wavenumber, AbinsParameters.bin_width)
        self._bins = self._bins[1:]


    # def test_optimize_array(self):
    #
    #     # wrong array
    #     with self.assertRaises(ValueError):
    #         self.simple_freq_generator.optimize_array(array="This is string not array")
    #
    #     # wrong dimension of array
    #     with self.assertRaises(ValueError):
    #         self.simple_freq_generator.optimize_array(array=np.asarray([[1, 2], [3, 4]]))
    #
    #     # simplest use case: no modification over array is expected
    #     array = np.asarray([1, 2, 4, 5, 6])
    #     correct_counts = np.ones(array.size)
    #
    #     generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array)
    #     self.assertEqual(True, np.allclose(array, generated_array))
    #     self.assertEqual(True, np.allclose(correct_counts, generated_counts))
    #
    #
    #     # use case: values below acoustic_phonon_threshold are present. In that case all values equal or lower than
    #     # AbinsConstants.acoustic_phonon_threshold should be removed from the array
    #     array_to_be_modified = np.asarray([6, 5, 4, 2, AbinsParameters.acoustic_phonon_threshold, AbinsParameters.acoustic_phonon_threshold - 0.00001, 1])
    #
    #     generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
    #     correct_array = np.asarray([1 ,2, 4, 5 ,6])
    #     correct_counts = np.ones(correct_array.size)
    #
    #     self.assertEqual(True, np.allclose(correct_array, generated_array))
    #     self.assertEqual(True, np.allclose(correct_counts, generated_counts))
    #
    #     # use case: array has larger size than self._bins and all bins are filled
    #     array_to_be_modified = np.arange(0, 10000, 0.1 * AbinsParameters.bin_width)
    #
    #
    #     generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
    #     correct_counts = np.ones(generated_array.size) * AbinsParameters.bin_width / 0.1
    #     # in case array_to_be_modified is bigger and covers whole interval (no nan values) i-th element of self._bins
    #     # and  generated_array should always differ by less than  AbinsParameters.bin_width
    #     self.assertEqual(True,  np.allclose(self._bins, generated_array, rtol=0.0, atol=AbinsParameters.bin_width))
    #     self.assertEqual(True, np.allclose(correct_counts, generated_counts))
    #
    #     # use case: array has larger size than self._bins and not all bins are filled
    #     # noinspection PyTypeChecker
    #     array_to_be_modified = np.tile(np.asarray([2.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width,
    #                                                1.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width]), 5000).ravel() # array with size 10000; alternating values 1+ AbinsParameters.bin_width ,2+ AbinsParameters.bin_width
    #     correct_counts = np.ones(2) * 5000
    #     generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
    #     # returned values should be sorted
    #     correct_array = np.asarray([1.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width,
    #                                 2.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width])
    #     self.assertEqual(True, np.allclose(correct_array, generated_array))
    #     self.assertEqual(True, np.allclose(correct_counts, generated_counts))


    def test_construct_freq_overtones(self):

        # wrong array
        with self.assertRaises(ValueError):
           self.simple_freq_generator.construct_freq_overtones(fundamentals_array="wrong_array", quantum_order=AbinsConstants.fundamentals)

        # wrong quantum order
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_overtones(fundamentals_array=np.asarray([1,2]), quantum_order=-1)

        # to high quantum order
        with     self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_overtones(fundamentals_array=np.asarray([1,2]), quantum_order=AbinsConstants.higher_order_quantum_effects_dim +
                                                                                                                    AbinsConstants.s_last_index +
                                                                                                                    1)

        # use case: fundamentals
        array = np.arange(AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        correct_coefficients = np.eye(array.size, dtype=AbinsConstants.int_type)

        generated_array, generated_coefficients = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=AbinsConstants.fundamentals)

        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_coefficients, generated_coefficients ))

        # use case: overtones
        q_order = AbinsConstants.higher_order_quantum_effects # the highest supported quantum effect
        correct_array = array * q_order
        correct_array = correct_array[correct_array < AbinsParameters.max_wavenumber]
        correct_coefficients = np.eye(correct_array.size, dtype=AbinsConstants.int_type) * q_order
        generated_array, generated_coefficients = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=q_order)

        self.assertEqual(True, np.allclose(array * q_order, generated_array))
        self.assertEqual(True, np.allclose(correct_coefficients, generated_coefficients))


    def test_construct_freq_combinations(self):

        # wrong previous array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=np.asarray([1,2]), previous_array=[1,2] , quantum_order=AbinsConstants.first_overtone)

        # wrong fundamentals
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=[1,2], previous_array=np.asarray([1,2]) , quantum_order=AbinsConstants.fundamentals)

        # wrong quantum order effect
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=np.asarray([1,2]), previous_array=np.asarray([1,2]) , quantum_order=-1)

        # use case quantum order effect 1 (fundamentals)
        array = np.arange(AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        correct_coefficients = np.eye(array.size, dtype=AbinsConstants.int_type)

        generated_array, generated_coefficients = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array,
                                                                                                         previous_array=array,
                                                                                                         quantum_order=AbinsConstants.fundamentals) # n = 1

        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_coefficients, generated_coefficients))

        # use case: second order quantum effect
        array_1 = array
        temp_size = array_1.size
        coefficients_1 = np.eye(temp_size, dtype=AbinsConstants.int_type)

        # array_1 = [ 1.  2.  3.  4.  5.  6.  7.  8.  9.]
        # coefficients_1 =[ 1., 0,  0,  0,  0,  0,  0,  0,  0,
        #                   0   1., 0,  0,  0,  0,  0,  0,  0,
        #                   0,  0,  1., 0,  0,  0,  0,  0,  0,
        #                   0,  0,  0,  1., 0,  0,  0,  0,  0,
        #                   0,  0,  0,  0,  1., 0,  0,  0,  0,
        #                   0,  0,  0,  0,  0,  1., 0,  0,  0,
        #                   0,  0,  0,  0,  0,  0,  1., 0,  0,
        #                   0,  0,  0,  0,  0,  0,  0,  1., 0,
        #                   0,  0,  0,  0,  0,  0,  0,  0,  1. ]

        generated_array_1, generated_coefficients_1 = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array_1,
                                                                                                         previous_array=array_1,
                                                                                                         previous_coefficients=coefficients_1,
                                                                                                         quantum_order=AbinsConstants.first_overtone) # n = 2

        correct_array_1 = np.tile(array_1, temp_size)
        correct_coefficients_1 =np.tile(np.eye(temp_size, dtype=AbinsConstants.int_type), (temp_size, 1))

        for i in range(temp_size):
            for j in range(temp_size):
                correct_array_1[i * temp_size + j] += array_1[i]

        for i in range(temp_size):
            correct_coefficients_1[i * temp_size: (i + 1) * temp_size , i] += 1

        # generated_array_1
        # [  2.   3.   4.   5.   6.   7.   8.   9.  10.
        #    3.   4.   5.   6.   7.   8.   9.  10.  11.
        #    4.   5.   6.   7.   8.   9.  10.  11.  12.
        #    5.   6.   7.   8.   9.  10.  11.  12.  13.
        #    6.   7.   8.   9.  10.  11.  12.  13.  14.
        #    7.   8.   9.  10.  11.  12.  13.  14.  15.
        #    8.   9.  10.  11.  12.  13.  14.  15.  16.
        #    9.  10.  11.  12.  13.  14.  15.  16.  17.
        #    10.  11.  12. 13.  14.  15.  16.  17.  18.]

        # generated_coefficients_1
        # [[2 0 0 0 0 0 0 0 0], [1 1 0 0 0 0 0 0 0], [1 0 1 0 0 0 0 0 0], [1 0 0 1 0 0 0 0 0], [1 0 0 0 1 0 0 0 0], [1 0 0 0 0 1 0 0 0], [1 0 0 0 0 0 1 0 0], [1 0 0 0 0 0 0 1 0], [1 0 0 0 0 0 0 0 1],
        #  [1 1 0 0 0 0 0 0 0], [0 2 0 0 0 0 0 0 0], [0 1 1 0 0 0 0 0 0], [0 1 0 1 0 0 0 0 0], [0 1 0 0 1 0 0 0 0], [0 1 0 0 0 1 0 0 0], [0 1 0 0 0 0 1 0 0], [0 1 0 0 0 0 0 1 0], [0 1 0 0 0 0 0 0 1],
        #  [1 0 1 0 0 0 0 0 0], [0 1 1 0 0 0 0 0 0], [0 0 2 0 0 0 0 0 0], [0 0 1 1 0 0 0 0 0], [0 0 1 0 1 0 0 0 0], [0 0 1 0 0 1 0 0 0], [0 0 1 0 0 0 1 0 0], [0 0 1 0 0 0 0 1 0], [0 0 1 0 0 0 0 0 1],
        #  [1 0 0 1 0 0 0 0 0], [0 1 0 1 0 0 0 0 0], [0 0 1 1 0 0 0 0 0], [0 0 0 2 0 0 0 0 0], [0 0 0 1 1 0 0 0 0], [0 0 0 1 0 1 0 0 0], [0 0 0 1 0 0 1 0 0], [0 0 0 1 0 0 0 1 0], [0 0 0 1 0 0 0 0 1],
        #  [1 0 0 0 1 0 0 0 0], [0 1 0 0 1 0 0 0 0], [0 0 1 0 1 0 0 0 0], [0 0 0 1 1 0 0 0 0], [0 0 0 0 2 0 0 0 0], [0 0 0 0 1 1 0 0 0], [0 0 0 0 1 0 1 0 0], [0 0 0 0 1 0 0 1 0], [0 0 0 0 1 0 0 0 1]
        #  [1 0 0 0 0 1 0 0 0], [0 1 0 0 0 1 0 0 0], [0 0 1 0 0 1 0 0 0], [0 0 0 1 0 1 0 0 0], [0 0 0 0 1 1 0 0 0], [0 0 0 0 0 2 0 0 0], [0 0 0 0 0 1 1 0 0], [0 0 0 0 0 1 0 1 0], [0 0 0 0 0 1 0 0 1]
        #  [1 0 0 0 0 0 1 0 0], [0 1 0 0 0 0 1 0 0], [0 0 1 0 0 0 1 0 0], [0 0 0 1 0 0 1 0 0], [0 0 0 0 1 0 1 0 0], [0 0 0 0 0 1 1 0 0], [0 0 0 0 0 0 2 0 0], [0 0 0 0 0 0 1 1 0], [0 0 0 0 0 0 1 0 1]
        #  [1 0 0 0 0 0 0 1 0], [0 1 0 0 0 0 0 1 0], [0 0 1 0 0 0 0 1 0], [0 0 0 1 0 0 0 1 0], [0 0 0 0 1 0 0 1 0], [0 0 0 0 0 1 0 1 0], [0 0 0 0 0 0 1 1 0], [0 0 0 0 0 0 0 2 0], [0 0 0 0 0 0 0 1 1]
        #  [1 0 0 0 0 0 0 0 1], [0 1 0 0 0 0 0 0 1], [0 0 1 0 0 0 0 0 1], [0 0 0 1 0 0 0 0 1], [0 0 0 0 1 0 0 0 1], [0 0 0 0 0 1 0 0 1], [0 0 0 0 0 0 1 0 1], [0 0 0 0 0 0 0 1 1], [0 0 0 0 0 0 0 0 2]]


        self.assertEqual(True, np.allclose(correct_array_1, generated_array_1))
        self.assertEqual(True, np.allclose(correct_coefficients_1, generated_coefficients_1))


        # use case: higher order quantum effect with the first frequency equal to the first fundamental multiplied by three
        array_2 = np.copy(array_1)
        array_2[0] = 3
        coefficients_2 = np.copy(coefficients_1)
        coefficients_2[0,0] = 3

        # array = [ 3.  2.  3.  4.  5.  6.  7.  8.  9.]
        # coefficients =[ 3., 0,  0,  0,  0,  0,  0,  0,  0,
        #                 0   1., 0,  0,  0,  0,  0,  0,  0,
        #                 0,  0,  1., 0,  0,  0,  0,  0,  0,
        #                 0,  0,  0,  1., 0,  0,  0,  0,  0,
        #                 0,  0,  0,  0,  1., 0,  0,  0,  0,
        #                 0,  0,  0,  0,  0,  1., 0,  0,  0,
        #                 0,  0,  0,  0,  0,  0,  1., 0,  0,
        #                 0,  0,  0,  0,  0,  0,  0,  1., 0,
        #                 0,  0,  0,  0,  0,  0,  0,  0,  1. ]

        generated_array_2, generated_coefficients_2 = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array_1,
                                                                                                         previous_array=array_2,
                                                                                                         previous_coefficients=coefficients_2,
                                                                                                         quantum_order=AbinsConstants.first_overtone) # n = 2

        correct_array_2 =  np.copy(correct_array_1)
        correct_coefficients_2 = np.copy(correct_coefficients_1)
        for i in range(temp_size):
          correct_array_2[i * temp_size] += 2
          correct_coefficients_2[i * temp_size, 0] += 2

        # generated_array_2
        # [  4.   3.   4.   5.   6.   7.   8.   9.  10.
        #    5.   4.   5.   6.   7.   8.   9.  10.  11.
        #    6.   5.   6.   7.   8.   9.  10.  11.  12.
        #    7.   6.   7.   8.   9.  10.  11.  12.  13.
        #    8.   7.   8.   9.  10.  11.  12.  13.  14.
        #    9.   8.   9.  10.  11.  12.  13.  14.  15.
        #    10.   9.  10.  11.  12.  13.  14.  15.  16.
        #    11.  10.  11.  12.  13.  14.  15.  16.  17.
        #    12.  11.  12. 13.  14.  15.  16.  17.  18.]

        # generated_coefficients_2
        # [[4 0 0 0 0 0 0 0 0], [1 1 0 0 0 0 0 0 0], [1 0 1 0 0 0 0 0 0], [1 0 0 1 0 0 0 0 0], [1 0 0 0 1 0 0 0 0], [1 0 0 0 0 1 0 0 0], [1 0 0 0 0 0 1 0 0], [1 0 0 0 0 0 0 1 0], [1 0 0 0 0 0 0 0 1],
        #  [3 1 0 0 0 0 0 0 0], [0 2 0 0 0 0 0 0 0], [0 1 1 0 0 0 0 0 0], [0 1 0 1 0 0 0 0 0], [0 1 0 0 1 0 0 0 0], [0 1 0 0 0 1 0 0 0], [0 1 0 0 0 0 1 0 0], [0 1 0 0 0 0 0 1 0], [0 1 0 0 0 0 0 0 1],
        #  [3 0 1 0 0 0 0 0 0], [0 1 1 0 0 0 0 0 0], [0 0 2 0 0 0 0 0 0], [0 0 1 1 0 0 0 0 0], [0 0 1 0 1 0 0 0 0], [0 0 1 0 0 1 0 0 0], [0 0 1 0 0 0 1 0 0], [0 0 1 0 0 0 0 1 0], [0 0 1 0 0 0 0 0 1],
        #  [3 0 0 1 0 0 0 0 0], [0 1 0 1 0 0 0 0 0], [0 0 1 1 0 0 0 0 0], [0 0 0 2 0 0 0 0 0], [0 0 0 1 1 0 0 0 0], [0 0 0 1 0 1 0 0 0], [0 0 0 1 0 0 1 0 0], [0 0 0 1 0 0 0 1 0], [0 0 0 1 0 0 0 0 1],
        #  [3 0 0 0 1 0 0 0 0], [0 1 0 0 1 0 0 0 0], [0 0 1 0 1 0 0 0 0], [0 0 0 1 1 0 0 0 0], [0 0 0 0 2 0 0 0 0], [0 0 0 0 1 1 0 0 0], [0 0 0 0 1 0 1 0 0], [0 0 0 0 1 0 0 1 0], [0 0 0 0 1 0 0 0 1]
        #  [3 0 0 0 0 1 0 0 0], [0 1 0 0 0 1 0 0 0], [0 0 1 0 0 1 0 0 0], [0 0 0 1 0 1 0 0 0], [0 0 0 0 1 1 0 0 0], [0 0 0 0 0 2 0 0 0], [0 0 0 0 0 1 1 0 0], [0 0 0 0 0 1 0 1 0], [0 0 0 0 0 1 0 0 1]
        #  [3 0 0 0 0 0 1 0 0], [0 1 0 0 0 0 1 0 0], [0 0 1 0 0 0 1 0 0], [0 0 0 1 0 0 1 0 0], [0 0 0 0 1 0 1 0 0], [0 0 0 0 0 1 1 0 0], [0 0 0 0 0 0 2 0 0], [0 0 0 0 0 0 1 1 0], [0 0 0 0 0 0 1 0 1]
        #  [3 0 0 0 0 0 0 1 0], [0 1 0 0 0 0 0 1 0], [0 0 1 0 0 0 0 1 0], [0 0 0 1 0 0 0 1 0], [0 0 0 0 1 0 0 1 0], [0 0 0 0 0 1 0 1 0], [0 0 0 0 0 0 1 1 0], [0 0 0 0 0 0 0 2 0], [0 0 0 0 0 0 0 1 1]
        #  [3 0 0 0 0 0 0 0 1], [0 1 0 0 0 0 0 0 1], [0 0 1 0 0 0 0 0 1], [0 0 0 1 0 0 0 0 1], [0 0 0 0 1 0 0 0 1], [0 0 0 0 0 1 0 0 1], [0 0 0 0 0 0 1 0 1], [0 0 0 0 0 0 0 1 1], [0 0 0 0 0 0 0 0 2]]

        self.assertEqual(True, np.allclose(correct_array_2, generated_array_2))
        self.assertEqual(True, np.allclose(correct_coefficients_2, generated_coefficients_2))


if __name__ == '__main__':
    unittest.main()