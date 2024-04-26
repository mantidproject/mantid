# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy as np

from mantid.api import IFunction1D, FunctionFactory
from scipy.special import spherical_jn


class TeixeiraWaterIqt(IFunction1D):

    def category(self):
        return "QuasiElastic"

    def init(self):
        # Fitting parameters
        self.declareParameter("Amp", 1.00, "Amplitude")
        self.declareParameter("Tau1", 0.05, "Relaxation time")
        self.declareParameter("Gamma", 1.2, "Line Width")
        self.addConstraints("Tau1 > 0")
        # Non-fitting parameters
        self.declareAttribute("Q", 0.4)
        self.declareAttribute("a", 0.98)

    def function1D(self, xvals):
        amp = self.getParameterValue("Amp")
        tau1 = self.getParameterValue("Tau1")
        gamma = self.getParameterValue("Gamma")

        q_value = self.getAttributeValue("Q")
        radius = self.getAttributeValue("a")

        xvals = np.array(xvals)

        qr = np.array(q_value * radius)
        j0 = spherical_jn(0, qr)
        j1 = spherical_jn(1, qr)
        j2 = spherical_jn(2, qr)
        with np.errstate(divide="ignore"):
            rotational = np.square(j0) + 3 * np.square(j1) * np.exp(-xvals / (3 * tau1)) + 5 * np.square(j2) * np.exp(-xvals / tau1)
            translational = np.exp(-gamma * xvals)
            iqt = amp * rotational * translational
        return iqt


FunctionFactory.subscribe(TeixeiraWaterIqt)
