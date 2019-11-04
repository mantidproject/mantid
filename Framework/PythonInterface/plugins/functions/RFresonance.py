# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class RFresonance(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("Boffset", 10, 'Relaxation rate (G)')
        self.declareParameter("B1", 10, 'Fluctuation (G)')
        self.declareParameter("B1GauWidth", 0.2, 'Width of resonance (MHz)')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Bcentre = self.getParameterValue("Boffset")
        B = self.getParameterValue("B1")
        width = self.getParameterValue("B1GauWidth")
        gmu = 0.0135538817 * 2 * np.pi
        Beff = np.sqrt(Bcentre ** 2 + B ** 2)
        RelAmp = (B / Beff) ** 2
        return A0 * (1 + (np.cos(Beff * gmu * x) * np.exp(- (width * x) ** 2) - 1) * RelAmp)

FunctionFactory.subscribe(RFresonance)
