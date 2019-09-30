# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import (mtd)
from mantid.simpleapi import (CloneWorkspace)
import numpy.testing
from testhelpers import assert_almost_equal, illhelpers, run_algorithm
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
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        assert_almost_equal(ys, (1.0 - ecFactor) * originalYs)

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
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        assert_almost_equal(ys, (1.0 - ecScaling * ecFactor) * originalYs)

    def testSelfShieldingCorrections(self):
        ws = self._cloneTestWorkspace()
        corrFactor = 0.789
        corrWS = self._cloneTestWorkspace('correctionWS')
        for i in range(corrWS.getNumberHistograms()):
            ys = corrWS.dataY(i)
            ys.fill(corrFactor)
            es = corrWS.dataE(i)
            es.fill(0)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'SelfShieldingCorrectionWorkspace': corrWS,
            'rethrow': True
        }
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        assert_almost_equal(ys, originalYs / corrFactor)
        es = outWS.extractE()
        originalEs = ws.extractE()
        assert_almost_equal(es, originalEs / corrFactor)

    def testNoOperationClonesInputWorkspace(self):
        ws = self._cloneTestWorkspace()
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        # If the previous run didn't clone the input workspace, the two later
        # calls will be triggered to use 'outWS' as the input.
        self.assertTrue(mtd.doesExist(outWSName))
        corrFactor = 0.43
        corrWS = self._cloneTestWorkspace('correctionWS')
        for i in range(corrWS.getNumberHistograms()):
            ys = corrWS.dataY(i)
            ys.fill(corrFactor)
            es = corrWS.dataE(i)
            es.fill(0)
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'SelfShieldingCorrectionWorkspace': corrWS,
            'rethrow': True
        }
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        run_algorithm('DirectILLApplySelfShielding', **algProperties)
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        ys = outWS.extractY()
        originalYs = ws.extractY()
        assert_almost_equal(ys, originalYs / corrFactor)
        es = outWS.extractE()
        originalEs = ws.extractE()
        assert_almost_equal(es, originalEs / corrFactor)

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
