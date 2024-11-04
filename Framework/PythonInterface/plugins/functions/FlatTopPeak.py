# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import FunctionFactory, IFunction1D
import scipy.special


class FlatTopPeak(IFunction1D):
    def category(self):
        return "SANS"

    def init(self):
        self.declareParameter("Scale", -2400.0, "The height of the mesa.")
        self.declareParameter("Centre", 50.0, "The location of the middle of the mesa.")
        self.declareParameter("EndGrad", 3.0, "The width of the transition between the base and the peak.")
        self.declareParameter("Background", 2500.0, "The baseline below the mesa.")
        self.declareParameter("Width", 20.0, "The breadth of the top of the mesa.")

    def function1D(self, xvals):
        scale = self.getParameterValue("Scale")
        centre = self.getParameterValue("Centre")
        endGrad = self.getParameterValue("EndGrad")
        background = self.getParameterValue("Background")
        width = self.getParameterValue("Width")

        out = (
            scale
            * (scipy.special.erfc((centre - 0.5 * width - xvals) / endGrad) - scipy.special.erfc((centre + 0.5 * width - xvals) / endGrad))
            + background
        )
        return out


FunctionFactory.subscribe(FlatTopPeak)
