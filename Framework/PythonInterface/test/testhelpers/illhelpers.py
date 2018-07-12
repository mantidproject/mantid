from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.kernel import DeltaEModeType, UnitConversion
import numpy
from testhelpers import create_algorithm, run_algorithm


def _gaussian(x, height, x0, sigma):
    """Return a point in the gaussian curve."""
    x = x - x0
    sigma2 = 2 * sigma * sigma
    return height * numpy.exp(- x * x / sigma2)


def _fillTemplateReflectometryWorkspace(ws):
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
    ws.getAxis(0).setUnit('TOF')
    kwargs = {
        'Workspace': ws,
        'LogName': 'time',
        'LogText': str(3600),
        'LogType': 'Number',
        'LogUnit': 'Sec',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'det.value',
        'LogText': str(3100),
        'LogType': 'Number',
        'LogUnit': 'mm',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'Distance.ChopperGap',
        'LogText': str(8.2),
        'LogType': 'Number',
        'LogUnit': 'cm',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'PSD.time_of_flight_0',
        'LogText': str(binWidth),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
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


def add_duration(ws, duration):
    kwargs = {
        'Workspace': ws,
        'LogName': 'duration',
        'LogText': str(float(duration)),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)


def add_chopper_configuration_D17(ws):
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualChopper.chopper1_phase_average',
        'LogText': str(180),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualChopper.chopper1_speed_average',
        'LogText': str(1000),
        'LogType': 'Number',
        'LogUnit': 'rpm',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualChopper.chopper2_phase_average',
        'LogText': str(225),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualChopper.open_offset',
        'LogText': str(-0.055),
        'LogType': 'Number',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)


def add_flipper_configuration_D17(ws, flipper1, flipper2):
    kwargs = {
        'Workspace': ws,
        'LogName': 'Flipper1.stateint',
        'LogText': str(int(flipper1)),
        'LogType': 'Number',
        'NumberType': 'Int',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'Flipper2.stateint',
        'LogText': str(int(flipper2)),
        'LogType': 'Number',
        'NumberType': 'Int',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)


def add_slit_configuration_D17(ws, slit2Width, slit3Width):
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualSlitAxis.s2w_actual_width',
        'LogText': str(float(slit2Width)),
        'LogType': 'Number',
        'LogUnit': 'mm',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)
    kwargs = {
        'Workspace': ws,
        'LogName': 'VirtualSlitAxis.s3w_actual_width',
        'LogText': str(float(slit3Width)),
        'LogType': 'Number',
        'LogUnit': 'mm',
        'NumberType': 'Double',
        'child': True
    }
    run_algorithm('AddSampleLog', **kwargs)


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
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    return ws


def refl_create_beam_position_ws(beamPosWSName, referenceWS, detectorAngle, beamCentre):
    args = {
        'OutputWorkspace': beamPosWSName
    }
    alg = create_algorithm('CreateEmptyTableWorkspace', **args)
    alg.execute()
    beamPos = mtd[beamPosWSName]
    beamPos.addColumn('double', 'DetectorAngle')
    beamPos.addColumn('double', 'DetectorDistance')
    beamPos.addColumn('double', 'PeakCentre')
    L2 = referenceWS.getInstrument().getComponentByName('detector').getPos().norm()
    beamPos.addRow((detectorAngle, L2, beamCentre))
    return beamPos


def refl_preprocess(outputWSName, ws, beamPosWS):
    args = {
        'InputWorkspace': ws,
        'BeamPositionWorkspace': beamPosWS,
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


def refl_sum_in_lambda(outputWSName, ws):
    args = {
        'InputWorkspace': ws,
        'OutputWorkspace': outputWSName,
        'SummationType': 'SumInLambda',
        'WavelengthRange': [0.1]
    }
    alg = create_algorithm('ReflectometryILLSumForeground', **args)
    alg.execute()
    return mtd[outputWSName]
