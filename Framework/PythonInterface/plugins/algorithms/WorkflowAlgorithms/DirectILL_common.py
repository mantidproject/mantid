# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import DeleteWorkspace


ABSOLUTE_UNITS_OFF = 'Absolute Units OFF'
ABSOLUTE_UNITS_ON = 'Absolute Units ON'

BEAM_STOP_DIAGNOSTICS_AUTO = 'Beam Stop Diagnostics AUTO'
BEAM_STOP_DIAGNOSTICS_OFF = 'Beam Stop Diagnostics OFF'
BEAM_STOP_DIAGNOSTICS_ON = 'Beam Stop Diagnostics ON'

BKG_AUTO = 'Flat Bkg AUTO'
BKG_OFF = 'Flat Bkg OFF'
BKG_ON = 'Flat Bkg ON'

BKG_DIAGNOSTICS_AUTO = 'Bkg Diagnostics AUTO'
BKG_DIAGNOSTICS_OFF = 'Bkg Diagnostics OFF'
BKG_DIAGNOSTICS_ON = 'Bkg Diagnostics ON'

CATEGORIES = 'ILL\\Direct;Inelastic\\Reduction;Workflow\\Inelastic'

CLEANUP_OFF = 'Cleanup OFF'
CLEANUP_ON = 'Cleanup ON'

DEFAULT_MASK_OFF = 'Default Mask OFF'
DEFAULT_MASK_ON = 'Default Mask ON'

DWF_OFF = 'Correction OFF'
DWF_ON = 'Correction ON'

ELASTIC_CHANNEL_AUTO = 'Elastic Channel AUTO'
ELASTIC_CHANNEL_FIT = 'Fit Elastic Channel'
ELASTIC_CHANNEL_SAMPLE_LOG = 'Default Elastic Channel'

ELASTIC_PEAK_DIAGNOSTICS_AUTO = 'Peak Diagnostics AUTO'
ELASTIC_PEAK_DIAGNOSTICS_OFF = 'Peak Diagnostics OFF'
ELASTIC_PEAK_DIAGNOSTICS_ON = 'Peak Diagnostics ON'

EPP_METHOD_AUTO = 'EPP Method AUTO'
EPP_METHOD_FIT = 'Fit EPP'
EPP_METHOD_CALCULATE = 'Calculate EPP'

INCIDENT_ENERGY_CALIBRATION_AUTO = 'Energy Calibration AUTO'
INCIDENT_ENERGY_CALIBRATION_OFF = 'Energy Calibration OFF'
INCIDENT_ENERGY_CALIBRATION_ON = 'Energy Calibration ON'

INDEX_TYPE_DET_ID = 'Detector ID'
INDEX_TYPE_WS_INDEX = 'Workspace Index'
INDEX_TYPE_SPECTRUM_NUMBER = 'Spectrum Number'

NORM_METHOD_MON = 'Normalisation Monitor'
NORM_METHOD_OFF = 'Normalisation OFF'
NORM_METHOD_TIME = 'Normalisation Time'

PROP_ABSOLUTE_UNITS = 'AbsoluteUnitsNormalisation'
PROP_BEAM_STOP_DIAGNOSTICS = 'BeamStopDiagnostics'
PROP_BEAM_STOP_THRESHOLD = 'BeamStopThreshold'
PROP_BINNING_PARAMS_Q = 'QBinningParams'
PROP_BKG_DIAGNOSTICS = 'BkgDiagnostics'
PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = 'NoisyBkgHighThreshold'
PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = 'NoisyBkgLowThreshold'
PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = 'NoisyBkgErrorThreshold'
PROP_BKG_SIGMA_MULTIPLIER = 'NonBkgRegionInSigmas'
PROP_CLEANUP_MODE = 'Cleanup'
PROP_DEFAULT_MASK = 'DefaultMask'
PROP_DIAGNOSTICS_WS = 'DiagnosticsWorkspace'
PROP_DWF_CORRECTION = 'DebyeWallerCorrection'
PROP_EC_SCALING = 'EmptyContainerScaling'
PROP_EC_WS = 'EmptyContainerWorkspace'
PROP_ELASTIC_PEAK_DIAGNOSTICS = 'ElasticPeakDiagnostics'
PROP_ELASTIC_CHANNEL_MODE = 'ElasticChannel'
PROP_ELASTIC_CHANNEL_WS = 'ElasticChannelWorkspace'
PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER = 'ElasticPeakWidthInSigmas'
PROP_EPP_METHOD = 'EPPCreationMethod'
PROP_EPP_SIGMA = 'SigmaForCalculatedEPP'
PROP_EPP_WS = 'EPPWorkspace'
PROP_FLAT_BKG = 'FlatBkg'
PROP_FLAT_BKG_SCALING = 'FlatBkgScaling'
PROP_FLAT_BKG_WINDOW = 'FlatBkgAveragingWindow'
PROP_FLAT_BKG_WS = 'FlatBkgWorkspace'
PROP_GROUPING_ANGLE_STEP = 'GroupingAngleStep'
PROP_INCIDENT_ENERGY_CALIBRATION = 'IncidentEnergyCalibration'
PROP_INCIDENT_ENERGY_WS = 'IncidentEnergyWorkspace'
PROP_INPUT_FILE = 'Run'
PROP_INPUT_WS = 'InputWorkspace'
PROP_MON_EPP_WS = 'MonitorEPPWorkspace'
PROP_MON_INDEX = 'Monitor'
PROP_MON_PEAK_SIGMA_MULTIPLIER = 'MonitorPeakWidthInSigmas'
PROP_NORMALISATION = 'Normalisation'
PROP_NUMBER_OF_SIMULATION_WAVELENGTHS = 'NumberOfSimulatedWavelengths'
PROP_OUTPUT_DET_EPP_WS = 'OutputEPPWorkspace'
PROP_OUTPUT_DIAGNOSTICS_REPORT = 'OutputReport'
PROP_OUTPUT_DIAGNOSTICS_REPORT_WS = 'OutputReportWorkspace'
PROP_OUTPUT_ELASTIC_CHANNEL_WS = 'OutputElasticChannelWorkspace'
PROP_OUTPUT_FLAT_BKG_WS = 'OutputFlatBkgWorkspace'
PROP_OUTPUT_INCIDENT_ENERGY_WS = 'OutputIncidentEnergyWorkspace'
PROP_OUTPUT_MON_EPP_WS = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_RAW_WS = 'OutputRawWorkspace'
PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS = 'OutputSelfShieldingCorrectionWorkspace'
PROP_OUTPUT_THETA_W_WS = 'OutputSofThetaEnergyWorkspace'
PROP_OUTPUT_WS = 'OutputWorkspace'
PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD = 'ElasticPeakHighThreshold'
PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD = 'ElasticPeakLowThreshold'
PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = 'ElasticPeakErrorThreshold'
PROP_REBINNING_PARAMS_W = 'EnergyRebinningParams'
PROP_REBINNING_W = 'EnergyRebinning'
PROP_SELF_SHIELDING_CORRECTION_WS = 'SelfShieldingCorrectionWorkspace'
PROP_SIMULATION_INSTRUMENT = 'SimulationInstrument'
PROP_SPARSE_INSTRUMENT_COLUMNS = 'SparseInstrumentColumns'
PROP_SPARSE_INSTRUMENT_ROWS = 'SparseInstrumentRows'
PROP_SUBALG_LOGGING = 'SubalgorithmLogging'
PROP_TEMPERATURE = 'Temperature'
PROP_TRANSPOSE_SAMPLE_OUTPUT = 'Transposing'
PROP_USER_MASK = 'MaskedDetectors'
PROP_USER_MASK_COMPONENTS = 'MaskedComponents'
PROP_VANA_WS = 'IntegratedVanadiumWorkspace'

PROPGROUP_OPTIONAL_OUTPUT = 'Optional Output'

SIMULATION_INSTRUMENT_FULL = 'Full Instrument'
SIMULATION_INSTRUMEN_SPARSE = 'Sparse Instrument'

SUBALG_LOGGING_OFF = 'Logging OFF'
SUBALG_LOGGING_ON = 'Logging ON'

TRANSPOSING_OFF = 'Transposing OFF'
TRANSPOSING_ON = 'Transposing ON'

WS_CONTENT_DETS = 0
WS_CONTENT_MONS = 1


class IntermediateWSCleanup:
    """A class to manage intermediate workspace cleanup."""

    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        """Initialize an instance of the class."""
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == CLEANUP_ON
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


class NameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = prefix
        if cleanupMode == CLEANUP_ON:
            self._prefix = '__' + prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'


class Report:
    """A logger for final report generation."""

    _LEVEL_NOTICE = 0
    _LEVEL_WARNING = 1
    _LEVEL_ERROR = 2

    class _Entry:
        """A private class for report entries."""

        def __init__(self, text, loggingLevel):
            """Initialize an entry."""
            self._text = text
            self._loggingLevel = loggingLevel

        def contents(self):
            """Return the contents of this entry."""
            return self._text

        def level(self):
            """Return the logging level of this entry."""
            return self._loggingLevel

    def __init__(self):
        """Initialize a report."""
        self._entries = list()

    def error(self, text):
        """Add an error entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_ERROR))

    def notice(self, text):
        """Add an information entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_NOTICE))

    def warning(self, text):
        """Add a warning entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_WARNING))

    def toLog(self, log):
        """Write this report to a log."""
        for entry in self._entries:
            loggingLevel = entry.level()
            if loggingLevel == Report._LEVEL_NOTICE:
                log.notice(entry.contents())
            elif loggingLevel == Report._LEVEL_WARNING:
                log.warning(entry.contents())
            elif loggingLevel == Report._LEVEL_ERROR:
                log.error(entry.contents())


def convertToWorkspaceIndex(i, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert given number to workspace index."""
    if indexType == INDEX_TYPE_WS_INDEX:
        return i
    elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
        return ws.getIndexFromSpectrumNumber(i)
    else:  # INDEX_TYPE_DET_ID
        for j in range(ws.getNumberHistograms()):
            if ws.getSpectrum(j).hasDetectorID(i):
                return j
        raise RuntimeError('No workspace index found for detector id {0}'.format(i))


def convertListToWorkspaceIndices(indices, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert a list of spectrum nubmers/detector IDs to workspace indices."""
    return [convertToWorkspaceIndex(i, ws, indexType) for i in indices]
