from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (CloneWorkspace, DeleteWorkspace, DirectILLCollectData)
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLCollectData(unittest.TestCase):
    _BKG_LEVEL = 2.3
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = None

    def setUp(self):
        if not self._testIN5WS:
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL, illhelpers.default_test_detectors)
        tempName = 'temp_testWS_'
        mtd.addOrReplace(tempName, self._testIN5WS)
        CloneWorkspace(InputWorkspace=tempName,
                       OutputWorkspace=self._TEST_WS_NAME)
        mtd.remove(tempName)

    def tearDown(self):
        mtd.clear()

    def testBackgroundSubtraction(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkgScaling': 1.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), self._testIN5WS.getNumberHistograms() - 1)
        ys = outWS.extractY()
        originalYs = self._testIN5WS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[1:, :] - self._BKG_LEVEL)

    def testBackgroundOutput(self):
        outWSName = 'outWS'
        outBkgWSName = 'outBkg'
        bkgScaling = 0.33  # Output should not be scaled, actually.
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
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
            'FlatBkgScaling': 0.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation Time',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        ys = outWS.extractY()
        originalYs = self._testIN5WS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[1:, :] / duration)
        es = outWS.extractE()
        originalEs = self._testIN5WS.extractE()
        numpy.testing.assert_almost_equal(es, originalEs[1:, :] / duration)

    def testSuccessWhenEverythingDisabled(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'FlatBkgScaling': 0.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLCollectData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), self._testIN5WS.getNumberHistograms() - 1)
        xs = outWS.extractX()
        originalXs = self._testIN5WS.extractX()
        numpy.testing.assert_almost_equal(xs, originalXs[1:, :])
        ys = outWS.extractY()
        originalYs = self._testIN5WS.extractY()
        numpy.testing.assert_almost_equal(ys, originalYs[1:, :])
        es = outWS.extractE()
        originalEs = self._testIN5WS.extractE()
        numpy.testing.assert_almost_equal(es, originalEs[1:, :])


if __name__ == '__main__':
    unittest.main()
