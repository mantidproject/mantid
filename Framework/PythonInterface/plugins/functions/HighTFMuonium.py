# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class HighTFMuonium(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.1)
        self.declareParameter("Field", 100, "Magnetic field (G)")
        self.declareParameter("Freq", 0.2, 'Isotropic hyperfine coupling constant (MHz)')
        self.declareParameter("Phi", 0.0, 'Phi (rad)')

    def function1D(self, x):
        A0= self.getParameterValue("A0")
        Field = self.getParameterValue("Field")
        Freq = self.getParameterValue("Freq")
        Phi = self.getParameterValue("Phi")
        gmu = 0.01355342
        ge = 2.8024
        OmegaMinus = (ge - gmu) * Field * np.pi
        Omega = 2 * np.pi * (np.sqrt(Freq ** 2 + ((ge + gmu) * Field) ** 2) / 2 - Freq / 2)
        Omega2 = Omega - OmegaMinus
        Omega1 = Omega2 + Freq * 2 * np.pi
        return A0 * 0.5 * (np.cos(Omega1 * x + Phi) + np.cos(Omega2 * x + Phi))

FunctionFactory.subscribe(HighTFMuonium)
