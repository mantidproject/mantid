# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import CompareWorkspaces, DeleteWorkspaces, LoadNexusProcessed, UnGroupWorkspace
from Calibration.tofpd import diagnostics


class TestDiagnostics(unittest.TestCase):
    PEAK = 1.2615

    @classmethod
    def setUpClass(cls):
        LoadNexusProcessed(Filename="VULCAN_192227_diagnostics.nxs", OutputWorkspace="diagtest")
        UnGroupWorkspace("diagtest")
        cls.workspaces = [
            "diag_dspacing",
            "diag_fitted",
            "diag_fitparam",
            "strain",
            "single_strain",
            "difference",
            "single_diff",
            "center_tof",
        ]

    @classmethod
    def tearDownClass(cls) -> None:
        if len(cls.workspaces) > 0:
            DeleteWorkspaces(cls.workspaces)

    def test_collect_peaks_strain(self):
        test_strain = diagnostics.collect_peaks("diag_dspacing", "test_strain", infotype="strain")
        result = CompareWorkspaces(test_strain, "strain", Tolerance=1e-6)
        self.assertTrue(result)
        DeleteWorkspaces(test_strain, result)

    def test_collect_peaks_diff(self):
        test_diff = diagnostics.collect_peaks("diag_dspacing", "test_diff", infotype="difference")
        result = CompareWorkspaces(test_diff, "difference", Tolerance=1e-6)
        self.assertTrue(result)
        DeleteWorkspaces(test_diff, result)

    def test_extract_peak_info(self):
        test_single_strain = diagnostics.extract_peak_info("strain", "test_single_strain", self.PEAK)
        result = CompareWorkspaces(test_single_strain, "single_strain", Tolerance=1e-6)
        self.assertTrue(result)
        DeleteWorkspaces(test_single_strain, result)

        test_single_diff = diagnostics.extract_peak_info("difference", "test_single_diff", self.PEAK)
        result = CompareWorkspaces(test_single_diff, "single_diff", Tolerance=1e-6)
        self.assertTrue(result)
        DeleteWorkspaces(test_single_diff, result)

    def test_collect_fit_result(self):
        test_center = diagnostics.collect_fit_result("diag_fitparam", "test_center", [self.PEAK], infotype="centre")
        result = CompareWorkspaces(test_center, "center_tof", Tolerance=1e-6)
        self.assertTrue(result)
        DeleteWorkspaces(test_center, result)


if __name__ == "__main__":
    unittest.main()
