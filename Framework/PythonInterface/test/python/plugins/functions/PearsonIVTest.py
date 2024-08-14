# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FunctionWrapper
from mantid.api import FunctionFactory

import numpy as np
import unittest


class PearsonIVTest(unittest.TestCase):
    def setUp(self):
        self.func = FunctionFactory.Instance().createPeakFunction("PearsonIV")
        pars = [1, 115, 1.0, 1.5, -5]  # intensity, centre, sigma, exponent, skew
        [self.func.setParameter(ipar, par) for ipar, par in enumerate(pars)]

    def test_exec_functionLocal(self):
        x = np.linspace(100, 200, 101)
        y = FunctionWrapper(self.func)(x)

        # assert maximum and integral
        self.assertAlmostEqual(y.max(), 0.2374, delta=1e-4)
        intensity = np.sum(y) * (x[1] - x[0])
        self.assertAlmostEqual(np.sum(y) * (x[1] - x[0]), 0.99, delta=1e-2)

    def test_intensity(self):
        self.assertAlmostEqual(self.func.intensity(), 1, delta=1e-3)

    def test_set_intensity(self):
        self.func.setIntensity(2)
        self.assertAlmostEqual(self.func.intensity(), 2, delta=1e-3)

    def test_fwhm(self):
        fwhm = self.func.fwhm()
        new_fwhm = 2 * fwhm
        self.func.setFwhm(new_fwhm)

        self.assertAlmostEqual(fwhm, 1.5328, delta=1e-3)
        self.assertAlmostEqual(new_fwhm, 2 * 1.5328, delta=1e-3)

    def test_setFwhm_keep_sigma_grtrthn_zero(self):
        self.func.setFwhm(0)
        self.assertAlmostEqual(self.func.getParameterValue("Sigma"), 1e-10, delta=1e-10)


if __name__ == "__main__":
    unittest.main()
