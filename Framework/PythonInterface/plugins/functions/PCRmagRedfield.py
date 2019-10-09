# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class PCRmagRedfield(IFunction1D):

	def category(self):
		return "Muon"

	def init(self):	
		self.declareParameter("A0", 0.2, 'Amplitude')
		self.declareParameter("Delta", 0.2)
		self.declareParameter("Nu", 0.2)

	def function1D(self, x):
		A0 = self.getParameterValue("A0")
		Delta = self.getParameterValue("Delta")
		Nu = self.getParameterValue("Nu")
		W = 2 * np.pi * Delta
		Q = (Nu ** 2 + Delta ** 2) * Nu
		if Q > 0:
			Lambda = Delta ** 4 / Q
		else:
			Lambda = 0
		return A0 * (1./3 + 2./3 * np.exp(- Lambda * x) * np.cos(W * x))

FunctionFactory.subscribe(PCRmagRedfield)
