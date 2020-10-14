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

    def setUp(self):
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')

    def tearDown(self):
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
        D7YIGPositionCalibration(InputWorkspace='shortWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                 YIGPeaksFile='D7_YIG_peaks.xml', CalibrationOutputFile='test_shortWavelength.xml',
                                 MinimalDistanceBetweenPeaks=1.75, BankOffsets=[-3, -3, 1],
                                 FitOutputWorkspace='test_shortWavelength')
        self.assertTrue(path.exists('test_shortWavelength.xml'))
        self.assertTrue(mtd['test_shortWavelength'])
        self.assertTrue(isinstance(mtd['test_shortWavelength'], ITableWorkspace))

if __name__ == '__main__':
    unittest.main()
