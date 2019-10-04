# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class RedfieldCutoff(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 1)
        self.declareParameter("Hloc", 0.1, "Local magnetic field (G)")
        self.declareParameter("Tau", 0.1, "Correlation time of muon spins (microsec)")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Hloc = self.getParameterValue("Hloc")
        tau = self.getParameterValue("Tau") 
        gmu = 0.01355342
        return A0 * 2 * (gmu * Hloc) ** 2 * tau / (1 + (gmu * x * tau) ** 2)

FunctionFactory.subscribe(RedfieldCutoff)
