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


    def calculate_q_powder(self, overtones=None, combinations=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        if not isinstance(overtones, bool):
            raise ValueError("Invalid value of overtones. Expected values are: True, False.")

        k_data = self._k_points_data.extract()
        fundamental_frequencies = np.multiply(k_data["frequencies"][0], 1.0 / AbinsConstants.cm1_2_hartree)

        # fundamentals and higher quantum order effects
        if overtones:
            q_dim = AbinsConstants.fundamentals_dim + AbinsConstants.higher_order_quantum_effects_dim

        # only fundamentals
        else:
            q_dim = AbinsConstants.fundamentals_dim

        q_data = {}

        local_freq = None
        for quantum_order in range(AbinsConstants.fundamentals, q_dim):
            if combinations:
                local_freq = self.construct_freq_combinations(previous_array=local_freq, fundamentals_array=fundamental_frequencies, quantum_order=quantum_order)
            else: # only fundamentals (and optionally overtones)
                local_freq = self.construct_freq_overtones(fundamentals=fundamental_frequencies, quantum_order=quantum_order)
            q_data["order_%s" % quantum_order] = np.multiply(np.multiply(local_freq, local_freq),  AbinsParameters.TOSCA_constant)

        return q_data


    def q_powder_for_scaling(self, fundamental_frequencies=None, q_dim=None):
        q_data = {}
        for quantum_order in range(AbinsConstants.fundamentals, q_dim):
            local_freq = self.construct_freq_overtones(fundamentals=fundamental_frequencies, quantum_order=quantum_order)
            q_data["order_%s" % quantum_order] = np.multiply(np.multiply(local_freq, local_freq),  AbinsParameters.TOSCA_constant)
        return q_data


    def collect_K_data(self, k_points_data=None):
        """
        Collect k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")

        self._k_points_data = k_points_data


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        """

        # noinspection PyTypeChecker
        all_points = AbinsParameters.pkt_per_peak * frequencies.shape[0]

        broadened_spectrum = np.zeros(shape=all_points, dtype=AbinsConstants.float_type)
        points_freq = np.zeros(shape=all_points, dtype=AbinsConstants.float_type)

        for indx, freq in np.ndenumerate(frequencies):

            sigma = AbinsParameters.TOSCA_A * freq * freq + AbinsParameters.TOSCA_B * freq + AbinsParameters.TOSCA_C
            local_start = indx[0] * AbinsParameters.pkt_per_peak

            temp = np.array(np.linspace(freq - AbinsParameters.fwhm * sigma,
                                        freq + AbinsParameters.fwhm * sigma,
                                        num=AbinsParameters.pkt_per_peak))

            points_freq[local_start:local_start + AbinsParameters.pkt_per_peak] = temp
            broadened_spectrum[local_start:local_start + AbinsParameters.pkt_per_peak] = np.convolve(s_dft[indx[0]], self._gaussian(sigma=sigma, points=temp, center=freq))

        return points_freq, broadened_spectrum


    def _gaussian(self, sigma=None, points=None, center=None):

        """
        @param sigma: sigma defines width of Gaussian
        @param points: points for which Gaussian should be evaluated
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma

        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-np.power(points - center, 2) / sigma_factor)
