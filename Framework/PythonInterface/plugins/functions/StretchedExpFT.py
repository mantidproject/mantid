# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

"""
@author Jose Borreguero, NScD
@date October 06, 2013

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
from StretchedExpFTHelper import surrogate, function1Dcommon


class StretchedExpFT(IFunction1D):

    # pylint: disable=super-on-old-class
    def __init__(self):
        super(self.__class__, self).__init__()
        self._parmList = list()

    def category(self):
        return 'QuasiElastic'

    @surrogate
    def init(self):
        """Declare parameters that participate in the fitting"""
        pass

    @surrogate
    def validateParams(self):
        """Check parameters are positive"""
        pass

    def function1D(self, xvals, **optparms):
        """ Fourier transform of the Symmetrized Stretched Exponential

        The Symmetrized Stretched Exponential:
                height * exp( - |t/tau|**beta )

        The Fourier Transform:
            F(E) \int_{-infty}^{infty} (dt/h) e^{-i2\pi Et/h} f(t)
            F(E) is normalized:
                \int_{-infty}^{infty} dE F(E) = 1
        """
        parms, de, energies, fourier = function1Dcommon(self, xvals, **optparms)
        if parms is None:
            return fourier  #return zeros if parameters not valid
        transform = parms['Height'] * np.interp(xvals-parms['Centre'], energies, fourier)
        return transform

    @surrogate
    def fillJacobian(self, xvals, jacobian, partials):
        """Fill the jacobian object with the dictionary of partial derivatives
        :param xvals: domain where the derivatives are to be calculated
        :param jacobian: jacobian object
        :param partials: dictionary with partial derivates with respect to the
        fitting parameters
        """
        pass

    @surrogate
    def functionDeriv1D(self, xvals, jacobian):
        """Numerical derivative except for Height parameter"""
        # partial derivatives with respect to the fitting parameters
        pass


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(StretchedExpFT)
