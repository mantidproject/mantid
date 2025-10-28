# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from abins.constants import MILLI_EV_TO_WAVENUMBER
from .indirectinstrument import IndirectInstrument


class LagrangeInstrument(IndirectInstrument):
    """Instrument class for IN1-LAGRANGE instrument at ILL"""

    def __init__(self, setting="Cu(220)"):
        super().__init__(name="Lagrange", setting=setting)

    def get_min_wavenumber(self):
        return 0.0  # Energy differences are measured down to zero

    def get_max_wavenumber(self):
        # Maximum observable energy transfer = E_init - E_final
        return self.get_parameter("Ei_range_meV")[1] * MILLI_EV_TO_WAVENUMBER - self.get_parameter("final_neutron_energy")

    def get_sigma(self, frequencies):
        ei_resolution = self.get_parameter("ei_resolution", default=0.0)
        abs_resolution_meV = self.get_parameter("abs_resolution_meV", default=0.0)

        if isinstance(abs_resolution_meV, list):  # interpret as polynomial in energy units
            abs_resolution_meV = np.abs(np.polyval(abs_resolution_meV, frequencies / MILLI_EV_TO_WAVENUMBER))

        abs_resolution_cm = abs_resolution_meV * MILLI_EV_TO_WAVENUMBER
        width = frequencies * ei_resolution + abs_resolution_cm
        width = np.maximum(width, self.get_parameter("minimum_resolution_meV", default=0.0) * MILLI_EV_TO_WAVENUMBER)
        return width / 2  # Lagrange reported resolution seems to equal 2*sigma

    def get_angles(self):
        parameters = self.get_parameters()

        start, end = parameters["scattering_angle_range"]
        n_samples = parameters["angles_per_detector"]
        angles, step = np.linspace(start, end, n_samples, endpoint=False, retstep=True)

        if n_samples == 1:
            step = end - start

        return (angles + step / 2).tolist()
