# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Jose Borreguero, ORNL
@date December 06, 2017
"""

import numpy as np
from scipy.special import spherical_jn
from mantid.api import IFunction1D, FunctionFactory


class EISFDiffSphereAlkyl(IFunction1D):
    r"""Models the internal elastic incoherent scattering intensity of an
    alkyl molecule.
    """

    vecbessel = np.vectorize(lambda z: spherical_jn(1, z) / z)

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("A", 1.0, "Amplitude")
        self.declareParameter("Rmin", 1.0, "Minimum radius, inverse units of Q.")
        self.declareParameter("Rmax", 2.0, "Maximum radius, inverse units of Q.")
        self.declareAttribute("M", 2)
        # Vectorize the calculation of the bessel functions

    def setAttributeValue(self, name, value):
        r"""Invoked the framework when an attribute is passed to Fit and
        its value set.

        It's main use is to store the attribute value on the object once
        to avoid repeated calls during the fitting process.

        Parameters
        ----------
        name : str
            Name of the attribute
        value : python object
            Value associated to the attribute (int, float, str,...)
        """
        if name == "M":
            self._M = value

    def function1D(self, xvals):
        r"""Calculate the intensities

        Parameters
        ----------
        xvals : sequence of floats
          The domain where to evaluate the function
        jacobian: 2D array
          partial derivative of the function with respect to the fitting
          parameters evaluated at the domain.

        Returns
        -------
        numpy.ndarray
            Function values
        """
        r = self.getParameterValue("Rmin")
        R = self.getParameterValue("Rmax")
        zs = np.outer(np.linspace(r, R, self._M), np.asarray(xvals))
        eisf = np.mean(np.square(3 * self.vecbessel(zs)), axis=0)
        return self.getParameterValue("A") * eisf


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(EISFDiffSphereAlkyl)
