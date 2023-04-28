# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
from mantid.kernel import PhysicalConstants

import numpy as np


class Redfield(IFunction1D):
    def category(self):
        return "Muon\\MuonModelling"

    def init(self):
        self.declareParameter("Hloc", 0.1, "Local magnetic field (G)")
        self.declareParameter("Tau", 0.1, "Correlation time of muon spins (microsec)")

    def function1D(self, x):
        Hloc = self.getParameterValue("Hloc")
        tau = self.getParameterValue("Tau")
        gmu = PhysicalConstants.MuonGyromagneticRatio
        return 2 * np.power((gmu * Hloc), 2) * tau / (1 + np.power((gmu * x * tau), 2))


FunctionFactory.subscribe(Redfield)
