import AbinsParameters
import AbinsConstants
import math
import numpy as np

class FrequencyPowderGenerator(object):
    """
    Class which generates frequencies for both overtones and combinations.
    """

    def __init__(self):
        super(FrequencyPowderGenerator, self).__init__()


    def construct_freq_overtones(self, fundamentals_array=None, quantum_order=None):

        """
        Calculates frequencies for fundamentals and overtones. It also calculates coefficients which are used to
        express those  frequencies as a linear combination of fundamentals. Each row in an array of coefficients
        corresponds to the  particular  frequency. Elements in the particular row store information about coefficients
        which are needed to decompose frequencies in terms of fundamentals. The number of elements in each row is equal
        to the number of fundamentals. An array with coefficients for n-overtone has the following form:

            [  n 0 0 0 0 ....
               0 n 0 0 0
               0 0 n .......
               .............
               .............  ]

        ,e.g, it is diagonal matrix  with all diagonal elements equal to n.

        @param fundamentals_array: numpy array with fundamentals
        @param quantum_order: quantum order event (n = 1, 2, 3, 4)
        @return: numpy array with frequencies, numpy array with coefficients.
        """
        if not (isinstance(fundamentals_array, np.ndarray) and
                len(fundamentals_array.shape) == 1 and
                fundamentals_array.dtype.num == AbinsConstants.float_id):

            raise ValueError("Fundamentals in the form of one dimentional array are expected.")

        if not  (isinstance(quantum_order, int) and
                 AbinsConstants.fundamentals <= quantum_order < AbinsConstants.higher_order_quantum_effects_dim + AbinsConstants.fundamentals_dim):
            raise ValueError("Improper value of quantum order effect.")

        new_array = fundamentals_array * quantum_order

        # remove frequencies above AbinsParameters.max_wavenumber
        indx =   new_array < AbinsParameters.max_wavenumber
        new_array = new_array[indx]

        # construct array with coefficients
        coefficients = np.eye(new_array.size, dtype=AbinsConstants.int_type) * quantum_order

        return new_array, coefficients


    def construct_freq_combinations(self, previous_array=None, previous_coefficients=None, fundamentals_array=None, quantum_order=None):
        """
        Generates frequencies for the given order of quantum event.
        @param previous_array: array with frequencies for the previous quantum effect
        @param previous_coefficients: coefficients which correspond to the previous order quantum effect
        @param fundamentals_array: array with frequencies for fundamentals
        @param quantum_order: number of quantum order effect for which new array should be constructed
        @return: array with frequencies for the required quantum number event, array which stores coefficients for all frequencies
        """
        if not (isinstance(fundamentals_array, np.ndarray) and
                len(fundamentals_array.shape) == 1 and
                fundamentals_array.dtype.num == AbinsConstants.float_id):

            raise ValueError("Fundamentals in the form of one dimentional array are expected.")

        if not (isinstance(quantum_order, int) and
                AbinsConstants.fundamentals <= quantum_order < AbinsConstants.higher_order_quantum_effects_dim  + AbinsConstants.fundamentals_dim):
            raise ValueError("Improper value of quantum order effect.")

        # frequencies for fundamentals
        if quantum_order == AbinsConstants.fundamentals:
            return fundamentals_array, np.eye(fundamentals_array.size, dtype=AbinsConstants.int_type)

        if not (isinstance(previous_array, np.ndarray) and
                len(previous_array.shape) == 1 and
                previous_array.dtype.num == AbinsConstants.float_id):
            raise ValueError("One dimentional array is expected.")

        if not (isinstance(previous_coefficients, np.ndarray) and
                len(previous_coefficients.shape) == 2 and
                previous_coefficients.dtype.num == AbinsConstants.int_id):
            raise ValueError("One dimentional array is expected.")

        # higher order effects quantum events
        else:

            fundamentals_size = fundamentals_array.size
            previous_size = previous_array.size

            initial_size =  previous_size + fundamentals_size
            new_array = np.zeros(shape=initial_size, dtype=AbinsConstants.float_type)
            new_coefficients =  np.zeros(shape=(initial_size, fundamentals_size), dtype=AbinsConstants.int_type)
            current_position = 0

            for j in range(fundamentals_size):
                for i in range(previous_size):

                    # in case array is really big stop constructing array and return what we have
                    if new_array.size > AbinsConstants.max_array_size * quantum_order:
                        return new_array, new_coefficients

                    # add frequency to the collection of frequencies
                    temp = previous_array[i] + fundamentals_array[j]
                    if AbinsParameters.acoustic_phonon_threshold < temp < AbinsParameters.max_wavenumber:

                        if current_position == new_array.size:

                            old_array = new_array
                            old_coefficients = new_coefficients

                            new_size = current_position + fundamentals_size
                            new_array = np.zeros(shape=new_size, dtype=AbinsConstants.float_type)
                            new_coefficients = np.zeros(shape=(new_size, fundamentals_size), dtype=AbinsConstants.int_type)

                            new_array[:current_position] = old_array
                            new_coefficients[:current_position] = old_coefficients

                        new_array[current_position] = temp

                        new_coefficients[current_position] = np.copy(previous_coefficients[i])
                        new_coefficients[current_position, j] += 1
                        current_position += 1

            # remove unnecessary zeros from the end of array
            inds = new_array > AbinsParameters.min_wavenumber

            return new_array[inds], new_coefficients[inds]



