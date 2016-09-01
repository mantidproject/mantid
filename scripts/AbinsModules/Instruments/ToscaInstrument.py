import numpy as np
import math

from Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules.KpointsData import KpointsData

class ToscaInstrument(Instrument):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name
        self._k_points_data = None


    def calculate_q_powder(self):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        _k_data = self._k_points_data.extract()
        frequencies = np.multiply(_k_data["frequencies"], 1.0 / AbinsParameters.cm1_2_hartree)
        return np.multiply(np.multiply(frequencies, frequencies), AbinsParameters.TOSCA_constant)


    def collect_K_data(self, k_points_data=None):
        """
        Collect k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")

        self._k_points_data = k_points_data


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, start=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """

        # noinspection PyTypeChecker
        all_points = AbinsParameters.pkt_per_peak * frequencies.shape[0]

        broadened_spectrum = np.zeros(all_points, dtype=AbinsParameters.float_type)
        start_broaden = start * AbinsParameters.pkt_per_peak
        for indx, freq in np.ndenumerate(frequencies[start:]):

            sigma = AbinsParameters.TOSCA_A * freq * freq + AbinsParameters.TOSCA_B * freq + AbinsParameters.TOSCA_C

            points_freq = np.array(np.linspace(freq - AbinsParameters.fwhm * sigma,
                                               freq + AbinsParameters.fwhm * sigma,
                                               num=AbinsParameters.pkt_per_peak))

            start = indx[0] * AbinsParameters.pkt_per_peak + start_broaden
            broadened_spectrum[start:start + AbinsParameters.pkt_per_peak] = np.convolve(s_dft[indx[0]], self._gaussian(sigma=sigma, points=points_freq, center=freq))

        return broadened_spectrum


    def produce_abscissa(self, frequencies=None, start=None):
        """
        Creates abscissa for convoluted spectrum in case of TOSCA.
        @param frequencies:  DFT frequencies for which frequencies which correspond to broadened spectrum should be regenerated (frequencies in cm-1)
        @return: abscissa for convoluted S
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """
        all_points = AbinsParameters.pkt_per_peak * frequencies.shape[0]
        abscissa = np.zeros(all_points, dtype=AbinsParameters.float_type)
        # noinspection PyTypeChecker
        start_broaden = start * AbinsParameters.pkt_per_peak

        for indx, freq in np.ndenumerate(frequencies[start:]):

            sigma = AbinsParameters.TOSCA_A * freq * freq + AbinsParameters.TOSCA_B * freq + AbinsParameters.TOSCA_C

            points_freq = np.array(np.linspace(freq - AbinsParameters.fwhm * sigma,
                                               freq + AbinsParameters.fwhm * sigma,
                                               num=AbinsParameters.pkt_per_peak))

            start = indx[0] * AbinsParameters.pkt_per_peak + start_broaden
            abscissa[start:start + AbinsParameters.pkt_per_peak] = points_freq

        return abscissa


    def _gaussian(self, sigma=None, points=None, center=None):

        """
        @param sigma: sigma defines width of Gaussian
        @param points: points for which Gaussian should be evaluated
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """
        sigma_factor = 2.0 * sigma * sigma

        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-np.power(points - center, 2) / sigma_factor)
