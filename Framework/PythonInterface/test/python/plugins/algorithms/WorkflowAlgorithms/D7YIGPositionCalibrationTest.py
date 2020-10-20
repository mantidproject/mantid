# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import config, mtd, D7YIGPositionCalibration, Load, LoadILLPolarizedDiffraction
from mantid.api import ITableWorkspace
import os.path
from os import path
import tempfile

class D7YIGPositionCalibrationTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')

    @classmethod
    def tearDownClass(cls):
        mtd.clear()
        output_path = os.path.join(tempfile.gettempdir(), 'test_shortWavelength.xml')
        if path.exists(output_path):
            os.remove(output_path)

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaises(RuntimeError):
            D7YIGPositionCalibration()

    def test_shortWavelength(self):
        approximate_wavelength = '3.14' # Angstrom
        self.assertTrue(mtd['shortWavelengthScan'])
        output_filename = os.path.join(tempfile.gettempdir(), 'test_shortWavelength.xml')
        D7YIGPositionCalibration(InputWorkspace='shortWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                 YIGPeaksFile='D7_YIG_peaks.xml', CalibrationOutputFile=output_filename,
                                 MinimalDistanceBetweenPeaks=1.75, BankOffsets=[-3, -3, 1],
                                 FitOutputWorkspace='test_shortWavelength')
        self.assertTrue(path.exists(output_filename))
        self.assertTrue(mtd['test_shortWavelength'])
        self.assertTrue(isinstance(mtd['test_shortWavelength'], ITableWorkspace))
        self._check_fit_output('test_shortWavelength')

    def _check_fit_output(self, fitTableName):
        """ Checks the TableWorkspace if the output values are reasonable,
        then check if the output IPF can be read by the Loader"""
        pixels_per_bank = 44
        self.assertNotEqual(mtd[fitTableName], None)
        self.assertTrue(isinstance(mtd[fitTableName], ITableWorkspace))

        wavelength = float(mtd[fitTableName].column(1)[1])
        self.assertAlmostEqual(wavelength, 1.0,  delta=5e-2) # +/- 5 %
        bank2_slope = 1.0 / float(mtd[fitTableName].column(1)[0])
        self.assertAlmostEqual(bank2_slope, 1.0, delta=2e-2) # +/- 1 %
        bank3_slope = 1.0 / float(mtd[fitTableName].column(1)[4*pixels_per_bank])
        self.assertAlmostEqual(bank3_slope, 1.0, delta=2e-2) # +/- 1 %
        bank4_slope = 1.0 / float(mtd[fitTableName].column(1)[8*pixels_per_bank])
        self.assertAlmostEqual(bank4_slope, 1.0, delta=2e-2) # +/- 1 %

        for row_no in range(mtd[fitTableName].rowCount()):
            row_data = mtd[fitTableName].row(row_no)
            if '.offset' in row_data['Name']:
                offset = row_data['Value']
                self.assertAlmostEqual(offset, 0.0, delta=24.0) # +- 24 degrees

if __name__ == '__main__':
    unittest.main()
