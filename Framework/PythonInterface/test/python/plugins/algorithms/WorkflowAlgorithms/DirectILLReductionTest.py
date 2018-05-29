from __future__ import (absolute_import, division, print_function)

import collections
from mantid.api import mtd
from scipy import constants
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLReductionTest(unittest.TestCase):
    _BKG_LEVEL = 0.0
    _EPP_WS_NAME = 'eppWS_'
    _TEST_WS_NAME = 'testWS_'
    _VANADIUM_WS_NAME = 'vanadiumWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = None

    def setUp(self):
        if not self._testIN5WS:
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL,
                                                                        illhelpers.default_test_detectors)
        inWSName = 'inputWS'
        mtd.addOrReplace(inWSName, self._testIN5WS)
        kwargs = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': self._TEST_WS_NAME,
            'OutputEPPWorkspace': self._EPP_WS_NAME
        }
        run_algorithm('DirectILLCollectData', **kwargs)
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': self._VANADIUM_WS_NAME,
            'EPPWorkspace': self._EPP_WS_NAME
        }
        run_algorithm('DirectILLIntegrateVanadium', **kwargs)
        vanadiumWS = mtd[self._VANADIUM_WS_NAME]
        for i in range(vanadiumWS.getNumberHistograms()):
            vanadiumYs = vanadiumWS.dataY(i)
            vanadiumYs.fill(1.0)
        mtd.remove(inWSName)

    def tearDown(self):
        mtd.clear()

    def testSuccessfulRun(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'SubalgorithmLogging': 'Logging ON',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))

    def testDetectorGrouping(self):
        ws = illhelpers.create_poor_mans_in5_workspace(0.0, _groupingTestDetectors)
        originalNDetectors = ws.getNumberHistograms()
        detectorIds = list()
        for i in range(originalNDetectors):
            detectorIds.append(ws.getDetector(i).getID())
        mtd.addOrReplace('inWS', ws)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': ws,
            'OutputWorkspace': outWSName,
            'Cleanup': 'Cleanup OFF',
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        groupedWSName = outWSName + '_grouped_detectors_'
        self.assertTrue(groupedWSName in mtd)
        groupedWS = mtd[groupedWSName]
        self.assertEqual(groupedWS.getNumberHistograms(), 1)
        groupIds = groupedWS.getDetector(0).getDetectorIDs()
        self.assertEqual(collections.Counter(detectorIds), collections.Counter(groupIds))

    def testOutputIsDistribution(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'OutputSofThetaEnergyWorkspace': 'SofThetaE',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertTrue(ws.isDistribution())
        self.assertTrue(mtd.doesExist('SofThetaE'))
        ws = mtd['SofThetaE']
        self.assertTrue(ws.isDistribution())

    def _checkAlgorithmsInHistory(self, ws, *args):
        """Return true if algorithm names listed in *args are found in the
        workspace's history.
        """
        # This method is currently unused and kept here for nostalgic reasons.
        # However, it might be interesting to start using it again in tests.
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        algHistories = reductionHistory.getChildHistories()
        algNames = [alg.name() for alg in algHistories]
        for algName in args:
            return algName in algNames


def _groupingTestDetectors(ws):
    """Mask detectors for detector grouping tests."""
    indexBegin = 63106  # Detector at L2 and at 2theta = 40.6.
    kwargs = {
        'Workspace': ws,
        'StartWorkspaceIndex': 0,
        'EndWorkspaceIndex': indexBegin - 1,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    referenceDetector = ws.getDetector(indexBegin)
    reference2Theta = ws.detectorTwoTheta(referenceDetector)
    mask = list()
    for i in range(indexBegin + 1, indexBegin + 10000):
        det = ws.getDetector(i)
        twoTheta = ws.detectorTwoTheta(det)
        if abs(reference2Theta - twoTheta) >= 0.01 / 180 * constants.pi:
            mask.append(i)
    kwargs = {
        'Workspace': ws,
        'DetectorList': mask,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    kwargs = {
        'Workspace': ws,
        'StartWorkspaceIndex': indexBegin + 10000,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    return ws

if __name__ == '__main__':
    unittest.main()
