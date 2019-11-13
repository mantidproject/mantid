# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np
from scipy.constants import k
from scipy.integrate import quad


class SCgapSwave(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("Delta", 1.2, 'Gap Value')
        self.declareParameter("Tcritical", 9.0, 'Critical Temperature')
        self.addConstraints("Delta >= 0")
        self.addConstraints("Tcritical >= 0")

    def function1D(self, x):
        Ec = 15.0
        Delta_0 = self.getParameterValue("Delta")
        Tc = self.getParameterValue("Tcritical")
        kb = k / (1.6 * 10 ** -22)
        Integral = []

        def Integrand(E):
            DeltaSwave = Delta_0
            a = 1.018
            c = 1.82
            Delta = DeltaSwave * np.tanh(c * (a*Tc / xx - 1.00) ** 0.51)
            return 1.00 / np.cosh(np.sqrt((Ec * E) ** 2 + Delta ** 2) / (2 * kb * xx)) ** 2

        for xx in x:
            if xx > Tc:
                Integral.append(1)
            else:
                Integral.append(quad(Integrand, 0, 1)[0] * Ec / (2 * kb * xx))

        return 1.00 - np.array(Integral)


FunctionFactory.subscribe(SCgapSwave)
