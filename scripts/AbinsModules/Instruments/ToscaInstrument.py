from __future__ import (absolute_import, division, print_function)
import numpy as np

from .Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants
from AbinsModules.FrequencyPowderGenerator import FrequencyPowderGenerator


class ToscaInstrument(Instrument, FrequencyPowderGenerator):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name
        self._k_points_data = None
        super(ToscaInstrument, self).__init__()

    def calculate_q_powder(self, input_data=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        :param input_data: frequencies which should be used to construct Q2
        """

        k2_i = (input_data + AbinsParameters.tosca_final_neutron_energy) * AbinsConstants.TOSCA_CONSTANT
        k2_f = AbinsParameters.tosca_final_neutron_energy * AbinsConstants.TOSCA_CONSTANT
        result = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * AbinsParameters.tosca_cos_scattering_angle
        return result

    def collect_K_data(self, k_points_data=None):
        """
        Collect k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """

        self._k_points_data = k_points_data

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        """
        if AbinsParameters.pkt_per_peak == 1:

            points_freq = frequencies
            broadened_spectrum = s_dft

        else:

            # freq_num: number of transition energies for the given quantum order event

            # sigma[freq_num]
            sigma = AbinsParameters.tosca_a * frequencies ** 2 + \
                    AbinsParameters.tosca_b * frequencies + \
                    AbinsParameters.tosca_c

            # start[freq_num]
            start = frequencies - AbinsParameters.fwhm * sigma

            # stop[freq_num]
            stop = frequencies + AbinsParameters.fwhm * sigma

            # step[freq_num]
            step = (stop - start) / float((AbinsParameters.pkt_per_peak - 1))

            # matrix_step[freq_num, AbinsParameters.pkt_per_peak]
            matrix_step = np.array([step, ] * AbinsParameters.pkt_per_peak).transpose()

            # matrix_start[freq_num, AbinsParameters.pkt_per_peak]
            matrix_start = np.array([start, ] * AbinsParameters.pkt_per_peak).transpose()

            # broad_freq[freq_num, AbinsParameters.pkt_per_peak]
            broad_freq = (np.arange(0, AbinsParameters.pkt_per_peak) * matrix_step) + matrix_start
            broad_freq[..., -1] = stop

            # sigma_matrix[freq_num, AbinsParameters.pkt_per_peak]
            sigma_matrix = np.array([sigma, ] * AbinsParameters.pkt_per_peak).transpose()

            # frequencies_matrix[freq_num, AbinsParameters.pkt_per_peak]
            frequencies_matrix = np.array([frequencies, ] * AbinsParameters.pkt_per_peak).transpose()

            # gaussian_val[freq_num, AbinsParameters.pkt_per_peak]
            gaussian_val = self._gaussian(sigma=sigma_matrix, points=broad_freq, center=frequencies_matrix)

            # s_dft_matrix[freq_num, AbinsParameters.pkt_per_peak]
            s_dft_matrix = np.array([s_dft, ] * AbinsParameters.pkt_per_peak).transpose()

            # temp_spectrum[freq_num, AbinsParameters.pkt_per_peak]
            temp_spectrum = s_dft_matrix * gaussian_val

            points_freq = np.ravel(broad_freq)
            broadened_spectrum = np.ravel(temp_spectrum)

        return points_freq, broadened_spectrum

    # noinspection PyMethodMayBeStatic
    def _gaussian(self, sigma=None, points=None, center=None):
        """
        @param sigma: sigma defines width of Gaussian
        @param points: points for which Gaussian should be evaluated
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma
        return 1.0 / np.sqrt(sigma_factor * np.pi) * np.exp(-(points - center) ** 2 / sigma_factor)
