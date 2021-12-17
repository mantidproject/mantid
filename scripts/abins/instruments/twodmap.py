from __future__ import (absolute_import, division, print_function)
import numpy as np
import math
from typing import Tuple

import abins
from abins.constants import FLOAT_TYPE, WAVENUMBER_TO_INVERSE_A
from .instrument import Instrument
from .broadening import broaden_spectrum  # , prebin_required_schemes


class TwoDMap(Instrument):
    """An instrument for q-resolved 2D maps"""
    def __init__(self, setting='', e_init=None):
        super().__init__(setting=setting)
        self._name = 'TwoDMap'

        if e_init is None:
            params = abins.parameters.instruments[self._name]
            self._e_init = params['e_init']
        else:
            self._e_init = e_init

    def get_angles(self):
        parameters = abins.parameters.instruments[self._name]
        return parameters['angles']

    def get_q_bounds(self, pad: float = 0.) -> Tuple[float, float]:
        params = abins.parameters.instruments[self._name]
        q_min, q_max = params.get('q_range')
        return (q_min, q_max * (1. + pad))

    def calculate_q_powder(self, *, input_data=None, angle=None):
        """
        Returns momentum transfer Q^2 corresponding to frequency series

        using cosine rule Q^2 = a^2 + b^2 - 2 ab cos(theta)
        where a, b are k_i and k_f

        Calculation is restricted to the region E < E_i

        :param input_data: list with frequencies for the given k-point.
        :param angle: scattering angle in degrees
        :type array-like:
        """

        # Set momentum transfer to zero for energy transitions larger than the
        # incident energy of neutrons and calculate momentum transfer according
        # to momentum conservation principle for energy transitions smaller than
        # the incident energy of neutrons.

        k2_f = np.zeros_like(input_data)
        k2_i = np.zeros_like(input_data)

        conservation_indx = self._e_init > input_data

        k2_f[conservation_indx] = (self._e_init - input_data[conservation_indx]) * WAVENUMBER_TO_INVERSE_A
        k2_i[conservation_indx] = self._e_init * WAVENUMBER_TO_INVERSE_A

        cos_angle = math.cos(math.radians(angle))
        result = k2_i + k2_f - 2 * cos_angle * (k2_i * k2_f) ** 0.5

        return result

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, scheme='auto'):
        """
        Convolve discrete frequency spectrum with the resolution function for the TwoDMap instrument.

        - The broadening parameters are set in abins.parameters.instruments['TwoDMap']

        :param frequencies: DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT
        """

        if scheme == 'auto':
            scheme = 'interpolate'

        # if scheme in prebin_required_schemes:
        #     s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
        #     frequencies = (bins[1:] + bins[:-1]) / 2

        sigma = np.full(frequencies.size, self._calculate_sigma(), dtype=FLOAT_TYPE)

        points_freq, broadened_spectrum = broaden_spectrum(frequencies, bins, s_dft, sigma,
                                                           scheme=scheme)
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

    def _calculate_sigma(self):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        return self._e_init * abins.parameters.instruments[self._name]['resolution']
