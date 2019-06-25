# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.kernel import DeltaEModeType, UnitConversion
import numpy
from testhelpers import create_algorithm, run_algorithm
import ReflectometryILL_common as common


def _gaussian(x, height, x0, sigma):
    """Return a point in the gaussian curve."""
    x = x - x0
    sigma2 = 2 * sigma * sigma
    return height * numpy.exp(- x * x / sigma2)


def _fillTemplateReflectometryWorkspace(ws, XUnit='TOF'):
    """Fill a reflectometry workspace with somewhat sane data."""
    nHistograms = ws.getNumberHistograms()
    binWidth = 57.
    templateXs = numpy.array(numpy.arange(-300., 55000., binWidth))
    nBins = len(templateXs) - 1
    xs = numpy.tile(templateXs, nHistograms)
    ys = numpy.zeros(nHistograms*nBins)
    es = numpy.zeros(nHistograms*nBins)
    kwargs = {
        'OutputWorkspace': 'unused_',
        'DataX': xs,
        'DataY': ys,
        'DataE': es,
        'NSpec': nHistograms,
        'ParentWorkspace': ws,
        'child': True,
        'rethrow': True
    }
    alg = run_algorithm('CreateWorkspace', **kwargs)
    ws = alg.getProperty('OutputWorkspace').value
    ws.getAxis(0).setUnit(XUnit)
    run = ws.run()
    run.addProperty('time', 3600, 'Sec', True)
    run.addProperty('det.value', 3100, 'mm', True)
    run.addProperty('Distance.ChopperGap', 8.2, 'cm', True)
    run.addProperty('PSD.time_of_flight_0', float(binWidth), True)
    return ws


def _fillTemplateTOFWorkspace(templateWS, bkgLevel):
    """Fill a TOF workspace with somewhat sane data."""
    nHistograms = templateWS.getNumberHistograms()
    E_i = 23.0
    nBins = 128
    binWidth = 2.63
    elasticIndex = int(nBins / 3)
    monitorElasticIndex = int(nBins / 2)
    xs = numpy.empty(nHistograms*(nBins+1))
    ys = numpy.empty(nHistograms*nBins)
    es = numpy.empty(nHistograms*nBins)
    spectrumInfo = templateWS.spectrumInfo()
    instrument = templateWS.getInstrument()
    l1 = spectrumInfo.l1()
    l2 = float(instrument.getStringParameter('l2')[0])
    tofElastic = UnitConversion.run('Energy', 'TOF', E_i, l1, l2, 0.0, DeltaEModeType.Direct, 0.0)
    tofBegin = tofElastic - elasticIndex * binWidth
    monitorSampleDistance = 0.5
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

    for histogramIndex in range(0, nHistograms - 1):
        trueL2 = spectrumInfo.l2(histogramIndex)
        trueTOF = UnitConversion.run('Energy', 'TOF', E_i, l1, trueL2, 0.0, DeltaEModeType.Direct, 0.0)
        fillBins(histogramIndex, trueTOF, elasticPeakHeight, bkgLevel)
    fillBins(nHistograms - 1, tofElasticMonitor, 1623 * elasticPeakHeight, bkgMonitor)
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
    run = ws.run()
    run.addProperty('Ei', float(E_i), True)
    wavelength = UnitConversion.run('Energy', 'Wavelength', E_i, l1, l2, 0.0, DeltaEModeType.Direct, 0.0)
    run.addProperty('wavelength', float(wavelength), True)
    pulseInterval = \
        tofMonitorDetector + (monitorElasticIndex - elasticIndex) * binWidth
    run.addProperty('pulse_interval', float(pulseInterval * 1e-6), True)
    run.addProperty('Detector.elasticpeak', int(elasticIndex), True)
    kwargs = {
        'Workspace': ws,
        'LogName': 'monitor.monsum',
        'LogText': str(1000),
        'LogType': 'Number',
        'NumberType': 'Int',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'ParameterName': 'default-incident-monitor-spectrum',
        'ParameterType': 'Number',
        'Value': str(98305),
        'child': True
    }
    run_algorithm('SetInstrumentParameter', **kwargs)
    return ws


def add_duration(ws, duration):
    ws.run().addProperty('duration', float(duration), 'Sec', True)


def add_chopper_configuration_D17(ws):
    run = ws.run()
    run.addProperty('VirtualChopper.chopper1_phase_average', 180, True)
    run.addProperty('VirtualChopper.chopper1_speed_average', 1000, True)
    run.addProperty('VirtualChopper.chopper2_phase_average', 225, True)
    run.addProperty('VirtualChopper.open_offset', -0.055, True)


def add_flipper_configuration_D17(ws, flipper1, flipper2):
    run = ws.run()
    run.addProperty('Flipper1.stateint', int(flipper1), True)
    run.addProperty('Flipper2.stateint', int(flipper2), True)


def add_slit_configuration_D17(ws, slit2Width, slit3Width):
    run = ws.run()
    run.addProperty('VirtualSlitAxis.s2w_actual_width', float(slit2Width), 'mm', True)
    run.addProperty('VirtualSlitAxis.s3w_actual_width', float(slit3Width), 'mm', True)


def create_poor_mans_d17_workspace():
    kwargs = {
        'InstrumentName': 'D17',
        'child': True
    }
    alg = run_algorithm('LoadEmptyInstrument', **kwargs)
    ws = alg.getProperty('OutputWorkspace').value
    ws = _fillTemplateReflectometryWorkspace(ws)
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
    ws = _fillTemplateTOFWorkspace(ws, bkgLevel)
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
        'EndWorkspaceIndex': ws.getNumberHistograms() - 2,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    return ws


def refl_add_line_position(ws, linePosition):
    ws.run().addProperty(common.SampleLogs.LINE_POSITION, float(linePosition), True)
    return ws


def refl_add_two_theta(ws, twoTheta):
    ws.run().addProperty(common.SampleLogs.TWO_THETA, float(twoTheta), 'degree', True)
    return ws


def refl_preprocess(outputWSName, ws):
    args = {
        'InputWorkspace': ws,
        'OutputWorkspace': outputWSName,
    }
    alg = create_algorithm('ReflectometryILLPreprocess', **args)
    alg.execute()
    return mtd[outputWSName]


def refl_preprocess_with_calibration(outputWSName, ws, directLineWS):
    args = {
        'InputWorkspace': ws,
        'DirectLineWorkspace': directLineWS,
        'OutputWorkspace': outputWSName,
    }
    alg = create_algorithm('ReflectometryILLPreprocess', **args)
    alg.execute()
    return mtd[outputWSName]


def refl_rotate_detector(ws, angle):
    r = ws.run().getProperty('det.value').value * 1e-3
    angle = numpy.deg2rad(angle)
    z = r * numpy.cos(angle)
    y = r * numpy.sin(angle)
    args = {
        'Workspace': ws,
        'ComponentName': 'detector',
        'X': 0.,
        'Y': y,
        'Z': z,
        'RelativePosition': False
    }
    run_algorithm('MoveInstrumentComponent', **args)
    args = {
        'Workspace': ws,
        'ComponentName': 'detector',
        'X': 1.,
        'Y': 0.,
        'Z': 0.,
        'Angle': numpy.rad2deg(angle),
        'RelativeRotation': False
    }
    run_algorithm('RotateInstrumentComponent', **args)


def refl_sum_foreground(outputWSName, sumType, ws, dirFgdWS=None, dirWS=None):
    args = {
        'InputWorkspace': ws,
        'OutputWorkspace': outputWSName,
        'SummationType': sumType,
        'DirectForegroundWorkspace': dirFgdWS,
        'DirectLineWorkspace': dirWS,
        'WavelengthRange': [0.1]
    }
    alg = create_algorithm('ReflectometryILLSumForeground', **args)
    alg.execute()
    return mtd[outputWSName]
