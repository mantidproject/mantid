from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (CloneWorkspace, DeleteWorkspace, DirectILLPrepareData)
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLPrepareDataTest(unittest.TestCase):
    _BKG_LEVEL = 2.3
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = None

    def setUp(self):
        if not self._testIN5WS:
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL)
        tempName = 'temp_testWS_'
        mtd.addOrReplace(tempName, self._testIN5WS)
        CloneWorkspace(InputWorkspace=tempName,
                       OutputWorkspace=self._TEST_WS_NAME)
        mtd.remove(tempName)

    def tearDown(self):
        mtd.clear()

    def testBackgroundSubtraction(self):
        cloneWS = CloneWorkspace(InputWorkspace=self._TEST_WS_NAME)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'DetectorsAtL2': '130, 390',
            'FlatBkgScaling': 1.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLPrepareData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), cloneWS.getNumberHistograms() - 1)
        ys = outWS.extractY()
        cloneYs = cloneWS.extractY()
        numpy.testing.assert_almost_equal(ys, cloneYs[1:, :] - self._BKG_LEVEL)

    def testBackgroundOutput(self):
        outWSName = 'outWS'
        outBkgWSName = 'outBkg'
        bkgScaling = 0.33  # Output should not be scaled, actually.
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'DetectorsAtL2': '130, 390',
            'FlatBkgScaling': bkgScaling,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'OutputFlatBkgWorkspace': outBkgWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLPrepareData', **algProperties)
        self.assertTrue(mtd.doesExist(outBkgWSName))
        outBkgWS = mtd[outBkgWSName]
        numpy.testing.assert_almost_equal(outBkgWS.extractY(), self._BKG_LEVEL)

    def testSuccessWhenEverythingDisabled(self):
        cloneWS = CloneWorkspace(InputWorkspace=self._TEST_WS_NAME)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'DetectorsAtL2': '130, 390',
            'FlatBkgScaling': 0.0,
            'IncidentEnergyCalibration': 'Energy Calibration OFF',
            'Normalisation': 'Normalisation OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLPrepareData', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), cloneWS.getNumberHistograms() - 1)
        ys = outWS.extractY()
        cloneYs = cloneWS.extractY()
        numpy.testing.assert_almost_equal(ys, cloneYs[1:, :])


if __name__ == '__main__':
    unittest.main()
