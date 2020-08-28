# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import config, mtd, ILLD7YIGPositionCalibration, Load, LoadILLPolarizedDiffraction
import xml.etree.ElementTree as ET
import math


class ILLD7YIGPositionCalibrationTest(unittest.TestCase):

    _pixels_per_bank = 44

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')

    def tearDown(self):
        mtd.clear()

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaises(RuntimeError):
            ILLD7YIGPositionCalibration()

    def test_algorithm_with_wrong_approximate_wavelength_raises_exception(self):
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')
        with self.assertRaises(ValueError):
            ILLD7YIGPositionCalibration(ScanWorkspace='shortWavelengthScan', YIGPeaksFile='YIG_peaks.xml',
                                      ApproximateWavelength="-1.0")

    def test_shortWavelength(self):
        Load('402652_403041.nxs', OutputWorkspace='shortWavelengthScan')
        approximate_wavelength = '3.14' # Angstrom
        ILLD7YIGPositionCalibration(ScanWorkspace='shortWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_shortWavelength.xml',
                                  MinimalDistanceBetweenPeaks=1.75, BankOffsets="-3,-3,1",
                                  DetectorFitOutput='test_shortWavelength')
        self._check_fit_output('test_shortWavelength')
        self._check_load_data_with_calibration('test_shortWavelength.xml')

    def test_intermediateWavelength(self):
        Load('396442_396831.nxs', OutputWorkspace='intermediateWavelengthScan')
        approximate_wavelength = '4.8' # Angstrom
        ILLD7YIGPositionCalibration(ScanWorkspace='intermediateWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_intermediateWavelength.xml',
                                  MinimalDistanceBetweenPeaks=1.5, BankOffsets="-3,-3,1",
                                  DetectorFitOutput='test_intermediateWavelength')
        self._check_fit_output('test_intermediateWavelength')
        self._check_load_data_with_calibration('test_intermediateWavelength.xml')

    def test_longWavelength(self):
        Load('394458_394882.nxs', OutputWorkspace='longWavelengthScan')
        approximate_wavelength = '5.7' # Angstrom
        ILLD7YIGPositionCalibration(ScanWorkspace='longWavelengthScan', ApproximateWavelength=approximate_wavelength,
                                  YIGPeaksFile='YIG_peaks.xml', CalibrationFilename='test_longWavelength.xml',
                                  MinimalDistanceBetweenPeaks=1.5, BankOffsets="-3,-3,1",
                                  DetectorFitOutput='test_longWavelength')
        self._check_fit_output('test_longWavelength')
        self._check_load_data_with_calibration('test_longWavelength.xml')

    def _check_fit_output(self, fitTableName):
        """ Checks the TableWorkspace if the output values are reasonable,
        then check if the output IPF can be read by the Loader"""

        self.assertNotEqual(mtd[fitTableName], None)

        wavelength = float(mtd[fitTableName].column(1)[1])
        self.assertAlmostEqual(wavelength, 1.0,  delta=5e-2) # +/- 5 %
        bank2_slope = 1.0 / float(mtd[fitTableName].column(1)[0])
        self.assertAlmostEqual(bank2_slope, 1.0, delta=2e-2) # +/- 1 %
        bank3_slope = 1.0 / float(mtd[fitTableName].column(1)[4*self._pixels_per_bank])
        self.assertAlmostEqual(bank3_slope, 1.0, delta=2e-2) # +/- 1 %
        bank4_slope = 1.0 / float(mtd[fitTableName].column(1)[8*self._pixels_per_bank])
        self.assertAlmostEqual(bank4_slope, 1.0, delta=2e-2) # +/- 1 %

        for row_no in range(mtd[fitTableName].rowCount()):
            row_data = mtd[fitTableName].row(row_no)
            if '.offset' in row_data['Name']:
                offset = row_data['Value']
                self.assertAlmostEqual(offset, 0.0, delta=24.0) # +- 24 degrees

    def _check_load_data_with_calibration(self, ipf_name):
        tree = ET.parse(ipf_name)
        self.assertNotEquals(tree, None)
        root = tree.getroot()
        bank_offsets = []
        bank_gradients = []
        pixel_offsets = []
        for elem in root:
            for subelem in elem:
                for value in subelem:
                    value = float(value.attrib['val'])
                    if 'pixel' in subelem.attrib['name']:
                        pixel_offsets.append(value)
                    if 'gradient' in subelem.attrib['name']:
                        bank_gradients.append(value)
                    if 'offset' in subelem.attrib['name']:
                        bank_offsets.append(value)
        positionCalibration = [pixel_offset + bank_offsets[math.floor(pixel_no / self._pixels_per_bank)]
                               for pixel_no, pixel_offset in enumerate(pixel_offsets)]
        self.assertEquals(len(positionCalibration), 132) # number of pixels
        LoadILLPolarizedDiffraction('401800', OutputWorkspace='output', PositionCalibration='YIGFile',
                                    YIGFilename=ipf_name, ConvertToScatteringAngle=True, TransposeMonochromatic=True)
        self.assertNotEquals('output', None)
        nexus_bank_offsets = [ mtd['output'].getItem(0).getRun().getLogData('2theta.actual_bank{}'.format(bank_no+2)).value
                               for bank_no in range(3) ]

        xAxisValues = mtd['output'].getItem(0).readX(0)
        self.assertAlmostEqual(xAxisValues[0], bank_gradients[0]*nexus_bank_offsets[0]-positionCalibration[0], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[43], bank_gradients[0]*nexus_bank_offsets[0]-positionCalibration[43], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[44], bank_gradients[1]*nexus_bank_offsets[1]-positionCalibration[44], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[87], bank_gradients[1]*nexus_bank_offsets[1]-positionCalibration[87], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[88], bank_gradients[2]*nexus_bank_offsets[2]-positionCalibration[88], delta=1e-2)
        self.assertAlmostEqual(xAxisValues[131], bank_gradients[2]*nexus_bank_offsets[2]-positionCalibration[131], delta=1e-2)


if __name__ == '__main__':
    unittest.main()
