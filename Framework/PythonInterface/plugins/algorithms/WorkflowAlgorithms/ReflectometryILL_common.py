# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.kernel import UnitConversion, DeltaEModeType
from mantid.simpleapi import CreateWorkspace, Multiply
import scipy.constants as constants


def chopperOpeningAngle(sampleLogs, instrumentName):
    """Return the chopper opening angle in degrees."""
    if instrumentName == 'D17':
        chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper1_phase_average').value
        if chopper1Phase > 360.:
            # Workaround for broken old D17 NeXus files.
            chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper2_speed_average').value
        chopper2Phase = sampleLogs.getProperty('VirtualChopper.chopper2_phase_average').value
        openoffset = sampleLogs.getProperty('VirtualChopper.open_offset').value
        return 45. - (chopper2Phase - chopper1Phase) - openoffset
    else:
        firstChopper = int(sampleLogs.getProperty('ChopperSetting.firstChopper').value)
        secondChopper = int(sampleLogs.getProperty('ChopperSetting.secondChopper').value)
        phase1Entry = 'CH{}.phase'.format(firstChopper)
        phase2Entry = 'CH{}.phase'.format(secondChopper)
        chopper1Phase = sampleLogs.getProperty(phase1Entry).value
        chopper2Phase = sampleLogs.getProperty(phase2Entry).value
        if chopper1Phase > 360.:
            # CH1.phase on FIGARO is set to an arbitrary value (999.9)
            chopper1Phase = 0.
        if sampleLogs.hasProperty('CollAngle.open_offset'):
            openoffset = sampleLogs.getProperty('CollAngle.open_offset').value
        else:
            openoffset = sampleLogs.getProperty('CollAngle.openOffset').value
        return 45. - (chopper2Phase - chopper1Phase) - openoffset


def chopperPairDistance(sampleLogs, instrumentName):
    """Return the gap between the two choppers."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('Distance.ChopperGap').value * 1e-2
    else:
        return sampleLogs.getProperty('ChopperSetting.distSeparationChopperPair').value * 1e-3


def chopperSpeed(sampleLogs, instrumentName):
    """Return the chopper speed."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('VirtualChopper.chopper1_speed_average').value
    else:
        firstChopper = int(sampleLogs.getProperty('ChopperSetting.firstChopper').value)
        speedEntry = 'CH{}.rotation_speed'.format(firstChopper)
        return sampleLogs.getProperty(speedEntry).value


def correctForChopperOpenings(ws, directWS, names, cleanup, logging):
    """Correct reflectivity values if chopper openings between RB and DB differ."""
    def opening(instrumentName, logs, Xs):
        chopperGap = chopperPairDistance(logs, instrumentName)
        chopperPeriod = 60. / chopperSpeed(logs, instrumentName)
        openingAngle = chopperOpeningAngle(logs, instrumentName)
        return chopperGap * constants.m_n / constants.h / chopperPeriod * Xs * 1e-10 + openingAngle / 360.
    instrumentName = ws.getInstrument().getName()
    Xs = ws.readX(0)
    if ws.isHistogramData():
        Xs = (Xs[:-1] + Xs[1:]) / 2.
    reflectedOpening = opening(instrumentName, ws.run(), Xs)
    directOpening = opening(instrumentName, directWS.run(), Xs)
    corFactorWSName = names.withSuffix('chopper_opening_correction_factors')
    corFactorWS = CreateWorkspace(
        OutputWorkspace=corFactorWSName,
        DataX=ws.readX(0),
        DataY= directOpening / reflectedOpening,
        UnitX=ws.getAxis(0).getUnit().unitID(),
        ParentWorkspace=ws,
        EnableLogging=logging)
    correctedWSName = names.withSuffix('corrected_by_chopper_opening')
    correctedWS = Multiply(
        LHSWorkspace=ws,
        RHSWorkspace=corFactorWS,
        OutputWorkspace=correctedWSName,
        EnableLogging=logging)
    cleanup.cleanup(corFactorWS)
    cleanup.cleanup(ws)
    return correctedWS


def detectorResolution():
    """Return the detector resolution in mm."""
    return 0.0022


def pixelSize(instrumentName):
    """Return the pixel size in mm."""
    return 0.001195 if instrumentName == 'D17' else 0.0012


def deflectionAngle(sampleLogs):
    """Return the deflection angle in degree."""
    if sampleLogs.hasProperty('CollAngle.actual_coll_angle'):
        # Must be FIGARO
        return sampleLogs.getProperty('CollAngle.actual_coll_angle').value
    else:
        return 0.0


def slitSizeLogEntry(instrumentName, slitNumber):
    """Return the sample log entry which contains the slit size for the given slit"""
    if slitNumber not in [1, 2]:
        raise RuntimeError('Slit number out of range.')
    entry = 'VirtualSlitAxis.s{}w_actual_width' if instrumentName == 'D17' else 'VirtualSlitAxis.S{}H_actual_height'
    return entry.format(slitNumber + 1)


def inTOF(value, l1, l2):
    """Return the number (tof) converted to wavelength"""
    return UnitConversion.run('Wavelength', 'TOF', value, l1, l2, 0., DeltaEModeType.Elastic, 0.)


def instrumentName(ws):
    """Return the instrument's name validating it is either D17 or FIGARO."""
    name = ws.getInstrument().getName()
    if name != 'D17' and name != 'FIGARO':
        raise RuntimeError('Unrecognized instrument {}. Only D17 and FIGARO are supported.'.format(name))
    return name


class SampleLogs:
    FOREGROUND_CENTRE = 'reduction.foreground.centre_workspace_index'
    FOREGROUND_END = 'reduction.foreground.last_workspace_index'
    FOREGROUND_START = 'reduction.foreground.first_workspace_index'
    LINE_POSITION = 'reduction.line_position'
    SUM_TYPE = 'reduction.foreground.summation_type'
    TWO_THETA = 'loader.two_theta'
