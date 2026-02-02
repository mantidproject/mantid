# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import config, mtd, CloneWorkspace, D7YIGPositionCalibration, Load
from mantid.api import ITableWorkspace, WorkspaceGroup
import os.path
from os import path
from tempfile import TemporaryDirectory


class D7YIGPositionCalibrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir("ILL/D7/")
        Load("402652_403041.nxs", OutputWorkspace="shortWavelengthScan")
        cls.TEMP_DIR = TemporaryDirectory()
        cls.TEMP_FILE_NAME = os.path.join(cls.TEMP_DIR.name, "D7YIGPositionCalibrationTest.xml")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()
        cls.TEMP_DIR.cleanup()

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaisesRegex(
            RuntimeError,
            "Either a list of file names containing YIG scan or the workspace with the loaded scan is required for calibration.",
        ):
            D7YIGPositionCalibration()

    def test_no_fitting(self):
        approximate_wavelength = "3.14"  # Angstrom
        self.assertTrue(mtd["shortWavelengthScan"])
        CloneWorkspace(InputWorkspace="shortWavelengthScan", OutputWorkspace="shortWavelengthScan_clone")
        D7YIGPositionCalibration(
            InputWorkspace="shortWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            FitOutputWorkspace="test_no_fitting",
            FittingMethod="None",
            ClearCache=False,
        )
        self.assertTrue(mtd["peak_fits_test_no_fitting"])
        self.assertTrue(isinstance(mtd["peak_fits_test_no_fitting"], WorkspaceGroup))

    def test_shortWavelength(self):
        approximate_wavelength = "3.14"  # Angstrom
        self.assertTrue(mtd["shortWavelengthScan"])
        CloneWorkspace(InputWorkspace="shortWavelengthScan", OutputWorkspace="shortWavelengthScan_clone")
        output_filename = self.TEMP_FILE_NAME
        D7YIGPositionCalibration(
            InputWorkspace="shortWavelengthScan_clone",
            ApproximateWavelength=approximate_wavelength,
            YIGPeaksFile="D7_YIG_peaks.xml",
            CalibrationOutputFile=output_filename,
            MinimalDistanceBetweenPeaks=1.75,
            BankOffsets=[3, 3, -1],
            FitOutputWorkspace="test_shortWavelength",
            FittingMethod="Individual",
        )
        self.assertTrue(path.exists(output_filename))
        self.assertTrue(mtd["test_shortWavelength"])
        self.assertTrue(isinstance(mtd["test_shortWavelength"], ITableWorkspace))
        self._check_fit_output("test_shortWavelength")

    def _check_fit_output(self, fitTableName):
        """Checks the TableWorkspace if the output values are reasonable,
        then check if the output IPF can be read by the Loader"""
        pixels_per_bank = 44
        self.assertNotEqual(mtd[fitTableName], None)
        self.assertTrue(isinstance(mtd[fitTableName], ITableWorkspace))

        wavelength = float(mtd[fitTableName].column(1)[1])
        self.assertAlmostEqual(wavelength, 1.0, delta=5e-2)  # +/- 5 %
        bank2_slope = 1.0 / float(mtd[fitTableName].column(1)[0])
        self.assertAlmostEqual(bank2_slope, 1.0, delta=2e-2)  # +/- 2 %
        bank3_slope = 1.0 / float(mtd[fitTableName].column(1)[4 * pixels_per_bank])
        self.assertAlmostEqual(bank3_slope, 1.0, delta=2e-2)  # +/- 2%
        bank4_slope = 1.0 / float(mtd[fitTableName].column(1)[8 * pixels_per_bank])
        self.assertAlmostEqual(bank4_slope, 1.0, delta=2e-2)  # +/- 2 %

        for row_no in range(mtd[fitTableName].rowCount()):
            row_data = mtd[fitTableName].row(row_no)
            if ".offset" in row_data["Name"]:
                offset = row_data["Value"]
                self.assertAlmostEqual(offset, 0.0, delta=24.0)  # +- 24 degrees


if __name__ == "__main__":
    unittest.main()
