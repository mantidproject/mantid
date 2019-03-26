# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.kernel import UnitConversion, DeltaEModeType
from mantid.simpleapi import (DeleteWorkspace, mtd)


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
    REDUCTION_TWO_THETA = 'reduction.two_theta'


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
