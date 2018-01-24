#pylint: disable=no-init,invalid-name
"""
@author Jose Borreguero, ORNL
@date December 05, 2017

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory,
NScD Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
"""
from __future__ import (absolute_import, division, print_function)

import numpy as np
try:
    from scipy.special import spherical_jn

    def j1(z): return spherical_jn(1, z)

    def j1d(z): return spherical_jn(1, z, derivative=True)
except ImportError:
    # spherical_jn removed from scipy >= 1.0.0
    from scipy.special import sph_jn

    def j1(z): return sph_jn(1, z)[0][1]

    def j1d(z): return sph_jn(1, z)[1][1]

from mantid.api import IFunction1D, FunctionFactory


class EISFDiffSphere(IFunction1D):
    r"""Models the elastic incoherent scattering intensity of a particle
    undergoing diffusion but confined to a spherical volume
    """

    vecbessel = np.vectorize(lambda z: j1(z) / z)

    def category(self):
        return 'QuasiElastic'

    def init(self):
        # Active fitting parameters
        self.declareParameter('A', 1.0, 'Amplitude')
        self.declareParameter('R', 1.0, 'Sphere radius, inverse units of Q.')

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
        zs = self.getParameterValue('R') * np.asarray(xvals)
        return self.getParameterValue('A') * np.square(3 * self.vecbessel(zs))

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
        amplitude = self.getParameterValue('A')
        radius = self.getParameterValue('R')
        i = 0
        for x in xvals:
            z = radius * x
            j = j1(z)/z
            jacobian.set(i, 0, np.square(3 * j))
            jacobian.set(i,1, amplitude * 2 * 9 * j * (j1d(z) - j) / radius)
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(EISFDiffSphere)
