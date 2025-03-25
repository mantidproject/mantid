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
        self.assertTrue(ADS.doesExist("S_fit_Parameters"))
        self.assertTrue(ADS.doesExist("S_fit_Workspace"))
        # assert fit converged by checking cost function value at minimum
        self.assertAlmostEqual(ADS.retrieve("S_fit_Parameters").column("Value")[-1], 0.7275, delta=1e-4)

    def test_calibration_run_single_run_backgorund_params_provided(self):
        # Provide very bad background params and show fit doesn't converge (would do if parameters estimated)
        PEARLTransfit(Files="PEARL00112777", Calibration=True, EstimateBackground=False, Bg0guessFraction=0.0, Bg1guess=-1.0, Bg2guess=1.0)
        self.assertTrue(ADS.doesExist("S_fit_Parameters"))
        self.assertTrue(ADS.doesExist("S_fit_Workspace"))
        # assert fit not converged by checking cost function value at minimum
        self.assertAlmostEqual(ADS.retrieve("S_fit_Parameters").column("Value")[-1], 0.7275, delta=1e-4)

    def test_calibration_run_multi_run(self):
        # Test that the calibration run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        self.assertTrue(ADS.doesExist("S_fit_Parameters"))
        self.assertTrue(ADS.doesExist("S_fit_Workspace"))

    def test_measure_run_single_run(self):
        # Test that the measurement run produces the correct workspaces for a single run
        PEARLTransfit(Files="PEARL00112777", Calibration=True)
        PEARLTransfit(Files="PEARL00112777", Calibration=False)
        self.assertTrue(ADS.doesExist("T_fit_Parameters"))
        self.assertTrue(ADS.doesExist("T_fit_Workspace"))

    def test_measure_run_multi_run(self):
        # Test that the measurement run produces the correct workspaces for multiple runs
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=True)
        PEARLTransfit(Files="PEARL00073987-00073990", Calibration=False)
        self.assertTrue(ADS.doesExist("T_fit_Parameters"))
        self.assertTrue(ADS.doesExist("T_fit_Workspace"))

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
        self.assertTrue(ADS.doesExist("S_fit_Parameters"))
        self.assertTrue(ADS.doesExist("S_fit_Workspace"))


if __name__ == "__main__":
    unittest.main()
