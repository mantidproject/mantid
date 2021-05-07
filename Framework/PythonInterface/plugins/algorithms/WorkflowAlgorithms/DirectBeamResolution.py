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

    def __init__(self, wavelength, delta_wavelength, beam_width, l2):
        """
        Sets up the parametrized formula
        Args:
            wavelength: Constant wavelength [A]
            delta_wavelength: Relative wavelength resolution []
            beam_width: Fitted horizontal beam width resolution [rad]
            l2: Sample to detector distance [m]
        """
        k = 2 * math.pi / wavelength
        self._coeff = k**2
        self._coeff_r2 = delta_wavelength**2 / l2**2
        self._beam_width = beam_width**2
        self._l2 = l2
        self._wavelength = wavelength

    def _delta_q(self, q):
        """
        Returns the sigma_Q at given q
        Args:
            q: Momentum transfer [inverse Angstrom]
        Returns: Absolute Q resolution [inverse Angstrom]
        """
        sin = q * self._wavelength / (4 * math.pi)
        sin_2theta = 2 * sin * math.sqrt(1 - sin ** 2)
        r = self._l2 * sin_2theta / math.sqrt(1 - sin_2theta ** 2)
        return math.sqrt(self._coeff * (self._beam_width + self._coeff_r2 * r ** 2))
