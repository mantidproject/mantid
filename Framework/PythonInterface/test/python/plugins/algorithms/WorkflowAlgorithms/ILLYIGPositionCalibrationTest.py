# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, Run
from mantid.simpleapi import ILLYIGPositionCalibration, Load, LoadILLPolarizedDiffraction, config, mtd
from mantid.geometry import Instrument

import numpy as np
import os
import xml.etree.ElementTree as ET

class ILLYIGPositionCalibrationTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')

    def tearDown(self):
        mtd.clear()

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaises(RuntimeError):
            ILLYIGPositionCalibration()

    def test_algorithm_with_wrong_approximate_wavelength_raises_exception(self):
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')
        with self.assertRaises(ValueError):
            ILLYIGPositionCalibration(ScanWorkspace='shortWavelengthScan', YIGPeaksFile='YIG_peaks.xml',
                                      ApproximateWavelength="0.0")
           
    def test_shortWavelength(self):
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')
        approximate_wavelength = '3.1' # Angstrom
        ILLYIGPositionCalibration(ScanWorkspace='shortWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_shortWavelength.xml',
                                  DetectorFitOutput='test_shortWavelength')
        self._check_fit_output('test_shortWavelength')
        self._check_load_data_with_calibration('test_shortWavelength.xml')

    def test_intermediateWavelength(self):
        Load('396442_396831.nxs', OutputWorkspace='intermediateWavelengthScan')
        approximate_wavelength = '4.8' # Angstrom
        ILLYIGPositionCalibration(ScanWorkspace='intermediateWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_intermediateWavelength.xml',
                                  DetectorFitOutput='test_intermediateWavelength')
        self._check_fit_output('test_intermediateWavelength')
        self._check_load_data_with_calibration('test_intermediateWavelength.xml')
        
    def test_longWavelength(self):
        Load('394458_394882.nxs', OutputWorkspace='longWavelengthScan')
        approximate_wavelength = '5.7' # Angstrom
        ILLYIGPositionCalibration(ScanWorkspace='longWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_longWavelength.xml',
                                  DetectorFitOutput='test_longWavelength')
        self._check_fit_output('test_longWavelength')
        self._check_load_data_with_calibration('test_longWavelength.xml')

    def _check_fit_output(self, parameters_name):
        """ Checks the TableWorkspace if the output values are reasonable, 
        then check if the output IPF can be read by the Loader"""
        D7_NUMBER_PIXELS = 132        

        self.assertNotEqual(mtd[parameters_name], None)
        
        wavelength = float(mtd[parameters_name].column(1)[1])
        self.assertAlmostEqual(wavelength, 1.0,  delta=2e-2) # 2 %
        bank2_slope = float(mtd[parameters_name].column(1)[0])
        self.assertAlmostEqual(bank2_slope, 1.0, delta=1e-2) # 1 %
        bank3_slope = float(mtd[parameters_name].column(1)[D7_NUMBER_PIXELS])
        self.assertAlmostEqual(bank3_slope, 1.0, delta=1e-2) # 1 %
        bank4_slope = float(mtd[parameters_name].column(1)[D7_NUMBER_PIXELS*2])
        self.assertAlmostEqual(bank4_slope, 1.0, delta=1e-2) # 1 %

        for pixel_no in range(D7_NUMBER_PIXELS):
            offset = mtd[parameters_name].column(1)[2+3*pixel_no] * 180.0 / np.pi
            self.assertAlmostEqual(offset, 0.0, delta=24.0) # +- 24 degrees
            
    def _check_load_data_with_calibration(self, ipf_name):
        tree = ET.parse(ipf_name)
        self.assertNotEquals(tree, None)
        root = tree.getroot()
        positionCalibration = []
        for elem in root:
            for subelem in elem:
                for value in subelem:
                    positionCalibration.append(float(value.attrib['val']))
        self.assertEquals(len(positionCalibration), 135) # number of pixels + monitors + wavelength value
        LoadILLPolarizedDiffraction('401800', OutputWorkspace='output', PositionCalibration='YIGFile',
                                    YIGFilename=ipf_name, ConvertToScatteringAngle=True, TransposeMonochromatic=True)
        self.assertNotEquals('output', None)
        bank_offsets = [ mtd['output'].getItem(0).getRun().getLogData('2theta.actual_bank{}'.format(bank_no+2)).value
                         for bank_no in range(3) ]

        xAxisValues = mtd['output'].getItem(0).readX(0)
        self.assertAlmostEqual(xAxisValues[0], bank_offsets[0]-positionCalibration[0], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[43], bank_offsets[0]-positionCalibration[43], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[44], bank_offsets[1]-positionCalibration[44], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[87], bank_offsets[1]-positionCalibration[87], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[88], bank_offsets[2]-positionCalibration[88], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[131], bank_offsets[2]-positionCalibration[131], delta=1e-2)       
        
        
if __name__ == '__main__':
    unittest.main()
