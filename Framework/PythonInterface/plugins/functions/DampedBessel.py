# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np
from scipy import special as sp


class DampedBessel(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2, 'Asymmetry')
        self.declareParameter("Phi", 0.0, 'Phase (rad)')
        self.declareParameter("Field", 10, 'B Field (G)')
        self.declareParameter("LambdaL", 0.1, 'Dynamic longitudinal spin relaxation rate')
        self.declareParameter("LambdaT", 0.1, 'Damping of the oscillation')
        self.declareParameter("FractionL", 0.1, 'Fraction of longitudinal signal component')
        self.addConstraints("0 < FractionL < 1")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        field = self.getParameterValue("Field")
        phi = self.getParameterValue("Phi")
        LambdaL = self.getParameterValue("LambdaL")
        LambdaT = self.getParameterValue("LambdaT")
        fraction = self.getParameterValue("FractionL")
        omega = 0.01355342 * 2 * np.pi * field
        return A0 * np.exp(- LambdaL * x) * ((1 - fraction) * np.exp(LambdaT * x) * sp.j0(omega * x + phi) + fraction)

FunctionFactory.subscribe(DampedBessel)
