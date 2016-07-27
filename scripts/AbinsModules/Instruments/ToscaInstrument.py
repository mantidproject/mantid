import numpy as np
import math

from Instrument import  Instrument
from AbinsModules import Constants

class ToscaInstrument(Instrument):
    """
    Class for TOSCA and TOSCA-like instruments.
    """
    def __init__(self, name):
        self._name = name

    def calculate_q(self, frequencies=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        return frequencies * frequencies * Constants.TOSCA_constant


    def convolve_with_resolution_function(self, frequencies=None, s_dft=None, points_per_peak=None):
        """
        Convolves discrete DFT spectrum with the  resolution function for the particular instrument.
        @param frequencies:   frequencies for which resolution function should be calculated
        @param s_dft:  discrete S calculated directly from DFT
        @param points_per_peak: number of points for each peak of broadened spectrum

        """

        all_points = points_per_peak * frequencies.shape[0]
        broadened_spectrum = np.zeros(all_points, dtype=Constants.float_type)

        for indx, freq in np.ndenumerate(frequencies):
            sigma = Constants.TOSCA_A * freq * freq + Constants.TOSCA_B * freq + Constants.TOSCA_C
            points_freq = np.array(np.linspace(freq - 3 * sigma, freq + 3 * sigma, num=points_per_peak))
            start = indx[0] * points_per_peak
            broadened_spectrum[start:start + points_per_peak]  = np.convolve(s_dft[indx[0]], self._gaussian(sigma=sigma, points=points_freq) )

        return broadened_spectrum


    def _gaussian(self, sigma=None, points=None):

        sigma_factor = 2 * sigma * sigma

        return 1.0 / math.sqrt(sigma_factor * np.pi) * np.exp(-np.power(points, 2) / sigma_factor )