# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Spencer Howells, ISIS
@date December 05, 2013
"""

import math
import numpy as np

from mantid.api import IFunction1D, FunctionFactory


# The model of Peters & Kneller (J Chem Phys 139 165102 2013) takes into account motional heterogeneity.
# The elastic intensity is propotional to 1/(1+msd*Q^2/6*beta)^beta
# where the mean square displacement msd = <r^2> and beta is the paramter of the Gamma function Gamma(beta)


class MsdPeters(IFunction1D):
    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Height", 1.0, "Height")
        self.declareParameter("Msd", 0.05, "Mean square displacement")
        self.declareParameter("Beta", 1.0, "beta")

    def function1D(self, xvals):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")
        beta = self.getParameterValue("Beta")

        xvals = np.array(xvals)
        i1 = 1.0 + (msd * xvals**2 / (6.0 * beta))
        i2 = np.power(i1, beta)
        intensity = height / i2
        return intensity

    def functionDeriv1D(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")
        beta = self.getParameterValue("Beta")

        for i, x in enumerate(xvals):
            x_sq = x**2
            q = msd * x_sq
            q6 = q / 6
            a1 = 1.0 + q6 / beta
            a = 1.0 / math.pow(a1, beta)
            b1 = q6 * x / beta + 1
            b = -(x_sq / 6) * math.pow(b1, (-beta - 1))
            cqx = q6 + beta
            c1 = math.pow((cqx / beta), -beta)
            c2 = q6 - cqx * math.log(cqx / beta)
            c = c1 * c2 / cqx
            jacobian.set(i, 0, a)
            jacobian.set(i, 1, b * height)
            jacobian.set(i, 2, c * height)


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(MsdPeters)
