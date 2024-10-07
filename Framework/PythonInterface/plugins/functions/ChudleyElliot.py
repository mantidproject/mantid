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
from scipy import constants


class ChudleyElliot(IFunction1D):
    planck_constant = constants.Planck / constants.e * 1e15  # meV*psec
    hbar = planck_constant / (2 * np.pi)  # meV * ps  = ueV * ns

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Tau", 1.0, "Residence time")
        self.declareParameter("L", 1.5, "Jump length")

    def function1D(self, xvals):
        tau = self.getParameterValue("Tau")
        length = self.getParameterValue("L")
        xvals = np.array(xvals)

        with np.errstate(divide="ignore"):
            hwhm = self.hbar * (1.0 - np.sin(xvals * length) / (xvals * length)) / tau
        return hwhm

    def functionDeriv1D(self, xvals, jacobian):
        tau = self.getParameterValue("Tau")
        length = self.getParameterValue("L")

        for i, x in enumerate(xvals, start=0):
            s = 1.0 - np.sinc(x * length / np.pi)
            hwhm = self.hbar * s / tau
            jacobian.set(i, 0, -hwhm / tau)
            jacobian.set(i, 1, (math.cos(x * length) - s) / (length * tau))


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(ChudleyElliot)
