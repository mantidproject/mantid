# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class PCRmagnetfnorm(IFunction1D):

	def category(self):
		return "Muon"

	def init(self):	
		self.declareParameter("A0", 0.2, 'Amplitude')
		self.declareParameter("SigmaOverW", 0.1, 'Normalised relaxation rate')
		self.declareParameter("H0", 3.0, 'Local magnetic field')
		self.declareParameter("Toff", 0.1. 'Time offset')

	def function1D(self, x):
		A0= self.getParameterValue("A0")
		sigma = self.getParameterValue("SigmaOverW")
		H0 = self.getParameterValue("H0")
		Toff = self.getParameterValue("Toff")
		gmu = 2 * np.pi * 0.01355342
		w = H0 * gmu
		x = x - Toff
		return A0 * (1./3 + 2./3 * np.exp(- (sigma * w * x) ** 2 / 2) * np.cos(w * x))

FunctionFactory.subscribe(PCRmagnetfnorm)
