# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

import numpy as np
from scipy.special import betaln, loggamma
from mantid.api import IPeakFunction, FunctionFactory


class PearsonIV(IPeakFunction):
    def init(self):
        self.declareParameter("Intensity", 1.0, "Area under the peak.")
        self.declareParameter(
            "Centre",
            0.0,
            "Position of the peak maximum (note this differs from the usual definition of the PearsonIV - "
            "for which the 'location' parameter coincides with the maximum only for skew=0).",
        )
        self.declareParameter(
            "Sigma",
            1.0,
            "One of the parameters controlling the width of peak (valid for Sigma > 0) - increasing Sigma increases the FWHM.",
        )
        self.declareParameter(
            "Exponent",
            1.5,
            "One of the parameters controlling the width of peak (valid for Exponent > 0.5) - increasing Exponent decreases the FWHM.",
        )
        self.declareParameter(
            "Skew",
            0,
            "Parameter determining the skew/asymmetry of the peak - a negative value of Skew produces a peak with "
            "centre of mass at larger x value than the peak centre/maximum.",
        )
        # Non-fitting parameters
        self.declareAttribute("CentreShift", 0.0)

    def functionLocal(self, xvals):
        intensity = self.getParameterValue("Intensity")
        centre = self.getParameterValue("Centre") + self.getAttributeValue("CentreShift")
        sigma = self.getParameterValue("Sigma")
        expon = self.getParameterValue("Exponent")
        skew = self.getParameterValue("Skew")

        dx = (xvals - centre - skew * sigma / (2 * expon)) / sigma  # adjusted centre to be at maximum of peak
        logprefactor = 2 * (np.real(loggamma(expon + skew * 0.5j)) - loggamma(expon)) - betaln(expon - 0.5, 0.5)
        return (intensity / sigma) * np.exp(logprefactor - expon * np.log1p(dx * dx) - skew * np.arctan(dx))

    def centre(self):
        return self.getParameterValue("Centre")

    def intensity(self):
        return self.getParameterValue("Intensity")

    def height(self):
        return self.functionLocal(self.getParameterValue("Centre") + self.getAttributeValue("CentreShift"))

    def fwhm(self):
        # no exact form for FWHM - this is valid for skew=0
        return 2 * self.getParameterValue("Sigma") * np.sqrt((2 ** (1 / self.getParameterValue("Exponent"))) - 1)

    def setCentre(self, new_centre):
        self.setParameter("Centre", new_centre)

    def setHeight(self, new_height):
        self.setParameter("Intensity", self.getParameterValue("Intensity") * new_height / self.height())

    def setIntensity(self, new_intensity):
        self.setParameter("Intensity", new_intensity)

    def getWidthParameterName(self):
        return "Sigma"

    def setFwhm(self, new_fwhm):
        height = self.height()  # to reset after
        new_sigma = max(new_fwhm / (2 * np.sqrt((2 ** (1 / self.getParameterValue("Exponent"))) - 1)), 1e-10)
        self.setParameter("Sigma", new_sigma)  # valid only for S > 0
        self.setHeight(height)


FunctionFactory.subscribe(PearsonIV)
