from __future__ import (absolute_import, division, print_function)
import numpy as np
import six

from .Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants


class TwoDMap(Instrument):
    def __init__(self, name):

        super(TwoDMap, self).__init__()
        self._name = name
        self._sigma = AbinsParameters.delta_width

        self._q_powder_length = np.arange(start=AbinsParameters.q_start, step=AbinsParameters.q_step,
                                          stop=AbinsParameters.q_end)

        self._q_powder = self._q_powder_length * self._q_powder_length

    def calculate_q_powder(self, input_data=None):
        """
        Returns Q powder data for index input_data.
        :param input_data: index of Q2
        """
        if isinstance(input_data, six.integer_types) and 0 <= input_data < self._q_powder.size:

            return self._q_powder[input_data]
        else:
            raise ValueError("Invalid Q index. (q_index = %s )" % input_data)

    def get_q_powder_size(self):
        return self._q_powder.size

    def get_q_length(self):
        return self._q_powder_length

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TwoDMap instrument.
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        """

        if AbinsParameters.pkt_per_peak == 1:

            points_freq = frequencies
            broadened_spectrum = s_dft

        else:
            # override user advanced parameters
            pkt_per_peak = 5
            fwhm = 0.1
            # freq_num: number of transition energies for the given quantum order event

            # sigma[freq_num]
            sigma = np.zeros(shape=frequencies.size, dtype=AbinsConstants.FLOAT_TYPE)
            sigma.fill(self._sigma)

            # start[freq_num]
            start = frequencies - fwhm * sigma

            # stop[freq_num]
            stop = frequencies + fwhm * sigma

            # step[freq_num]
            step = (stop - start) / float((pkt_per_peak - 1))

            # matrix_step[freq_num, pkt_per_peak]
            matrix_step = np.array([step, ] * pkt_per_peak).transpose()

            # matrix_start[freq_num, pkt_per_peak]
            matrix_start = np.array([start, ] * pkt_per_peak).transpose()

            # broad_freq[freq_num, pkt_per_peak]
            broad_freq = (np.arange(0, pkt_per_peak) * matrix_step) + matrix_start
            broad_freq[..., -1] = stop

            # sigma_matrix[freq_num, pkt_per_peak]
            sigma_matrix = np.array([sigma, ] * pkt_per_peak).transpose()

            # frequencies_matrix[freq_num, pkt_per_peak]
            frequencies_matrix = np.array([frequencies, ] * pkt_per_peak).transpose()

            # gaussian_val[freq_num, pkt_per_peak]
            dirac_val = self._dirac_delta(sigma=sigma_matrix, points=broad_freq, center=frequencies_matrix)

            # s_dft_matrix[freq_num, pkt_per_peak]
            s_dft_matrix = np.array([s_dft, ] * pkt_per_peak).transpose()

            # temp_spectrum[freq_num, pkt_per_peak]
            temp_spectrum = s_dft_matrix * dirac_val

            points_freq = np.ravel(broad_freq)
            broadened_spectrum = np.ravel(temp_spectrum)

        return points_freq, broadened_spectrum

    # noinspection PyMethodMayBeStatic
    def _dirac_delta(self, sigma=None, points=None, center=None):

        """
        Dirac delta is implemented as a very narrow Gaussian function.
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma
        return 1.0 / np.sqrt(sigma_factor * np.pi) * np.exp(-(points - center) ** 2 / sigma_factor)

    def get_nspec(self):
        return self._q_powder.size
