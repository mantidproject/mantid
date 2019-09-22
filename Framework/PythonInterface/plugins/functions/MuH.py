# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class MuH(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.5, 'Amplitude')
        self.declareParameter("FreqD", 0.5, 'Frequency (MHz)' )
        self.declareParameter("Lambda", 0.3, 'Exponential decay rate')
        self.declareParameter("Sigma", 0.05, 'Gaussian decay rate')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        FreqD = self.getParameterValue("FreqD")
        Lambda = self.getParameterValue("Lambda")
        sigma = self.getParameterValue("Sigma")
        OmegaD = FreqD * 2 * np.pi
        gau = np.exp(-0.5 * (x * sigma) ** 2)
        Lor = np.exp( - Lambda * x)
        return A0 * gau * Lor * (1 + np.cos(OmegaD * x) + 2 * np.cos(0.5 * OmegaD * x) + 2 * np.cos(1.5 * OmegaD * x)) / 6

FunctionFactory.subscribe(MuH)
