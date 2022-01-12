from __future__ import (absolute_import, division, print_function)
import math
from typing import Tuple

import numpy as np

import abins
from abins.constants import WAVENUMBER_TO_INVERSE_A
from .instrument import Instrument
from .broadening import broaden_spectrum, prebin_required_schemes


class PantherInstrument(Instrument):
    """Simulated direct-geometry INS with ILL-PANTHER

    Angle range is drawn from parameters file

    Resolution fitting is in progress
    """
    def __init__(self, setting=''):
        self._name = 'PANTHER'
        self._e_init = None

        super().__init__(setting=setting)

    def get_abs_q_limits(self, energy: np.ndarray) -> np.ndarray:
        """Get absolute q range accessible for given energy transfer at set incident energy

        Args:
            energy: 1-D array of energy transfer values


        Returns:
            limits: (2, N) array of q-limits corresponding to energy series
        """
        assert len(energy.shape) == 1

        min_angle, max_angle = self.get_angle_range()

        # Actual limits depend on |2θ|, so some shuffling is necessary to get
        # absolute values in the right order, spanning the whole range.
        if (min_angle < 0) and (max_angle <= 0):
            min_angle, max_angle = -max_angle, -min_angle
        elif (min_angle < 0):
            # e.g. if -10 < 2θ < 20, we don't want to set limits 10 < |2θ| < 20
            # as this would omit contributions from the range -10 < θ < 10
            min_angle = 0

        q_limits = np.empty((2, energy.size))
        q_limits[0, :] = np.sqrt(self.calculate_q_powder(input_data=energy, angle=min_angle))
        q_limits[1, :] = np.sqrt(self.calculate_q_powder(input_data=energy, angle=max_angle))
        return q_limits

    def get_q_bounds(self, pad: float = 0.05) -> Tuple[float, float]:
        """Calculate appropriate q range for instrument

        Coarsely sample the outer E-Q lines to determine plotting extent

        Args:
            pad: Additional fraction of maximum value to use as padding

        """
        energy_samples = np.linspace(0, self._e_init, 10)
        q_values = self.get_abs_q_limits(energy_samples).flatten()
        q_min, q_max = np.min(q_values), np.max(q_values)
        if q_min - (pad * q_max) < 0:
            q_min = 0
        return (q_min, q_max * (1 + pad))

    def get_angle_range(self):
        parameters = abins.parameters.instruments[self._name]
        return parameters['angle_range']

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
        Convolve discrete frequency spectrum with Panther resolution function

        :param frequencies: DFT frequencies for which resolution function should be calculated (frequencies in cm-1)
        :param s_dft:  discrete S calculated directly from DFT
        """

        if scheme == 'auto':
            scheme = 'interpolate'

        if scheme in prebin_required_schemes:
            s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
            frequencies = (bins[1:] + bins[:-1]) / 2

        sigma = self._calculate_sigma(frequencies)

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

    def _calculate_sigma(self, frequencies):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        from abins.constants import MILLI_EV_TO_WAVENUMBER
        parameters = abins.parameters.instruments[self._name]['resolution']

        ei_meV = self._e_init / MILLI_EV_TO_WAVENUMBER
        frequencies_meV = frequencies / MILLI_EV_TO_WAVENUMBER

        return (np.polyval(parameters['abs_meV'], frequencies_meV)
                + np.polyval(parameters['ei_dependence'], ei_meV)
                + np.polyval(parameters['ei_energy_product'], ei_meV * frequencies_meV)
                ) * MILLI_EV_TO_WAVENUMBER
