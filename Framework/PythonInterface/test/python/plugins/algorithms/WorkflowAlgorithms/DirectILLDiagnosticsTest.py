# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLDiagnosticsTest(unittest.TestCase):
    _BKG_LEVEL = 1.42
    _EPP_WS_NAME = 'eppWS_'
    _RAW_WS_NAME = 'rawWS_'
    _TEST_WS = None
    _TEST_WS_NAME = 'testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)

    def setUp(self):
        if DirectILLDiagnosticsTest._TEST_WS is None:
            DirectILLDiagnosticsTest._TEST_WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL,
                                                                                          illhelpers.default_test_detectors)
        inWSName = 'inputWS'
        mtd.addOrReplace(inWSName, DirectILLDiagnosticsTest._TEST_WS)
        kwargs = {
            'InputWorkspace': DirectILLDiagnosticsTest._TEST_WS,
            'OutputWorkspace': self._TEST_WS_NAME,
            'EPPCreationMethod': 'Fit EPP',
            'FlatBkg': 'Flat Bkg ON',
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
            'InputWorkspace': self._RAW_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'BeamStopDiagnostics': 'Beam Stop Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        inWS = mtd[self._RAW_WS_NAME]
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), inWS.getNumberHistograms())
        self.assertEqual(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(outWS.getNumberHistograms()):
            self.assertEqual(outWS.readY(i)[0], 0)
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
            'InputWorkspace': self._RAW_WS_NAME,
            'OutputWorkspace': outWSName,
            'ElasticPeakDiagnostics': 'Peak Diagnostics OFF',
            'EPPWorkspace': self._EPP_WS_NAME,
            'BkgDiagnostics': 'Bkg Diagnostics ON',
            'NoisyBkgLowThreshold': 0.01,
            'NoisyBkgHighThreshold': 9.99,
            'BeamStopDiagnostics': 'Beam Stop Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), spectraCount)
        self.assertEqual(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(spectraCount):
            self.assertFalse(spectrumInfo.isMasked(i))
            ys = outWS.readY(i)
            if i in highBkgIndices + lowBkgIndices:
                self.assertEqual(ys[0], 1)
            else:
                self.assertEqual(ys[0], 0)

    def testDirectBeamMasking(self):
        beamWSName = 'beam_masking_ws'
        kwargs = {
            'OutputWorkspace': beamWSName,
            'Function': 'One Peak',
            'rethrow': True
        }
        run_algorithm('CreateSampleWorkspace', **kwargs)
        kwargs = {
            'Workspace': beamWSName,
            'ParameterName': 'beam_stop_diagnostics_spectra',
            'ParameterType': 'String',
            'Value': '43-57, 90-110, 145-155', # Spectrum numbers.
            'rethrow': True
        }
        run_algorithm('SetInstrumentParameter', **kwargs)
        beamWS = mtd[beamWSName]
        # From now on, we work on workspace indices.
        # First range is fully covered by the beam stop.
        for i in range(42, 57):
            ys = beamWS.dataY(i)
            ys *= 0.0
        # Second range is partially covered by the beam stop.
        # Actually, only this range will be recongnized as beam stop's shadow.
        for i in range(92, 105):
            ys = beamWS.dataY(i)
            ys *= 0.0
        # The third range is not covered by the beam stop at all.
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': beamWSName,
            'OutputWorkspace': outWSName,
            'ElasticPeakDiagnostics': 'Peak Diagnostics OFF',
            'BkgDiagnostics': 'Bkg Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), beamWS.getNumberHistograms())
        self.assertEqual(outWS.blocksize(), 1)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i >= 92 and i < 105:
                self.assertEqual(ys[0], 1)
            else:
                self.assertEqual(ys[0], 0)

    def testElasticPeakDiagnostics(self):
        inWS = mtd[self._RAW_WS_NAME]
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
            'InputWorkspace': self._RAW_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'ElasticPeakDiagnostics': 'Peak Diagnostics ON',
            'ElasticPeakLowThreshold': 0.2,
            'ElasticPeakHighThreshold': 9.7,
            'BkgDiagnostics': 'Bkg Diagnostics OFF',
            'BeamStopDiagnostics': 'Beam Stop Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), spectraCount)
        self.assertEqual(outWS.blocksize(), 1)
        spectrumInfo = outWS.spectrumInfo()
        for i in range(spectraCount):
            self.assertFalse(spectrumInfo.isMasked(i))
            ys = outWS.readY(i)
            if i in highPeakIndices + lowPeakIndices:
                self.assertEqual(ys[0], 1)
            else:
                self.assertEqual(ys[0], 0)

    def testMaskedComponents(self):
        inWS = mtd[self._RAW_WS_NAME]
        spectraCount = inWS.getNumberHistograms()
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._RAW_WS_NAME,
            'OutputWorkspace': outWSName,
            'ElasticPeakDiagnostics': 'Peak Diagnostics OFF',
            'BkgDiagnostics': 'Bkg Diagnostics OFF',
            'BeamStopDiagnostics': 'Beam Stop Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'MaskedComponents': 'tube_1',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), spectraCount)
        self.assertEqual(outWS.blocksize(), 1)
        for i in range(spectraCount):
            Ys = outWS.readY(i)
            detector = outWS.getDetector(i)
            componentName = detector.getFullName()
            if 'tube_1' in componentName:
                self.assertEqual(Ys[0], 1)
            else:
                self.assertEqual(Ys[0], 0)

    def testOutputIsUsable(self):
        inWS = mtd[self._RAW_WS_NAME]
        spectraCount = inWS.getNumberHistograms()
        maskedIndices = [0, int(spectraCount / 3), spectraCount - 1]
        for i in maskedIndices:
            ys = inWS.dataY(i)
            ys *= 10.0
        outWSName = 'diagnosticsWS'
        kwargs = {
            'InputWorkspace': self._RAW_WS_NAME,
            'OutputWorkspace': outWSName,
            'EPPWorkspace': self._EPP_WS_NAME,
            'ElasticPeakDiagnostics': 'Peak Diagnostics ON',
            'ElasticPeakLowThreshold': 0.2,
            'ElasticPeakHighThreshold': 9.7,
            'BkgDiagnostics': 'Bkg Diagnostics OFF',
            'BeamStopDiagnostics': 'Beam Stop Diagnostics OFF',
            'DefaultMask': 'Default Mask OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLDiagnostics', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), spectraCount)
        self.assertEqual(outWS.blocksize(), 1)
        kwargs = {
            'Workspace': self._RAW_WS_NAME,
            'MaskedWorkspace': outWSName,
            'rethrow': True
        }
        run_algorithm('MaskDetectors', **kwargs)
        maskedWS = mtd[self._RAW_WS_NAME]
        spectrumInfo = maskedWS.spectrumInfo()
        for i in range(spectraCount):
            if i in maskedIndices:
                self.assertTrue(spectrumInfo.isMasked(i))
            else:
                self.assertFalse(spectrumInfo.isMasked(i))


if __name__ == '__main__':
    unittest.main()
