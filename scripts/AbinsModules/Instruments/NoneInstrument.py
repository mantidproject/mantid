import numpy as np
import math

from Instrument import Instrument
from AbinsModules import AbinsParameters
from AbinsModules.KpointsData import KpointsData

class NoneInstrument(Instrument):
    def __init__(self, name):
        self._name = name
        self._k_points_data = None
        self._sigma = AbinsParameters.delta_width


    def collect_K_data(self, k_points_data=None):
        """
        Collect k-points data from DFT calculations.
        @param k_points_data: object of type KpointsData with data from DFT calculations
        """

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")

        self._k_points_data = k_points_data


    def calculate_q_powder(self):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """



    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None, start=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the TOSCA instrument (and TOSCA-like).
        @param frequencies:   DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak of broadened spectrum
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """

        # noinspection PyTypeChecker
        all_points = points_per_peak * frequencies.shape[0]

        broadened_spectrum = np.zeros(all_points, dtype=AbinsParameters.float_type)
        start_broaden = start * points_per_peak
        for indx, freq in np.ndenumerate(frequencies[start:]):

            start = indx[0] * points_per_peak + start_broaden
            broadened_spectrum[start:start + points_per_peak] = np.convolve(s_dft[indx[0]], self._dirac_delta(center=freq))

        return broadened_spectrum


    def produce_abscissa(self, frequencies=None, points_per_peak=None, start=None):
        """
        Creates abscissa for convoluted spectrum in case of TOSCA.
        @param frequencies:  DFT frequencies for which frequencies which correspond to broadened spectrum should be regenerated (frequencies in cm-1)
        @param points_per_peak:  number of points for each peak of broadened spectrum
        @return: abscissa for convoluted S
        @param start: 3 if acoustic modes at Gamma point, otherwise this should be set to zero
        """
        all_points = points_per_peak * frequencies.shape[0]
        abscissa = np.zeros(all_points, dtype=AbinsParameters.float_type)
        # noinspection PyTypeChecker
        start_broaden = start * points_per_peak

        for indx, freq in np.ndenumerate(frequencies[start:]):

            points_freq = np.array(np.linspace(freq - AbinsParameters.fwhm * self._sigma,
                                               freq + AbinsParameters.fwhm * self._sigma,
                                               num=points_per_peak))

            start = indx[0] * points_per_peak + start_broaden
            abscissa[start:start + points_per_peak] = points_freq

        return abscissa


    def _dirac_delta(self, center=None):

        """
        Dirac delta is implemented as a very narrow Gaussian function.
        @param center: center of Gaussian
        @return: numpy array with calculated Gaussian values
        """

        points = np.array(np.linspace(center - AbinsParameters.fwhm * self._sigma,
                                      center + AbinsParameters.fwhm * self._sigma,
                                      num=AbinsParameters.points_per_peak))

        sigma_factor = 2.0 * self._sigma * self._sigma

        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-np.power(points - center, 2) / sigma_factor)
