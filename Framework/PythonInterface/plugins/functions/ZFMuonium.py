# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init


from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class ZFMuonium(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("FreqA", 0.3, 'Isotropic hyperfine coupling constant (MHz)')
        self.declareParameter("FreqD", 0.2, 'Anisotropic hyperfine coupling constant (MHz)')
        self.declareParameter("FCut", 1.0, 'Frequency cut (MHz)')
        self.declareParameter("Phi", 0.0, 'Phase')
        self.addConstraints("FreqA > 0")
        self.addConstraints("FreqD > 0")
        self.addConstraints("FCut > 0")

    def function1D(self, x):
        A0= self.getParameterValue("A0")
        FreqA = self.getParameterValue("FreqA")
        FreqD = self.getParameterValue("FreqD")
        Fcut = self.getParameterValue("FCut")
        Phi = self.getParameterValue("Phi")
        W1 = 2 * np.pi * (FreqA - FreqD)
        W2 = 2 * np.pi * (FreqA + FreqD / 2)
        W3 = 3 * np.pi * FreqD
        if Fcut > 0:
            A1 = 1 / (1 + (W1 / (2 * np.pi * Fcut)) ** 2)
            A2 = 2 / (1 + (W2 / (2 * np.pi * Fcut)) ** 2)
            A3 = 2 / (1 + (W3 / (2 * np.pi * Fcut)) ** 2)
        else:
            A1 = 1
            A2 = 2
            A3 = 2
        return A0 * (A1 * np.cos(W1 * x + Phi) + A2 * np.cos(W2 * x + Phi) + A3 * np.cos(W3 * x + Phi)) / 6

FunctionFactory.subscribe(ZFMuonium)
