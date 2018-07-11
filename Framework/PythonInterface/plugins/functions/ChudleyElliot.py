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
from __future__ import (absolute_import, division, print_function)
import math
import numpy as np
from mantid.api import IFunction1D, FunctionFactory
from scipy import constants


class ChudleyElliot(IFunction1D):

    planck_constant = constants.Planck / constants.e * 1E15  # meV*psec
    hbar = planck_constant / (2 * np.pi)  # meV * ps  = ueV * ns

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Tau", 1.0, 'Residence time')
        self.declareParameter("L", 1.5, 'Jump length')

    def function1D(self, xvals):
        tau = self.getParameterValue("Tau")
        length = self.getParameterValue("L")
        xvals = np.array(xvals)
        hwhm = self.hbar*(1.0 - np.sin(xvals * length) / (xvals * length))/tau

        return hwhm


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(ChudleyElliot)
