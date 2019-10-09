# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class AFM_LF(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2, 'Amplitude')
        self.declareParameter("Freq", 2, 'ZF Frequency (MHz)')
        self.declareParameter("Angle", 50, 'Angle of internal field w.r.t. to applied field (degrees)')
        self.declareParameter("Field", 10, 'Applied Field (G)')        
        self.declareParameter("Phi", 0.0, 'Phase (rad)')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Freq = self.getParameterValue("Freq")
        theta = self.getParameterValue("Angle")
        B = self.getParameterValue("Field")
        phi = self.getParameterValue("Phi")
        FreqInt = Freq
        FreqExt= 0.01355 * B
        theta = np.pi / 180 * theta
        omega1 = 2 * np.pi * np.sqrt(FreqInt ** 2 + FreqExt ** 2 + 2 * FreqInt * FreqExt * np.cos(theta))
        omega2 = 2 * np.pi * np.sqrt(FreqInt ** 2 + FreqExt ** 2 - 2 * FreqInt * FreqExt * np.cos(theta))
        a1 = (FreqInt * np.sin(theta)) ** 2 / ((FreqExt + FreqInt * np.cos(theta)) ** 2 + (FreqInt * np.sin(theta)) ** 2)
        a2 = (FreqInt * np.sin(theta)) ** 2 / ((FreqExt - FreqInt * np.cos(theta)) ** 2 + (FreqInt * np.sin(theta)) ** 2)
        return A0 * ((1 - a1) + a1 * np.cos(omega1 * x + phi) + (1 - a2) + a2 * np.cos(omega2 * x + phi)) / 2

FunctionFactory.subscribe(AFM_LF)
