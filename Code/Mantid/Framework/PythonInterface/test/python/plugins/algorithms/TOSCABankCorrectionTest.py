import unittest
from mantid.simpleapi import *
from mantid.api import *


class TOSCABankCorrectionTest(unittest.TestCase):

    def setUp(self):
        """
        Loads sample workspace.
        """

        self._original = '__TOSCABankCorrectionTest_original'

        Load(Filename='TSC14007_graphite002_red.nxs',
             OutputWorkspace=self._original)


    def tearDown(self):
        """
        Removes workspaces.
        """

        DeleteWorkspace(self._original)


    def test_automatic_peak_selection(self):
        """
        Tests automatically finding a peak in the default range.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = \
          TOSCABankCorrection(InputWorkspace=self._original)

        self.assertAlmostEqual(peak_position, 1077.47222328)
        self.assertAlmostEqual(scale_factor_1, 1.0059271)
        self.assertAlmostEqual(scale_factor_2, 0.9941423)


    def test_automatic_peak_in_range(self):
        """
        Tests automatically finding a peak in a given range.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = \
          TOSCABankCorrection(InputWorkspace=self._original,
                              SearchRange=[200, 800])

        self.assertAlmostEqual(peak_position, 713.20080359)
        self.assertAlmostEqual(scale_factor_1, 1.006076146)
        self.assertAlmostEqual(scale_factor_2, 0.993996806)


    def test_manual_peak_selection(self):
        """
        Tests using a peak provided by the user.
        """

        corrected_reduction, peak_position, scale_factor_1, scale_factor_2 = \
          TOSCABankCorrection(InputWorkspace=self._original,
                              PeakPosition='715')

        self.assertAlmostEqual(peak_position, 713.4430671)
        self.assertAlmostEqual(scale_factor_1, 1.00611439)
        self.assertAlmostEqual(scale_factor_2, 0.99395947)


    def test_manual_peak_not_found(self):
        """
        Tests error handling when a peak cannot be found using a manual peak position.
        """

        self.assertRaises(RuntimeError,
                          TOSCABankCorrection,
                          InputWorkspace=self._original,
                          OutputWorkspace='__TOSCABankCorrectionTest_output',
                          PeakPosition='900')


    def test_validation_search_range_order(self):
        """
        Tests validation to ensure low and high values are entered in correct order.
        """

        self.assertRaises(RuntimeError,
                          TOSCABankCorrection,
                          InputWorkspace=self._original,
                          OutputWorkspace='__TOSCABankCorrectionTest_output',
                          SearchRange=[500, 50])


    def test_validation_search_range_count(self):
        """
        Tests validation to ensure two values exist values are entered in correct order.
        """

        self.assertRaises(RuntimeError,
                          TOSCABankCorrection,
                          InputWorkspace=self._original,
                          OutputWorkspace='__TOSCABankCorrectionTest_output',
                          SearchRange=[500])


    def test_validation_peak_position_in_search_range(self):
        """
        Tests validation to ensure that the PeakPosition falls inside SearchRange.
        """

        self.assertRaises(RuntimeError,
                          TOSCABankCorrection,
                          InputWorkspace=self._original,
                          OutputWorkspace='__TOSCABankCorrectionTest_output',
                          SearchRange=[200, 2000],
                          PeakPosition='2500')


if __name__ == '__main__':
    unittest.main()
