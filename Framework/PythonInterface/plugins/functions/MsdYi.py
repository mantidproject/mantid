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


# The model of Yi et al(J Phys Chem B 1316 5029 2012) takes into account motional heterogeneity.
# The elastic intensity is propotional to exp(-1/6*Q^2*msd)*(1+Q^4*sigma/72)
# where the mean square displacement msd = <r^2> and sigma^2 is the variance of the msd.

class MsdYi(IFunction1D):
    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Height", 1.0, 'Height')
        self.declareParameter("MSD", 0.05, 'Mean square displacement')
        self.declareParameter("Sigma", 1.0, 'Sigma')

    def function1D(self, xvals):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("MSD")
        sigma = self.getParameterValue("Sigma")

        xvals = np.array(xvals)
        i1 = np.exp(-1.0 / (6.0 * xvals**2 * msd))
        i2 = 1.0 + (np.power(xvals, 4) * sigma / 72.0)
        intensity = height * i1 * i2

        return intensity

    def functionDeriv1D(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("MSD")
        sigma = self.getParameterValue("Sigma")

        for i, x in enumerate(xvals):
            q = msd * x**2
            f1 = math.exp(-1.0 / (6.0 * q))
            df1 = f1 / (6.0 * x * q)
            x4 = math.pow(x, 4)
            f2 = 1.0 + (x4 * sigma / 72.0)
            df2 = x4 / 72.0
            jacobian.set(i, 0, f1 * f2)
            jacobian.set(i, 1, height * df1 * f2)
            jacobian.set(i, 2, height * f1 * df2)


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(MsdYi)
