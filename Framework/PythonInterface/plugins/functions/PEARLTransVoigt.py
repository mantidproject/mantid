# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# -----------------------------------------------------------------------
# Approximation to Voigt function
# written by C J Ridley for Transfit v2 in Mantid
# Last updated: 05th October 2020
#
# Define approximation to Voigt function consistent with transfit v1 see Igor TN026 for technical details and references

from mantid.api import IFunction1D, FunctionFactory
import numpy as np
from numpy import vectorize


def voigtFunction(X, Y):
    Y = abs(Y)
    S = abs(X) + Y
    T = Y - (X * 1j)

    # Determine values based on value of S
    # REGION 1
    if S >= 15:
        W = T * 0.5641896 / (0.5 + T * T)

    # REGION 2
    else:
        if S >= 5.5:
            U = T * T
            W = T * (1.410474 + U * 0.5641896) / (0.75 + U * (3 + U))
    # REGION 3
        else:
            if Y >= (0.195 * np.abs(X) - 0.176):
                W = (16.4955 + T * (20.20933 + T * (11.96482 + T * (3.778987 + T * 0.5642236))))
                W /= (16.4955 + T * (38.82363 + T * (39.27121 + T * (21.69274 + T * (6.699398 + T)))))
    # REGION 4
            else:
                U = T * T
                step = (1540.787 - U * (219.0313 - U * (35.76683 - U * (1.320522 - U * 0.56419))))
                W = T * (36183.31 - U * (3321.9905 - U * step))
                step = (9022.228 - U * (2186.181 - U * (364.2191 - U * (61.57037 - U * (1.841439 - U)))))
                W /= (32066.6 - U * (24322.84 - U * step))
                W = (np.exp(np.real(U)) * np.cos(np.imag(U)) + 0j) - W
    return np.real(W)


class PEARLTransVoigt(IFunction1D):
    def init(self):
        # Starting parameters as fitted from run PRL111643
        self.declareParameter("Position", 1096.3)
        self.declareParameter("LorentzianFWHM", 45.8)
        self.declareParameter("GaussianFWHM", 25.227)
        self.declareParameter("Amplitude", 2.08)
        # Legacy parameters from Transfit v1:
        self.declareParameter("Bg0", 25)
        self.declareParameter("Bg1", 0.015)
        self.declareParameter("Bg2", 0)

    def function1D(self, xvals):
        # Vectorise the function to allow if/else statements to work quickly
        vVoigtFunction = vectorize(voigtFunction)
        # Declare fitting parameters
        pos = self.getParameterValue("Position")
        amp = self.getParameterValue("Amplitude")
        lorFWHM = self.getParameterValue("LorentzianFWHM")
        gaussFWHM = self.getParameterValue("GaussianFWHM")
        # Legacy parameters from Transfit v1:
        bg0 = self.getParameterValue("Bg0")
        bg1 = self.getParameterValue("Bg1")
        bg2 = self.getParameterValue("Bg2")
        #
        # Legacy background function included from Transfit v1
        # Define background function
        bg = bg0 + bg1**xvals + bg2 * xvals * xvals
        # Correct using Beer's law to fit measured absorption, not Xsection
        # np.sqrt(np.log(2)) replaced with 1 as legacy
        width = 1 / gaussFWHM
        shape = lorFWHM * width
        outVal = np.exp(-1 * np.abs(amp * vVoigtFunction(width * (xvals - pos), shape)))
        outVal = bg * outVal
        return outVal


FunctionFactory.subscribe(PEARLTransVoigt)
