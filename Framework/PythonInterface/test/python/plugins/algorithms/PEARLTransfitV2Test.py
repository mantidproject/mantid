# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from plugins.algorithms.PEARLTransfit2 import PEARLTransfit as _PEARLTransfit
from mantid.simpleapi import PEARLTransfit
from mantid.api import AnalysisDataService as ADS
import numpy as np


class PEARLTransfitV2Test(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def test_calibration_run_single_run(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=1.02511, peak_cen=1096.79, peak_cen_err=0.20729)
        # assert number energy bins
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 509)

    def test_custom_output_name_and_history(self):
        PEARLTransfit(Files="PEARL00112777", Calibration=True, Output="Test")
        self._assert_output_workspaces_exist("Test")
        self.assertFalse(ADS.retrieve("Test_Workspace").getHistory().empty())

    def test_lhs_output(self):
        out = PEARLTransfit(Files="PEARL00112777", Calibration=True, Output="Test")
        self.assertEqual(out[0].name(), "Test_Workspace")
        self.assertEqual(out[1].name(), "Test_Parameters")

    def test_validation_error_for_wrong_table_format_in_calibration(self):
        params_dict = {"Bg0": 0.0, "Bg1": -1.0, "Bg3": 1.0}
        table = _PEARLTransfit.generate_table(params_dict)
        err_msg = "Parameter Bg2 missing from FitParametersTable."
        with self.assertRaisesRegex(RuntimeError, err_msg):
            PEARLTransfit(Files="PEARL00112777", Calibration=True, FitParametersTable=table)

    def test_validation_error_if_missing_params_table_in_non_calibration_call(self):
        err_msg = "FitParametersTable is missing."
        with self.assertRaisesRegex(RuntimeError, err_msg):
            PEARLTransfit(Files="PEARL00112777", Calibration=False)

    def test_calibration_run_single_run_background_table_provided(self):
        params_dict = {"Bg0": 0.0, "Bg1": -1.0, "Bg2": 1.0}
        table = _PEARLTransfit.generate_table(params_dict)
        PEARLTransfit(Files="PEARL00112777", Calibration=True, FitParametersTable=table)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=1.02511, peak_cen=1096.79, peak_cen_err=0.20729)

    def test_calibration_run_multi_run(self):
        # Test that the calibration run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")

    def test_measure_run_single_run(self):
        # Test that the measurement run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True)
        PEARLTransfit(Files="PEARL00112777", FitParametersTable=ADS.retrieve("S_fit_Parameters"), Calibration=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_output_workspaces_exist("T_fit")

    def test_measure_run_multi_run(self):
        # Test that the measurement run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        PEARLTransfit(Files="PEARL00073987-00073990", FitParametersTable=ADS.retrieve("S_fit_Parameters"), Calibration=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_output_workspaces_exist("T_fit")

    def test_calibration_run_single_run_with_nexus_file(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777.nxs", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")

    def test_calibration_run_non_default_foil_no_rebin(self):
        PEARLTransfit(Files="PEARL00112777", Calibration=True, FoilType="Hf02")
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=0.89391, peak_cen=2380.03, peak_cen_err=0.31927)
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 96)

    def test_debug_info_output_as_a_table(self):
        debug_params = {
            "Debye Temp. (K)": 252.0,
            "Eff. Temp. (K)": 290.3199,
            "Fit. Min. (meV)": 600.1015,
            "Fit. Max. (meV)": 1700.2284,
            "Gaussian Width at Reference Temp (meV)": 24.7618,
            "Instrumental contribution (meV)": 0.04525,
            "Temperature Contribution (meV)": 24.76185,
            "Sample Temperature (K)": 279.7596,
        }

        out_calib = PEARLTransfit(Files="PEARL00112777", Calibration=True)
        PEARLTransfit(Files="PEARL00112777", Calibration=False, FitParametersTable=out_calib[1], CreateDebugTable=True)

        self._assert_output_workspaces_exist("S_fit")
        self._assert_output_workspaces_exist("T_fit")
        self.assertTrue(ADS.doesExist("T_fit_DebugParameters"))

        table = ADS.retrieve("T_fit_DebugParameters")
        self.assertEqual(table.column("Name"), list(debug_params.keys()))
        np.testing.assert_almost_equal(table.column("Value"), list(debug_params.values()), 4)

    def _assert_fit_parameters(self, ws_prefix, cost, peak_cen, peak_cen_err, frac_delta=1e-4):
        param_table = ADS.retrieve(f"{ws_prefix}_Parameters")
        self.assertAlmostEqual(param_table.column("Value")[-1], cost, delta=frac_delta * cost)
        self.assertAlmostEqual(param_table.column("Value")[0], peak_cen, delta=frac_delta * peak_cen)
        self.assertAlmostEqual(param_table.column("Error")[0], peak_cen_err, delta=frac_delta * peak_cen)

    def _assert_output_workspaces_exist(self, basename):
        for suffix in ["_Parameters", "_Workspace"]:
            self.assertTrue(ADS.doesExist(f"{basename}{suffix}"))


if __name__ == "__main__":
    unittest.main()
