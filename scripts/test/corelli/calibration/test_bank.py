# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from numpy.testing import assert_equal
from os import path
import unittest

# Mantid imports
from corelli.calibration.bank import (calibrate_bank, calibrate_banks, criterium_peak_pixel_position, fit_bank,
                                      mask_bank, purge_table, sufficient_intensity)
from mantid import AnalysisDataService, config, mtd
from mantid.simpleapi import DeleteWorkspaces, LoadNexusProcessed


class TestBank(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        r"""
        Load the tests cases for calibrate_bank, consisting of data for only one bank
        CORELLI_123455_bank20, control bank, it has no problems
        CORELLI_123454_bank58, beam center intensity spills over adjacent tubes, tube15 and tube16
        CORELLI_124018_bank45, tube11 is not working at all
        CORELLI_123555_bank20, insufficient intensity for all tubes in the bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        CORELLI_124023_bank14, wire shadows very faint, only slightly larger than fluctuations of the background
        CORELLI_124023_bank15, one spurious shadow in tube14
        Load the test case for calibrate_banks, consisting of data for two banks
        CORELLI_124023_banks_14_15
        """
        config.appendDataSearchSubDir('CORELLI/calibration')
        for directory in config.getDataSearchDirs():
            if 'UnitTest' in directory:
                data_dir = path.join(directory, 'CORELLI', 'calibration')
                break
        cls.cases = dict()
        for bank_case in ('123454_bank58', '124018_bank45', '123555_bank20', '123455_bank20',
                          '124023_bank10', '124023_bank14', '124023_bank15', '124023_banks_14_15'):
            workspace = 'CORELLI_' + bank_case
            # DEBUG
            #data_dir = '/home/jbq/repositories/mantidproject/mantid2/Testing/Data/UnitTest/CORELLI/calibration/'
            LoadNexusProcessed(Filename=path.join(data_dir, workspace + '.nxs'), OutputWorkspace=workspace)
            cls.cases[bank_case] = workspace

        def assert_missing_tube(cls_other, calibration_table, tube_number):
            r"""Check detector ID's from a failing tube are not in the calibration table"""
            table = mtd[str(calibration_table)]
            first = table.cell('Detector ID', 0)  # first detector ID
            # Would-be first and last detectors ID's for the failing tube
            begin, end = first + (tube_number - 1) * 256, first + tube_number * 256 - 1
            detectors_ids = table.column(0)
            assert begin not in detectors_ids
            assert end not in detectors_ids
        # sneak in a class method, make sure it's loaded before any tests is executed
        cls.assert_missing_tube = assert_missing_tube

    @classmethod
    def tearDown(cls) -> None:
        r"""Delete the workspaces associated to the test cases"""
        DeleteWorkspaces(list(cls.cases.values()))

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

        DeleteWorkspaces(['CalibTable', 'PeakTable'])  # a bit of clean-up

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

        # one spurious shadow in tube14, not enough to flag a discrepancy
        fit_bank(self.cases['124023_bank15'], 'bank15')
        expected = np.ones(16, dtype=bool)
        assert_equal(criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3), expected)

        # check for the summary workspace
        fit_bank(self.cases['123455_bank20'], 'bank20')
        criterium_peak_pixel_position('PeakTable', summary='summary', zscore_threshold=2.5, deviation_threshold=3)
        assert AnalysisDataService.doesExist('summary')
        workspace = mtd['summary']
        axis = workspace.getAxis(1)
        assert [axis.label(workspace_index) for workspace_index in (0, 1, 2)] == ['success', 'deviation', 'Z-score']
        self.assertEqual(min(workspace.readY(0)), 1.0)
        self.assertAlmostEqual(max(workspace.readY(2)), 1.728, delta=0.001)

        DeleteWorkspaces(['CalibTable', 'PeakTable', 'summary'])  # a bit of clean-up

    def test_purge_table(self):
        with self.assertRaises(AssertionError) as exception_info:
            purge_table('I am not here', 'table', [True, False])
        assert 'Input workspace I am not here does not exists' in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            purge_table(self.cases['123455_bank20'], 'I am not here', [True, False])
        assert 'Input table I am not here does not exists' in str(exception_info.exception)

        # control bank, it has no problems. Thus, no tubes to purge
        fit_bank(self.cases['123455_bank20'], 'bank20')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        unpurged_row_count = mtd['CalibTable'].rowCount()
        purge_table(self.cases['123455_bank20'], 'CalibTable', tube_fit_success)
        assert mtd['CalibTable'].rowCount() == unpurged_row_count

        # tube11 is not working at all. Thus, purge only one tube
        fit_bank(self.cases['124018_bank45'], 'bank45')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        unpurged_row_count = mtd['CalibTable'].rowCount()
        purge_table(self.cases['124018_bank45'], 'CalibTable', tube_fit_success)
        assert mtd['CalibTable'].rowCount() == unpurged_row_count - 256
        self.assert_missing_tube('CalibTable', 11)

        # tubes 3, 8, and 13 have very faint wire shadows. Thus, purge three tubes
        fit_bank(self.cases['124023_bank14'], 'bank14')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        unpurged_row_count = mtd['CalibTable'].rowCount()
        purge_table(self.cases['124023_bank14'], 'CalibTable', tube_fit_success)
        assert mtd['CalibTable'].rowCount() == unpurged_row_count - 256 * 3
        for tube_number in (3, 8, 13):
            self.assert_missing_tube('CalibTable', tube_number)

        DeleteWorkspaces(['CalibTable', 'PeakTable'])  # a bit of clean-up

    def test_mask_bank(self):
        # control bank, it has no problems. Thus, no mask to be created
        fit_bank(self.cases['123455_bank20'], 'bank20')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        assert mask_bank('bank20', tube_fit_success, 'masked_tubes') is None

        # tube11 is not working at all. Thus, mask this tube
        fit_bank(self.cases['124018_bank45'], 'bank45')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        mask_bank('bank45', tube_fit_success, 'masked_tubes')
        detector_ids = mtd['masked_tubes'].column(0)
        assert detector_ids[0], detector_ids[-1] == [182784, 182784 + 256]

        # tubes 3, 8, and 13 have very faint wire shadows. Thus, mask these tubes
        fit_bank(self.cases['124023_bank14'], 'bank14')
        tube_fit_success = criterium_peak_pixel_position('PeakTable', zscore_threshold=2.5, deviation_threshold=3)
        mask_bank('bank14', tube_fit_success, 'masked_tubes')
        detector_ids = mtd['masked_tubes'].column(0)
        assert detector_ids[0], detector_ids[-1] == [182784, 182784 + 3 * 256]

        DeleteWorkspaces(['CalibTable', 'PeakTable', 'masked_tubes'])  # a bit of clean-up

    def test_calibrate_bank(self):

        # control bank, it has no problems. Thus, no mask to be created
        calibration, mask = calibrate_bank(self.cases['123455_bank20'], 'bank20', 'calibration_table')
        assert calibration.rowCount() == 256 * 16
        assert calibration.columnCount() == 3
        assert AnalysisDataService.doesExist('calibration_table')
        assert mask is None
        assert AnalysisDataService.doesExist('MaskTable') is False

        # tubes 3, 8, and 13 have very faint wire shadows. Thus, mask these tubes
        calibration, mask = calibrate_bank(self.cases['124023_bank14'], 'bank14',
                                           calibration_table='calibration_table')
        assert calibration.rowCount() == 256 * (16 - 3)
        assert calibration.columnCount() == 2  # Detector ID, Position
        assert AnalysisDataService.doesExist('calibration_table')
        assert mask.rowCount() == 256 * 3
        assert mask.columnCount() == 1
        assert AnalysisDataService.doesExist('MaskTable')

        # check for the summary workspace
        calibrate_bank(self.cases['123455_bank20'], 'bank20', 'calibration_table',
                       criterium_kwargs=dict(summary='summary'))
        assert AnalysisDataService.doesExist('summary')
        workspace = mtd['summary']
        axis = workspace.getAxis(1)
        assert [axis.label(workspace_index) for workspace_index in (0, 1, 2)] == ['success', 'deviation', 'Z-score']
        self.assertEqual(min(workspace.readY(0)), 1.0)
        self.assertAlmostEqual(max(workspace.readY(2)), 1.728, delta=0.001)

        DeleteWorkspaces(['calibration_table', 'MaskTable', 'summary'])  # a bit of clean-up

    def test_calibrate_banks(self):
        calibrations, masks = calibrate_banks(self.cases['124023_banks_14_15'], '14-15')
        assert list(calibrations.getNames()) == ['calib14', 'calib15']
        assert list(masks.getNames()) == ['mask14']
        assert mtd['calib14'].rowCount() == 256 * (16 - 3)
        assert mtd['mask14'].rowCount() == 256 * 3
        assert mtd['calib15'].rowCount() == 256 * 16
        self.assertEqual(mtd['acceptance14'].readY(0).tolist(), [1, 1, 0., 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1])
        self.assertEqual(mtd['acceptance15'].readY(0).tolist(), [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1])

        DeleteWorkspaces(['calibrations', 'masks', 'acceptances'])


if __name__ == "__main__":
    unittest.main()
