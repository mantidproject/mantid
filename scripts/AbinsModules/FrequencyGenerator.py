import AbinsParameters
import math
import numpy as np

class FrequencyGenerator(object):

    def truncate_array(self, array=None):
        end = array.size
        for el in range(array.size - 1, 0, -1):
            if array[el] > AbinsParameters.eps:
                end = el
                break
        return array[:end]


    def expand_freq(self, previous_array=None, fundamentals_array=None, quantum_order=None):
        """
        Generates frequencies for the given order of quantum event.
        @param previous_array: array with frequencies for the previous quantum effect
        @param fundamentals_array: array with frequencies for fundamentals
        @param quantum_order: number of quantum order effect for which new array should be constructed
        @return: array with frequencies quantum number
        """
        # case of fundamentals
        if quantum_order == 1:
            return fundamentals_array

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

                        return self.truncate_array(array=new_array)

                    temp = previous_array[i] + fundamentals_array[j]
                    if 0 < temp < AbinsParameters.max_wavenumber:

                        if current_position == new_array.size:

                            old_array = new_array
                            new_array = np.zeros(shape=current_position + fundamentals_size, dtype=AbinsParameters.float_type)
                            new_array[:current_position] = old_array

                        new_array[current_position] = temp
                        current_position += 1

                    else:
                        break

            return self.truncate_array(array=new_array)



