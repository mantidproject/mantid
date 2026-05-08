# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Jose Borreguero, ORNL
@date December 05, 2017
"""

import numpy as np
from scipy.special import spherical_jn

from mantid.api import IFunction1D, FunctionFactory


def j1(z):
    return spherical_jn(1, z)


def j1d(z):
    return spherical_jn(1, z, derivative=True)


class EISFDiffSphere(IFunction1D):
    r"""Models the elastic incoherent scattering intensity of a particle
    undergoing diffusion but confined to a spherical volume
    """

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("A", 1.0, "Amplitude")
        self.declareParameter("R", 1.0, "Sphere radius, inverse units of Q.")

    def function1D(self, xvals):
        r"""Calculate the intensities

        Parameters
        ----------
        xvals : sequence of floats
          The domain where to evaluate the function

        Returns
        -------
        numpy.ndarray
            Function values
        """
        zs = self.getParameterValue("R") * np.asarray(xvals)
        return self.getParameterValue("A") * np.square(3 * j1(zs) / zs)

    def functionDeriv1D(self, xvals, jacobian):
        r"""Calculate the partial derivatives

        Parameters
        ----------
        xvals : sequence of floats
          The domain where to evaluate the function
        jacobian: 2D array
          partial derivatives of the function with respect to the fitting
          parameters, evaluated at the domain.
        """
        amplitude = self.getParameterValue("A")
        radius = self.getParameterValue("R")

        for i, x in enumerate(xvals):
            z = radius * x
            j = j1(z) / z
            jacobian.set(i, 0, np.square(3 * j))
            jacobian.set(i, 1, amplitude * 2 * 9 * j * (j1d(z) - j) / radius)


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(EISFDiffSphere)
