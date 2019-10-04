# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import *
import numpy as np


class SilverBaseline(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0",0.1)

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        return A0*np.exp(-0.0015*x)

FunctionFactory.subscribe(SilverBaseline)
