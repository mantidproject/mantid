import AbinsParameters
import AbinsConstants
import math
import numpy as np

class FrequencyPowderGenerator(object):
    """
    Class which generates frequencies for both overtones and combinations.
    """

    def __init__(self):
        self._bins = np.arange(AbinsParameters.acoustic_phonon_threshold, AbinsParameters.max_wavenumber, AbinsParameters.bin_width)
        self._bin_size = len(self._bins)
        super(FrequencyPowderGenerator, self).__init__()


    def optimize_array(self, array=None):
        """
        Rebins 1D array in order to save memory and speed up calculations. After rebining array is sorted.
        @param array: array to be rebined and sorted
        @return: sorted and rebined array
        """
        if not (isinstance(array, np.ndarray) and len(array.shape) == 1):
            raise ValueError("One dimentional array is expected.")

        if array.size < self._bin_size:

            # noinspection PyTypeChecker
            return np.sort(np.extract(array > AbinsParameters.acoustic_phonon_threshold, array)) # remove acoustic frequencies and sort

        inds = np.digitize(array, self._bins)
        new_array = np.asarray([array[inds == i].mean() for i in range(1, self._bin_size)]) #rebin values only inside bins interval
        new_array = np.extract(np.isfinite(new_array), new_array) # remove nan from array
        # noinspection PyTypeChecker
        return np.sort(np.extract(new_array > AbinsParameters.acoustic_phonon_threshold, new_array)) # remove acoustic frequencies and sort


    def construct_freq_overtones(self, fundamentals_array=None, quantum_order=None):

        if not (isinstance(fundamentals_array, np.ndarray) and len(fundamentals_array.shape) == 1):
            raise ValueError("Fundamentals in the form of one dimentional array are expected.")
        if not  (isinstance(quantum_order, int) and
                 AbinsConstants.fundamentals <= quantum_order <= AbinsConstants.higher_order_quantum_effects_dim + AbinsConstants.fundamentals_dim):
            raise ValueError("Improper value of quantum order effect.")

        return self.optimize_array(np.multiply(fundamentals_array, max(1, quantum_order)))


    def construct_freq_combinations(self, previous_array=None, fundamentals_array=None, quantum_order=None):
        """
        Generates frequencies for the given order of quantum event.
        @param previous_array: array with frequencies for the previous quantum effect
        @param fundamentals_array: array with frequencies for fundamentals
        @param quantum_order: number of quantum order effect for which new array should be constructed
        @return: array with frequencies for the required quantum number event
        """
        if not (isinstance(fundamentals_array, np.ndarray) and len(fundamentals_array.shape) == 1):
            raise ValueError("Fundamentals in the form of one dimentional array are expected.")
        if not (isinstance(previous_array, np.ndarray) and len(previous_array.shape) == 1):
            raise ValueError("One dimentional array is expected.")
        if not (isinstance(quantum_order, int) and
                            AbinsConstants.fundamentals <= quantum_order <= AbinsConstants.higher_order_quantum_effects_dim  + AbinsConstants.fundamentals_dim):
            raise ValueError("Improper value of quantum order effect.")

        # frequencies for fundamentals
        if quantum_order == AbinsConstants.fundamentals:
            return self.optimize_array(fundamentals_array)

        # higher order effects quantum events
        else:

            fundamentals_size = fundamentals_array.size
            previous_size = previous_array.size
            new_array = np.zeros(shape=previous_size + fundamentals_size, dtype=AbinsConstants.float_type)
            current_position = 0

            # frequencies for higher overtones and combinations
            for j in range(fundamentals_size):
                for i in range(previous_size):

                    # in case array is really big stop constructing array and return what we have
                    if new_array.size > AbinsConstants.max_quantum_order_array_size * quantum_order:

                        return self.optimize_array(array=new_array)

                    temp = previous_array[i] + fundamentals_array[j]

                    # if value of combination is within desired interval add it to the array
                    if AbinsParameters.acoustic_phonon_threshold < temp < AbinsParameters.max_wavenumber:

                        if current_position == new_array.size:

                            old_array = new_array
                            new_array = np.zeros(shape=current_position + fundamentals_size, dtype=AbinsConstants.float_type)
                            new_array[:current_position] = old_array

                        new_array[current_position] = temp
                        current_position += 1

            return self.optimize_array(array=new_array)



