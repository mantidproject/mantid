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

from mantid.api import IFunction1D, FunctionFactory
from mantid import logger
import math, numpy as np

class HallRoss(IFunction1D):

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Tau", 1.0, 'Residence time')
        self.declareParameter("L", 0.2, 'Jump length')

    def function1D(self, xvals):
        tau = self.getParameterValue("Tau")
        l = self.getParameterValue("L")
        l = l**2 / 2

        xvals = np.array(xvals)
        hwhm = (1.0 - np.exp( -l * xvals * xvals )) / tau

        return hwhm

    def functionDeriv1D(self, xvals, jacobian):
        tau = self.getParameterValue("Tau")
        l = self.getParameterValue("L")
        l = l**2 / 2

        i = 0
        for x in xvals:
            ex = math.exp(-l*x*x)
            h = (1.0-ex)/tau
            jacobian.set(i,0,-h/tau)
            jacobian.set(i,1,x*x*ex/tau)
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(HallRoss)
