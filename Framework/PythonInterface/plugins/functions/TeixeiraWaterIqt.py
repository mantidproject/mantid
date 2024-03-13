# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

"""
@author Spencer Howells, ISIS
@date December 05, 2013
"""

import numpy as np

from mantid.api import IFunction1D, FunctionFactory

from scipy import constants
from scipy.special import spherical_jn  # bessel


class TeixeiraWaterIqt(IFunction1D):
    planck_constant = constants.Planck / constants.e * 1e15  # meV*psec
    hbar = planck_constant / (2 * np.pi)  # meV * ps  = ueV * ns

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Amp", 1.00, "Amplitude")
        self.declareParameter("Tau0", 1.25, "Residence time")
        self.declareParameter("Tau1", 0.05, "Relaxation time")
        self.declareParameter("a", 1.3, "Radius rotation")
        self.declareParameter("Diff", 2.3, "Diffusion coefficient")
        self.declareAttribute("Q", 0.4)

    def setAttributeValue(self, name, value):
        if name == "Q":
            self.Q = value

    def function1D(self, xvals):
        amp = self.getParameterValue("Amp")
        tau0 = self.getParameterValue("Tau0")
        tau1 = self.getParameterValue("Tau1")
        radius = self.getParameterValue("a")
        diff = self.getParameterValue("Diff")
        xvals = np.array(xvals)

        qr = np.array(self.Q * radius)
        j0 = spherical_jn(0, qr)
        j1 = spherical_jn(1, qr)
        j2 = spherical_jn(2, qr)
        with np.errstate(divide="ignore"):
            rotational = j0 * j0 + 3 * j1 * j1 * np.exp(-xvals / (3 * tau1)) + 5 * j2 * j2 * np.exp(-xvals / tau1)
            gamma = diff * self.Q * self.Q / (1 + diff * self.Q * self.Q * tau0)
            translational = np.exp(-gamma * xvals)
            iqt = amp * rotational * translational
        return iqt


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(TeixeiraWaterIqt)
