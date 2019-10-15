# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class StandardSC(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.16, 'Amplitude of interal field oscillation')
        self.declareParameter("Sigma", 0.2, 'Gaussian decay rate')
        self.declareParameter("FieldSC", 300.0, 'Internal Field (G)')
        self.declareParameter("FieldBG", 300.0, 'External Field (G)')
        self.declareParameter("Phi", 0.0, 'Phase')
        self.declareParameter("Abg", 0.1, 'Amplitude of external field oscillation')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        sigma = self.getParameterValue("Sigma")
        FieldSC = self.getParameterValue("FieldSC")
        FieldBG = self.getParameterValue("FieldBG")
        phi = self.getParameterValue("Phi")
        Abg = self.getParameterValue("Abg")
        omegaSC = FieldSC * 0.1355 * 2 * np.pi
        omegaBG = FieldBG * 0.1355 * 2 * np.pi
        return A0 * np.exp(- 0.5 * sigma * sigma * x * x) * np.cos(omegaSC * x + phi)+ Abg * np.cos(omegaBG * x + phi)

FunctionFactory.subscribe(StandardSC)
