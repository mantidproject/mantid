import AbinsParameters
import AbinsConstants
import math
import numpy as np


class FrequencyPowderGenerator(object):
    """
    Class which generates frequencies for quantum order events.
    """

    def __init__(self):
        super(FrequencyPowderGenerator, self).__init__()

    def construct_freq_combinations(self, previous_array=None, previous_coefficients=None,
                                    fundamentals_array=None, quantum_order=None):
        """
        Generates frequencies for the given order of quantum event.
        @param previous_array: array with frequencies for the previous quantum event
        @param previous_coefficients: coefficients which correspond to the previous order quantum event
        @param fundamentals_array: array with frequencies for fundamentals
        @param quantum_order: number of quantum order event for which new array should be constructed
        @return: array with frequencies for the required quantum number event, array which stores coefficients for all
                 frequencies
        """
        if not (isinstance(fundamentals_array, np.ndarray) and
                len(fundamentals_array.shape) == 1 and
                fundamentals_array.dtype.num == AbinsConstants.float_id):

            raise ValueError("Fundamentals in the form of one dimentional array are expected.")

        if not (isinstance(quantum_order, int) and
                AbinsConstants.fundamentals <= quantum_order <=
                AbinsConstants.higher_order_quantum_events + AbinsConstants.fundamentals):
            raise ValueError("Improper value of quantum order event (quantum_order = %s)" % quantum_order)

        # frequencies for fundamentals
        if quantum_order == AbinsConstants.fundamentals:
            return fundamentals_array, np.arange(fundamentals_array.size)

        # higher order quantum events
        else:

            if not (isinstance(previous_array, np.ndarray) and
                    len(previous_array.shape) == 1 and
                    previous_array.dtype.num == AbinsConstants.float_id):
                raise ValueError("One dimentional array is expected.")

            if not isinstance(previous_coefficients, np.ndarray):
                raise ValueError("Numpy array is expected.")

            # generate indices
            fundamentals_size = fundamentals_array.size
            fund_indices = np.arange(fundamentals_size)
            previous_size = previous_array.size
            prev_indices = np.arange(previous_size)
            n = fundamentals_size * previous_size
            num_of_arrays = 2
            ind = np.zeros(shape=(n, num_of_arrays), dtype=AbinsConstants.int_type)
            ind[:, 0] = np.repeat(prev_indices, fundamentals_size)
            ind[:, 1] = np.tile(fund_indices, previous_size)

            # calculate energies for quantum order event
            energies = np.take(a=previous_array, indices=ind[:, 0]) + np.take(a=fundamentals_array, indices=ind[:, 1])

            # calculate coefficients which allow to express those energies in terms of fundamentals
            coeff = np.zeros(shape=(quantum_order, energies.size), dtype=AbinsConstants.int_type)

            if previous_coefficients.ndim == 1:
                previous_coefficients_dim = 1
            else:
                previous_coefficients_dim = previous_coefficients.shape[-1]

            coeff[:previous_coefficients_dim] = np.take(a=previous_coefficients, indices=ind[:, 0])
            coeff[previous_coefficients_dim] = np.take(a=fund_indices, indices=ind[:, 1])
            coeff = coeff.T

            # extract energies within valid energy window
            valid_indices = energies < AbinsParameters.max_wavenumber

            return energies[valid_indices], coeff[valid_indices]
