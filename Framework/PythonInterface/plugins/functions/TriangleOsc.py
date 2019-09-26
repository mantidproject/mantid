# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class TriangleOsc(IFunction1D):

    def category(self):
        return "Muon"

    def init(self):
        self.declareParameter("A0", 0.2)
        self.declareParameter("Freq", 1, 'Frequency of the triangular oscillation (MHz)')
        self.declareParameter("Phi", 0.0,'Phase')

    def function1D(self, x):
        A0 = self.getParameterValue("A0")
        Freq = self.getParameterValue("Freq")
        phi = self.getParameterValue("Phi")
        q = Freq * x - np.trunc(Freq * x) + phi
        TriangleOsc = []
        for i in range(0,np.size(q)):
            if q[i] < 0 :
                q[i] = q[i] + 1
            if q[i] < 0.25 :
                TriangleOsc.append(A0 * 4 * q[i])
            elif q[i] < 0.75 :
                TriangleOsc.append(A0 * 4 * (0.5 - q[i]))
            else :
                TriangleOsc.append(A0 * 4 * (q[i] - 1))
        return np.array(TriangleOsc)

FunctionFactory.subscribe(TriangleOsc)
