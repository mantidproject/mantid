from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (DirectILLDetectorDiagnostics, DirectILLPrepareData)
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLDetectorDiagnosticsTest(unittest.TestCase):
    _BKG_LEVEL = 1.42
    _BKG_WS_NAME = 'bkgWS_'
    _EPP_WS_NAME = 'eppWS_'
    _TEST_WS_NAME = 'testWS_'

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
            'IndexType': 'Detector ID',
            'Monitor': 0,
            'DetectorsAtL2': '130, 390',
            'OutputEPPWorkspace': self._EPP_WS_NAME,
            'OutputFlatBkgWorkspace': self._BKG_WS_NAME,
        }
        run_algorithm('DirectILLPrepareData', **kwargs)
        mtd.remove(inWSName)

    def tearDown(self):
        mtd.clear()

    def testAllDetectorsPass(self):
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'IndexType': 'Detector ID',
            'DetectorsAtL2': '130, 390',
            'FlatBkgWorkspace': self._BKG_WS_NAME,
            'rethrow': True
        }
        run_algorithm('DirectILLDetectorDiagnostics', **kwargs)
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
        bkgWS = mtd[self._BKG_WS_NAME]
        spectraCount = bkgWS.getNumberHistograms()
        highBkgIndices = [0, int(spectraCount / 3), spectraCount - 1]
        for i in highBkgIndices:
            ys = bkgWS.dataY(i)
            ys *= 10.0
        lowBkgIndices = [int(spectraCount / 4), int(2 * spectraCount / 3)]
        for i in lowBkgIndices:
            ys = bkgWS.dataY(i)
            ys *= 0
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'IndexType': 'Detector ID',
            'DetectorsAtL2': '130, 390',
            'FlatBkgWorkspace': self._BKG_WS_NAME,
            'NoisyBkgDiagnosticsLowThreshold': 0.01,
            'NoisyBkgDiagnosticsHighThreshold': 9.99,
            'rethrow': True
        }
        run_algorithm('DirectILLDetectorDiagnostics', **kwargs)
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
            'IndexType': 'Detector ID',
            'DetectorsAtL2': '130, 390',
            'FlatBkgWorkspace': self._BKG_WS_NAME,
            'ElasticPeakDiagnosticsLowThreshold': 0.2,
            'ElasticPeakDiagnosticsHighThreshold': 9.7,
            'rethrow': True
        }
        run_algorithm('DirectILLDetectorDiagnostics', **kwargs)
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
