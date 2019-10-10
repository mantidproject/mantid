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

class GaussBessel(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2, 'Amplitude')
        self.declareParameter("Freq", 0.5, 'ZF Frequency (MHz)')
        self.declareParameter("Sigma", 0.2, 'Gaussian relaxation for oscillatory component')        
        self.declareParameter("Phi", 0.0, 'Phase (rad)')


    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Freq = self.getParameterValue("Freq")
        sigma = self.getParameterValue("Sigma")
        phi = self.getParameterValue("Phi")
        omega = 2 * np.pi * Freq
        return A0 * (1./3. + 2./3. * sp.j0(omega * x + phi) * np.exp(- 0.5 * (sigma * x) ** 2))

FunctionFactory.subscribe(GaussBessel)
