# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class SpinGlass(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2, 'Asymmetry')
        self.declareParameter("Width", 0.1, 'Half-width half maximum of the local field Lorentzian Distribution')
        self.declareParameter("Nu", 1, 'Rate of Markovian modulation')
        self.declareParameter("Q", 0.1, 'Order Parameter')
        self.addConstraints("Nu > 0")
        self.addConstraints("0 < Q < 1")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        a = self.getParameterValue("Width")
        nu = self.getParameterValue("Nu")
        q = self.getParameterValue("Q")
        x = x[1 : np.size(x)]
        term1 = 4 * (a ** 2) * (1 - q) * x / nu
        term2 = q * (a ** 2) * (x ** 2)
        term13 = np.exp(- np.sqrt(term1))
        term23 = (1 - (term2 / np.sqrt(term1 + term2))) * np.exp(- np.sqrt(term1 + term2))
        SpinGlass = A0 * ((1./3.) * term13 + (2./3.) * term23)
        SpinGlass = np.insert(SpinGlass, 0, A0)
        return SpinGlass

FunctionFactory.subscribe(SpinGlass)
