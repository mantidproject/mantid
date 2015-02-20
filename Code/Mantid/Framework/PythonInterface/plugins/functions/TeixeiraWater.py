#pylint: disable=no-init,invalid-name
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
import numpy as np

class TeixeiraWater(IFunction1D):

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Tau", 1.0, 'Residence time')
        self.declareParameter("L", 1.5, 'Jump length')

    def function1D(self, xvals):
        tau = self.getParameterValue("Tau")
        length = self.getParameterValue("L")
        length = length**2

        xvals = np.array(xvals)
        hwhm = xvals * xvals * length / (tau * (1 + xvals * xvals * length))

        return hwhm

    def functionDeriv1D(self, xvals, jacobian):
        tau = self.getParameterValue("Tau")
        length = self.getParameterValue("L")
        length = length**2

        i = 0
        for x in xvals:
            h = x*x*length/(tau*(1+x*x*length))
            jacobian.set(i,0,-h/tau)
            jacobian.set(i,1,h*(1.0-h*tau)/length)
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(TeixeiraWater)
