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
        self.declareParameter("Spin", 3.5, 'J, Total angular momentum quanutm number')
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
        J2 = round(2 * J)
        J = J2/2
        Wm = []
        lamp = []
        lamm = []
        cosSQ2alpha = []
        sinSQ2alpha = []
        cosSQalpha = []
        sinSQalpha = []

        for i in range(0 , int(J2 + 2)):
            m = i - J
            q1 = (OmegaQ + OmegaD) * (2 * m - 1)
            q2 = OmegaD * np.sqrt(J * (J + 1) - m * (m - 1))
            qq = q1 ** 2 + q2 ** 2
            q3 = OmegaQ * (2 * m ** 2 - 2 * m + 1) + OmegaD
            Wm.append(np.sqrt(qq))
            if i < J2 + 1:
                lamp.append(0.5 * (q3 + Wm[i]))
            else:
                lamp.append(OmegaQ * J ** 2 - OmegaD * J)
            if i > 0:
                lamm.append(0.5 * (q3 - Wm[i]))
            else:
                lamm.append(OmegaQ * J ** 2 - OmegaD * J)
            if qq > 0:
                cosSQ2alpha.append(q1 ** 2 / qq)
            else:
                cosSQ2alpha.apend(0)
            sinSQ2alpha.append(1 - cosSQ2alpha[i])
            cosSQalpha.append(0.5 * (1 + np.sqrt(cosSQ2alpha[i])))
            sinSQalpha.append(1 - cosSQalpha[i])

        tz = 0
        tx = 0
        for i in range(1, int(J2) + 1):
            tz = tz + cosSQ2alpha[i] + sinSQ2alpha[i] * np.cos((lamp[i] - lamm[i]) * x)
        Pz = (1 + tz) / (2 * J + 1)

        for i in range(0, int(J2) + 1):
            a = cosSQalpha[i+1] * sinSQalpha[i] * np.cos((lamp[i + 1] - lamp[i]) * x)
            b = cosSQalpha[i+1] * cosSQalpha[i] * np.cos((lamp[i + 1] - lamm[i]) * x)
            c = sinSQalpha[i+1] * sinSQalpha[i] * np.cos((lamm[i + 1] - lamp[i]) * x)
            d = sinSQalpha[i+1] * cosSQalpha[i] * np.cos((lamm[i + 1] - lamm[i]) * x)
            tx = tx + a + b + c + d
        Px = tx / (2 * J + 1)
        return A0 * gau * Lor * (1./3.) * (2 * Px + Pz)

FunctionFactory.subscribe(Meier)
