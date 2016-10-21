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


    def test_optimize_array(self):

        # wrong array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.optimize_array(array="This is string not array")

        # wrong dimension of array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.optimize_array(array=np.asarray([[1, 2], [3, 4]]))

        # simplest use case: no modification over array is expected
        array = np.asarray([1, 2, 4, 5, 6])
        correct_counts = np.ones(array.size)

        generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array)
        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))


        # use case: values below acoustic_phonon_threshold are present. In that case all values equal or lower than
        # AbinsConstants.acoustic_phonon_threshold should be removed from the array
        array_to_be_modified = np.asarray([6, 5, 4, 2, AbinsParameters.acoustic_phonon_threshold, AbinsParameters.acoustic_phonon_threshold - 0.00001, 1])

        generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        correct_array = np.asarray([1 ,2, 4, 5 ,6])
        correct_counts = np.ones(correct_array.size)

        self.assertEqual(True, np.allclose(correct_array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))

        # use case: array has larger size than self._bins and all bins are filled
        array_to_be_modified = np.arange(0, 10000, 0.1 * AbinsParameters.bin_width)


        generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        correct_counts = np.ones(generated_array.size) * AbinsParameters.bin_width / 0.1
        # in case array_to_be_modified is bigger and covers whole interval (no nan values) i-th element of self._bins
        # and  generated_array should always differ by less than  AbinsParameters.bin_width
        self.assertEqual(True,  np.allclose(self._bins, generated_array, rtol=0.0, atol=AbinsParameters.bin_width))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))

        # use case: array has larger size than self._bins and not all bins are filled
        # noinspection PyTypeChecker
        array_to_be_modified = np.tile(np.asarray([2.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width,
                                                   1.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width]), 5000).ravel() # array with size 10000; alternating values 1+ AbinsParameters.bin_width ,2+ AbinsParameters.bin_width
        correct_counts = np.ones(2) * 5000
        generated_array, generated_counts = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        # returned values should be sorted
        correct_array = np.asarray([1.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width,
                                    2.0 - 0.01 * AbinsParameters.bin_width + AbinsParameters.bin_width])
        self.assertEqual(True, np.allclose(correct_array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))


    def test_construct_freq_overtones(self):

        # wrong array
        with self.assertRaises(ValueError):
           self.simple_freq_generator.construct_freq_overtones(fundamentals_array="wrong_array", quantum_order=AbinsConstants.fundamentals)

        # wrong quantum order
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_overtones(fundamentals_array=np.asarray([1,2]), quantum_order=-1)

        # use case: fundamentals
        array = np.arange(AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        correct_counts = np.ones(array.size)
        generated_array, generated_counts = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=AbinsConstants.fundamentals)
        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts ))

        # use case: overtones
        q_order = 4
        array = np.arange(AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        correct_counts = np.ones(array.size)
        generated_array, generated_counts = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=q_order)

        self.assertEqual(True, np.allclose(array * q_order, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))


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
        correct_counts = np.ones(array.size)

        generated_array, generated_counts = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array, previous_array=array , quantum_order=AbinsConstants.fundamentals) #1st

        self.assertEqual(True, np.allclose(array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts, generated_counts))

        # use case: higher quantum effects and array with number of combinations larger than bins
        # (array with combinations should be rebined in that case) and all counts for previous array are ones

        array = np.arange(1.0 * AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)

        temp_size = array.size
        counts = np.ones(temp_size)

        correct_array = np.tile(array, temp_size)

        # array = [ 1.  2.  3.  4.  5.  6.  7.  8.  9.]
        # counts =[ 1., 1., 1., 1., 1., 1., 1., 1.,  1.]
        generated_array, generated_counts = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array,
                                                                                                        previous_array=array,
                                                                                                        previous_counts=counts,
                                                                                                        quantum_order=AbinsConstants.first_overtone + 1) #3rd

        for i in range(array.size):
            for j in range(temp_size):
                correct_array[i * temp_size + j] += array[i]


        correct_array, correct_counts_1 = np.unique(np.sort(correct_array), return_counts=True)

        # Expected values:
        # correct_array= [  2.,   3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.,  12.,  13.,  14.,  15.,  16., 17.,  18.]
        # correct_counts_1 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1]
        self.assertEqual(True, np.allclose(correct_array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts_1, generated_counts))

        # use case: higher quantum effects and array with number of combinations larger than bins
        # (array with combinations should be rebined in that case) and all but first count are ones; first count is three
        counts = np.ones(temp_size)
        counts[0] = 3 # first count is three



        # array = [ 1.  2.  3.  4.  5.  6.  7.  8.  9.]
        # counts =[ 3., 1., 1., 1., 1., 1., 1., 1.,  1.]
        generated_array, generated_counts = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array,
                                                                                                   previous_array=array,
                                                                                                   previous_counts=counts,
                                                                                                   quantum_order=AbinsConstants.first_overtone + 1) #3rd

        # Expected values:
        # correct_array = [  2.,   3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.,  12.,  13.,  14.,  15.,  16., 17.,  18.]
        # correct_counts_2 =[3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.,   8.,   7.,   6.,   5.,   4.,   3., 2.,   1.]

        correct_counts_2 = np.copy(correct_counts_1) # here we want a copy not original correct_counts_1
        correct_counts_2[:9] =correct_counts_2[:9] + 2

        self.assertEqual(True, np.allclose(correct_array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts_2, generated_counts))

        # use case: higher quantum effects and array with number of combinations larger than bins
        # (array with combinations should be rebined in that case) and all counts are 3; first count is three
        counts = np.ones(temp_size) * 3 # all counts are 3

        correct_counts_3 = correct_counts_1 * 3

        # array = [ 1.  2.  3.  4.  5.  6.  7.  8.  9.]
        # counts =[ 3., 3., 3., 3., 3., 3., 3., 3., 3.]
        generated_array, generated_counts = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array,
                                                                                                   previous_array=array,
                                                                                                   previous_counts=counts,
                                                                                                   quantum_order=AbinsConstants.first_overtone + 1) #3rd

        # Expected values:
        # correct_array = [  2.,   3.,   4.,   5.,   6.,   7.,   8.,   9.,  10.,  11.,  12.,  13.,  14.,  15.,  16., 17.,  18.]
        # correct_counts_3 = [ 3.,   6.,   9.,  12.,  15.,  18.,  21.,  24.,  27.,  24.,  21.,  18.,  15.,  12.,   9.,  6.,   3.]
        self.assertEqual(True, np.allclose(correct_array, generated_array))
        self.assertEqual(True, np.allclose(correct_counts_3, generated_counts))

if __name__ == '__main__':
    unittest.main()