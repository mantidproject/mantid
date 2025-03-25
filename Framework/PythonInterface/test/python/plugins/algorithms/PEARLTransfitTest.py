# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import mtd, PEARLTransfit
from mantid.api import AnalysisDataService as ADS


class PEARLTransfitTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_calibration_run_single_run(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=0.72751, peak_cen=1096.79, peak_cen_err=0.20729)
        # assert number energy bins
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 440)

    def test_calibration_run_single_run_no_rebin(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True, RebinInEnergy=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=1.02511, peak_cen=1096.79, peak_cen_err=0.20719)
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 509)

    def test_calibration_run_single_run_backgorund_params_provided(self):
        # Provide very bad background params and show fit doesn't converge (would do if parameters estimated)
        PEARLTransfit(Files="PEARL00112777", Calibration=True, EstimateBackground=False, Bg0guessFraction=0.0, Bg1guess=-1.0, Bg2guess=1.0)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=0.72751, peak_cen=1096.79, peak_cen_err=0.20729)

    def test_calibration_run_multi_run(self):
        # Test that the calibration run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")

    def test_measure_run_single_run(self):
        # Test that the measurement run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True)
        PEARLTransfit(Files="PEARL00112777", Calibration=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_output_workspaces_exist("T_fit")

    def test_measure_run_multi_run(self):
        # Test that the measurement run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_output_workspaces_exist("T_fit")

    def test_algorithm_cancels_if_no_calib(self):
        # Test that the algorithm is aborted if run in measurement mode without a calibration file
        PEARLTransfit(Files="PEARL00073987", Calibration=False)
        self.assertFalse(ADS.doesExist("T_fit_Parameters"))
        self.assertFalse(ADS.doesExist("T_fit_Workspace"))
        self.assertFalse(ADS.doesExist("S_fit_Parameters"))
        self.assertFalse(ADS.doesExist("S_fit_Workspace"))

    def test_calibration_run_single_run_with_nexus_file(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777.nxs", Calibration=True)
        self._assert_output_workspaces_exist("S_fit")

    def test_calibration_run_non_default_foil_no_rebin(self):
        # Test with non default resonance, note default Ediv not suitable so using RebinInEnergy=False
        PEARLTransfit(Files="PEARL00112777", Calibration=True, FoilType="Hf02", RebinInEnergy=False)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=0.89391, peak_cen=2380.03, peak_cen_err=0.31927)
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 96)

    def test_calibration_run_non_default_foil_non_default_Ediv(self):
        # Test with non default resonance, note default Ediv not suitable
        PEARLTransfit(Files="PEARL00112777", Calibration=True, FoilType="Hf02", Ediv=0.01)
        self._assert_output_workspaces_exist("S_fit")
        self._assert_fit_parameters("S_fit", cost=0.74853, peak_cen=2380.03, peak_cen_err=0.32064)
        self.assertEqual(ADS.retrieve("S_fit_Workspace").blocksize(), 70)

    def _assert_fit_parameters(self, ws_prefix, cost, peak_cen, peak_cen_err, frac_delta=1e-4):
        param_table = ADS.retrieve(f"{ws_prefix}_Parameters")
        self.assertAlmostEqual(param_table.column("Value")[-1], cost, delta=frac_delta * cost)
        self.assertAlmostEqual(param_table.column("Value")[0], peak_cen, delta=frac_delta * peak_cen)
        self.assertAlmostEqual(param_table.column("Error")[0], peak_cen_err, delta=frac_delta * peak_cen)

    def _assert_output_workspaces_exist(self, ws_prefix):
        self.assertTrue(ADS.doesExist(f"{ws_prefix}_Parameters"))
        self.assertTrue(ADS.doesExist(f"{ws_prefix}_Workspace"))


if __name__ == "__main__":
    unittest.main()
