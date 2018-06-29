# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (CreateWorkspace, DeleteWorkspace, mtd, Multiply)
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
        firstChopper = sampleLogs.getProperty('ChopperSettings.firstChopper').value
        secondChopper = sampleLogs.getProperty('ChopperSettings.secondChopper').value
        phase1Entry = 'CH{}.phase'.format(firstChopper)
        phase2Entry = 'CH{}.phase'.format(secondChopper)
        chopper1Phase = sampleLogs.getProperty(phase1Entry).value
        chopper2Phase = sampleLogs.getProperty(phase2Entry).value
        openoffset = sampleLogs.getProperty('CollAngle.openOffset').value
        return 45. - (chopper2Phase - chopper1Phase) - openoffset


def chopperPairDistance(sampleLogs, instrumentName):
    """Return the gap between the two choppers."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('Distance.ChopperGap').value * 1e-2
    else:
        return sampleLogs.getProperty('ChopperSettings.distSeparationChopperPair').value * 1e-2


def chopperSpeed(sampleLogs, instrumentName):
    """Return the chopper speed."""
    if instrumentName == 'D17':
        return sampleLogs.getProperty('VirtualChopper.chopper1_speed_average').value
    else:
        firstChopper = sampleLogs.getProperty('ChopperSettings.firstChopper').value
        speedEntry = 'CH{}.rotation_speed'.format(firstChopper)
        return sampleLogs.getProperty(speedEntry).value


def correctForChopperOpenings(ws, directWS, names, cleanup, logging):
    """Correct reflectivity values if chopper openings between RB and DB differ."""
    def opening(instrumentName, logs, Xs):
        chopperGap = chopperPairDistance(logs, instrumentName)
        chopperPeriod = 60. / chopperSpeed(logs, instrumentName)
        openingAngle = chopperOpeningAngle(logs, instrumentName)
        return chopperGap * constants.m_n / constants.h / chopperPeriod * Xs + openingAngle / 360.
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
        DataY=reflectedOpening / directOpening,
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
    return 0.0022


def slitSizeLogEntry(instrumentName, slitNumber):
    """Return the sample log entry which contains the slit size for the given slit"""
    if slitNumber not in [1, 2]:
        raise RuntimeError('Slit number out of range.')
    entry = 'VirtualSlitAxis.s{}w_actual_width' if instrumentName == 'D17' else 'VirtualSlitAxis.S{}H_actual_height'
    return entry.format(slitNumber + 1)


class SampleLogs:
    FOREGROUND_CENTRE = 'foreground.centre_workspace_index'
    FOREGROUND_END = 'foreground.last_workspace_index'
    FOREGROUND_START = 'foreground.first_workspace_index'
    SUM_TYPE = 'foreground.summation_type'


class WSCleanup:
    """A class to manage intermediate workspace cleanup."""

    OFF = 'Cleanup OFF'
    ON = 'Cleanup ON'

    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        """Initialize an instance of the class."""
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == self.ON
        self._protected = set()
        self._toBeDeleted = set()

    def cleanup(self, *args):
        """Delete the workspaces listed in *args."""
        for ws in args:
            self._delete(ws)

    def cleanupLater(self, *args):
        """Mark the workspaces listed in *args to be cleaned up later."""
        for arg in args:
            self._toBeDeleted.add(str(arg))

    def finalCleanup(self):
        """Delete all workspaces marked to be cleaned up later."""
        for ws in self._toBeDeleted:
            self._delete(ws)

    def protect(self, *args):
        """Mark the workspaces listed in *args to be never deleted."""
        for arg in args:
            self._protected.add(str(arg))

    def _delete(self, ws):
        """Delete the given workspace in ws if it is not protected, and
        deletion is actually turned on.
        """
        if not self._doDelete:
            return
        try:
            ws = str(ws)
        except RuntimeError:
            return
        if ws not in self._protected and mtd.doesExist(ws):
            DeleteWorkspace(Workspace=ws, EnableLogging=self._deleteAlgorithmLogging)


class WSNameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = '__' + prefix if cleanupMode == WSCleanup.ON else prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'
