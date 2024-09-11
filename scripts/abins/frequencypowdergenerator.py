# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

import abins
from abins.constants import FLOAT_ID, INT_ID, INT_TYPE, FUNDAMENTALS, HIGHER_ORDER_QUANTUM_EVENTS


class FrequencyPowderGenerator:
    """
    Class which generates frequencies for quantum order events.
    """

    def __init__(self):
        raise Exception("FrequencyPowderGenerator does not need to be instantiated.")

    @staticmethod
    def construct_freq_combinations(
        previous_array=None, previous_coefficients=None, fundamentals_array=None, fundamentals_coefficients=None, quantum_order=None
    ):
        """
        Generates frequencies for the given order of quantum event.

        :param previous_array: array with frequencies for the previous quantum event
        :param previous_coefficients: coefficients which correspond to the previous order quantum event
        :param fundamentals_array: array with frequencies for fundamentals
        :param fundamentals_coefficients: coefficients for fundamentals
        :param quantum_order: number of quantum order event for which new array should be constructed
        :returns: array with frequencies for the required quantum number event, array which stores coefficients for all
                 frequencies
        """
        if not (
            isinstance(fundamentals_array, np.ndarray) and len(fundamentals_array.shape) == 1 and fundamentals_array.dtype.num == FLOAT_ID
        ):
            raise ValueError("Fundamentals in the form of one dimensional array are expected.")

        if not (
            isinstance(fundamentals_coefficients, np.ndarray)
            and len(fundamentals_coefficients.shape) == 1
            and fundamentals_coefficients.dtype.num == INT_ID
        ):
            raise ValueError("Coefficients of fundamentals in the form of one dimensional array are expected.")

        if fundamentals_coefficients.size != fundamentals_array.size:
            raise ValueError(
                "Inconsistent size of fundamentals and corresponding coefficients. "
                "(%s != %s)" % (fundamentals_coefficients.size, fundamentals_array.size)
            )

        if not (isinstance(quantum_order, int) and FUNDAMENTALS <= quantum_order <= HIGHER_ORDER_QUANTUM_EVENTS + FUNDAMENTALS):
            raise ValueError("Improper value of quantum order event (quantum_order = %s)" % quantum_order)

        # frequencies for fundamentals
        if quantum_order == FUNDAMENTALS:
            return fundamentals_array, np.arange(start=0, step=1, stop=fundamentals_array.size, dtype=INT_TYPE)

        # higher order quantum events.
        else:
            if not (isinstance(previous_array, np.ndarray) and len(previous_array.shape) == 1 and previous_array.dtype.num == FLOAT_ID):
                raise ValueError("One dimensional previous_array is expected.")

            if not (
                isinstance(previous_coefficients, np.ndarray)
                and len(previous_coefficients.shape) == min(2, quantum_order - 1)
                and previous_coefficients.dtype.num == INT_ID
            ):
                raise ValueError(
                    "Numpy array of previous_coefficients is expected. (%s)" % previous_coefficients,
                    type(previous_coefficients),
                    previous_coefficients.dtype,
                )

            # generate indices
            fundamentals_size = fundamentals_array.size
            previous_size = previous_array.size
            prev_indices = np.arange(start=0, step=1, stop=previous_size, dtype=INT_TYPE)

            # indices in fundamentals array. Not necessarily the same as fundamentals_coefficients!!!
            # This will be the same in case full array with transitions is processed
            # but in case array of transitions is huge and we proceed chunk by chunk then
            # fundamentals_ind differ from fundamentals_coefficients
            fundamentals_ind = np.arange(start=0, step=1, stop=fundamentals_size, dtype=INT_TYPE)

            n = fundamentals_size * previous_size
            num_of_arrays = 2
            ind = np.zeros(shape=(n, num_of_arrays), dtype=INT_TYPE)
            ind[:, 0] = np.repeat(prev_indices, fundamentals_size)
            ind[:, 1] = np.tile(fundamentals_ind, previous_size)

            # calculate energies for quantum order event
            energies = np.take(a=previous_array, indices=ind[:, 0]) + np.take(a=fundamentals_array, indices=ind[:, 1])

            # calculate coefficients which allow to express those energies in terms of fundamentals
            coeff = np.zeros(shape=(quantum_order, energies.size), dtype=INT_TYPE)

            if previous_coefficients.ndim == 1:
                previous_coefficients_dim = 1
            else:
                previous_coefficients_dim = previous_coefficients.shape[-1]

            coeff[:previous_coefficients_dim] = np.take(a=previous_coefficients, indices=ind[:, 0])
            coeff[previous_coefficients_dim] = np.take(a=fundamentals_coefficients, indices=ind[:, 1])
            coeff = coeff.T

            # extract energies within valid energy window
            valid_indices = energies < abins.parameters.sampling["max_wavenumber"]

            return energies[valid_indices], coeff[valid_indices]
