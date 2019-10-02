# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np
import scipy.special as sp
from scipy.integrate import quad


class StaticLorentzianKT(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("A", 0.1, 'Half-width of half maximum of Lorentzian distribution (microsecs)')
        self.declareParameter("Field", 0.1, 'Longitudinal B-field (G)')
        self.addConstraints("Field >= 0")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        A = self.getParameterValue("A")
        B = self.getParameterValue("Field")
        gmu = 0.0135538817 * 2 * np.pi
        omega = gmu * B
        y = []

        def Lorfun(x, omega, A):
            return  sp.j0(omega * x) * np.exp(- A * x)

        if omega == 0 :
            LorKT = 1./3. + 2./3. * (1 - A * x) * np.exp(- A * x)
        else:
            q1 = A / omega * sp.j1(omega * x) * np.exp(- A * x)
            q2 = (A / omega) ** 2 * (sp.j0(omega * x) * np.exp(- A * x) - 1)
            for i in range(0, np.size(q1)):
                y.append(quad(Lorfun, 0, x[i], args = (omega, A))[0])
            q3 = (1 + (A / omega) ** 2) * A * np.array(y)
            LorKT = 1 - q1 - q2 - q3
        return A0 * LorKT

FunctionFactory.subscribe(StaticLorentzianKT)
