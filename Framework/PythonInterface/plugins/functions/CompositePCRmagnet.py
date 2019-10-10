# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, anomalous-backslash-in-string, attribute-defined-outside-init

from mantid.api import IFunction1D, FunctionFactory
import numpy as np


class CompositePCRmagnet(IFunction1D):

	def category(self):
		return "Muon"

	def init(self):	
		self.declareParameter("A0", 0.2, 'Amplitude')
		self.declareParameter("KTdelta", 0.1, 'KT Relaxation rate')
		self.declareParameter("B0", 10, 'Local Magnetic Field (G)')
		self.declareParameter("Gauss", 0.2, 'Gaussian relaxation rate')

	def function1D(self, x):
		A0= self.getParameterValue("A0")
		KTdelta = self.getParameterValue("KTdelta")
		B0 = self.getParameterValue("B0")
		Gaus = self.getParameterValue("Gauss")
		gmu= 0.01355342 * 2 * np.pi
		if B0 > 0:
			output = 1./3 + 2./3 * np.cos(gmu * B0 * x) * np.exp(- (Gaus * x) ** 2 + 2./5 * (KTdelta * x) ** 2)
		else:
			output = 1./3 + 2./3 * (1 - (KTdelta * x) ** 2) * np.exp(-(KTdelta * x) ** 2 / 2)
		return A0 * output
		
FunctionFactory.subscribe(CompositePCRmagnet)
