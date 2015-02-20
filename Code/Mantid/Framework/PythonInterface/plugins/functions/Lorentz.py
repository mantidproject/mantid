#pylint: disable=no-init,invalid-name
'''
@author Mathieu Doucet, ORNL
@date Oct 13, 2014

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
import math
import numpy as np

class Lorentz(IFunction1D):
    """
        Provide a Lorentz model for SANS

        I(q) = scale / ( 1 + q^2 L^2 ) + background
    """

    def category(self):
        return "SANS"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Scale", 1.0, 'Scale')
        self.declareParameter("Length", 50.0, 'Length')
        self.declareParameter("Background", 0.0, 'Background')

    def function1D(self, xvals):
        """
            Evaluate the model
            @param xvals: numpy array of q-values
        """
        return self.getParameterValue("Scale") / (1.0 + np.power(xvals*self.getParameterValue('Length'), 2)) + self.getParameterValue('Background')

    def functionDeriv1D(self, xvals, jacobian):
        """
            Evaluate the first derivatives
            @param xvals: numpy array of q-values
            @param jacobian: Jacobian object
        """
        i = 0
        for x in xvals:
            jacobian.set(i,0, 1.0 / (1.0 + np.power(x*self.getParameterValue('Length'), 2)))
            denom = math.pow(1.0 + math.pow(x*self.getParameterValue('Length'), 2), -2)
            jacobian.set(i,1, -2.0 * self.getParameterValue("Scale") * x * x * self.getParameterValue('Length') * denom)
            jacobian.set(i,2, 1.0)
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(Lorentz)

