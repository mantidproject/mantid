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
        generated_array = self.simple_freq_generator.optimize_array(array=array)
        self.assertEqual(True, np.allclose(array, generated_array))


        # use case: values below acoustic_phonon_threshold are  present. In that case all values equal or lower than
        # AbinsConstants.acoustic_phonon_threshold should be removed from the array
        array_to_be_modified = np.asarray([6, 5, 4, 2, AbinsParameters.acoustic_phonon_threshold, AbinsParameters.acoustic_phonon_threshold - 0.00001, 1])
        generated_array = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        correct_array = np.asarray([1 ,2, 4, 5 ,6])
        self.assertEqual(True, np.allclose(correct_array, generated_array))

        # use case: array has larger size than self._bins and all bins are filled
        array_to_be_modified = np.arange(0, 10000, 0.1)
        generated_array = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        # in case array_to_be_modified is bigger and covers whole interval (no nan values) i-th element of self._bins
        # and  generated_array should always differ by less than  AbinsParameters.bin_width
        self.assertEqual(True,  np.allclose(self._bins, generated_array, rtol=0.0, atol=AbinsParameters.bin_width))

        # use case: array has larger size than self._bins and not all bins are filled
        # noinspection PyTypeChecker
        array_to_be_modified = np.tile(np.asarray([2.0 + AbinsParameters.bin_width, 1.0 + AbinsParameters.bin_width]), 5000).ravel() # array with size 10000; alternating values 1+ AbinsParameters.bin_width ,2+ AbinsParameters.bin_width
        generated_array = self.simple_freq_generator.optimize_array(array=array_to_be_modified)
        # returned values should be sorted
        correct_array = np.asarray([1.0 + AbinsParameters.bin_width, 2.0 + AbinsParameters.bin_width])
        self.assertEqual(True, np.allclose(correct_array, generated_array))


    def test_construct_freq_overtones(self):

        # wrong array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_overtones(fundamentals_array="wrong_array", quantum_order=0)

        # wrong quantum order
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_overtones(fundamentals_array=np.asarray([1,2]), quantum_order=-1)

        # use case: fundamentals
        array = np.arange(1.0, 10.0 ,1.0)
        generated_array = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=AbinsConstants.fundamentals)
        self.assertEqual(True, np.allclose(array, generated_array))

        # use case: overtones
        q_order = 4
        array = np.arange(1.0, 10.0 ,1.0)
        generated_array = self.simple_freq_generator.construct_freq_overtones(fundamentals_array=array, quantum_order=q_order)
        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(np.multiply(array, q_order), generated_array))


    def test_construct_freq_combinations(self):


        # wrong previous array
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=np.asarray([1,2]), previous_array=[1,2] , quantum_order=AbinsConstants.fundamentals)

        # wrong fundamentals
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=[1,2], previous_array=np.asarray([1,2]) , quantum_order=AbinsConstants.fundamentals)

        # wrong quantum order effect
        with self.assertRaises(ValueError):
            self.simple_freq_generator.construct_freq_combinations(fundamentals_array=np.asarray([1,2]), previous_array=np.asarray([1,2]) , quantum_order=-1)

        # use case quantum order effect 1 (fundamentals)
        array = np.arange(1.0, 10.0 ,1.0)
        generated_array = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array, previous_array=array , quantum_order=AbinsConstants.fundamentals) #1st
        self.assertEqual(True, np.allclose(array, generated_array))

        # use case: higher quantum effects and array smaller than bins
        array = np.arange(1.0 * AbinsParameters.bin_width, 10.0 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        temp_size = array.size
        generated_array = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array, previous_array=array, quantum_order=AbinsConstants.first_overtone + 1) #3rd
        correct_array = np.tile(array, temp_size)
        for i in range(array.size):
            for j in range(temp_size):
                correct_array[i * temp_size + j] += array[i]
        correct_array = np.sort(correct_array)
        self.assertEqual(True, np.allclose(correct_array, generated_array))

        # use case: higher quantum effects and array larger than bins
        # array has the same elements as bins and two extra entries which are outside interval of bins
        array = np.arange(AbinsParameters.acoustic_phonon_threshold, AbinsParameters.max_wavenumber + 2 * AbinsParameters.bin_width, AbinsParameters.bin_width)
        generated_array = self.simple_freq_generator.construct_freq_combinations(fundamentals_array=array, previous_array=array, quantum_order=AbinsConstants.first_overtone + 1) #3rd

        # -2 because 2 entries outside bins interval are neglected
        #
        # we start from 2 because we first construct an array with values larger than
        # AbinsParameters.acoustic_phonon_threshold and smaller than AbinsParameters.max_wavenumber
        # (size of that array is array.size * array.size - 3 ) and because number of bins is one less then  number of
        # points in the array to be rebined the resulting array should have size array.size - 4
        # we have to shift all elements by AbinsParameters.bin_width because the first element in generated_array is AbinsParameters.bin_width
        # NOT 2 * AbinsParameters.bin_width

        self.assertEqual(True, np.allclose(array[2:array.size - 2] - AbinsParameters.bin_width, generated_array))


if __name__ == '__main__':
    unittest.main()