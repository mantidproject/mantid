# -*- coding: utf-8 -*-
from __future__ import (absolute_import, division, print_function)

from mantid.api import (mtd)
from mantid.simpleapi import (CloneWorkspace)
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLApplySelfShieldingTest(unittest.TestCase):
    _BKG_LEVEL = 0.0
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL, illhelpers.default_test_detectors)

    def tearDown(self):
        mtd.clear()

    def testEmptyContainerSubtraction(self):
        ws = self._cloneTestWorkspace()
        ecWSName = 'testECWS_'
        ecWS = self._cloneTestWorkspace(ecWSName)
        ecFactor = 0.13
        ecWS *= ecFactor
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EmptyContainerWorkspace': ecWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        numpy.testing.assert_almost_equal(ys, (1.0 - ecFactor) *originalYs)

    def testEmptyContainerSubtractionWithScaling(self):
        ws = self._cloneTestWorkspace()
        ecWSName = 'testECWS_'
        ecWS = self._cloneTestWorkspace(ecWSName)
        ecFactor = 0.13
        ecWS *= ecFactor
        outWSName = 'outWS'
        ecScaling = 0.876
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EmptyContainerWorkspace': ecWSName,
            'EmptyContainerScaling': ecScaling,
            'rethrow': True
        }
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        numpy.testing.assert_almost_equal(ys, (1.0 - ecScaling * ecFactor) *originalYs)

    def _cloneTestWorkspace(self, wsName=None):
        if not wsName:
            # Cannot use as default parameter as 'self' is not know in argument list.
            wsName = self._TEST_WS_NAME
        tempName = 'temp_testWS_'
        mtd.addOrReplace(tempName, self._testIN5WS)
        ws = CloneWorkspace(InputWorkspace=tempName,
                            OutputWorkspace=wsName)
        mtd.remove(tempName)
        return ws

if __name__ == '__main__':
    unittest.main()
