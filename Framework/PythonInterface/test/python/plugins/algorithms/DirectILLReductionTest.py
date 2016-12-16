from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
import numpy
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


def _createPartialIN5Workspace():
    '''
    Creates an IN5 workspace with monitor and first 149 spectra.
    '''
    emptyWS = LoadEmptyInstrument(InstrumentName='IN5')
    MaskDetectors(Workspace=emptyWS, StartWorkspaceIndex=151)
    emptyWS = RemoveMaskedSpectra(InputWorkspace=emptyWS)
    nHistograms = emptyWS.getNumberHistograms()
    E_i = 23.0
    nBins = 128
    binWidth = 2.63
    elasticIndex = nBins / 3
    monitorElasticIndex = nBins / 2
    xs = numpy.empty(nHistograms*(nBins+1))
    ys = numpy.empty(nHistograms*nBins)
    es = numpy.empty(nHistograms*nBins)
    in5 = emptyWS.getInstrument()
    sample = in5.getSample()
    l1 = sample.getDistance(in5.getSource())
    l2 = float(in5.getStringParameter('l2')[0])
    tofElastic = _timeOfFlight(E_i, l1+l2)
    tofBegin = tofElastic - elasticIndex * binWidth
    monitor = in5.getDetector(0)
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
        trueL2 = sample.getDistance(emptyWS.getDetector(histogramIndex))
        trueTOF = _timeOfFlight(E_i, l1+trueL2)
        fillBins(histogramIndex, trueTOF, elasticPeakHeight, bkgMonitor)
    ws = CreateWorkspace(DataX=xs,
                         DataY=ys,
                         DataE=es,
                         NSpec=nHistograms,
                         ParentWorkspace=emptyWS)
    ws.getAxis(0).setUnit('TOF')
    ws.run().addProperty('Ei', E_i, True)
    ws.run().addProperty('wavelength', float(_wavelength(E_i)), True)
    pulseInterval = tofMonitorDetector + (monitorElasticIndex - elasticIndex) * binWidth
    ws.run().addProperty('pulse_interval', float(pulseInterval * 1e-6), True)
    return ws


class DirectILLReductionTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        for ws in mtd.getObjectNames():
            if mtd.doesExist(ws):
                DeleteWorkspace(Workspace = ws)

    def test_det_diagnostics_no_bad_detectors(self):
        ws = _createPartialIN5Workspace()
        outWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': ws,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '128, 129',
            'OutputDiagnosticsWorkspace': outWSName,
            'child': True,
            'rethrow': True
        }
        alg = run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = alg.getProperty('OutputDiagnosticsWorkspace').value
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         ws.getNumberHistograms() - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)

    def test_det_diagnostics_noisy_background(self):
        ws = _createPartialIN5Workspace()
        nHistograms = ws.getNumberHistograms()
        noisyWSIndices = [1, int(nHistograms / 5), nHistograms - 1]
        for i in noisyWSIndices:
            ys = ws.dataY(i)
            ys *= 2
        outWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': ws,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '128, 129',
            'NoisyBkgDiagnosticsHighThreshold': 1.9,
            'OutputDiagnosticsWorkspace': outWSName,
            'child': True,
            'rethrow': True
        }
        alg = run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = alg.getProperty('OutputDiagnosticsWorkspace').value
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in noisyWSIndices:
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0)
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)

    def test_det_diagnostics_bad_elastic_intensity(self):
        ws = _createPartialIN5Workspace()
        nHistograms = ws.getNumberHistograms()
        noPeakIndices = [1, int(nHistograms / 6)]
        highPeakIndices = [int(nHistograms / 3), nHistograms - 1]
        for i in noPeakIndices:
            ys = ws.dataY(i)
            ys *= 0.01
        for i in highPeakIndices:
            ys = ws.dataY(i)
            ys *= 5
        outWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': ws,
            'ReductionType': 'Empty Container/Cadmium',
            'IndexType': 'Detector ID',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '128, 129',
            'ElasticPeakDiagnosticsLowThreshold': 0.1,
            'ElasticPeakDiagnosticsHighThreshold': 4.9,
            'OutputDiagnosticsWorkspace': outWSName,
            'child': True,
            'rethrow': True
        }
        alg = run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = alg.getProperty('OutputDiagnosticsWorkspace').value
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        shouldBeMasked = noPeakIndices + highPeakIndices
        print('These should be marked: {}'.format(shouldBeMasked))
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in shouldBeMasked:
                print('Marked index: {}'.format(i))
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0)
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)


if __name__ == '__main__':
    unittest.main()
