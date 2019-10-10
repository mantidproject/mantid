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


class ModOsc(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("Freq", 1.0, 'Oscillation Frequency (MHz)')
        self.declareParameter("ModFreq", 0.1, 'Modulation Frequency (MHz)')
        self.declareParameter("Phi", 0.0, 'Phase')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Freq = self.getParameterValue("Freq")
        ModFreq = self.getParameterValue("ModFreq")
        phi = self.getParameterValue("Phi")
        return A0 * np.cos(2 * np.pi * Freq * x + phi) * sp.j0(2 * np.pi * ModFreq * x + phi)

FunctionFactory.subscribe(ModOsc)
