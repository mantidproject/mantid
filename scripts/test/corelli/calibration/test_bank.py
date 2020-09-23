# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from numpy.testing import assert_allclose, assert_equal
import unittest

# Mantid imports
from corelli.calibration.bank import (wire_positions, fit_bank, criterium_peak_pixel_position, sufficient_intensity)
from mantid import AnalysisDataService, config
from mantid.simpleapi import DeleteWorkspaces, LoadNexusProcessed


class TestBank(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        r"""
        Load the tests cases, consisting of data for only one bank
        CORELLI_123455_bank20, control bank, it has no problems
        CORELLI_123454_bank58, beam center intensity spills over adjacent tubes, tube15 and tube16
        CORELLI_124018_bank45, tube11 is not working at all
        CORELLI_123555_bank20, insufficient intensity for all tubes in the bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        CORELLI_124023_bank14, wire shadows very faint, only slightly larger than fluctuations of the background
        CORELLI_124023_bank15, one spurious shadow in tube14
        """
        config.appendDataSearchSubDir('CORELLI/calibration')
        cls.cases = dict()
        for bank_case in ('123454_bank58', '124018_bank45', '123555_bank20', '123455_bank20',
                          '124023_bank10', '124023_bank14', '124023_bank15'):
            workspace = 'CORELLI_' + bank_case
            LoadNexusProcessed(Filename=workspace + '.nxs', OutputWorkspace=workspace)
            cls.cases[bank_case] = workspace

    @classmethod
    def tearDown(cls) -> None:
        r"""Delete the workspaces associated to the test cases"""
        DeleteWorkspaces(list(cls.cases.values()))

    def test_wire_positions(self):
        with self.assertRaises(AssertionError) as exception_info:
            wire_positions(units='mm')
        assert 'units mm must be one of' in str(exception_info.exception)
        expected = [-0.396, -0.343, -0.290, -0.238, -0.185, -0.132, -0.079, -0.026,
                    0.026, 0.079, 0.132, 0.185, 0.238, 0.290, 0.343, 0.396]
        assert_allclose(wire_positions(units='meters'), np.array(expected), atol=0.001)
        expected = [15.4, 30.4, 45.4, 60.4, 75.4, 90.5, 105.5, 120.5,
                    135.5, 150.5, 165.5, 180.6, 195.6, 210.6, 225.6, 240.6]
        assert_allclose(wire_positions(units='pixels'), np.array(expected), atol=0.1)

    def test_sufficient_intensity(self):
        assert sufficient_intensity(self.cases['123555_bank20'], 'bank20') is False
        assert sufficient_intensity(self.cases['123455_bank20'], 'bank20')

    def test_fit_bank(self):
        with self.assertRaises(AssertionError) as exception_info:
            fit_bank('I_am_not_here', 'bank42')
        assert 'Input workspace I_am_not_here does not exists' in str(exception_info.exception)

        control = self.cases['123455_bank20']  # a bank with a reasonable wire scan
        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(control, 'bank20', shadow_height=-1000)
        assert 'shadow height must be positive' in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(control, 'tree/bank51')
        assert 'The bank name must be of the form' in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(self.cases['123555_bank20'], 'bank20')
        assert 'Insufficient counts per pixel in workspace CORELLI_123555_bank20' in str(exception_info.exception)

        fit_bank(control, 'bank20')
        # Expect default name for calibration table
        assert AnalysisDataService.doesExist('CalibTable')
        # Expect default name for peak pixel position table
        assert AnalysisDataService.doesExist('PeakTable')
        DeleteWorkspaces(['CalibTable', 'PeakTable'])

        fit_bank(control, 'bank20', calibration_table='table_20', peak_pixel_positions_table='pixel_20')
        assert AnalysisDataService.doesExist('table_20')
        assert AnalysisDataService.doesExist('pixel_20')

    def test_criterium_peak_pixel_position(self):

        # control bank, it has no problems
        fit_bank(self.cases['123455_bank20'], 'bank20')
        expected = np.ones(16, dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # beam center intensity spills over adjacent tubes, tube15 and tube16
        fit_bank(self.cases['123454_bank58'], 'bank58')
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # tube11 is not working at all
        fit_bank(self.cases['124018_bank45'], 'bank45')
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1], dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # tube 13 has shadows at pixel numbers quite different from the rest
        fit_bank(self.cases['124023_bank10'], 'bank10')
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1], dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # tubes 3, 8, and 13 have very faint wire shadows
        fit_bank(self.cases['124023_bank14'], 'bank14')
        expected = np.array([1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1], dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # one spurious shadow in tube14, not enought to flag a discrepancy
        fit_bank(self.cases['124023_bank15'], 'bank15')
        expected = np.ones(16, dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)


if __name__ == "__main__":
    unittest.main()
