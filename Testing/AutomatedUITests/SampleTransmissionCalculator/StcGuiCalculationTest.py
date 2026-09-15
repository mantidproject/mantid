# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Sample Transmission Calculator's calculations.

Covers the "Basic Usage" and "Multiple atoms in chemical formula" sections of
``dev-docs/source/Testing/General/SampleTransmissionCalculatorTestGuide.rst``.

The guide's step 3 asks the tester to compare the window against a screenshot. A screenshot cannot
be asserted on, so it is replaced by the numbers the rest of the guide quotes in prose - the
transmission at the lowest wavelength, and the shape of the curve. That is a weaker check than the
picture a human looks at, and is recorded as such in ``FUTURE_WORK.md``.

The tolerances are wide on purpose. The guide says "~0.65", "~0.9", "~1", and pinning these to more
digits than it states would be inventing a regression check that the manual test never made - it
would fail on a legitimate change to a scattering cross-section rather than on a bug in the
interface.
"""

import unittest

from sample_transmission_gui_test_base import (
    GUIDE_HIGH,
    GUIDE_LOW,
    GUIDE_WIDTH,
    MASS_DENSITY,
    NUMBER_DENSITY,
    OUTPUT_WORKSPACE,
    SampleTransmissionGuiTestBase,
)

# "transmission ~0.65 at the lowest wavelength" and friends. A tenth either way: enough to tell 0.65
# from 0.9 from 1.0, which is the only distinction any of the guide's steps rests on.
TOLERANCE = 0.1


class StcGuiBasicUsageTest(SampleTransmissionGuiTestBase):
    """Guide section 'Basic Usage': a vanadium sample by mass density and then by number density."""

    def test_basic_usage(self):
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)
        self.set_sample(chemical_formula="V", density=6.0, thickness=0.5)
        self.calculate()

        self.assertEqual("", self.validation_text(), "the guide's own inputs were rejected")

        with self.subTest("Basic Usage / step 3 (the calculation produces a transmission curve)"):
            wavelengths, transmission = self.transmission()
            self.assertEqual(len(wavelengths), len(transmission))
            self.assertTrue(len(transmission) > 0, "no transmission was plotted")

        with self.subTest("Basic Usage / step 3 (transmission is ~0.65 at the lowest wavelength)"):
            _wavelengths, transmission = self.transmission()
            self.assertAlmostEqual(0.65, transmission[0], delta=TOLERANCE)

        with self.subTest("Basic Usage / step 3 (transmission falls with wavelength for vanadium)"):
            # the shape the screenshot shows: absorption grows with wavelength, so the curve
            # decreases monotonically across the range
            _wavelengths, transmission = self.transmission()
            self.assertTrue(
                all(later <= earlier + 1e-9 for earlier, later in zip(transmission, transmission[1:])),
                f"the transmission curve is not decreasing: {list(transmission)}",
            )

        with self.subTest("Basic Usage / step 3 (the results table reports the statistics)"):
            results = self.results_table()
            self.assertIn("Scattering", results)
            for statistic in ("Min", "Max", "Mean", "Median", "Std. Dev."):
                self.assertIn(statistic, results)

        with self.subTest("Basic Usage / step 3 (the wavelength range asked for is the range used)"):
            wavelengths, _transmission = self.transmission()
            self.assertGreaterEqual(wavelengths[0], GUIDE_LOW)
            self.assertLessEqual(wavelengths[-1], GUIDE_HIGH)

        self._check_number_density_agrees()

    def _check_number_density_agrees(self):
        """Guide step 4: number density 0.072 must reproduce the mass density 6 result."""
        _wavelengths, by_mass = self.transmission()

        self.set_sample(chemical_formula="V", density=0.072, thickness=0.5, density_type=NUMBER_DENSITY)
        self.calculate()

        with self.subTest("Basic Usage / step 4 (number density 0.072 gives a near identical result)"):
            _wavelengths, by_number = self.transmission()
            self.assertEqual(len(by_mass), len(by_number))
            for expected, actual in zip(by_mass, by_number):
                self.assertAlmostEqual(expected, actual, delta=TOLERANCE)

        with self.subTest("Basic Usage / step 4 (transmission is still ~0.65 at the lowest wavelength)"):
            _wavelengths, by_number = self.transmission()
            self.assertAlmostEqual(0.65, by_number[0], delta=TOLERANCE)


class StcGuiMultipleAtomFormulaTest(SampleTransmissionGuiTestBase):
    """Guide section 'Multiple atoms in chemical formula'.

    Every step here is a statement about the *physics* the interface is passing through to
    ``CalculateSampleTransmission`` - doubling the formula unit must not change the answer, and
    deuterating it must raise the transmission to ~1 - so they are checked as relations between two
    calculations rather than against absolute numbers.
    """

    THICKNESS = 0.015

    def setUp(self):
        super(StcGuiMultipleAtomFormulaTest, self).setUp()
        self.set_single_range(GUIDE_LOW, GUIDE_WIDTH, GUIDE_HIGH)

    def test_multiple_atoms_in_chemical_formula(self):
        self.set_sample(chemical_formula="C2 H4", density=0.93, thickness=self.THICKNESS)
        self.calculate()
        self.assertEqual("", self.validation_text(), "the guide's own inputs were rejected")
        _wavelengths, c2h4 = self.transmission()

        with self.subTest("Multiple atoms / step 2 (transmission is ~0.9 at every wavelength)"):
            for value in c2h4:
                self.assertAlmostEqual(0.9, value, delta=TOLERANCE)

        with self.subTest("Multiple atoms / step 2 (the curve is flat - absorption is not dominant)"):
            self.assertAlmostEqual(min(c2h4), max(c2h4), delta=TOLERANCE)

        self.set_sample(chemical_formula="C4 H8", density=0.93, thickness=self.THICKNESS)
        self.calculate()

        with self.subTest("Multiple atoms / step 3 (doubling the formula unit does not change the result)"):
            _wavelengths, c4h8 = self.transmission()
            for expected, actual in zip(c2h4, c4h8):
                self.assertAlmostEqual(expected, actual, places=6)

        self.set_sample(chemical_formula="C2 H4", density=0.12, thickness=self.THICKNESS, density_type=NUMBER_DENSITY)
        self.calculate()

        with self.subTest("Multiple atoms / step 4 (number density 0.12 gives ~0.9 at the lowest wavelength)"):
            _wavelengths, by_number = self.transmission()
            self.assertAlmostEqual(0.9, by_number[0], delta=TOLERANCE)

        self.set_sample(chemical_formula="C2 D4", density=0.93, thickness=self.THICKNESS, density_type=MASS_DENSITY)
        self.calculate()

        with self.subTest("Multiple atoms / step 5 (deuterating the sample gives a transmission of ~1)"):
            _wavelengths, deuterated = self.transmission()
            for value in deuterated:
                self.assertAlmostEqual(1.0, value, delta=TOLERANCE)

        with self.subTest("Multiple atoms / step 5 (deuterium transmits more than hydrogen)"):
            _wavelengths, deuterated = self.transmission()
            self.assertGreater(deuterated[0], c2h4[0])

        with self.subTest("Multiple atoms / the calculation leaves 'transmission_ws' in the ADS"):
            from mantid.api import AnalysisDataService as ADS

            self.assertTrue(ADS.doesExist(OUTPUT_WORKSPACE))


if __name__ == "__main__":
    unittest.main()
