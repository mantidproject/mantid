'''
@author Spencer Howells, ISIS
@date December 05, 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory,
NScD Oak Ridge National Laboratory & European Spallation Source

pylint: disable=no-init,invalid-name

This file is part of Mantid.
Mantid Repository : https://github.com/mantidproject/mantid
File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
'''
from __future__ import (absolute_import, division, print_function)
import math
import numpy as np
from mantid.api import IFunction1D, FunctionFactory

# The model of Yi et al(J Phys Chem B 1316 5029 2012) takes into account motional heterogeneity.
# The elastic intensity is proportional to exp(-(1/6)*Q^2*msd)*(1+Q^4*sigma/72)
# where the mean square displacement msd = <r^2> and sigma^2 is the variance of the msd.
# In the limit sigma = 0 the model becomes Gaussian.


class MsdYi(IFunction1D):

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Height", 1.0, 'Height')
        self.declareParameter("Msd", 0.05, 'Mean square displacement')
        self.declareParameter("Sigma", 1.0, 'Sigma')

    def function1D(self, xvals):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")
        sigma = self.getParameterValue("Sigma")

        xvals = np.array(xvals)
        i1 = np.exp((-1.0 / 6.0) * xvals * xvals * msd)
        i2 = 1.0 + (np.power(xvals, 4) * sigma / 72.0)
        intensity = height * i1 * i2

        return intensity

    def functionDeriv1D(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")
        sigma = self.getParameterValue("Sigma")

        for i, x in enumerate(xvals):
            f1 = math.exp((-1.0 / 6.0) * x * x * msd)
            df1 = -f1 * ((2.0 / 6.0) * x * msd)
            x4 = math.pow(x, 4)
            f2 = 1.0 + (x4 * sigma / 72.0)
            df2 = x4 / 72.0
            jacobian.set(i, 0, f1 * f2)
            jacobian.set(i, 1, height * df1 * f2)
            jacobian.set(i, 2, height * f1 * df2)


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(MsdYi)
