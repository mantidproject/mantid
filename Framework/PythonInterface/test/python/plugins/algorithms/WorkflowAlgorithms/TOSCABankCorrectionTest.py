# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import DeleteWorkspace, Load, TOSCABankCorrection


class TOSCABankCorrectionTest(unittest.TestCase):
    def setUp(self):
        """
        Loads sample workspace.
        """

        self._original = "__TOSCABankCorrectionTest_original"

        Load(Filename="TSC14007_graphite002_red.nxs", OutputWorkspace=self._original)

    def tearDown(self):
        """
        Removes workspaces.
        """

        DeleteWorkspace(self._original)

    def test_automatic_peak_selection(self):
        """
        Tests automatically finding a peak in the default range.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = TOSCABankCorrection(InputWorkspace=self._original)

        self.assertAlmostEqual(peak_position, 1079.84991188)
        self.assertAlmostEqual(scale_factor_1, 1.0060389)
        self.assertAlmostEqual(scale_factor_2, 0.9940331)

    def test_automatic_peak_in_range(self):
        """
        Tests automatically finding a peak in a given range.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = TOSCABankCorrection(
            InputWorkspace=self._original, SearchRange=[200, 800]
        )

        self.assertAlmostEqual(peak_position, 714.008962427)
        self.assertAlmostEqual(scale_factor_1, 1.004949468)
        self.assertAlmostEqual(scale_factor_2, 0.995099045)

    def test_manual_peak_selection(self):
        """
        Tests using a peak provided by the user.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = TOSCABankCorrection(
            InputWorkspace=self._original, PeakPosition="715"
        )

        self.assertAlmostEqual(peak_position, 714.29114157)
        self.assertAlmostEqual(scale_factor_1, 1.00491105)
        self.assertAlmostEqual(scale_factor_2, 0.99513671)

    def test_manual_peak_not_found(self):
        """
        Tests error handling when a peak cannot be found using a manual peak position.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Could not find any peaks. Try increasing width of SearchRange and/or ClosePeakTolerance",
            TOSCABankCorrection,
            InputWorkspace=self._original,
            OutputWorkspace="__TOSCABankCorrectionTest_output",
            PeakPosition="900",
        )

    def test_validation_search_range_order(self):
        """
        Tests validation to ensure low and high values are entered in correct order.
        """

        self.assertRaisesRegex(
            RuntimeError,
            'Search range must be in format "low,high"',
            TOSCABankCorrection,
            InputWorkspace=self._original,
            OutputWorkspace="__TOSCABankCorrectionTest_output",
            SearchRange=[500, 50],
        )

    def test_validation_search_range_count(self):
        """
        Tests validation to ensure two values exist values are entered in correct order.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Search range must have two values",
            TOSCABankCorrection,
            InputWorkspace=self._original,
            OutputWorkspace="__TOSCABankCorrectionTest_output",
            SearchRange=[500],
        )

    def test_validation_peak_position_in_search_range(self):
        """
        Tests validation to ensure that the PeakPosition falls inside SearchRange.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Peak position must be inside SearchRange",
            TOSCABankCorrection,
            InputWorkspace=self._original,
            OutputWorkspace="__TOSCABankCorrectionTest_output",
            SearchRange=[200, 2000],
            PeakPosition="2500",
        )


if __name__ == "__main__":
    unittest.main()
