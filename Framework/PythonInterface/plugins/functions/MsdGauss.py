# pylint: disable=no-init,invalid-name
'''
@author Spencer Howells, ISIS
@date December 05, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
'''
from __future__ import (absolute_import, division, print_function)

import math
import numpy as np

from mantid.api import IFunction1D, FunctionFactory


# For a Gaussian distribution the elastic intensity is propotional to exp(-msd*Q^2)
# where the mean square displacement msd = <r^2>.

class MsdGauss(IFunction1D):
    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Height", 1.0, 'Height')
        self.declareParameter("MSD", 0.05, 'Mean square displacement')

    def function1D(self, xvals):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("MSD")

        xvals = np.array(xvals)
        intensity = height * np.exp(-msd * xvals**2)

        return intensity

    def functionDeriv1D(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("MSD")

        for i, x in enumerate(xvals):
            e = math.exp(-msd * x**2)
            jacobian.set(i, 0, e)
            jacobian.set(i, 1, -(x**2) * e * height)
            i += 1


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(MsdGauss)
