from __future__ import (absolute_import, division, print_function)

import numpy as np

import abins
from .directinstrument import DirectInstrument


class PantherInstrument(DirectInstrument):
    """Simulated direct-geometry INS with ILL-PANTHER

    Angle range is drawn from parameters file

    Resolution fitting is in progress
    """
    def __init__(self, setting=''):
        super().__init__(setting=setting, name='PANTHER')

    def get_angle_range(self):
        parameters = abins.parameters.instruments[self._name]
        return parameters['angle_range']

    def calculate_sigma(self, frequencies):
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
