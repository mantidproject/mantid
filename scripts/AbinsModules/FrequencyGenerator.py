import AbinsParameters
import math
import numpy as np

def expand_freq(previous_array=None, reference_array=None, quantum_order=None, start=None):
    """
    Generates frequencies for the given order of quantum event.
    @param previous_array: array with frequencies for the previous quantum effect
    @param reference_array: array with frequencies for fundamentals
    @param quantum_order: number of quantum order effect for which new array should be constructed
    @return: array with frequencies quantum number
    """
    # case of fundamentals
    if quantum_order == 1:
        return reference_array

    # higher order effects
    else:

        # frequencies for simple overtones
        reference_size = reference_array.size
        new_array = np.multiply(reference_array, quantum_order)
        current_position = new_array.size

        # frequencies for combinations
        for i in range(previous_array.size):
            for j in range(start, reference_array.size):
                temp = previous_array[i] + reference_array[j]
                if 0 < temp < AbinsParameters.max_wavenumber:
                    if current_position == new_array.size:

                        old_array = new_array
                        new_array = np.zeros(shape=current_position + reference_size, dtype=AbinsParameters.float_type)
                        new_array[:current_position] = old_array

                    new_array[current_position] = temp
                    current_position += 1

        # trunk extra space at the end of the array to save some memory
        end = new_array.size
        for el in range(new_array.size -1, 0, -1):

            if new_array[el] > AbinsParameters.eps:
                end = el
                break

        return new_array[:end]
