# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLCollectDataTest(unittest.TestCase):
    _BKG_LEVEL = 2.3
    _TEST_WS_NAME = 'testWS_'
    _TEST_WS = None

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)

    def setUp(self):
        if self._TEST_WS is None:
            self._TEST_WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL,
                                                                      illhelpers.default_test_detectors)
        algProperties = {
            'InputWorkspace': self._TEST_WS,
            'OutputWorkspace': self._TEST_WS_NAME,
        }
        run_algorithm('CloneWorkspace', **algProperties)

    def tearDown(self):
        mtd.clear()

    def testBackgroundSubtraction(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkg': 'Flat Bkg ON',
            'FlatBkgScaling': 1.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        inWS = mtd[self._TEST_WS_NAME]
        self.assertEqual(outWS.getNumberHistograms(), inWS.getNumberHistograms() - 1)
        ys = outWS.extractY()
        originalYs = inWS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[:-1, :] - self._BKG_LEVEL)

    def testBackgroundOutput(self):
        outWSName = 'outWS'
        outBkgWSName = 'outBkg'
        bkgScaling = 0.33  # Output should not be scaled, actually.
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkg': 'Flat Bkg ON',
            'FlatBkgScaling': bkgScaling,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'OutputFlatBkgWorkspace': outBkgWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outBkgWSName))
        outBkgWS = mtd[outBkgWSName]
        numpy.testing.assert_almost_equal(outBkgWS.extractY(), self._BKG_LEVEL)

    def testNormalisationToTime(self):
        outWSName = 'outWS'
        duration = 3612.3
        mtd[self._TEST_WS_NAME].mutableRun().addProperty('duration', duration, True)
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkg': 'Flat Bkg OFF',
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation Time',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        inWS = mtd[self._TEST_WS_NAME]
        ys = outWS.extractY()
        originalYs = inWS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[:-1, :] / duration)
        es = outWS.extractE()
        originalEs = inWS.extractE()
        numpy.testing.assert_almost_equal(es, originalEs[:-1, :] / duration)

    def testNormalisationToTimeWhenMonitorCountsAreTooLow(self):
        outWSName = 'outWS'
        duration = 3612.3
        logs = mtd[self._TEST_WS_NAME].mutableRun()
        logs.addProperty('duration', duration, True)
        monsum = 10
        logs.addProperty('monitor.monsum', monsum, True)
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkg': 'Flat Bkg OFF',
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation Monitor',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        inWS = mtd[self._TEST_WS_NAME]
        ys = outWS.extractY()
        originalYs = inWS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[:-1, :] / duration)
        es = outWS.extractE()
        originalEs = inWS.extractE()
        numpy.testing.assert_almost_equal(es, originalEs[:-1, :] / duration)

    def testRawWorkspaceOutput(self):
        outWSName = 'outWS'
        rawWSName = 'rawWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'OutputRawWorkspace': rawWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        inWS = mtd[self._TEST_WS_NAME]
        self.assertTrue(mtd.doesExist(rawWSName))
        rawWS = mtd[rawWSName]
        ys = rawWS.extractY()
        originalYS = inWS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYS[:-1, :])
        es = rawWS.extractE()
        originalES = inWS.extractE()
        numpy.testing.assert_almost_equal(es, originalES[:-1, :])
        xs = rawWS.extractX()
        outXS = outWS.extractX()
        numpy.testing.assert_almost_equal(xs, outXS)
        Ei = rawWS.getRun().getProperty('Ei').value
        outEi = outWS.getRun().getProperty('Ei').value
        self.assertEqual(Ei, outEi)
        wavelength = outWS.getRun().getProperty('wavelength').value
        outWavelength = outWS.getRun().getProperty('wavelength').value
        self.assertEqual(wavelength, outWavelength)

    def testSuccessWhenEverythingDisabled(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkg': 'Flat Bkg OFF',
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'ElasticChannel': 'Default Elastic Channel',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        inWS = mtd[self._TEST_WS_NAME]
        self.assertEqual(outWS.getNumberHistograms(), inWS.getNumberHistograms() - 1)
        xs = outWS.extractX()
        originalXs = inWS.extractX()
        numpy.testing.assert_almost_equal(xs, originalXs[:-1, :])
        ys = outWS.extractY()
        originalYs = inWS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[:-1, :])
        es = outWS.extractE()
        originalEs = inWS.extractE()
        numpy.testing.assert_almost_equal(es, originalEs[:-1, :])

    def testOutputIncidentEnergyWorkspaceWhenEnergyCalibrationIsOff(self):
        outWSName = 'outWS'
        eiWSName = 'Ei'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'OutputIncidentEnergyWorkspace': eiWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(eiWSName))
        eiWS = mtd[eiWSName]
        inWS = mtd[self._TEST_WS_NAME]
        E_i = inWS.run().getProperty('Ei').value
        self.assertEqual(eiWS.readY(0)[0], E_i)


if __name__ == '__main__':
    unittest.main()
