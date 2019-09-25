# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class FmuF(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.5, 'Amplitude')
        self.declareParameter("FreqD", 0.2, 'Dipolar interaction frequency (MHz)' )
        self.declareParameter("Lambda", 0.1, 'Exponential decay rate')
        self.declareParameter("Sigma", 0.2, 'Gaussian decay rate')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        FreqD = self.getParameterValue("FreqD")
        Lambda = self.getParameterValue("Lambda")
        Sigma = self.getParameterValue("Sigma")
        OmegaD = FreqD * 2 * np.pi
        Gauss = np.exp( - (Sigma * x) ** 2 / 2)
        Lor = np.exp( - Lambda * x)
        term1 = np.cos(np.sqrt(3) * OmegaD * x)
        term2 = (1 - 1 / np.sqrt(3)) * np.cos(0.5 * (3 - np.sqrt(3)) * OmegaD * x)
        term3 = (1 + 1 / np.sqrt(3)) * np.cos(0.5 * (3 + np.sqrt(3)) * OmegaD * x)
        G = (3 + term1 + term2 + term3) / 6
        return A0 * Gauss * Lor * G

FunctionFactory.subscribe(FmuF)
