# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

import abins
from abins.constants import MILLI_EV_TO_WAVENUMBER
from .indirectinstrument import IndirectInstrument


class LagrangeInstrument(IndirectInstrument):
    """Instrument class for IN1-LAGRANGE instrument at ILL

    """
    def __init__(self, setting='Cu(220)'):
        super().__init__(name='Lagrange', setting=setting)
        self.parameters = abins.parameters.instruments[self._name]

    def get_sigma(self, frequencies):
        setting = self.get_setting()
        ei_resolution = self.parameters['settings'][setting].get('ei_resolution', 0)
        abs_resolution_meV = self.parameters['settings'][setting].get('abs_resolution_meV', 0)

        if isinstance(abs_resolution_meV, list):  # interpret as polynomial in energy units
            abs_resolution_meV = np.abs(np.polyval(abs_resolution_meV, frequencies / MILLI_EV_TO_WAVENUMBER))

        abs_resolution_cm = abs_resolution_meV * MILLI_EV_TO_WAVENUMBER

        width = frequencies * ei_resolution + abs_resolution_cm

        low_energy_indices = frequencies < (self.parameters['settings'][setting].get(
            'low_energy_cutoff_meV', float('-Inf')))
        width[low_energy_indices] = (self.parameters['settings'][setting].get('low_energy_resolution_meV', 0) *
                                     MILLI_EV_TO_WAVENUMBER)
        return width / 2  # Lagrange reported resolution seems to equal 2*sigma

    def get_angles(self):
        start, end = self.parameters['scattering_angle_range']
        n_samples = self.parameters['angles_per_detector']
        angles, step = np.linspace(start, end, n_samples, endpoint=False, retstep=True)

        if n_samples == 1:
            step = end - start

        return (angles + step / 2).tolist()
