from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (DirectILLDiagnostics, DirectILLCollectData)
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLDiagnosticsTest(unittest.TestCase):
    _BKG_LEVEL = 1.42
    _EPP_WS_NAME = 'eppWS_'
    _RAW_WS_NAME = 'rawWS_'
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = None

    def setUp(self):
        if not self._testIN5WS:
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL, illhelpers.default_test_detectors)
        inWSName = 'inputWS'
        mtd.addOrReplace(inWSName, self._testIN5WS)
        kwargs = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': self._TEST_WS_NAME,
            'OutputEPPWorkspace': self._EPP_WS_NAME,
            'OutputRawWorkspace': self._RAW_WS_NAME
        }
        run_algorithm('DirectILLCollectData', **kwargs)
        mtd.remove(inWSName)

    def tearDown(self):
        mtd.clear()

    def testAllDetectorsPass(self):
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'RawWorkspace': self._RAW_WS_NAME,
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        inWS = mtd[self._TEST_WS_NAME]
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), inWS.getNumberHistograms())
        self.assertEquals(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(outWS.getNumberHistograms()):
            self.assertEquals(outWS.readY(i)[0], 0)
            self.assertFalse(spectrumInfo.isMasked(i))

    def testBackgroundDiagnostics(self):
        rawWS = mtd[self._RAW_WS_NAME]
        spectraCount = rawWS.getNumberHistograms()
        highBkgIndices = [0, int(spectraCount / 3), spectraCount - 1]
        for i in highBkgIndices:
            ys = rawWS.dataY(i)
            ys += 10.0 * self._BKG_LEVEL
        lowBkgIndices = [int(spectraCount / 4), int(2 * spectraCount / 3)]
        for i in lowBkgIndices:
            ys = rawWS.dataY(i)
            ys -= self._BKG_LEVEL
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'ElasticPeakDiagnostics': 'Peak Diagnostics OFF',
            'EPPWorkspace': self._EPP_WS_NAME,
            'RawWorkspace': self._RAW_WS_NAME,
            'NoisyBkgLowThreshold': 0.01,
            'NoisyBkgHighThreshold': 9.99,
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        inWS = mtd[self._TEST_WS_NAME]
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), spectraCount)
        self.assertEquals(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(spectraCount):
            self.assertFalse(spectrumInfo.isMasked(i))
            ys = outWS.readY(i)
            if i in highBkgIndices + lowBkgIndices:
                self.assertEquals(ys[0], 1)
            else:
                self.assertEquals(ys[0], 0)

    def testElasticPeakDiagnostics(self):
        inWS = mtd[self._TEST_WS_NAME]
        spectraCount = inWS.getNumberHistograms()
        highPeakIndices = [0, int(spectraCount / 3), spectraCount - 1]
        for i in highPeakIndices:
            ys = inWS.dataY(i)
            ys *= 10.0
        lowPeakIndices = [int(spectraCount / 4), int(2 * spectraCount / 3)]
        for i in lowPeakIndices:
            ys = inWS.dataY(i)
            ys *= 0.1
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'ElasticPeakLowThreshold': 0.2,
            'ElasticPeakHighThreshold': 9.7,
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEquals(outWS.getNumberHistograms(), spectraCount)
        self.assertEquals(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(spectraCount):
            self.assertFalse(spectrumInfo.isMasked(i))
            ys = outWS.readY(i)
            if i in highPeakIndices + lowPeakIndices:
                self.assertEquals(ys[0], 1)
            else:
                self.assertEquals(ys[0], 0)


if __name__ == '__main__':
    unittest.main()
