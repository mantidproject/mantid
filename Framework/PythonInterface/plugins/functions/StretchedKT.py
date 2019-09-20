# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class StretchedKT(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 1)
        self.declareParameter("Beta", 1)
        self.declareParameter("Sigma", 0.1)

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        beta = self.getParameterValue("Beta")
        Sigma = self.getParameterValue("Sigma")
        product = Sigma * x
        A=(2./3.) * (1 - product ** beta)
        B=np.exp(- product ** beta / beta)
        return A0*(1./3. + A * B)

FunctionFactory.subscribe(StretchedKT)
