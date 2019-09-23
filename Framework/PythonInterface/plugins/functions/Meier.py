# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class Meier(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.5, 'Amplitude')
        self.declareParameter("FreqD", 0.01, 'Frequency due to dipolar coupling (MHz)')
        self.declareParameter("FreqQ", 0.05, 'Frequency due to quadrupole interaction of the nuclear spin (MHz)')
        self.declareParameter("Spin", 3.5, 'J, Total orbital quanutm number')
        self.declareParameter("Sigma", 0.2, 'Gaussian decay rate')
        self.declareParameter("Lambda", 0.1, 'Exponential decay rate')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        FreqD = self.getParameterValue("FreqD")
        FreqQ = self.getParameterValue("FreqQ")
        J = self.getParameterValue("Spin")
        Lambda = self.getParameterValue("Lambda")
        sigma = self.getParameterValue("Sigma")
        OmegaD = 2 * np.pi * FreqD
        OmegaQ = 2 * np.pi * FreqQ
        gau = np.exp(- 0.5 * (sigma * x) ** 2)
        Lor = np.exp(- Lambda * x)

        def W(m):
            return np.sqrt((2 * m - 1) ** 2 * (OmegaD + OmegaQ) ** 2 + OmegaD ** 2 * (J * (J + 1) - m * (m - 1)))

        def lamp(m):
            lampValue = 0
            if m == J + 1:
                lampValue = (OmegaQ * J ** 2) - (OmegaD * J)
            else:
                lampValue = 0.5 * (OmegaQ * (2 * m * m - 2 * m + 1) + OmegaD + W(m))
            return lampValue

        def lamm(m):
            lammValue = 0
            if m == -J:
                lammValue = (OmegaQ * J ** 2) - (OmegaD * J)
            else:
                lammValue = 0.5 * (OmegaQ * (2 * m ** 2 - 2 * m + 1) + OmegaD - W(m))
            return lammValue

        def alpha(m):
            alphaValue= 0.5 * np.arctan(OmegaD * np.sqrt(J * (J + 1) - m * (m - 1)) / ((1 - 2 * m) * (OmegaD + OmegaQ)))
            return alphaValue

        tz = 0
        tx = 0
        for mm in np.arange(- J + 1, J + 1, 1):
            tz = tz + np.cos(2 * alpha(mm)) ** 2 + np.sin(2 * alpha(mm)) ** 2 * np.cos((lamp(mm) - lamm(mm)) * x)
        Pz=(1 + tz) / (2 * J + 1)
        for mm in np.arange(- J, J + 1, 1):
            a = np.cos(alpha(mm + 1)) ** 2 * np.sin(alpha(mm)) ** 2 * np.cos((lamp(mm + 1) - lamp(mm)) * x)
            b = np.cos(alpha(mm + 1)) ** 2 * np.cos(alpha(mm)) ** 2 * np.cos((lamp(mm + 1) - lamm(mm)) * x)
            c = np.sin(alpha(mm + 1)) ** 2 * np.sin(alpha(mm)) ** 2 * np.cos((lamm(mm + 1) - lamp(mm)) * x)
            d = np.sin(alpha(mm + 1)) ** 2 * np.cos(alpha(mm)) ** 2 * np.cos((lamm(mm + 1) - lamm(mm)) * x)
            tx = tx + a + b + c + d
        Px = tx / (2 * J + 1)
        return A0 * gau * Lor * (1./3.) * (2 * Px + Pz)

FunctionFactory.subscribe(Meier)
