# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class AFM_ZF(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2, 'Amplitude')
        self.declareParameter("Nu", 1, 'ZF Frequency (MHz)')
        self.declareParameter("Angle", 50, 'Angle of internal field w.r.t. to applied field (degrees)')
        self.declareParameter("Sigma", 0.2, 'Gaussian relaxation for oscillatory component')        
        self.declareParameter("Phi", 0.0, 'Phase (rad)')
        self.addConstraints("Sigma > 0")
        self.addConstraints("Freq > 0")


    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Freq = self.getParameterValue("Nu")
        theta = self.getParameterValue("Angle")
        sigma = self.getParameterValue("Sigma")
        phi = self.getParameterValue("Phi")
        omega = 2 * np.pi * Freq
        theta = np.pi / 180 * theta
        a1 = np.sin(theta) ** 2
        return A0 * ((1 - a1) + a1 * np.cos(omega * x + phi) * np.exp(- 0.5 * (sigma * x) ** 2))

FunctionFactory.subscribe(AFM_ZF)
