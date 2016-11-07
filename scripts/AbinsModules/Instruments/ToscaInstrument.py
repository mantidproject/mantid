import numpy as np
import math

from Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules import AbinsConstants
from AbinsModules.KpointsData import KpointsData
from AbinsModules.FrequencyPowderGenerator import FrequencyPowderGenerator


class ToscaInstrument(Instrument, FrequencyPowderGenerator):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name
        self._k_points_data = None
        super(ToscaInstrument, self).__init__()

    def calculate_q_powder(self, quantum_order_events_num=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        if not isinstance(quantum_order_events_num, int):
            raise ValueError("Invalid value of overtones. Expected values are: True, False.")

        const = 1.0 / AbinsConstants.cm1_2_hartree
        fundamental_frequencies = self._k_points_data["frequencies"][0][AbinsConstants.first_optical_phonon:] * const

        q_data = {}

        local_freq = fundamental_frequencies
        local_coeff = np.eye(fundamental_frequencies.size, dtype=AbinsConstants.float_type)
        for quantum_order in range(AbinsConstants.fundamentals, quantum_order_events_num + AbinsConstants.q_last_index):

            local_freq, local_coeff = self.construct_freq_combinations(previous_array=local_freq,
                                                                       previous_coefficients=local_coeff,
                                                                       fundamentals_array=fundamental_frequencies,
                                                                       quantum_order=quantum_order)

            k2_i = (local_freq + AbinsParameters.TOSCA_final_neutron_energy) * AbinsConstants.TOSCA_constant
            k2_f = AbinsParameters.TOSCA_final_neutron_energy * AbinsConstants.TOSCA_constant
            temp = k2_i + k2_f - 2 * (k2_i * k2_f) ** 0.5 * AbinsParameters.TOSCA_cos_scattering_angle

            q_data["order_%s" % quantum_order] = temp

        return q_data

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

        #  noinspection PyTypeChecker
        all_points = AbinsParameters.pkt_per_peak * frequencies.shape[0]

        broadened_spectrum = np.zeros(shape=all_points, dtype=AbinsConstants.float_type)
        points_freq = np.zeros(shape=all_points, dtype=AbinsConstants.float_type)

        sigma = AbinsParameters.TOSCA_A * frequencies ** 2 + \
                AbinsParameters.TOSCA_B * frequencies + \
                AbinsParameters.TOSCA_C

        for indx, freq in np.ndenumerate(frequencies):

            local_start = indx[0] * AbinsParameters.pkt_per_peak
            broad_freq = np.array(np.linspace(freq - AbinsParameters.fwhm * sigma[indx[0]],
                                              freq + AbinsParameters.fwhm * sigma[indx[0]],
                                              num=AbinsParameters.pkt_per_peak))

            points_freq[local_start:local_start + AbinsParameters.pkt_per_peak] = broad_freq

            # noinspection PyPep8
            temp_spectrum = np.convolve(s_dft[indx[0]], self._gaussian(sigma=sigma[indx[0]],
                                                                      points=broad_freq,
                                                                     center=freq))
            
            broadened_spectrum[local_start:local_start + AbinsParameters.pkt_per_peak] = temp_spectrum


        return points_freq, broadened_spectrum

    def _gaussian(self, sigma=None, points=None, center=None):

        """
        @param sigma: sigma defines width of Gaussian
        @param points: points for which Gaussian should be evaluated
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma
        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-(points - center) ** 2 / sigma_factor)
