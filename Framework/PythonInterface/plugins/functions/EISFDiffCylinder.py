#pylint: disable=no-init,invalid-name
"""
@author Jose Borreguero, ORNL
@date December 07, 2017

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
from mantid.api import IFunction1D, FunctionFactory


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
        return 'QuasiElastic'

    def init(self):
        # Active fitting parameters
        self.declareParameter('A', 1.0, 'Amplitude, or Scaling factor')
        self.declareParameter('R', 1.0, 'Cylinder radius, inverse units of Q.')
        self.declareParameter('L', 2.0, 'Cylinder length, inverse units of Q.')

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
        radius = self.getParameterValue('R')
        length = self.getParameterValue('L')
        x = np.asarray(xvals)  # Q values
        z = length * np.outer(x, self.cos_theta)
        # EISF along cylinder Z-axis
        a = np.square(np.where(z < 1e-9, 1 - z * z / 6, np.sin(z) / z))
        z = radius * np.outer(x, self.sin_theta)
        #  EISF on cylinder cross-section (diffusion on a disc)
        b = np.square(np.where(z < 1e-6, 1 - z * z / 10,
                               3 * (np.sin(z) - z * np.cos(z)) / (z * z * z)))
        # integrate in theta
        eisf = self.d_theta * np.sum(self.sin_theta * a * b, axis=1)
        return self.getParameterValue('A') * eisf

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(EISFDiffCylinder)
