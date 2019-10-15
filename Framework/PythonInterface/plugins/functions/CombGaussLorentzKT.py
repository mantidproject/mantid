# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class CombGaussLorentzKT(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.5, 'Amplitude')
        self.declareParameter("Lambda", 0.1, 'Exponential decay rate')
        self.declareParameter("Sigma", 0.2, 'KuboToyabe decay rate')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Lambda = self.getParameterValue("Lambda")
        Sigma = self.getParameterValue("Sigma")
        return A0 * ((1./3.) + (2./3.) * (1.-(Sigma * x) ** 2 - Lambda * x) * np.exp(-0.5*(Sigma * x) ** 2-Lambda * x))

FunctionFactory.subscribe(CombGaussLorentzKT)
