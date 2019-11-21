# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
'''
@author Spencer Howells, ISIS
@date December 05, 2013
'''
from __future__ import (absolute_import, division, print_function)

import math
import numpy as np

from mantid.api import IFunction1D, FunctionFactory
from scipy import constants


class HallRoss(IFunction1D):

    planck_constant = constants.Planck / constants.e * 1E15  # meV*psec
    hbar = planck_constant / (2 * np.pi)  # meV * ps  = ueV * ns

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Active fitting parameters
        self.declareParameter("Tau", 1.0, 'Residence time')
        self.declareParameter("L", 0.2, 'Jump length')

    def function1D(self, xvals):
        tau = self.getParameterValue("Tau")
        l = self.getParameterValue("L")
        l = l**2 / 2
        xvals = np.array(xvals)

        with np.errstate(divide='ignore'):
            hwhm = self.hbar*(1.0 - np.exp(-l * np.square(xvals))) / tau
        return hwhm

    def functionDeriv1D(self, xvals, jacobian):
        tau = self.getParameterValue("Tau")
        l = self.getParameterValue("L")
        l = l**2 / 2

        for i, x in enumerate(xvals, start=0):
            ex = math.exp(-l*x*x)
            hwhm = self.hbar*(1.0-ex)/tau
            jacobian.set(i, 0, -hwhm/tau)
            jacobian.set(i, 1, x*x*ex/tau)


# Required to have Mantid recognise the new function
FunctionFactory.subscribe(HallRoss)
