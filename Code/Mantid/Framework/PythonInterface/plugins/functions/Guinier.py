'''
@author Mathieu Doucet, ORNL
@date Oct 10, 2014

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

class Guinier(IFunction1D):
    """
        Provide a Guinier fit function for SANS

        I(q) = I(0) exp(-R^2 q^2 / 3)
    """
    def category(self):
        return "SANS"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Scale", 1.0, 'Scale')
        self.declareParameter("Rg", 60.0, 'Radius of gyration')

    def function1D(self, xvals):
        """
            Evaluate the model
            @param xvals: numpy array of q-values
        """
        return self.getParameterValue("Scale") * np.exp(-(self.getParameterValue('Rg')*xvals)**2/3.0 )

    def functionDeriv1D(self, xvals, jacobian):
        """
            Evaluate the first derivatives
            @param xvals: numpy array of q-values
            @param jacobian: Jacobian object
        """
        i = 0
        rg = self.getParameterValue('Rg')
        for x in xvals:
            jacobian.set(i,0, math.exp(-(rg*x)**2/3.0 ) )
            jacobian.set(i,1, -self.getParameterValue("Scale") * math.exp(-(rg*x)**2/3.0 )*2.0/3.0*rg*x*x )
            i += 1

# Required to have Mantid recognise the new function
FunctionFactory.subscribe(Guinier)
