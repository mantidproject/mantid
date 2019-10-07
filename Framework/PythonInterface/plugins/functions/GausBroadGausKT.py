# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class GauBroadGauKT(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("R", 0.1, 'Broadening ratio')
        self.declareParameter("Delta0", 0.1, 'Central Field width')
        self.addConstraints("Delta0 >= 0")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        R = self.getParameterValue("R")
        Delta = self.getParameterValue("Delta0")
        omega = R * Delta
        DeltaEff = np.sqrt(Delta ** 2 + omega ** 2)
        denom = 1 + R ** 2 + R ** 2 * DeltaEff ** 2 * x ** 2
        term1 = (DeltaEff * x) ** 2 / denom
        return A0 * (1./3. + 2./3. * ((1 + R ** 2) / denom) ** 1.5 * (1 - term1) * np.exp(- term1 / 2))

FunctionFactory.subscribe(GauBroadGauKT)
