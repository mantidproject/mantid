# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np
from scipy import special as sp


class Bessel(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 1, 'Amplitude')
        self.declareParameter("Phi", 0.1, 'Phase(rad)')
        self.declareParameter("Nu", 0.1, 'Frequency(MHz)')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Phi = self.getParameterValue("Phi")
        Nu = self.getParameterValue("Nu")
        return A0 * sp.j0(2 * np.pi * Nu * x + Phi)

FunctionFactory.subscribe(Bessel)
