# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Jose Borreguero, ORNL
@date December 07, 2017
"""
import numpy as np
from mantid.api import IFunction1D, FunctionFactory
from scipy.special import jv


class EISFDiffCylinder(IFunction1D):
    r"""Models the elastic incoherent scattering intensity of a particle
    diffusing within a cylinder.
    """

    # For integration over all directions of vector Q
    n_theta = 128
    d_theta = (np.pi / 2.0) / n_theta
    theta = d_theta * np.arange(1, n_theta)
    sin_theta = np.sin(theta)
    cos_theta = np.cos(theta)

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("A", 1.0, "Amplitude, or Scaling factor")
        self.declareParameter("R", 1.0, "Cylinder radius, inverse units of Q.")
        self.declareParameter("L", 2.0, "Cylinder length, inverse units of Q.")

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
        radius = self.getParameterValue("R")
        length = self.getParameterValue("L")
        x = np.asarray(xvals)  # Q values
        z = length * np.outer(x, self.cos_theta)
        # EISF along cylinder Z-axis
        # np.sin(z) / z is equal to scipy.special.spherical_jn(0, z) and is faster
        a = np.square(np.where(z < 1e-9, 1 - z * z / 6, np.sin(z) / z))
        z = radius * np.outer(x, self.sin_theta)
        #  EISF on cylinder cross-section (diffusion on a disc)
        b = np.square(np.where(z < 1e-6, 1 - z * z / 10, 2 * jv(1, z) / z))
        # integrate in theta
        eisf = self.d_theta * np.sum(self.sin_theta * a * b, axis=1)
        return self.getParameterValue("A") * eisf


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(EISFDiffCylinder)
