# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class ZFelectronDipole(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):	
        self.declareParameter("A0", 0.2)
        self.declareParameter("Radius", 10, 'Radius (Angstrom)')
        self.declareParameter("LambdaTrans", 0.2, 'Lambda-Trans (MHz)')
        self.addConstraints("Radius > 0")
        self.addConstraints("LambdaTrans > 0")

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        radius = self.getParameterValue("Radius")
        LambdaTrans = self.getParameterValue("LambdaTrans")
        gmu = 2 * np.pi * 0.01355342
        if radius > 0:
            B = 8290 / (radius ** 3)
        else:
            B = 0
        Omega = gmu * B
        return A0 * (1./6) * (1 + np.exp(- LambdaTrans * x) * (np.cos(Omega * x) + 2 * np.cos(1.5 * Omega * x) + 2 * np.cos(0.5 * Omega * x)))

FunctionFactory.subscribe(ZFelectronDipole)
