from __future__ import (absolute_import, division, print_function)

from mantid.kernel import DeltaEModeType, UnitConversion
import numpy
from testhelpers import run_algorithm


def _gaussian(x, height, x0, sigma):
    """Return a point in the gaussian curve."""
    x = x - x0
    sigma2 = 2 * sigma * sigma
    return height * numpy.exp(- x * x / sigma2)


def _fillTemplateWorkspace(templateWS, bkgLevel):
    """Fill a workspace with somewhat sane data."""
    nHistograms = templateWS.getNumberHistograms()
    E_i = 23.0
    nBins = 128
    binWidth = 2.63
    elasticIndex = int(nBins / 3)
    monitorElasticIndex = int(nBins / 2)
    xs = numpy.empty(nHistograms*(nBins+1))
    ys = numpy.empty(nHistograms*nBins)
    es = numpy.empty(nHistograms*nBins)
    instrument = templateWS.getInstrument()
    sample = instrument.getSample()
    l1 = sample.getDistance(instrument.getSource())
    l2 = float(instrument.getStringParameter('l2')[0])
    tofElastic = UnitConversion.run('Energy', 'TOF', E_i, l1, l2, 0.0, DeltaEModeType.Direct, 0.0)
    tofBegin = tofElastic - elasticIndex * binWidth
    monitor = instrument.getDetector(0)
    monitorSampleDistance = sample.getDistance(monitor)
    tofElasticMonitor = tofBegin + monitorElasticIndex * binWidth
    tofMonitorDetector = UnitConversion.run('Energy', 'TOF', E_i, monitorSampleDistance, l2, 0.0,
                                            DeltaEModeType.Direct, 0.0)
    elasticPeakSigma = nBins * binWidth * 0.03
    elasticPeakHeight = 1723.0
    bkgMonitor = 1

    def fillBins(histogramIndex, elasticTOF, elasticPeakHeight, bkgLevel):
        xIndexOffset = histogramIndex*(nBins+1)
        yIndexOffset = histogramIndex*nBins
        xs[xIndexOffset] = tofBegin - binWidth / 2
        for binIndex in range(nBins):
            x = tofBegin + binIndex * binWidth
            xs[xIndexOffset+binIndex+1] = x + binWidth / 2
            y = round(_gaussian(x, elasticPeakHeight, elasticTOF,
                                elasticPeakSigma)) + bkgLevel
            ys[yIndexOffset+binIndex] = y
            es[yIndexOffset+binIndex] = numpy.sqrt(y)

    fillBins(0, tofElasticMonitor, 1623 * elasticPeakHeight, bkgMonitor)
    for histogramIndex in range(1, nHistograms):
        trueL2 = sample.getDistance(templateWS.getDetector(histogramIndex))
        trueTOF = UnitConversion.run('Energy', 'TOF', E_i, l1, trueL2, 0.0, DeltaEModeType.Direct, 0.0)
        fillBins(histogramIndex, trueTOF, elasticPeakHeight, bkgLevel)
    kwargs = {
        'DataX': xs,
        'DataY': ys,
        'DataE': es,
        'NSpec': nHistograms,
        'ParentWorkspace': templateWS,
        'child': True
    }
    alg = run_algorithm('CreateWorkspace', **kwargs)
    ws = alg.getProperty('OutputWorkspace').value
    ws.getAxis(0).setUnit('TOF')
    kwargs = {
        'Workspace': ws,
        'LogName': 'Ei',
        'LogText': str(E_i),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    wavelength = UnitConversion.run('Energy', 'Wavelength', E_i, l1, l2, 0.0, DeltaEModeType.Direct, 0.0)
    kwargs = {
        'Workspace': ws,
        'LogName': 'wavelength',
        'LogText': str(float(wavelength)),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    pulseInterval = \
        tofMonitorDetector + (monitorElasticIndex - elasticIndex) * binWidth
    kwargs = {
        'Workspace': ws,
        'LogName': 'pulse_interval',
        'LogText': str(float(pulseInterval * 1e-6)),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'Detector.elasticpeak',
        'LogText': str(elasticIndex),
        'LogType': 'Number',
        'NumberType': 'Int',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'ParameterName': 'default-incident-monitor-spectrum',
        'ParameterType': 'Number',
        'Value': '1',
        'child': True
    }
    run_algorithm('SetInstrumentParameter', **kwargs)
    return ws


def create_poor_mans_in5_workspace(bkgLevel, removeDetectors):
    kwargs = {
        'InstrumentName': 'IN5',
        'child': True
    }
    alg = run_algorithm('LoadEmptyInstrument', **kwargs)
    ws = removeDetectors(alg.getProperty('OutputWorkspace').value)
    kwargs = {
        'InputWorkspace': ws,
        'child': True
    }
    alg = run_algorithm('RemoveMaskedSpectra', **kwargs)
    ws = alg.getProperty('OutputWorkspace').value
    ws = _fillTemplateWorkspace(ws, bkgLevel)
    return ws


def default_test_detectors(ws):
    mask = list()
    for i in range(513):
        if i % 10 != 0:
            mask.append(i)
    kwargs = {
        'Workspace': ws,
        'DetectorList': mask,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    kwargs = {
        'Workspace': ws,
        'StartWorkspaceIndex': 512,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    return ws
