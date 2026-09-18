# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

import numpy as np
from mantid.api import IPeakFunction, FunctionFactory

_SIG2FWHM = 2 * np.sqrt(2 * np.log(2))


class PoldiSidelobeGaussian(IPeakFunction):
    """Gaussian core minus a broader, shallower Gaussian sidelobe.

    ``Intensity`` is the area of the *core* Gaussian, not the net integral of the function, which
    is negative for typical sidelobe parameters.  ``IPeakFunctionAdapter`` dispatches only
    centre/height/fwhm and their setters to Python, so the framework's ``intensity()`` returns that
    negative integral and cannot be overridden here - callers wanting the core area must read the
    ``Intensity`` parameter directly.  Setting ``SidelobeFraction=0`` reduces the function exactly
    to a Gaussian.
    """

    def init(self):
        # Centre is declared first deliberately: getCentreParameterName cannot be overridden from
        # Python and resolves the name by evaluating the derivatives at x=0, which all underflow to
        # zero for a narrow peak centred well away from the origin, leaving it to return parameter 0.
        self.declareParameter("Centre", 0.0, "Position of the peak maximum.")
        self.declareParameter("Intensity", 1.0, "Area under the Gaussian core.")
        self.declareParameter("Sigma", 0.002, "Standard deviation of the Gaussian core.")
        self.declareParameter("SidelobeFraction", 0.05, "Sidelobe depth as a fraction of the core amplitude.")
        self.declareParameter("SidelobeSigma", 0.04, "Standard deviation of the sidelobe.")

    def _amplitude(self):
        return self.getParameterValue("Intensity") / (self.getParameterValue("Sigma") * np.sqrt(2 * np.pi))

    def functionLocal(self, xvals):
        dx = np.asarray(xvals) - self.getParameterValue("Centre")
        core = np.exp(-0.5 * (dx / self.getParameterValue("Sigma")) ** 2)
        lobe = np.exp(-0.5 * (dx / self.getParameterValue("SidelobeSigma")) ** 2)
        return self._amplitude() * (core - self.getParameterValue("SidelobeFraction") * lobe)

    def centre(self):
        return self.getParameterValue("Centre")

    def height(self):
        return self._amplitude() * (1.0 - self.getParameterValue("SidelobeFraction"))

    def fwhm(self):
        # the core's FWHM - the sidelobe widens the apparent FWHM slightly
        return _SIG2FWHM * self.getParameterValue("Sigma")

    def setCentre(self, new_centre):
        self.setParameter("Centre", new_centre)

    def setHeight(self, new_height):
        self.setParameter("Intensity", self.getParameterValue("Intensity") * new_height / self.height())

    def setFwhm(self, new_fwhm):
        height = self.height()  # to reset after
        self.setParameter("Sigma", max(new_fwhm / _SIG2FWHM, 1e-10))
        self.setHeight(height)


FunctionFactory.subscribe(PoldiSidelobeGaussian)
