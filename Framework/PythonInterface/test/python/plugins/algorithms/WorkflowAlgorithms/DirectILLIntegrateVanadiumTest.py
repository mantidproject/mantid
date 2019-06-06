# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid import mtd
from mantid.simpleapi import (CloneWorkspace, FindEPP)
import numpy
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

    def testIntegrationWithDebyeWallerCorrection(self):
        ws = self._cloneTestWorkspace()
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i).fill(float(i + 1))
            ws.dataE(i).fill(numpy.sqrt(float(i + 1)))
        numBins = ws.blocksize()
        eppWSName = 'eppWS'
        self._EPPTable(ws, eppWSName)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': eppWSName,
            'DebyeWallerCorrection': 'Correction ON',
            'rethrow': True
        }
        run_algorithm('DirectILLIntegrateVanadium', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        self.assertEqual(outWS.blocksize(), 1)
        for i in range(outWS.getNumberHistograms()):
            self.assertGreater(outWS.readY(i)[0], float(i + 1) * numBins)
            self.assertGreater(outWS.readE(i)[0], numpy.sqrt(float(i + 1) * numBins))

    def testIntegrationWithoutDebyeWallerCorrection(self):
        ws = self._cloneTestWorkspace()
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i).fill(float(i + 1))
            ws.dataE(i).fill(numpy.sqrt(float(i + 1)))
        numBins = ws.blocksize()
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
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        self.assertEqual(outWS.blocksize(), 1)
        for i in range(outWS.getNumberHistograms()):
            self.assertEqual(outWS.readY(i)[0], float(i + 1) * numBins)
            self.assertAlmostEqual(outWS.readE(i)[0], numpy.sqrt(float(i + 1) * numBins))

    def testZeroMasking(self):
        ws = self._cloneTestWorkspace()
        zeroIndices = [5, 23]
        for i in zeroIndices:
            ws.dataY(i).fill(0.)
        eppWSName = 'eppWS'
        self._EPPTable(ws, eppWSName)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': eppWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLIntegrateVanadium', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), ws.getNumberHistograms())
        self.assertEqual(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(outWS.getNumberHistograms()):
            if i in zeroIndices:
                self.assertEqual(outWS.readY(i)[0], 0.)
                self.assertTrue(spectrumInfo.isMasked(i))
            else:
                self.assertGreater(outWS.readY(i)[0], 0.)
                self.assertFalse(spectrumInfo.isMasked(i))

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
