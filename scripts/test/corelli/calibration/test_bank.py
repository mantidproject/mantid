# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from numpy.testing import assert_equal, assert_allclose
from os import path
import unittest

# Mantid imports
from mantid import AnalysisDataService, config, mtd
from mantid.simpleapi import DeleteWorkspaces, LoadNexusProcessed

from corelli.calibration.bank import (
    calibrate_bank,
    calibrate_banks,
    collect_bank_fit_results,
    criterion_peak_pixel_position,
    criterion_peak_vertical_position,
    fit_bank,
    mask_bank,
    purge_table,
    sufficient_intensity,
)
from corelli.calibration.utils import TUBES_IN_BANK


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
        CORELLI_124023_banks_10_15
        """
        config.appendDataSearchSubDir("CORELLI/calibration")
        for directory in config.getDataSearchDirs():
            if "UnitTest" in directory:
                data_dir = path.join(directory, "CORELLI", "calibration")
                break
        cls.cases = dict()
        for bank_case in (
            "123454_bank58",
            "124018_bank45",
            "123555_bank20",
            "123455_bank20",
            "124023_bank10",
            "124023_bank14",
            "124023_bank15",
            "124023_banks_10_15",
        ):
            workspace = "CORELLI_" + bank_case
            LoadNexusProcessed(Filename=path.join(data_dir, workspace + ".nxs"), OutputWorkspace=workspace)
            cls.cases[bank_case] = workspace

        def assert_missing_tube(cls_other, calibration_table, tube_number):
            r"""Check detector ID's from a failing tube are not in the calibration table"""
            table = mtd[str(calibration_table)]
            first = table.cell("Detector ID", 0)  # first detector ID
            # Would-be first and last detectors ID's for the failing tube
            begin, end = first + (tube_number - 1) * 256, first + tube_number * 256 - 1
            detectors_ids = table.column(0)
            assert begin not in detectors_ids
            assert end not in detectors_ids

        # sneak in a class method, make sure it's loaded before any tests is executed
        cls.assert_missing_tube = assert_missing_tube

    @classmethod
    def tearDownClass(cls) -> None:
        r"""Delete the workspaces associated to the test cases"""
        DeleteWorkspaces(list(cls.cases.values()))

    def test_sufficient_intensity(self):
        assert sufficient_intensity(self.cases["123555_bank20"], "bank20") is False
        assert sufficient_intensity(self.cases["123455_bank20"], "bank20")

    def test_fit_bank(self):
        control = self.cases["123455_bank20"]  # a bank with a reasonable wire scan

        with self.assertRaises(AssertionError) as exception_info:
            fit_bank("I_am_not_here", "bank42")
        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(control, "bank20", shadow_height=-1000)
        assert "shadow height must be positive" in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(control, "tree/bank51")
        assert "The bank name must be of the form" in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            fit_bank(self.cases["123555_bank20"], "bank20")
        assert "Insufficient counts per pixel in workspace CORELLI_123555_bank20" in str(exception_info.exception)

        fit_bank(control, "bank20", parameters_table_group="parameters_table")
        # Expect default name for calibration table
        assert AnalysisDataService.doesExist("CalibTable")
        # Expect default name for peak pixel position table
        assert AnalysisDataService.doesExist("PeakTable")
        # Expect default name for peak pixel position table
        assert AnalysisDataService.doesExist("PeakYTable")
        # assert the parameters tables are created
        assert AnalysisDataService.doesExist("parameters_table")
        for tube_number in range(TUBES_IN_BANK):
            assert AnalysisDataService.doesExist(f"parameters_table_{tube_number}")
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "parameters_table"])

        fit_bank(control, "bank20", calibration_table="table_20", peak_pixel_positions_table="pixel_20")
        assert AnalysisDataService.doesExist("table_20")
        assert AnalysisDataService.doesExist("pixel_20")
        DeleteWorkspaces(["table_20", "pixel_20", "PeakYTable", "ParametersTable"])  # a bit of clean-up

    def test_criterion_peak_vertical_position(self):
        # control bank, it has no problems
        fit_bank(self.cases["123455_bank20"], "bank20")
        expected = np.ones(16, dtype=bool)
        actual = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert_equal(actual, expected)
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable"])  # a bit of clean-up

        # beam center intensity spills over adjacent tubes, tube15 and tube16
        fit_bank(self.cases["123454_bank58"], "bank58")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=bool)
        actual = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert_equal(actual, expected)
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable"])  # a bit of clean-up

        # tube11 is not working at all
        fit_bank(self.cases["124018_bank45"], "bank45")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1], dtype=bool)
        actual = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert_equal(actual, expected)
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable"])  # a bit of clean-up

        # tube 13 has shadows at pixel numbers quite different from the rest, but similar vertical positions
        fit_bank(self.cases["124023_bank10"], "bank10")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], dtype=bool)
        actual = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert_equal(actual, expected)
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable"])  # a bit of clean-up

        # one spurious shadow in tube14 throws away the fit
        fit_bank(self.cases["124023_bank15"], "bank15")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1], dtype=bool)
        actual = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert_equal(actual, expected)
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable"])  # a bit of clean-up

        # check for the summary workspace
        fit_bank(self.cases["123455_bank20"], "bank20")
        criterion_peak_vertical_position("PeakYTable", summary="summary", zscore_threshold=2.5, deviation_threshold=0.0035)
        assert AnalysisDataService.doesExist("summary")
        workspace = mtd["summary"]
        axis = workspace.getAxis(1)
        assert [axis.label(workspace_index) for workspace_index in (0, 1, 2)] == ["success", "deviation", "Z-score"]
        self.assertEqual(min(workspace.readY(0)), 1.0)  # check success of first tube
        self.assertAlmostEqual(max(workspace.readY(2)), 2.73, delta=0.01)  # check maximum Z-score
        DeleteWorkspaces(["CalibTable", "ParametersTable", "PeakTable", "PeakYTable", "summary"])  # a bit of clean-up

    def test_criterion_peak_pixel_position(self):
        # control bank, it has no problems
        fit_bank(self.cases["123455_bank20"], "bank20")
        expected = np.ones(16, dtype=bool)
        assert_equal(criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3), expected)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # beam center intensity spills over adjacent tubes, tube15 and tube16
        fit_bank(self.cases["123454_bank58"], "bank58")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=bool)
        assert_equal(criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3), expected)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # tube11 is not working at all
        fit_bank(self.cases["124018_bank45"], "bank45")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1], dtype=bool)
        assert_equal(criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3), expected)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # tube 13 has shadows at pixel numbers quite different from the rest
        fit_bank(self.cases["124023_bank10"], "bank10")
        expected = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1], dtype=bool)
        assert_equal(criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3), expected)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # one spurious shadow in tube14, not enough to flag a discrepancy
        fit_bank(self.cases["124023_bank15"], "bank15")
        expected = np.ones(16, dtype=bool)
        assert_equal(criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3), expected)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # check for the summary workspace
        fit_bank(self.cases["123455_bank20"], "bank20")
        criterion_peak_pixel_position("PeakTable", summary="summary", zscore_threshold=2.5, deviation_threshold=3)
        assert AnalysisDataService.doesExist("summary")
        workspace = mtd["summary"]
        axis = workspace.getAxis(1)
        assert [axis.label(workspace_index) for workspace_index in (0, 1, 2)] == ["success", "deviation", "Z-score"]
        self.assertEqual(min(workspace.readY(0)), 1.0)
        self.assertAlmostEqual(max(workspace.readY(2)), 1.728, delta=0.001)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable", "summary"])  # a bit of clean-up

    def test_purge_table(self):
        r"""We use either of two criterion functions"""
        with self.assertRaises(AssertionError) as exception_info:
            purge_table("I am not here", "table", [True, False])
        assert "Input workspace I am not here does not exists" in str(exception_info.exception)

        with self.assertRaises(AssertionError) as exception_info:
            purge_table(self.cases["123455_bank20"], "I am not here", [True, False])
        assert "Input table I am not here does not exists" in str(exception_info.exception)

        # control bank, it has no problems. Thus, no tubes to purge
        fit_bank(self.cases["123455_bank20"], "bank20")
        tube_fit_success = criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3)
        unpurged_row_count = mtd["CalibTable"].rowCount()
        purge_table(self.cases["123455_bank20"], "CalibTable", tube_fit_success)
        assert mtd["CalibTable"].rowCount() == unpurged_row_count
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

        # tube11 is not working at all. Thus, purge only one tube
        fit_bank(self.cases["124018_bank45"], "bank45")
        tube_fit_success = criterion_peak_vertical_position("PeakYTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        unpurged_row_count = mtd["CalibTable"].rowCount()
        purge_table(self.cases["124018_bank45"], "CalibTable", tube_fit_success)
        assert mtd["CalibTable"].rowCount() == unpurged_row_count - 256
        self.assert_missing_tube("CalibTable", 11)
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])  # a bit of clean-up

    def test_mask_bank(self):
        # control bank, it has no problems. Thus, no mask to be created
        fit_bank(self.cases["123455_bank20"], "bank20")
        tube_fit_success = criterion_peak_pixel_position("PeakTable", zscore_threshold=2.5, deviation_threshold=3)
        assert mask_bank("bank20", tube_fit_success, "masked_tubes") is None
        DeleteWorkspaces(["CalibTable", "PeakTable", "PeakYTable", "ParametersTable"])

        # tube11 is not working at all. Thus, mask this tube
        fit_bank(self.cases["124018_bank45"], "bank45")
        tube_fit_success = criterion_peak_vertical_position("PeakTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        mask_bank("bank45", tube_fit_success, "masked_tubes")
        detector_ids = mtd["masked_tubes"].column(0)
        assert detector_ids[0], detector_ids[-1] == [182784, 182784 + 256]
        DeleteWorkspaces(["CalibTable", "masked_tubes", "PeakTable", "PeakYTable", "ParametersTable"])

        # tubes 3, 8, and 13 have very faint wire shadows. Thus, mask these tubes
        fit_bank(self.cases["124023_bank14"], "bank14")
        tube_fit_success = criterion_peak_vertical_position("PeakTable", zscore_threshold=2.5, deviation_threshold=0.0035)
        mask_bank("bank14", tube_fit_success, "masked_tubes")
        detector_ids = mtd["masked_tubes"].column(0)
        assert detector_ids[0], detector_ids[-1] == [182784, 182784 + 3 * 256]
        DeleteWorkspaces(["CalibTable", "masked_tubes", "PeakTable", "PeakYTable", "ParametersTable"])

    def test_collect_bank_fit_results(self):
        def spectra_labels(workspace_name):
            workspace = mtd[str(workspace_name)]
            axis = workspace.getAxis(1)
            return [axis.label(spectrum_index) for spectrum_index in range(workspace.getNumberHistograms())]

        fit_bank(self.cases["123455_bank20"], "bank20", parameters_table_group="parameters_tables")
        criterion_peak_vertical_position("PeakTable", summary="summary", zscore_threshold=2.5, deviation_threshold=0.0035)

        with self.assertRaises(AssertionError) as exception_info:
            collect_bank_fit_results("fit_results", acceptance_summary=None, parameters_table_group=None)
        assert "fit results should be different than None" in str(exception_info.exception)

        # collect only the acceptance criteria results
        collect_bank_fit_results("fit_results", acceptance_summary="summary", parameters_table_group=None)
        assert spectra_labels("fit_results") == ["success", "deviation", "Z-score"]
        assert_allclose(mtd["fit_results"].readY(0), mtd["summary"].readY(0))
        DeleteWorkspaces(["fit_results"])

        # collect only the polynomial coefficients
        collect_bank_fit_results("fit_results", acceptance_summary=None, parameters_table_group="parameters_tables")
        assert spectra_labels("fit_results") == ["A0", "A1", "A2"]
        for coefficient_idx in range(3):  # we have three polynomial coefficients
            for tube_idx in range(TUBES_IN_BANK):
                self.assertAlmostEqual(
                    mtd["fit_results"].readY(coefficient_idx)[tube_idx],
                    mtd[f"parameters_tables_{tube_idx}"].row(coefficient_idx)["Value"],
                    delta=1e-6,
                )
                self.assertAlmostEqual(
                    mtd["fit_results"].readE(coefficient_idx)[tube_idx],
                    mtd[f"parameters_tables_{tube_idx}"].row(coefficient_idx)["Error"],
                    delta=1e-6,
                )
        DeleteWorkspaces(["fit_results"])

        # collect both acceptance criteria and polynomial coefficients
        collect_bank_fit_results("fit_results", acceptance_summary="summary", parameters_table_group="parameters_tables")
        assert spectra_labels("fit_results") == ["success", "deviation", "Z-score", "A0", "A1", "A2"]
        for spectrum_idx in [0, 1, 2]:
            assert_allclose(mtd["fit_results"].readY(spectrum_idx), mtd["summary"].readY(spectrum_idx))
            assert_allclose(mtd["fit_results"].readE(spectrum_idx), mtd["summary"].readE(spectrum_idx))
        for coefficient_idx, spectrum_idx in [(0, 3), (1, 4), (2, 5)]:  # we have three polynomial coefficients
            for tube_idx in range(TUBES_IN_BANK):
                self.assertAlmostEqual(
                    mtd["fit_results"].readY(spectrum_idx)[tube_idx],
                    mtd[f"parameters_tables_{tube_idx}"].row(coefficient_idx)["Value"],
                    delta=1e-6,
                )
                self.assertAlmostEqual(
                    mtd["fit_results"].readE(spectrum_idx)[tube_idx],
                    mtd[f"parameters_tables_{tube_idx}"].row(coefficient_idx)["Error"],
                    delta=1e-6,
                )
        DeleteWorkspaces(["CalibTable", "fit_results", "parameters_tables", "PeakTable", "PeakYTable", "summary"])

    def test_calibrate_bank(self):
        # control bank, it has no problems. Thus, no mask to be created
        calibration, mask = calibrate_bank(self.cases["123455_bank20"], "bank20", "calibration_table")
        assert calibration.rowCount() == 256 * 16
        assert calibration.columnCount() == 2
        assert AnalysisDataService.doesExist("calibration_table")
        assert mask is None
        assert AnalysisDataService.doesExist("MaskTable") is False
        DeleteWorkspaces(["calibration_table"])  # clean-up

        # beam center intensity spills over adjacent tubes, tube15 and tube16
        calibration, mask = calibrate_bank(self.cases["123454_bank58"], "bank58", "calibration_table")
        assert calibration.rowCount() == 256 * (16 - 2)
        assert calibration.columnCount() == 2  # Detector ID, Position
        assert AnalysisDataService.doesExist("calibration_table")
        assert mask.rowCount() == 256 * 2
        assert mask.columnCount() == 1
        assert AnalysisDataService.doesExist("MaskTable")
        DeleteWorkspaces(["calibration_table", "MaskTable"])  # clean-up

        # check for the fits workspace
        calibrate_bank(self.cases["123455_bank20"], "bank20", "calibration_table", fit_results="fits")
        assert AnalysisDataService.doesExist("fits")
        workspace = mtd["fits"]
        axis = workspace.getAxis(1)
        labels = [axis.label(i) for i in range(workspace.getNumberHistograms())]
        assert labels == ["success", "deviation", "Z-score", "A0", "A1", "A2"]
        assert_allclose(workspace.readY(0), [1.0] * TUBES_IN_BANK)  # success status for first tube
        self.assertAlmostEqual(max(workspace.readY(2)), 2.73, delta=0.01)  # maximum Z-score
        self.assertAlmostEqual(max(workspace.readY(3)), -0.445, delta=0.001)  # maximum A0 value
        self.assertAlmostEqual(max(workspace.readE(3)), 1.251, delta=0.001)  # maximum A0 error
        DeleteWorkspaces(["calibration_table", "fits"])  # a bit of clean-up

    def test_calibrate_banks(self):
        calibrations, masks = calibrate_banks(self.cases["124023_banks_10_15"], "10,15")
        assert list(calibrations.getNames()) == ["calib10", "calib15"]
        assert list(masks.getNames()) == ["mask15"]
        assert mtd["calib10"].rowCount() == 256 * 16
        assert mtd["calib15"].rowCount() == 256 * (16 - 1)  # one uncalibrated tubes
        assert mtd["mask15"].rowCount() == 256
        # Check for success status
        self.assertEqual(mtd["fit10"].readY(0).tolist(), [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1])
        self.assertEqual(mtd["fit15"].readY(0).tolist(), [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1])
        # Check for A1 coefficient values
        self.assertAlmostEqual(max(mtd["fit10"].readY(4)), 0.0038, delta=0.0001)
        self.assertAlmostEqual(max(mtd["fit15"].readY(4)), 0.0037, delta=0.0001)
        # Check for A2 coefficient errors
        self.assertAlmostEqual(max(mtd["fit10"].readE(4)), 0.0219, delta=0.0001)
        self.assertAlmostEqual(max(mtd["fit15"].readE(4)), 0.0221, delta=0.0001)
        DeleteWorkspaces(["calibrations", "masks", "fits"])


if __name__ == "__main__":
    unittest.main()
