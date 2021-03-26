# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import mtd, PEARLTransfit


class PEARLTransfitTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_calibration_run_single_run(self):
        # Test that the calibration run produces the correct workspaces for a single run
        PEARLTransfit(Files='PEARL00112777', Calibration=True)
        self.assertIn('S_fit_Parameters', mtd)
        self.assertIn('S_fit_Workspace', mtd)

    def test_calibration_run_multi_run(self):
        # Test that the calibration run produces the correct workspaces for multiple runs
        PEARLTransfit(Files='PEARL00073987-00073990', Calibration=True)
        self.assertIn('S_fit_Parameters', mtd)
        self.assertIn('S_fit_Workspace', mtd)

    def test_measure_run_single_run(self):
        # Test that the measurement run produces the correct workspaces for a single run
        PEARLTransfit(Files='PEARL00112777', Calibration=True)
        PEARLTransfit(Files='PEARL00112777', Calibration=False)
        self.assertIn('T_fit_Parameters', mtd)
        self.assertIn('T_fit_Workspace', mtd)

    def test_measure_run_multi_run(self):
        # Test that the measurement run produces the correct workspaces for multiple runs
        PEARLTransfit(Files='PEARL00073987-00073990', Calibration=True)
        PEARLTransfit(Files='PEARL00073987-00073990', Calibration=False)
        self.assertIn('T_fit_Parameters', mtd)
        self.assertIn('T_fit_Workspace', mtd)

    def test_algorithm_cancels_if_no_calib(self):
        # Test that the algorithm is aborted if run in measurement mode without a calibration file
        PEARLTransfit(Files='PEARL00073987', Calibration=False)
        self.assertNotIn('T_fit_Parameters', mtd)
        self.assertNotIn('T_fit_Workspace', mtd)
        self.assertNotIn('S_fit_Parameters', mtd)
        self.assertNotIn('S_fit_Workspace', mtd)


if __name__ == '__main__':
    unittest.main()
