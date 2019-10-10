# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class LowTFMuonium(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):	
        self.declareParameter("A0", 0.2, 'Amplitude')
        self.declareParameter("Field", 0.1, 'Magnetic Field (G)')
        self.declareParameter("A", 0.2, 'Isotropic hyperfine coupling constant (MHz)')
        self.declareParameter("Phi", 0.0,'Phase (rad)')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        B = self.getParameterValue("Field")
        A = self.getParameterValue("A")
        phi = self.getParameterValue("Phi")
        gm = 0.01355342
        ge = 2.8024
        fcut = 10**32
        k =(gm + ge) * B / A
        d =(ge - gm) / (gm + ge)
        delta = k / np.sqrt(1 + k ** 2)
        E1 = A / 4 * (1 + 2 * d * k)
        E2 = A / 4 * (- 1 + 2 * np.sqrt(1 + k ** 2))
        E3 = A / 4 * (1 - 2 * d * k)
        f12 = E1 - E2
        f23 = E2 - E3
        a12 = 1 / (1 + (f12 / fcut) ** 2)
        a23 = 1 / (1 + (f23 / fcut) ** 2)
        return A0 * 0.25 * ((1 + delta) * a12 * np.cos(2 * np.pi * f12 * x + phi) + (1 - delta) * a23 * np.cos(2 * np.pi * f23 * x + phi))
        
FunctionFactory.subscribe(LowTFMuonium)
