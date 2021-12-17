from __future__ import (absolute_import, division, print_function)
import logging
import math
from typing import Optional, Tuple, Union

import numpy as np
import mantid.kernel
from mantidqtinterfaces.PyChop import PyChop2
from mantidqtinterfaces.PyChop import Instruments as pychop_instruments

import abins
from abins.constants import FLOAT_TYPE, MILLI_EV_TO_WAVENUMBER, WAVENUMBER_TO_INVERSE_A
from .instrument import Instrument
from .broadening import broaden_spectrum, prebin_required_schemes


class PyChopInstrument(Instrument):
    """Simulated direct-geometry INS with PyChop2

    PyChop is used to compute energy resolution as a function of energy for
    given instrument settings.

    The "tthlims" data from PyChop is used to determine sampling angles.
    """
    def __init__(self,
                 name: str = 'MAPS',
                 setting: str = '',
                 chopper_frequency: Optional[int] = None) -> None:
        self._name = name
        self._e_init = None
        self._chopper = setting
        self._chopper_frequency = self._check_chopper_frequency(chopper_frequency)

        self._polyfits = {}
        self._tthlims = pychop_instruments.Instrument(self._name).detector.tthlims

        super().__init__(setting=setting)

    def _check_chopper_frequency(
            self,
            chopper_frequency: Union[int, None],
            logger: Union[logging.Logger, mantid.kernel.Logger, None] = None
            ) -> int:

        if chopper_frequency is None:
            parameters = abins.parameters.instruments[self._name]
            chopper_frequency = parameters['chopper_frequency_default']

            if logger is None:
                mantid_logger: mantid.kernel.Logger = mantid.kernel.logger
                logger = mantid_logger

            logger.notice(f'Using default chopper frequency for instrument {self._name}: '
                          f'{chopper_frequency} Hz')

        return chopper_frequency

    def get_abs_q_limits(self, energy: np.ndarray) -> np.ndarray:
        """Get absolute q range accessible for given energy transfer at set incident energy

        Args:
            energy: 1-D array of energy transfer values


        Returns:
            limits: (2, N) array of q-limits corresponding to energy series
        """
        assert len(energy.shape) == 1

        min_angle, max_angle = np.min(self._tthlims), np.max(self._tthlims)

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

    def get_angles(self):
        parameters = abins.parameters.instruments[self._name]

        angle_ranges = [(start, end) for (start, end) in zip(self._tthlims[0::2],
                                                             self._tthlims[1::2])]
        angles = np.concatenate([np.linspace(start, end, parameters['angles_per_detector'])
                                 for (start, end) in angle_ranges])
        return angles

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

        if scheme in prebin_required_schemes:
            s_dft, _ = np.histogram(frequencies, bins=bins, weights=s_dft, density=False)
            frequencies = (bins[1:] + bins[:-1]) / 2

        sigma = np.full(frequencies.size, self._calculate_sigma(frequencies),
                        dtype=FLOAT_TYPE)

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
        if self._e_init not in self._polyfits:
            self._polyfit_resolution()

        return np.polyval(self._polyfits[self._e_init], frequencies)

    def _polyfit_resolution(self, n_values=40, order=4):
        frequencies_invcm = np.linspace(0, self._e_init, n_values, endpoint=False)
        frequencies_mev = frequencies_invcm / MILLI_EV_TO_WAVENUMBER
        ei_mev = self._e_init / MILLI_EV_TO_WAVENUMBER

        setting_params = abins.parameters.instruments[self._name]['settings'][self._setting]

        resolution, _ = PyChop2.calculate(inst=self._name,
                                          package=setting_params['chopper'],
                                          freq=self._chopper_frequency,
                                          ei=ei_mev,
                                          etrans=frequencies_mev.tolist())
        fit = np.polyfit(frequencies_invcm, resolution / MILLI_EV_TO_WAVENUMBER, order)

        self._polyfits[self._e_init] = fit
