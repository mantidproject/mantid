# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
@author Mathieu Doucet, ORNL
@date Oct 10, 2014
"""

import math
import numpy as np
from mantid.api import IFunction1D, FunctionFactory


class Guinier(IFunction1D):
    """
    Provide a Guinier fit function for SANS

    I(q) = I(0) exp(-R^2 q^2 / 3)
    """

    def category(self):
        return "SANS"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Scale", 1.0, "Scale")
        self.declareParameter("Rg", 60.0, "Radius of gyration")

    def function1D(self, xvals):
        """
        Evaluate the model
        @param xvals: numpy array of q-values
        """
        return self.getParameterValue("Scale") * np.exp(-((self.getParameterValue("Rg") * xvals) ** 2) / 3.0)

    def functionDeriv1D(self, xvals, jacobian):
        """
        Evaluate the first derivatives
        @param xvals: numpy array of q-values
        @param jacobian: Jacobian object
        """
        i = 0
        rg = self.getParameterValue("Rg")
        for x in xvals:
            jacobian.set(i, 0, math.exp(-((rg * x) ** 2) / 3.0))
            jacobian.set(i, 1, -self.getParameterValue("Scale") * math.exp(-((rg * x) ** 2) / 3.0) * 2.0 / 3.0 * rg * x * x)
            i += 1


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(Guinier)
