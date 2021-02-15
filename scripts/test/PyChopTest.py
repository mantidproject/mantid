# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test suite for the PyChop package
"""
import unittest
import warnings
import numpy as np

from PyChop import PyChop2


class PyChop2Tests(unittest.TestCase):

    # Tests the Fermi chopper instruments
    def test_pychop_fermi(self):
        instnames = ['maps', 'mari', 'merlin']
        res = []
        flux = []
        for inc, instname in enumerate(instnames):
            chopobj = PyChop2(instname)
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            chopobj.setChopper('s', 200)
            chopobj.setEi(18)
            rr, ff = chopobj.getResFlux(np.linspace(0, 17, 10))
            res.append(rr)
            flux.append(ff)
        # Checks that the flux should be highest for MERLIN, MARI and MAPS in that order
        self.assertGreater(flux[2], flux[1])
        # Note that MAPS has been upgraded so now should have higher flux than MARI.
        self.assertGreater(flux[0], flux[1])
        # Checks that the resolution should be best for MAPS, MARI, and MERLIN in that order
        # actually MAPS and MARI resolutions are very close (previous error in MAPS distances
        # meant that MARI was calculated to have a better resolution, but it *should* be MAPS)
        self.assertLess(res[0][0], res[1][0])
        self.assertLess(res[1][0], res[2][0])
        # Now tests the standalone function
        for inc, instname in enumerate(instnames):
            rr, ff = PyChop2.calculate(instname, 's', 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    # Tests the different variants of LET
    def test_pychop_let(self):
        variants = ['High flux', 'Intermediate', 'High resolution']
        res = []
        flux = []
        for inc, variant in enumerate(variants):
            chopobj = PyChop2('LET', variant)
            # Checks that it instantiates the correct variant
            self.assertTrue(variant in chopobj.getChopper())
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            chopobj.setFrequency(200)
            chopobj.setEi(18)
            rr, ff = chopobj.getResFlux(np.linspace(0, 17, 10))
            res.append(rr)
            flux.append(ff)
        # Checks that the flux should be highest for 'High flux', then 'Intermediate', 'High resolution'
        self.assertGreater(flux[0], flux[1])
        self.assertGreaterEqual(flux[1], flux[2])
        # Checks that the resolution should be best for 'High resolution', then 'Intermediate', 'High flux'
        self.assertLessEqual(res[2][0], res[1][0])
        self.assertLessEqual(res[1][0], res[0][0])
        # Now tests the standalone function
        for inc, variant in enumerate(variants):
            rr, ff = PyChop2.calculate('LET', variant, 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    def test_pychop_invalid_ei(self):
        chopobj = PyChop2('MARI', 'G', 400.)
        chopobj.setEi(120)
        with warnings.catch_warnings(record=True) as w:
            res = chopobj.getResolution(130.)
            assert len(w) == 1
            assert issubclass(w[0].category, UserWarning)
            assert "Cannot calculate for energy transfer greater than Ei" in str(w[0].message)
            assert np.isnan(res[0])


if __name__ == "__main__":
    unittest.main()
