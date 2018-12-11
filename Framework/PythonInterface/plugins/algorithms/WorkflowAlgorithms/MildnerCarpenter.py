# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# Contains parametrized formulae for SANS Q resolution based on papers by Mildner, Carpenter.
import math
import numpy


class _MildnerCarpenterBase():

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

    def __init__(self):
        raise NotImplementedError('MildnerCarpenterBase cannot be initialized directly.')


class MonochromaticScalarQCartesian(_MildnerCarpenterBase):
    # Formula C2 in J. Appl. Cryst. (1984). 17, 249-256

    def __init__(self, Lambda, delta_lambda, x1, y1, x2, y2, x3, y3, l1, l2):
        """
        Sets up the parametrized formula
        Args:
            Lambda: Constant wavelength [A]
            delta_lambda: Relative wavelength resolution
            x1: Source aperture width [m]
            y1: Source aperture height [m]
            x2: Sample aperture width [m]
            y2: Sample aperture height [m]
            x3: Detector pixel width [m]
            y3: Detector pixel height [m]
            l1: Source to sample distance [m]
            l2: Sample to detector distance [m]
        """
        lprime = l1 * l2 / (l1 + l2)
        k = 2 * math.pi / Lambda
        self._coeff = k**2 / 12
        self._delta_theta = (x1**2 + y1**2) / (2 * l1**2) + (x2**2 + y2**2) / (2 * lprime**2)
        self._delta_pixel = (x3**2 + y3**2) / l2**2
        self._coeff_r2 = delta_lambda**2 / l2**2
        self._l2 = l2
        self._Lambda = Lambda

    def _delta_q(self, q):
        """
        Returns the sigma_Q at given q
        Args:
            q: Momentum transfer [inverse Angstrom]
        Returns: Absolute Q resolution [inverse Angstrom]
        """
        sin = q * self._Lambda / (4 * math.pi)
        sin_2theta = 2 * sin * math.sqrt(1 - sin ** 2)
        r = self._l2 * sin_2theta / math.sqrt(1 - sin_2theta ** 2)
        return math.sqrt(self._coeff * (self._delta_theta + self._delta_pixel + self._coeff_r2 * r ** 2))


class MonochromaticScalarQCylindric(_MildnerCarpenterBase):
    # Formula C3 in J. Appl. Cryst. (1984). 17, 249-256

    def __init__(self, Lambda, delta_lambda, r1, r2, x3, y3, l1, l2):
        """
        Sets up the parametrized formula
        Args:
            Lambda: Constant wavelength [A]
            delta_lambda: Relative wavelength resolution
            r1: Source aperture radius [m]
            r2: Sample aperture radius [m]
            x3: Detector pixel width [m]
            y3: Detector pixel height [m]
            l1: Source to sample distance [m]
            l2: Sample to detector distance [m]
        """
        lprime = l1 * l2 / (l1 + l2)
        k = 2 * math.pi / Lambda
        self._coeff = k ** 2 / 12
        self._delta_theta = 3 * r1**2 / l1**2 + 3 * r2**2 / lprime**2
        self._delta_pixel = (x3**2 + y3**2) / l2**2
        self._coeff_r2 = delta_lambda ** 2 / l2 ** 2
        self._l2 = l2
        self._Lambda = Lambda

    def _delta_q(self, q):
        """
        Returns the sigma_Q at given q
        Args:
           q: Momentum transfer [inverse Angstrom]
        Returns: Absolute Q resolution [inverse Angstrom]
        """
        sin = q * self._Lambda / (4 * math.pi)
        sin_2theta = 2 * sin * math.sqrt(1 - sin ** 2)
        r = self._l2 * sin_2theta / math.sqrt(1 - sin_2theta ** 2)
        return math.sqrt(self._coeff * (self._delta_theta + self._delta_pixel + self._coeff_r2 * r ** 2))
