from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (DirectILLCollectData, DirectILLReduction)
import numpy
import numpy.testing
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
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL)
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

if __name__ == '__main__':
    unittest.main()
