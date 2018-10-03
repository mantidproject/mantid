# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid import mtd
from mantid.simpleapi import (CloneWorkspace, FindEPP)
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLIntegrateVanadiumTest(unittest.TestCase):
    _BKG_LEVEL = 0.0
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL, illhelpers.default_test_detectors)

    def tearDown(self):
        mtd.clear()

    def testIntegration(self):
        ws = self._cloneTestWorkspace()
        eppWSName = 'eppWS'
        self._EPPTable(ws, eppWSName)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': eppWSName,
            'DebyeWallerCorrection': 'Correction OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLIntegrateVanadium', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), ws.getNumberHistograms())
        self.assertEquals(outWS.blocksize(), 1)

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

    def _EPPTable(self, ws, eppWSName):
        eppWS = FindEPP(InputWorkspace=ws,
                        OutputWorkspace=eppWSName,
                        EnableLogging=False)
        return eppWS


if __name__ == '__main__':
    unittest.main()
