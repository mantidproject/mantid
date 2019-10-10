# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class MuMinusExpTF(IFunction1D):

	def category(self):
		return "Muon"

	def init(self):
		self.declareParameter("A", 1)
		self.declareParameter("Lambda", 0.1, 'Decay rate of oscillating term')
		self.declareParameter("N0", 1)
		self.declareParameter("Tau", 5.0)    
		self.declareParameter("Phi", 0.3, 'Phase')
		self.declareParameter("Nu", 0.2, 'Oscillating frequency (MHz)')

	def function1D(self, x):
		A = self.getParameterValue("A")
		Lambda = self.getParameterValue("Lambda")
		N0 = self.getParameterValue("N0")
		tau = self.getParameterValue("Tau")
		phi = self.getParameterValue("Phi")
		nu = self.getParameterValue("Nu")
		return N0 * np.exp(- x / tau) * (1 + A * np.exp(- Lambda * x) * np.cos(2 * np.pi * nu * x + phi))

FunctionFactory.subscribe(MuMinusExpTF)
