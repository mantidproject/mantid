import AbinsParameters
import math
import numpy as np

class FrequencyGenerator(object):
    """
    Class which generates frequencies for both overtones and combinations.
    """

    def __init__(self):
        self._bins = np.arange(0.0, AbinsParameters.max_wavenumber, AbinsParameters.bin_width)
        self._bin_size = len(self._bins)
        super(FrequencyGenerator, self).__init__()


    def rebin_array(self, array=None):
        """
        Rebins array in order to save memory and speed up calculations.
        @param array: array to be rebined
        @return: rebined array
        """
        inds = np.digitize(array, self._bins)
        new_array = np.asarray([array[inds == i].mean() for i in range(1, self._bin_size)])
        return np.extract(np.isfinite(new_array), new_array)


    def construct_freq_overtones(self, fundamentals=None, quantum_order=None):
        return self.rebin_array(np.multiply(fundamentals, max(1, quantum_order)))


    def construct_freq_combinations(self, previous_array=None, fundamentals_array=None, quantum_order=None):
        """
        Generates frequencies for the given order of quantum event.
        @param previous_array: array with frequencies for the previous quantum effect
        @param fundamentals_array: array with frequencies for fundamentals
        @param quantum_order: number of quantum order effect for which new array should be constructed
        @return: array with frequencies quantum number
        """
        # case of fundamentals
        if quantum_order == AbinsParameters.fundamentals:
            return self.rebin_array(fundamentals_array)

        # higher order effects
        else:

            fundamentals_size = fundamentals_array.size
            previous_size = previous_array.size
            new_array = np.zeros(shape=previous_size + fundamentals_size, dtype=AbinsParameters.float_type)
            current_position = 0

            # frequencies for overtones and combinations
            for j in range(fundamentals_size):
                for i in range(previous_size):
                    if new_array.size > AbinsParameters.max_quantum_order_array_size * quantum_order:

                        return self.rebin_array(array=new_array)

                    temp = previous_array[i] + fundamentals_array[j]
                    if 0 < temp < AbinsParameters.max_wavenumber:

                        if current_position == new_array.size:

                            old_array = new_array
                            new_array = np.zeros(shape=current_position + fundamentals_size, dtype=AbinsParameters.float_type)
                            new_array[:current_position] = old_array

                        new_array[current_position] = temp
                        current_position += 1

            return self.rebin_array(array=new_array)



