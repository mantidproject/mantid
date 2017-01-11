from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (CloneWorkspace, CreateWorkspace, DeleteWorkspace,
                              LoadEmptyInstrument, MaskDetectors, mtd,
                              RemoveMaskedSpectra)
import numpy
import numpy.testing
from scipy import constants
from testhelpers import run_algorithm
import unittest


def _timeOfFlight(energy, length):
    velocity = numpy.sqrt(2 * energy * 1e-3 * constants.e / constants.m_n)
    return length / velocity * 1e6


def _wavelength(energy):
    velocity = numpy.sqrt(2 * energy * 1e-3 * constants.e / constants.m_n)
    return constants.h / (velocity * constants.m_n) * 1e10


def _gaussian(x, height, x0, sigma):
    x = x - x0
    sigma2 = 2 * sigma * sigma
    return height * numpy.exp(- x * x / sigma2)


def _fillTemplateWorkspace(templateWS):
    '''
    Fills a workspace with somewhat sane data.
    '''
    nHistograms = templateWS.getNumberHistograms()
    E_i = 23.0
    nBins = 128
    binWidth = 2.63
    elasticIndex = nBins / 3
    monitorElasticIndex = nBins / 2
    xs = numpy.empty(nHistograms*(nBins+1))
    ys = numpy.empty(nHistograms*nBins)
    es = numpy.empty(nHistograms*nBins)
    instrument = templateWS.getInstrument()
    sample = instrument.getSample()
    l1 = sample.getDistance(instrument.getSource())
    l2 = float(instrument.getStringParameter('l2')[0])
    tofElastic = _timeOfFlight(E_i, l1+l2)
    tofBegin = tofElastic - elasticIndex * binWidth
    monitor = instrument.getDetector(0)
    monitorSampleDistance = sample.getDistance(monitor)
    tofElasticMonitor = tofBegin + monitorElasticIndex * binWidth
    tofMonitorDetector = _timeOfFlight(E_i, monitorSampleDistance+l2)
    elasticPeakSigma = nBins * binWidth * 0.03
    elasticPeakHeight = 1723.0
    bkg = 2
    bkgMonitor = 1

    def fillBins(histogramIndex, elasticTOF, elasticPeakHeight, bkg):
        xIndexOffset = histogramIndex*(nBins+1)
        yIndexOffset = histogramIndex*nBins
        xs[xIndexOffset] = tofBegin - binWidth / 2
        for binIndex in range(nBins):
            x = tofBegin + binIndex * binWidth
            xs[xIndexOffset+binIndex+1] = x + binWidth / 2
            y = round(_gaussian(x, elasticPeakHeight, elasticTOF,
                                elasticPeakSigma)) + bkg
            ys[yIndexOffset+binIndex] = y
            es[yIndexOffset+binIndex] = numpy.sqrt(y)

    fillBins(0, tofElasticMonitor, 1623 * elasticPeakHeight, bkgMonitor)
    for histogramIndex in range(1, nHistograms):
        trueL2 = sample.getDistance(templateWS.getDetector(histogramIndex))
        trueTOF = _timeOfFlight(E_i, l1+trueL2)
        fillBins(histogramIndex, trueTOF, elasticPeakHeight, bkg)
    ws = CreateWorkspace(DataX=xs,
                         DataY=ys,
                         DataE=es,
                         NSpec=nHistograms,
                         ParentWorkspace=templateWS)
    ws.getAxis(0).setUnit('TOF')
    ws.run().addProperty('Ei', E_i, True)
    ws.run().addProperty('wavelength', float(_wavelength(E_i)), True)
    pulseInterval = \
        tofMonitorDetector + (monitorElasticIndex - elasticIndex) * binWidth
    ws.run().addProperty('pulse_interval', float(pulseInterval * 1e-6), True)
    return ws


class DirectILLReductionTest(unittest.TestCase):

    _TEMPLATE_IN5_WS_NAME = '__IN5templateWS'

    def setUp(self):
        self._testIN5WS = CloneWorkspace(InputWorkspace=self._in5WStemplate,
                                         OutputWorkspace='__IN5testWS')

    @classmethod
    def setUpClass(cls):
        cls._in5WStemplate = \
            LoadEmptyInstrument(InstrumentName='IN5',
                                OutputWorkspace=cls._TEMPLATE_IN5_WS_NAME)
        mask = list()
        for i in range(513):
            if i % 10 != 0:
                mask.append(i)
        MaskDetectors(Workspace=cls._in5WStemplate, DetectorList=mask)
        MaskDetectors(Workspace=cls._in5WStemplate, StartWorkspaceIndex=512)
        cls._in5WStemplate = \
            RemoveMaskedSpectra(InputWorkspace=cls._in5WStemplate,
                                OutputWorkspace=cls._in5WStemplate)
        cls._in5WStemplate = _fillTemplateWorkspace(cls._in5WStemplate)

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_det_diagnostics_bad_elastic_intensity(self):
        nHistograms = self._testIN5WS.getNumberHistograms()
        noPeakIndices = [1, int(nHistograms / 6)]
        highPeakIndices = [int(nHistograms / 3), nHistograms - 1]
        for i in noPeakIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 0.01
        for i in highPeakIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 5
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnosticsWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'ElasticPeakDiagnosticsLowThreshold': 0.1,
            'ElasticPeakDiagnosticsHighThreshold': 4.8,
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName], diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        shouldBeMasked = noPeakIndices + highPeakIndices
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in shouldBeMasked:
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0,
                                 ('Detector at ws index {0} should be ' +
                                  'masked').format(i))
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWS)

    def test_det_diagnostics_no_bad_detectors(self):
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName], diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         self._testIN5WS.getNumberHistograms() - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWS)

    def test_det_diagnostics_noisy_background(self):
        nHistograms = self._testIN5WS.getNumberHistograms()
        noisyWSIndices = [1, int(nHistograms / 5), nHistograms - 1]
        for i in noisyWSIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 2
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'NoisyBkgDiagnosticsHighThreshold': 1.9,
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName], diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in noisyWSIndices:
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0)
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWSName)

    def test_det_diagnostics_output_when_disabled(self):
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnosticsWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertFalse(mtd.doesExist(diagnosticsWSName))

    def test_input_ws_not_deleted(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'Cleanup': 'Delete Intermediate Workspaces',
            'ReductionType': 'Empty Container/Cadmium',
            'Normalisation': 'No Normalisation',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        try:
            self._testIN5WS.getNumberHistograms()
        except RuntimeError:
            self.fail('Workspace used as InputWorkspace has been deleted.')
        finally:
            DeleteWorkspace(outWSName)

    def test_rebinning_manual_mode(self):
        rebinningBegin = -2.3
        rebinningEnd = 4.2
        binWidth = 0.66
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'Reductiontype': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Manual Rebinning',
            'EnergyRebinningParams': '{0}, {1}, {2}'.format(rebinningBegin,
                                                            binWidth,
                                                            rebinningEnd),
            'QRebinningParams' : '0, 0.1, 10.0',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self._checkAlgorithmsInHistory(ws, 'Rebin')
        for i in range(ws.getNumberHistograms()):
            xs = ws.readX(i)
            self.assertAlmostEqual(xs[0], rebinningBegin)
            self.assertAlmostEqual(xs[-1], rebinningEnd)
            for j in range(len(xs) - 2):
                # Skip the last bin which is smaller.
                self.assertAlmostEqual(xs[j+1] - xs[j], binWidth)

    def test_rebinning_manul_mode_fails_without_params(self):
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'Reductiontype': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Manual Rebinning',
            'QRebinningParams' : '0, 0.1, 10.0',
            'child': True,
            'rethrow': True
        }
        self.assertRaises(RuntimeError, run_algorithm, 'DirectILLReduction',
                          **algProperties)

    def test_rebinning_to_elastic_bin_width(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Bin Width at Elastic Peak',
            'QRebinningParams' : '0, 0.1, 10.0',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self._checkAlgorithmsInHistory(ws, 'BinWidthAtX', 'Rebin')
        binWidth = ws.readX(0)[1] - ws.readX(0)[0]
        xs = ws.extractX()[:, :-1]  # The last bin is smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), binWidth, decimal=5)
        DeleteWorkspace(ws)

    def test_rebinning_to_median_bin_width(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Median Bin Width',
            'QRebinningParams' : '0, 0.1, 10.0',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self._checkAlgorithmsInHistory(ws, 'MedianBinWidth', 'Rebin')
        binWidth = ws.readX(0)[1] - ws.readX(0)[0]
        xs = ws.extractX()[:, :-1]  # The last bin is smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), binWidth, decimal=5)
        DeleteWorkspace(ws)

    def test_user_mask(self):
        numHistograms = self._testIN5WS.getNumberHistograms()
        userMask = [0, int(3 * numHistograms / 5), numHistograms - 2]
        maskString = '{0},'.format(userMask[0])
        for i in userMask:
            maskString += str(i) + ','
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'MaskedDetectors': maskString,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        outWS = mtd[outWSName]
        self._checkAlgorithmsInHistory(outWS, 'MaskDetectors')
        for i in range(outWS.getNumberHistograms()):
            if i in userMask:
                self.assertTrue(outWS.getDetector(i).isMasked())
            else:
                self.assertFalse(outWS.getDetector(i).isMasked())
        DeleteWorkspace(outWSName)

    def test_component_mask(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'MaskedComponents': 'tube_1',  # Mask workspace indices 0-31
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        outWS = mtd[outWSName]
        self._checkAlgorithmsInHistory(outWS, 'MaskDetectors')
        nHistograms = outWS.getNumberHistograms()
        for i in range(int(nHistograms / 2)):
            self.assertTrue(outWS.getDetector(i).isMasked())
        for i in range(int(nHistograms / 2), nHistograms):
            self.assertFalse(outWS.getDetector(i).isMasked())
        DeleteWorkspace(outWSName)

    def _checkAlgorithmsInHistory(self, ws, *arg):
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        algHistories = reductionHistory.getChildHistories()
        algNames = [alg.name() for alg in algHistories]
        for algName in arg:
            self.assertTrue(algName in algNames)

    def _checkDiagnosticsAlgorithmsInHistory(self, outWS, diagnosticsWS):
        self._checkAlgorithmsInHistory(outWS, 'MedianDetectorTest',
                                       'MaskDetectors')
        self._checkAlgorithmsInHistory(diagnosticsWS, 'MedianDetectorTest',
                                       'ClearMaskFlag', 'Plus')

if __name__ == '__main__':
    unittest.main()
