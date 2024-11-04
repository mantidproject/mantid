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


# For a Gaussian distribution the elastic intensity is proportional to exp(-(msd*Q^2)/6)
# where the mean square displacement msd = <r^2>.


class MsdGauss(IFunction1D):
    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Height", 1.0, "Height")
        self.declareParameter("Msd", 0.05, "Mean square displacement")

    def function1D(self, xvals):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")

        xvals = np.array(xvals)
        intensity = height * np.exp((-msd * xvals**2) / 6)

        return intensity

    def functionDeriv1D(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        msd = self.getParameterValue("Msd")

        for i, x in enumerate(xvals):
            e = math.exp((-msd * x**2) / 6)
            jacobian.set(i, 0, e)
            jacobian.set(i, 1, -((x**2) / 6) * e * height)
            i += 1


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(MsdGauss)
