# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class TFMuonium(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.5, 'Amplitude')
        self.declareParameter("Field", 5, 'B-field (G)')
        self.declareParameter("A", 600, 'Isotropic hyperfine coupling constant (MHz)')
        self.declareParameter("Phi", 0.0, 'Phase')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        B = self.getParameterValue("Field")
        A = self.getParameterValue("A")
        phi = self.getParameterValue("Phi")
        gm = 0.01355342
        ge = 2.8024
        fcut = 10**32
        chi =(gm + ge) * B / A
        diff =(ge - gm) / (gm + ge)
        delta = chi / np.sqrt(1 + chi ** 2)
        E1 = A / 4 * (1 + 2 * diff * chi)
        E2 = A / 4 * (- 1 + 2 * np.sqrt(1 + chi ** 2))
        E3 = A / 4 * (1 - 2 * diff * chi)
        E4 = A / 4 * (- 1 - 2 * np.sqrt(1 + chi ** 2))
        w12 = 2 * np.pi * (E1 - E2)
        w14 = 2 * np.pi * (E1 - E4)
        w34 = 2 * np.pi * (E3 - E4)
        w23 = 2 * np.pi * (E2 - E3)
        a12 = 1 / (1 + (w12 / (2 * np.pi * fcut)) ** 2)
        a14 = 1 / (1 + (w14 / (2 * np.pi * fcut)) ** 2)
        a34 = 1 / (1 + (w34 / (2 * np.pi * fcut)) ** 2)
        a23 = 1 / (1 + (w23 / (2 * np.pi * fcut)) ** 2)
        Term1 = (1 + delta) * a12 * np.cos(w12 * x + phi)
        Term2 = (1 - delta) * a14 * np.cos(w14 * x + phi)
        Term3 = (1 + delta) * a34 * np.cos(w34 * x + phi)
        Term4 = (1 - delta) * a23 * np.cos(w23 * x + phi)
        return A0 * 0.25 * (Term1 + Term2 + Term3 + Term4)

FunctionFactory.subscribe(TFMuonium)
