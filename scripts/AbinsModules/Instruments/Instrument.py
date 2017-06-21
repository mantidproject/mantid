from __future__ import (absolute_import, division, print_function)
from AbinsModules import AbinsParameters
import numpy as np


# noinspection PyPep8Naming
class Instrument(object):

    _name = None

    def calculate_q_powder(self, input_data=None):
        """
        Calculates Q2.


        :param  input_data: data from which Q2 should be calculated
        :return:  numpy array with Q2 data
        """

        return None

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete spectrum with the  resolution function for the particular instrument.

        :param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT

       """
        return None

    # should be treated as a protected function
    def _convolve_with_resolution_function_helper(self, frequencies=None, s_dft=None, sigma=None, pkt_per_peak=None,
                                                  gaussian=None):
        """
        :param frequencies: discrete frequencies in cm^-1
        :parameter s_dft: discrete values of S
        :param sigma: width of resolution function
        :param pkt_per_peak: number of points per peak
        :param gaussian: gaussian-like function used to broaden peaks
        :return: frequencies for which peaks have been broadened, corresponding S
        """
        fwhm = AbinsParameters.fwhm

        # freq_num: number of transition energies for the given quantum order event
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
        dirac_val = gaussian(sigma=sigma_matrix, points=broad_freq, center=frequencies_matrix)

        # s_dft_matrix[freq_num, pkt_per_peak]
        s_dft_matrix = np.array([s_dft, ] * pkt_per_peak).transpose()

        # temp_spectrum[freq_num, pkt_per_peak]
        temp_spectrum = s_dft_matrix * dirac_val

        points_freq = np.ravel(broad_freq)
        broadened_spectrum = np.ravel(temp_spectrum)

        return points_freq, broadened_spectrum

    # should be treated as a protected function
    def _gaussian(self, sigma=None, points=None, center=None):
        """
        :param sigma: sigma defines width of Gaussian
        :param points: points for which Gaussian should be evaluated
        :param center: center of Gaussian
        :return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma
        return 1.0 / np.sqrt(sigma_factor * np.pi) * np.exp(-(points - center) ** 2 / sigma_factor)

    def __str__(self):
        return self._name

    def get_name(self):
        return self._name
