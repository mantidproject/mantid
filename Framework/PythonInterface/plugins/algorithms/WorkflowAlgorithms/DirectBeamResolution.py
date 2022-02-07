# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import math
import numpy


class DirectBeamResolution:

    def __call__(self, q):
        if isinstance(q, numpy.ndarray):
            if q.ndim > 1:
                raise NotImplementedError('Resolution calculation not supported for arrays with more than one dimension.')
            else:
                return numpy.array([self._delta_q(qi) for qi in q])
        elif isinstance(q, list):
            return [self._delta_q(qi) for qi in q]
        else:
            return self._delta_q(q)

    def __init__(self, wavelength, delta_wavelength, beam_width):
        """
        Sets up the parametrized formula
        Args:
            wavelength: Constant wavelength [A]
            delta_wavelength: Relative wavelength resolution []
            beam_width: Fitted horizontal beam width resolution [rad]
            l2: Sample to detector distance [m]
        """
        self._wavelength = wavelength
        self._delta_wavelength = delta_wavelength**2
        self._delta_theta = (beam_width*0.5)**2 # converts 2theta to theta
        self._wavelength_coeff = (1/(2.0 * math.sqrt(2.0*math.log(2.0))))**2
        self._k2 = (4.0 * math.pi / self._wavelength)**2

    def _delta_q(self, q):
        """
        Returns the sigma_Q at given q. Based on formula 59 in Small-Angle Neutron Scattering
        and Applications in Soft Condensed Matter (2001), p. 723-782, doi:10.1007/978-1-4020-4465-6_13.
        Please note that delta_theta refers to theta, which is half of the scattering angle.
        Args:
            q: Momentum transfer [inverse Angstrom]
        Returns: Absolute Q resolution [inverse Angstrom]
        """
        return math.sqrt(q**2 * self._wavelength_coeff * self._delta_wavelength + self._delta_theta * (self._k2 - q**2))
