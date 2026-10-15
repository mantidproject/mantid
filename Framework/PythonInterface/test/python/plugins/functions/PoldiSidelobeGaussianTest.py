# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory

import numpy as np
import unittest

CENTRE, SIGMA, INTENSITY, FRAC, SIGMA2 = 2.0, 0.002, 5.0, 0.06, 0.039
RTOL = 1e-5  # FunctionWrapper does not evaluate at exactly the x values given


class PoldiSidelobeGaussianTest(unittest.TestCase):
    def setUp(self):
        self.func = FunctionFactory.Instance().createPeakFunction("PoldiSidelobeGaussian")
        # centre, intensity, sigma, sidelobe fraction, sidelobe sigma
        for ipar, par in enumerate([CENTRE, INTENSITY, SIGMA, FRAC, SIGMA2]):
            self.func.setParameter(ipar, par)

    def _evaluate(self, xvals):
        return FunctionWrapper(self.func)(np.asarray(xvals))

    def test_exec_functionLocal(self):
        dx = np.array([0.0, SIGMA, 10 * SIGMA])
        amplitude = INTENSITY / (SIGMA * np.sqrt(2 * np.pi))
        expected = amplitude * (np.exp(-0.5 * (dx / SIGMA) ** 2) - FRAC * np.exp(-0.5 * (dx / SIGMA2) ** 2))
        self.assertLess(expected[-1], 0.0)  # the lobe must still be evaluated 10 core sigma out
        np.testing.assert_allclose(self._evaluate(CENTRE + dx), expected, rtol=RTOL)

    def test_zero_fraction_reduces_to_a_gaussian(self):
        self.func.setParameter("SidelobeFraction", 0.0)
        gauss = FunctionFactory.Instance().createPeakFunction("Gaussian")
        gauss.setParameter("PeakCentre", CENTRE)
        gauss.setParameter("Sigma", SIGMA)
        gauss.setParameter("Height", INTENSITY / (SIGMA * np.sqrt(2 * np.pi)))
        x = np.linspace(CENTRE - 5 * SIGMA, CENTRE + 5 * SIGMA, 51)
        np.testing.assert_allclose(self._evaluate(x), FunctionWrapper(gauss)(x), rtol=RTOL)

    def test_getCentreParameterName(self):
        # cannot be overridden from Python - it falls back to the first parameter when every
        # derivative underflows, which is why Centre is declared first
        self.assertEqual(self.func.getCentreParameterName(), "Centre")

    def test_intensity_is_the_net_integral_not_the_core_area(self):
        # the framework integrates the function, so callers wanting the core area must read the
        # Intensity parameter - see the class docstring
        self.assertAlmostEqual(self.func.intensity(), INTENSITY * (1 - FRAC * SIGMA2 / SIGMA), delta=1e-6)

    def test_fwhm(self):
        self.assertAlmostEqual(self.func.fwhm(), 2 * np.sqrt(2 * np.log(2)) * SIGMA, delta=1e-6)

    def test_setFwhm_keeps_height(self):
        height = self._evaluate([CENTRE])[0]
        self.func.setFwhm(0.01)
        self.assertAlmostEqual(self.func.fwhm(), 0.01, delta=1e-6)
        self.assertAlmostEqual(self._evaluate([CENTRE])[0], height, delta=RTOL * height)

    def test_setFwhm_keep_sigma_grtrthn_zero(self):
        self.func.setFwhm(0)
        self.assertAlmostEqual(self.func.getParameterValue("Sigma"), 1e-10, delta=1e-10)

    def test_setCentre_and_setHeight(self):
        self.func.setCentre(1.5)
        self.func.setHeight(100.0)
        self.assertAlmostEqual(self.func.getParameterValue("Centre"), 1.5)
        self.assertAlmostEqual(self._evaluate([1.5])[0], 100.0, delta=RTOL * 100)


if __name__ == "__main__":
    unittest.main()
