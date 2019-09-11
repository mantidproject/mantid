from __future__ import (absolute_import, division, print_function)
import numpy as np
import math
import AbinsModules
from .Instrument import Instrument


class TwoDMap(Instrument):
    """An instrument for q-resolved 2D maps"""
    def __init__(self, name):

        super(TwoDMap, self).__init__()
        self._name = name
        self._e_init = None
        self._angle = None

    def calculate_q_powder(self, input_data=None):
        """
        Returns momentum transfer Q^2 corresponding to frequency series

        using cosine rule Q^2 = a^2 + b^2 - 2 ab cos(theta)
        where a, b are k_i and k_f

        Calculation is restricted to the region E < E_i

        :param input_data: list with frequencies for the given k-point.
        :type array-like:
        """

        const = AbinsModules.AbinsConstants.WAVENUMBER_TO_INVERSE_A
        # Set momentum transfer to zero for energy transitions larger than the
        # incident energy of neutrons and calculate momentum transfer according
        # to momentum conservation principle for energy transitions smaller than
        # the incident energy of neutrons.

        k2_f = np.zeros_like(input_data)
        k2_i = np.zeros_like(input_data)

        conservation_indx = self._e_init > input_data

        k2_f[conservation_indx] = (self._e_init - input_data[conservation_indx]) * const
        k2_i[conservation_indx] = self._e_init * const

        cos_angle = math.cos(math.radians(self._angle))
        result = k2_i + k2_f - 2 * cos_angle * (k2_i * k2_f) ** 0.5

        return result

    def convolve_with_resolution_function(self, frequencies=None, s_dft=None):
        """
        Convolve discrete frequency spectrum with the resolution function for the TwoDMap instrument.

        - The number of points using in the convolution kernel is set as AbinsParameters.pkt_per_peak; setting this value to 1 prevents any broadening.
        - The resolution width is set as AbinsParameters.direct_instrument_resolution

        :param frequencies: DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT
        """

        if AbinsModules.AbinsParameters.pkt_per_peak == 1:
            points_freq = frequencies
            broadened_spectrum = s_dft

        else:
            resolution = AbinsModules.AbinsParameters.direct_instrument_resolution
            pkt_per_peak = int(round(AbinsModules.AbinsParameters.pkt_per_peak * self._e_init * resolution, 0))

            # sigma[freq_num]
            sigma = np.zeros(shape=frequencies.size, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
            sigma.fill(self._calculate_sigma())

            points_freq, broadened_spectrum = self._convolve_with_resolution_function_helper(frequencies=frequencies,
                                                                                             s_dft=s_dft,
                                                                                             sigma=sigma,
                                                                                             pkt_per_peak=pkt_per_peak,
                                                                                             gaussian=self._gaussian)
        return points_freq, broadened_spectrum

    def set_incident_energy(self, e_init=None):
        """
        Setter for incident energy.
        :param e_init: new incident energy
        """
        if isinstance(e_init, float):
            self._e_init = e_init
        else:
            raise ValueError("Invalid value of incident energy (%s, type(%s) = %s; should be float)."
                             % (e_init, e_init, type(e_init)))

    def set_detector_angle(self, angle=None):
        """
        Setter for detector angle.
        :param angle: new angle of detector
        """
        if isinstance(angle, float):
            self._angle = angle
        else:
            raise ValueError("Invalid value of a detector angle (%s, type(%s) = %s; should be float)."
                             % (angle, angle, type(angle)))

    def _calculate_sigma(self):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        return self._e_init * AbinsModules.AbinsParameters.direct_instrument_resolution
