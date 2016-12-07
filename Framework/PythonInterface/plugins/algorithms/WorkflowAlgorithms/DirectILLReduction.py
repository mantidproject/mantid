# -*- coding: utf-8 -*-

import collections
import glob
from mantid.api import AlgorithmFactory, AnalysisDataServiceImpl, DataProcessorAlgorithm, FileAction, FileProperty, ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, PropertyMode,  WorkspaceProperty
from mantid.kernel import Direct, Direction, IntArrayProperty, StringListValidator, StringMandatoryValidator, UnitConversion
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground, ClearMaskFlag,\
                             CloneWorkspace, ComputeCalibrationCoefVan,\
                             ConvertUnits, CorrectKiKf, CreateSingleValuedWorkspace, CreateWorkspace, DeleteWorkspace, DetectorEfficiencyCorUser, Divide, ExtractMonitors, ExtractSpectra, \
                             FindDetectorsOutsideLimits, FindEPP, GetEiMonDet, GroupWorkspaces, Integration, Load,\
                             MaskDetectors, MedianDetectorTest, MergeRuns, Minus, Multiply, NormaliseToMonitor, Plus, Rebin, Scale
import numpy
from os import path

CLEANUP_DELETE = 'DeleteIntermediateWorkspaces'
CLEANUP_KEEP   = 'KeepIntermediateWorkspaces'

DIAGNOSTICS_YES   = 'DiagnoseDetectors'
DIAGNOSTICS_NO    = 'OmitDetectorDiagnostics'

INCIDENT_ENERGY_CALIBRATION_NO  = 'OmitIncidentEnergyCalibration'
INCIDENT_ENERGY_CALIBRATION_YES = 'CalibrateIncidentEnergy'

INDEX_TYPE_DETECTOR_ID     = 'DetectorID'
INDEX_TYPE_SPECTRUM_NUMBER = 'SpectrumNumber'
INDEX_TYPE_WORKSPACE_INDEX = 'WorkspaceIndex'

NORM_METHOD_MONITOR = 'Monitor'
NORM_METHOD_OFF     = 'No Normalisation'
NORM_METHOD_TIME    = 'AcquisitionTime'

PROP_BINNING_Q                        = 'QBinning'
PROP_BINNING_W                        = 'WBinning'
PROP_CD_WORKSPACE                     = 'CadmiumWorkspace'
PROP_CLEANUP_MODE                     = 'Cleanup'
PROP_DIAGNOSTICS_WORKSPACE            = 'DiagnosticsWorkspace'
PROP_DETECTORS_FOR_EI_CALIBRATION     = 'IncidentEnergyCalibrationDetectors'
PROP_DETECTOR_DIAGNOSTICS             = 'Diagnostics'
PROP_EC_WORKSPACE                     = 'EmptyCanWorkspace'
PROP_EPP_WORKSPACE                    = 'EPPWorkspace'
PROP_FLAT_BACKGROUND_SCALING          = 'FlatBackgroundScaling'
PROP_FLAT_BACKGROUND_WINDOW           = 'FlatBackgroundAveragingWindow'
PROP_FLAT_BACKGROUND_WORKSPACE        = 'FlatBackgroundWorkspace'
PROP_INCIDENT_ENERGY_CALIBRATION      = 'IncidentEnergyCalibration'
PROP_INCIDENT_ENERGY_WORKSPACE        = 'IncidentEnergyWorkspace'
PROP_INDEX_TYPE                       = 'IndexType'
PROP_INITIAL_ELASTIC_PEAK_REFERENCE   = 'InitialElasticPeakReference'
PROP_INPUT_FILE                       = 'InputFile'
PROP_INPUT_WORKSPACE                  = 'InputWorkspace'
PROP_MONITOR_EPP_WORKSPACE            = 'MonitorEPPWorkspace'
PROP_MONITOR_INDEX                    = 'Monitor'
PROP_NORMALISATION                    = 'Normalisation'
PROP_OUTPUT_DIAGNOSTICS_WORKSPACE     = 'OutputDiagnosticsWorkspace'
PROP_OUTPUT_DETECTOR_EPP_WORKSPACE    = 'OutputEPPWorkspace'
PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE = 'OutputFlatBackgroundWorkspace'
PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE = 'OutputIncidentEnergyWorkspace'
PROP_OUTPUT_MONITOR_EPP_WORKSPACE     = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_WORKSPACE                 = 'OutputWorkspace'
PROP_REDUCTION_TYPE                   = 'ReductionType'
PROP_TRANSMISSION                     = 'Transmission'
PROP_USER_MASK                        = 'MaskedDetectors'
PROP_VANADIUM_WORKSPACE               = 'VanadiumWorkspace'

REDUCTION_TYPE_CD = 'Empty can/cadmium'
REDUCTION_TYPE_EC = REDUCTION_TYPE_CD
REDUCTION_TYPE_SAMPLE = 'Sample'
REDUCTION_TYPE_VANADIUM = 'Vanadium'

WS_CONTENT_DETECTORS = 0
WS_CONTENT_MONITORS = 1

class IntermediateWsCleanup:
    '''
    Manages intermediate workspace cleanup.
    '''
    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == CLEANUP_DELETE
        self._protected = set()
        self._toBeDeleted = set()

    def cleanup(self, *args):
        '''
        Deletes the given workspaces.
        '''
        for ws in args:
            self._delete(ws)

    def cleanupLater(self, *args):
        '''
        Marks the given workspaces to be cleaned up later.
        '''
        map(self._toBeDeleted.add, map(str, args))

    def finalCleanup(self):
        '''
        Deletes all workspaces marked to be cleaned up later.
        ''' 
        for ws in self._toBeDeleted:
            self._delete(ws)

    def protect(self, *args):
        '''
        Marks the given workspaces to be never deleted.
        '''
        map(self._protected.add, map(str, args))

    def _delete(self, ws):
        '''
        Deletes the given workspace if it is not protected, and
        deletion is actually turned on.
        '''
        if not self._doDelete:
            return
        ws = str(ws)
        if not ws in self._protected and mtd.doesExist(ws):
            DeleteWorkspace(Workspace=ws,
                            EnableLogging=self._deleteAlgorithmLogging)

class NameSource:

    def __init__(self, prefix, cleanupMode):
        self._names = set()
        self._prefix = prefix
        if cleanupMode == CLEANUP_DELETE:
            self._prefix = '__' + prefix

    def withSuffix(self, suffix):
        return self._prefix + '_' + suffix

class Report:
    '''
    Logger for final report generation.
    '''
    _LEVEL_NOTICE = 0
    _LEVEL_WARNING = 1
    _LEVEL_ERROR = 2

    class _Entry:
        '''
        Private class for report entries.
        '''
        def __init__(self, text, loggingLevel):
            '''
            Initializes an entry.
            '''
            self._text = text
            self._loggingLevel = loggingLevel

        def contents(self):
            '''
            Returns the contents of this entry.
            '''
            return self._text

        def level(self):
            '''
            Returns the logging level of this entry.
            '''
            return self._loggingLevel

    def __init__(self):
        '''
        Initializes a report.
        '''
        self._entries = list()

    def error(self, text):
        '''
        Adds an error entry to this report.
        '''
        self._entries.append(Report._Entry(text, Report._LEVEL_ERROR))

    def notice(self, text):
        '''
        Adds an information entry to this report.
        '''
        self._entries.append(Report._Entry(text, Report._LEVEL_NOTICE))

    def warning(self, text):
        '''
        Adds a warning entry to this report.
        '''
        self._entries.append(Report._Entry(text, Report._LEVEL_WARNING))

    def toLog(self, log):
        '''
        Writes this report to a log.
        '''
        for entry in self._entries:
            loggingLevel = entry.level()
            if loggingLevel == Report._LEVEL_NOTICE:
                log.notice(entry.contents())
            elif loggingLevel == Report._LEVEL_WARNING:
                log.warning(entry.contents())
            elif loggingLevel == Report._LEVEL_ERROR:
                log.error(entry.contents())


def setAsBad(ws, index):
    ws.dataY(index)[0] += 1

def _loadFiles(inputFilename, eppReference, wsNames, wsCleanup, log, algorithmLogging):
    '''
    Loads files specified by filenames, merging them into a single
    workspace.
    '''
    # TODO Stop this epp reference madness when LoadILLTOF v.2 is available.
    # TODO Explore ways of loading data file-by-file and merging pairwise.
    #      Should save some memory (IN5!).
    rawWSName = wsNames.withSuffix('raw')
    if eppReference:
        if mtd.doesExist(str(eppReference)):
            ws = Load(Filename=inputFilename,
                      OutputWorkspace=rawWSName,
                      WorkspaceVanadium=eppReference,
                      EnableLogging=algorithmLogging)
        else:
            ws = Load(Filename=inputFilename,
                      OutputWorkspace=rawWSName,
                      FilenameVanadium=eppReference,
                      EnableLogging=algorithmLogging)
    else:
        # For multiple files, we need an EPP reference workspace
        # anyway, so we'll use the first file the user wants to
        # load.
        head, tail = path.split(inputFilename)
        # Loop over known MultipleFileProperty file list
        # specifiers.
        for separator in [':', ',', '-', '+']:
            referenceFilename, sep, rest = tail.partition(separator)
            if referenceFilename:
                break;
        if referenceFilename:
            referenceFilename = path.join(head, referenceFilename)
            log.information('Using ' + referenceFilename + ' as initial EPP reference.')
            eppReferenceWSName = wsNames.withSuffix('initial_epp_reference')
            Load(Filename=referenceFilename,
                 OutputWorkspace=eppReferenceWSName,
                 EnableLogging=algorithmLogging)
            ws = Load(Filename=inputFilename,
                      OutputWorkspace=rawWSName,
                      WorkspaceVanadium=eppReferenceWSName,
                      EnableLogging=algorithmLogging)
            wsCleanup.cleanup(eppReferenceWSName)
        else:
            # Single file, no EPP reference.
            ws = Load(Filename=inputFilename,
                      OutputWorkspace=rawWSName,
                      EnableLogging=algorithmLogging)
    # Now merge the loaded files
    mergedWSName = wsNames.withSuffix('merged')
    ws = MergeRuns(InputWorkspaces=ws,
                   OutputWorkspace=mergedWSName,
                   EnableLogging=algorithmLogging)
    wsCleanup.cleanup(rawWSName)
    return ws

def _extractMonitorWs(ws, wsNames, algorithmLogging):
    '''
    Separates detector and monitor histograms from a workspace.
    '''
    detWSName = wsNames.withSuffix('extracted_detectors')
    monWSName = wsNames.withSuffix('extracted_monitors')
    detectorWS, monitorWS = ExtractMonitors(InputWorkspace=ws,
                                            DetectorWorkspace=detWSName,
                                            MonitorWorkspace=monWSName,
                                            EnableLogging=algorithmLogging)
    return detectorWS, monitorWS

def _createFlatBackground(ws, windowWidth, wsNames, algorithmLogging):
    '''
    Returns a flat background workspace.
    '''
    bkgWSName = wsNames.withSuffix('flat_background')
    bkgWS = CalculateFlatBackground(InputWorkspace=ws,
                                    OutputWorkspace=bkgWSName,
                                    Mode='Moving Average',
                                    OutputMode='Return Background',
                                    SkipMonitors=False,
                                    NullifyNegativeValues=False,
                                    AveragingWindowWidth=windowWidth,
                                    EnableLogging=algorithmLogging)
    return bkgWS

def _subtractFlatBackground(ws, bkgWorkspace, bkgScaling, wsNames, wsCleanup, algorithmLogging):
    '''
    Subtracts a scaled flat background from a workspace.
    '''
    subtractedWSName = wsNames.withSuffix('background_subtracted')
    scaledBkgWSName = wsNames.withSuffix('flat_background_scaled')
    Scale(InputWorkspace=bkgWorkspace,
          OutputWorkspace=scaledBkgWSName,
          Factor = bkgScaling,
          EnableLogging=algorithmLogging)
    subtractedWS = Minus(LHSWorkspace=ws,
                      RHSWorkspace=scaledBkgWSName,
                      OutputWorkspace=subtractedWSName,
                      EnableLogging=algorithmLogging)
    wsCleanup.cleanup(scaledBkgWSName)
    return subtractedWS

def _findEPP(ws, wsType, wsNames, algorithmLogging):
    '''
    Returns EPP table for a workspace.
    '''
    if wsType == WS_CONTENT_DETECTORS:
        eppWSName = wsNames.withSuffix('epp_detectors')
    else:
        eppWSName = wsNames.withSuffix('epp_monitors')
    eppWS = FindEPP(InputWorkspace = ws,
                    OutputWorkspace = eppWSName,
                    EnableLogging=algorithmLogging)
    return eppWS

def _reportSingleZeroCountDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of zero count diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass # Nothing to report.
    elif diagnose == 1:
        report.notice('Workspace index {0} has zero counts.'.format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad for ' +
            'masking by zero count diagnostics for unknown reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.error(('Workspace index {0} has been marked as masked by ' +
            'zero count diagnostics for unknown reasons.').format(wsIndex))

def _reportSingleBackgroundDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of background diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass # Nothing to report.
    elif diagnose == 1:
        report.notice('Workspace index {0} has noisy background.'.format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad ' +
            'by noisy background diagnostics for unknown reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.notice(('Workspace index {0} has been marked as an outlier ' +
            'in noisy background diagnostics.').format(wsIndex))

def _reportDiagnostics(report, zeroCountDiagnostics, bkgDiagnostics):
    '''
    Parses the mask workspaces and fills in the report accordingly.
    '''
    for i in range(zeroCountDiagnostics.getNumberHistograms()):
        _reportSingleZeroCountDiagnostics(report,
                                          zeroCountDiagnostics,
                                          i)
        _reportSingleBackgroundDiagnostics(report,
                                           bkgDiagnostics,
                                           i)

def _diagnoseDetectors(ws, bkgWS, wsNames, wsCleanup, report, algorithmLogging):
    '''
    Returns a diagnostics workspace.
    '''
    # 1. Detectors with zero counts.
    zeroCountWSName = wsNames.withSuffix('diagnostics_zero_counts')
    zeroCountDiagnostics, nFailures = FindDetectorsOutsideLimits(InputWorkspace=ws,
                                                                 OutputWorkspace=zeroCountWSName,
                                                                 EnableLogging=algorithmLogging)
    # 2. Detectors with high background.
    noisyBkgWSName = wsNames.withSuffix('diagnostics_noisy_background')
    noisyBkgDiagnostics, nFailures = MedianDetectorTest(InputWorkspace=bkgWS,
                                                        OutputWorkspace=noisyBkgWSName,
                                                        LowThreshold=0.0,
                                                        HighThreshold=10,
                                                        LowOutlier=0.0,
                                                        EnableLogging=algorithmLogging)
    _reportDiagnostics(report, zeroCountDiagnostics, noisyBkgDiagnostics)
    ClearMaskFlag(Workspace=noisyBkgDiagnostics,
                  EnableLogging=algorithmLogging)
    combinedDiagnosticsWSName = wsNames.withSuffix('diagnostics')
    diagnosticsWS = Plus(LHSWorkspace=zeroCountDiagnostics,
                         RHSWorkspace=noisyBkgDiagnostics,
                         OutputWorkspace=combinedDiagnosticsWSName,
                         EnableLogging=algorithmLogging)
    wsCleanup.cleanup(zeroCountWSName)
    wsCleanup.cleanup(noisyBkgWSName)
    return diagnosticsWS

def _maskDiagnosedDetectors(ws, diagnosticsWS, wsNames, algorithmLogging):
    '''
    Mask detectors according to diagnostics.
    '''
    maskedWSName = wsNames.withSuffix('diagnostics_applied')
    maskedWS = CloneWorkspace(InputWorkspace=ws,
                              OutputWorkspace=maskedWSName,
                              EnableLogging=algorithmLogging)
    MaskDetectors(Workspace=maskedWS,
                  MaskedWorkspace=diagnosticsWS,
                  EnableLogging=algorithmLogging)
    return maskedWS

def _applyUserMask(ws, mask, indexType, wsNames, algorithmLogging):
    '''
    Applies mask.
    '''
    maskedWSName = wsNames.withSuffix('masked')
    maskedWorkspace = CloneWorkspace(InputWorkspace = ws,
                                     OutputWorkspace = maskedWSName,
                                     EnableLogging=algorithmLogging)
    if indexType == INDEX_TYPE_DETECTOR_ID:
        MaskDetectors(Workspace=maskedWorkspace,
                      DetectorList=mask,
                      EnableLogging=algorithmLogging)
    elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
        MaskDetectors(Workspace=maskedWorkspace,
                      SpectraList=mask,
                      EnableLogging=algorithmLogging)
    elif indexType == INDEX_TYPE_WORKSPACE_INDEX:
        MaskDetectors(Workspace=maskedWorkspace,
                      WorkspaceIndexList=mask,
                      EnableLogging=algorithmLogging)
    else:
        raise RuntimeError('Unknown ' + PROP_INDEX_TYPE)
    return maskedWorkspace

def _calibratedIncidentEnergy(detWorkspace, detEPPWorkspace, monWorkspace, monEPPWorkspace, indexType, eiCalibrationDets, eiCalibrationMon, wsNames, log, algorithmLogging):
    '''
    Returns the calibrated incident energy.
    '''
    instrument = detWorkspace.getInstrument().getName()
    eiWorkspace = None
    if instrument in ['IN4', 'IN6']:
        pulseInterval = detWorkspace.getRun().getLogData('pulse_interval').value
        energy = GetEiMonDet(DetectorWorkspace=detWorkspace,
                             DetectorEPPTable=detEPPWorkspace,
                             IndexType=indexType,
                             Detectors=eiCalibrationDets,
                             MonitorWorkspace=monWorkspace,
                             MonitorEppTable=monEPPWorkspace,
                             Monitor=eiCalibrationMon,
                             PulseInterval=pulseInterval,
                             EnableLogging=algorithmLogging)
        eiWSName = wsNames.withSuffix('incident_energy')
        eiWorkspace = CreateSingleValuedWorkspace(OutputWorkspace=eiWSName,
                                                  DataValue=energy,
                                                  EnableLogging=algorithmLogging)
    else:
        log.error('Instrument ' + instrument + ' not supported for incident energy calibration')
    return eiWorkspace

def _applyIncidentEnergyCalibration(ws, wsType, eiWS, wsNames, report, algorithmLogging):
    '''
    Updates incident energy and wavelength in the sample logs.
    '''
    originalEnergy = ws.getRun().getLogData('Ei').value
    originalWavelength = ws.getRun().getLogData('wavelength').value
    energy = eiWS.readY(0)[0]
    wavelength = UnitConversion.run('Energy', 'Wavelength', energy, 0, 0, 0, Direct, 5)
    if wsType == WS_CONTENT_DETECTORS:
        calibratedWSName = wsNames.withSuffix('incident_energy_calibrated_detectors')
    elif wsType == WS_CONTENT_MONITORS:
        calibratedWSName = wsNames.withSuffix('incident_energy_calibrated_monitors')
    else:
        raise RuntimeError('Unknown workspace content type')
    calibratedWS = CloneWorkspace(InputWorkspace=ws,
                                  OutputWorkspace=calibratedWSName,
                                  EnableLogging=algorithmLogging)
    AddSampleLog(Workspace=calibratedWS,
                 LogName='Ei',
                 LogText=str(energy),
                 LogType='Number',
                 NumberType='Double',
                 LogUnit='meV',
                 EnableLogging=algorithmLogging)
    AddSampleLog(Workspace=calibratedWS,
                 Logname='wavelength',
                 LogText=str(wavelength),
                 LogType='Number',
                 NumberType='Double',
                 LogUnit='Ångström',
                 EnableLogging=algorithmLogging)
    report.notice("Applied Ei calibration to '" + str(ws) + "'.")
    report.notice('Original Ei: {} new Ei: {}.'.format(originalEnergy, energy))
    report.notice('Original wavelength: {} new wavelength {}.'.format(originalWavelength, wavelength))
    return calibratedWS

def _normalizeToMonitor(ws, monWS, monEPPWS, monIndex, wsNames, wsCleanup, algorithmLogging):
    '''
    Normalizes to monitor counts.
    '''
    normalizedWSName = wsNames.withSuffix('normalized_to_monitor')
    normalizationFactorWsName = wsNames.withSuffix('normalization_factor_monitor')
    eppRow = monEPPWS.row(monIndex)
    sigma = eppRow['Sigma']
    centre = eppRow['PeakCentre']
    begin = centre - 3 * sigma
    end = centre + 3 * sigma
    normalizedWS, normalizationFactorWS = NormaliseToMonitor(InputWorkspace=ws,
                                                             OutputWorkspace=normalizedWSName,
                                                             MonitorWorkspace=monWS,
                                                             MonitorWorkspaceIndex=monIndex,
                                                             IntegrationRangeMin=begin,
                                                             IntegrationRangeMax=end,
                                                             NormFactorWS=normalizationFactorWsName,
                                                             EnableLogging=algorithmLogging)
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS

def _normalizeToTime(ws, wsNames, wsCleanup, algorithmLogging):
    '''
    Normalizes to 'actual_time' sample log.
    '''
    normalizedWSName = wsNames.withSuffix('normalized_to_time')
    normalizationFactorWsName = wsNames.withSuffix('normalization_factor_time')
    time = inWs.getLogData('actual_time').value
    normalizationFactorWS = CreateSingleValuedWorkspace(OutputWorkspace=normalizationFactorWsName,
                                                        DataValue = time,
                                                        EnableLogging=algorithmLogging)
    normalizedWS = Divide(LHSWorkspace=ws,
                          RHSWorkspace=normalizationFactorWS,
                          OutputWorkspace=normalizedWSName,
                          EnableLogging=algorithmLogging)
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS

def _subtractECWithCd(ws, ecWS, cdWS, transmission, wsNames, wsCleanup, algorithmLogging):
    '''
    Subtracts cadmium corrected emtpy can.
    '''
    # out = (in - Cd) / transmission - (EC - Cd)
    transmissionWSName = wsNames.withSuffix('transmission')
    transmissionWS = CreateSingleValuedWorkspace(OutputWorkspace=transmissionWSName,
                                                 DataValue=transmission,
                                                 EnableLogging=algorithmLogging)
    cdSubtractedECWSName = wsNames.withSuffix('Cd_subtracted_EC')
    cdSubtractedECWS = Minus(LHSWorkspace=ecWS,
                             RHSWorkspace=cdWS,
                             OutputWorkspace=cdSubtractedECWSName,
                             EnableLogging=algorithmLogging)
    cdSubtractedWSName = wsNames.withSuffix('Cd_subtracted')
    cdSubtractedWS = Minus(LHSWorkspace=ws,
                           RHSWorkspace=cdWS,
                           OutputWorkspace=cdSubtractedWSName,
                           EnableLogging=algorithmLogging)
    correctedCdSubtractedWsName = wsNames.withSuffix('transmission_corrected_Cd_subtracted')
    correctedCdSubtractedWS = Divide(LHSWorkspace=cdSubtractedWS,
                                     RHSWorkspace=transmissionWS,
                                     OutputWorkspace=correctedCdSubtractedWSName,
                                     EnableLogging=algorithmLogging)
    ecSubtractedWSName = wsNames.withSuffix('EC_subtracted')
    ecSubtractedWS = Minus(LHSWorkspace=correctedCdSubtractedWS,
                           RHSWorkspace=cdSubtractedECWS,
                           OutputWorkspace=ecSubtractedWSName,
                           EnableLogging=algorithmLogging)
    wsCleanup.cleanup(cdSubtractedECWS,
                      cdSubtractedWS,
                      correctedCdSubtractedWS,
                      EnableLogging=algorithmLogging)
    return ecSubtractedWS

def _subtractEC(ws, ecWS, transmission, wsNames, wsCleanup, algorithmLogging):
    '''
    Subtracts empty can.
    '''
    # out = in - transmission * EC
    transmissionWSName = wsNames.withSuffix('transmission')
    transmissionWS = CreateSingleValuedWorkspace(OutputWorkspace=transmissionWSName,
                                                 DataValue=transmission,
                                                 EnableLogging=algorithmLogging)
    correctedECWSName = wsNames.withSuffix('transmission_corrected_EC')
    correctedECWS = Multiply(LHSWorkspace=ecWS,
                             RHSWorkspace=transmissionWS,
                             OutputWorkspace=correctedECWSName,
                             EnableLogging=algorithmLogging)
    ecSubtractedWSName = wsNames.withSuffix('EC_subtracted')
    ecSubtractedWS = Minus(LHSWorkspace=ws,
                           RHSWorkspace=correctedECWS,
                           OutputWorkspace=ecSubtractedWSName,
                           EnableLogging=algorithmLogging)
    wsCleanup.cleanup(transmissionWS)
    wsCleanup.cleanup(correctedECWS)
    return ecSubtractedWS

def _normalizeToVanadium(ws, vanaWS, wsNames, algorithmLogging):
    '''
    Normalizes to vanadium workspace.
    '''
    vanaNormalizedWSName = wsNames.withSuffix('vanadium_normalized')
    vanaNormalizedWS = Divide(LHSWorkspace=ws,
                              RHSWorkspace=vanaWS,
                              OutputWorkspace=vanaNormalizedWSName,
                              EnableLogging=algorithmLogging)
    return vanaNormalizedWS

def _convertTOFToDeltaE(ws, wsNames, algorithmLogging):
    '''
    Converts the X units from time-of-flight to energy transfer.
    '''
    energyConvertedWSName = wsNames.withSuffix('energy_converted')
    energyConvertedWS = ConvertUnits(InputWorkspace = ws,
                                     OutputWorkspace = energyConvertedWSName,
                                     Target = 'DeltaE',
                                     EMode = 'Direct',
                                     EnableLogging=algorithmLogging)
    return energyConvertedWS

def _rebin(ws, params, wsNames, algorithmLogging):
    '''
    Rebins a workspace.
    '''
    rebinnedWSName = wsNames.withSuffix('rebinned')
    rebinnedWS = Rebin(InputWorkspace = ws,
                       OutputWorkspace = rebinnedWSName,
                       Params = params,
                       EnableLogging=algorithmLogging)
    return rebinnedWS

def _correctByKiKf(ws, wsNames, algorithmLogging):
    '''
    Applies the k_i / k_f correction.
    '''
    correctedWSName = wsNames.withSuffix('kikf')
    correctedWS = CorrectKiKf(InputWorkspace = ws,
                              OutputWorkspace = correctedWSName,
                              EnableLogging=algorithmLogging)
    return correctedWS

def _correctByDetectorEfficiency(ws, wsNames, algorithmLogging):
    correctedWSName = wsNames.withSuffix('detector_efficiency_corrected')
    correctedWS = DetectorEfficiencyCorUser(InputWorkspace = ws,
                                            OutputWorkspace = correctedWSName,
                                            EnableLogging=algorithmLogging)
    return correctedWS


class DirectILLReduction(DataProcessorAlgorithm):

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\Inelastic'

    def name(self):
        return 'DirectILLReduction'

    def summary(self):
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL'

    def version(self):
        return 1

    def PyExec(self):
        report = Report()
        childAlgorithmLogging = False
        reductionType = self.getProperty(PROP_REDUCTION_TYPE).value
        wsNamePrefix = self.getProperty(PROP_OUTPUT_WORKSPACE).valueAsStr
        cleanupMode = self.getProperty(PROP_CLEANUP_MODE).value
        wsNames = NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = IntermediateWsCleanup(cleanupMode, childAlgorithmLogging)
        indexType = self.getProperty(PROP_INDEX_TYPE).value

        # The variable 'mainWS' shall hold the current main data
        # throughout the algorithm.

        # Init workspace.
        inputFile = self.getProperty(PROP_INPUT_FILE).value
        if inputFile:
            eppReference = self.getProperty(PROP_INITIAL_ELASTIC_PEAK_REFERENCE).value
            mainWS = _loadFiles(inputFile,
                                   eppReference,
                                   wsNames,
                                   wsCleanup,
                                   self.log(),
                                   childAlgorithmLogging)
        elif self.getProperty(PROP_INPUT_WORKSPACE).value:
            mainWS = self.getProperty(PROP_INPUT_WORKSPACE).value

        # Extract monitors to a separate workspace
        detectorWorkspace, monitorWorkspace = _extractMonitorWs(mainWS,
                                                                wsNames,
                                                                childAlgorithmLogging)
        monitorIndex = self.getProperty(PROP_MONITOR_INDEX).value
        monitorIndex = self._convertToWorkspaceIndex(monitorIndex,
                                                     monitorWorkspace)
        wsCleanup.cleanup(mainWS)
        mainWS = detectorWorkspace
        del(detectorWorkspace)
        wsCleanup.cleanupLater(monitorWorkspace)

        # Time-independent background
        # ATM monitor background is ignored
        bkgInWS = self.getProperty(PROP_FLAT_BACKGROUND_WORKSPACE).value
        if not bkgInWS:
            windowWidth = self.getProperty(PROP_FLAT_BACKGROUND_WINDOW).value
            bkgWS = _createFlatBackground(mainWS, 
                                          windowWidth,
                                          wsNames,
                                          childAlgorithmLogging)
        else:
            bkgWS = bkgInWS
            wsCleanup.protect(bkgWS)
        if not self.getProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE, bkgWS)
        # Subtract the time-independent background.
        bkgScaling = self.getProperty(PROP_FLAT_BACKGROUND_SCALING).value
        bkgSubtractedWorkspace = _subtractFlatBackground(mainWS,
                                                         bkgWS,
                                                         bkgScaling,
                                                         wsNames,
                                                         wsCleanup,
                                                         childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = bkgSubtractedWorkspace
        del(bkgSubtractedWorkspace)
        wsCleanup.cleanupLater(bkgWS)

        # Find elastic peak positions for detectors.
        detectorEPPInWS = self.getProperty(PROP_EPP_WORKSPACE).value
        if not detectorEPPInWS:
            detectorEPPWS = _findEPP(mainWS,
                                     WS_CONTENT_DETECTORS,
                                     wsNames,
                                     childAlgorithmLogging)
        else:
            detectorEPPWS = detectorInWS
            wsCleanup.protect(detectorEPPWS)
        if not self.getProperty(PROP_OUTPUT_DETECTOR_EPP_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_DETECTOR_EPP_WORKSPACE,
                             detectorEPPWS)
        # Elastic peaks for monitors
        monitorEPPInWS = self.getProperty(PROP_MONITOR_EPP_WORKSPACE).value
        if not monitorEPPInWS:
            monitorEPPWS = _findEPP(mainWS,
                                    WS_CONTENT_MONITORS,
                                    wsNames,
                                    childAlgorithmLogging)
        else:
            monitorEPPWS = monitorInWS
            wsCleanup.protect(monitorEPPWS)
        if not self.getProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE,
                             monitorEPPWS)
        wsCleanup.cleanupLater(detectorEPPWS, monitorEPPWS)

        # Detector diagnostics, if requested.
        if self.getProperty(PROP_DETECTOR_DIAGNOSTICS).value == DIAGNOSTICS_YES:
            diagnosticsInWS = self.getProperty(PROP_DIAGNOSTICS_WORKSPACE).value
            if not diagnosticsInWS:
                diagnosticsWS = _diagnoseDetectors(mainWS,
                                                   bkgWS,
                                                   wsNames,
                                                   wsCleanup,
                                                   report,
                                                   childAlgorithmLogging)
            else:
                diagnosticsWS = diagnosticsInWS
                wsCleanup.protect(diagnosticsWS)
            if not self.getProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE).isDefault:
                self.setProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE, diagnosticsWS)
            diagnosedWorkspace = _maskDiagnosedDetectors(mainWS,
                                                         diagnosticsWS,
                                                         wsNames,
                                                         childAlgorithmLogging)
            wsCleanup.cleanup(diagnosticsWS)
            del(diagnosticsWS)
            wsCleanup.cleanup(mainWS)
            mainWS = diagnosedWorkspace
            del(diagnosedWorkspace)
        # Apply user mask.
        userMask = self.getProperty(PROP_USER_MASK).value
        maskedWorkspace = _applyUserMask(mainWS, 
                                         userMask,
                                         indexType,
                                         wsNames,
                                         childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = maskedWorkspace
        del(maskedWorkspace)

        # Get calibrated incident energy
        eiCalibration = self.getProperty(PROP_INCIDENT_ENERGY_CALIBRATION).value
        if eiCalibration == INCIDENT_ENERGY_CALIBRATION_YES:
            eiInWS = self.getProperty(PROP_INCIDENT_ENERGY_WORKSPACE).value
            if not eiInWS:
                eiCalibrationDets = self.getProperty(PROP_DETECTORS_FOR_EI_CALIBRATION).value
                eiCalibrationWS = _calibratedIncidentEnergy(mainWS,
                                                            detectorEPPWS,
                                                            monitorWorkspace,
                                                            monitorEPPWS,
                                                            indexType,
                                                            eiCalibrationDets,
                                                            monitorIndex,
                                                            wsNames,
                                                            self.log(),
                                                            childAlgorithmLogging)
            else:
                eiCalibrationWS = eiInWS
                wsCleanup.protect(eiCalibrationWS)
            if eiCalibrationWS:
                eiCalibratedDetWS = _applyIncidentEnergyCalibration(mainWS,
                                                                    WS_CONTENT_DETECTORS,
                                                                    eiCalibrationWS,
                                                                    wsNames,
                                                                    report,
                                                                    childAlgorithmLogging)
                wsCleanup.cleanup(mainWS)
                mainWS = eiCalibratedDetWS
                del(eiCalibratedDetWS)
                eiCalibratedMonWS = _applyIncidentEnergyCalibration(monitorWorkspace,
                                                                    WS_CONTENT_MONITORS,
                                                                    eiCalibrationWS,
                                                                    wsNames,
                                                                    report,
                                                                    childAlgorithmLogging)
                wsCleanup.cleanup(monitorWorkspace)
                monitorWorkspace = eiCalibratedMonWS
                del(eiCalibratedMonWS)
            if not self.getProperty(PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE).isDefault:
                self.setProperty(PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE, eiCalibrationWS)
            wsCleanup.cleanup(eiCalibrationWS)
            del(eiCalibrationWS)

        # Normalisation to monitor/time
        normalisationMethod = self.getProperty(PROP_NORMALISATION).value
        if normalisationMethod != NORM_METHOD_OFF:
            if normalisationMethod == NORM_METHOD_MONITOR:
                normalizedWS = _normalizeToMonitor(mainWS,
                                                   monitorWorkspace,
                                                   monitorEPPWS,
                                                   monitorIndex,
                                                   wsNames,
                                                   wsCleanup,
                                                   childAlgorithmLogging)
            elif normalisationMethod == NORM_METHOD_TIME:
                normalizedWS = _normalizeToTime(mainWS,
                                                wsNames,
                                                wsCleanup,
                                                childAlgorithmLogging)
            else:
                raise RuntimeError('Unknonwn normalisation method ' + normalisationMethod)
            wsCleanup.cleanup(mainWS)
            mainWS = normalizedWS
            del(normalizedWS)

        # Reduction for empty can and cadmium ends here.
        if reductionType == REDUCTION_TYPE_CD or reductionType == REDUCTION_TYPE_EC:
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with vanadium and sample reductions.

        # Empty can subtraction
        ecInWS = self.getProperty(PROP_EC_WORKSPACE).value
        if ecInWS:
            cdInWS = self.getProperty(PROP_CD_WORKSPACE).value
            transmission = self.getProperty(PROP_TRANSMISSION).value
            if cdInWS:
                ecSubtractedWS = _subtractECWithCd(mainWS,
                                                   ecInWS,
                                                   cdInWS,
                                                   transmission,
                                                   wsNames,
                                                   wsCleanup,
                                                   childAlgorithmLogging)
            else:
                ecSubtractedWS = _subtractEC(mainWS,
                                             ecInWS,
                                             transmission,
                                             wsNames,
                                             wsCleanup,
                                             childAlgorithmLogging)
            wsCleanup.cleanup(mainWS)
            mainWS = ecSubtractedWS
            del(ecSubtractedWS)

        # Reduction for vanadium ends here.
        if reductionType == REDUCTION_TYPE_VANADIUM:
            # We output an integrated vanadium, ready to be used for
            # normalization.
            outWS = self.getPropertyValue(PROP_OUTPUT_WORKSPACE)
            # TODO For the time being, we may just want to integrate
            # the vanadium data as `ComputeCalibrationCoef` does not do
            # the best possible Debye-Waller correction.
            mainWS = ComputeCalibrationCoefVan(VanadiumWorkspace=mainWS,
                                                  EPPTable=detectorEPPWS,
                                                  OutputWorkspace=outWS,
                                                  EnableLogging=childAlgorithmLogging)
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with sample reduction.

        # Vanadium normalization.
        # TODO Absolute normalization.
        vanaWS = self.getProperty(PROP_VANADIUM_WORKSPACE).value
        if vanaWS:
            vanaNormalizedWS = _normalizeToVanadium(mainWS,
                                                    vanaWS,
                                                    wsNames,
                                                    childAlgorithmLogging)
            wsCleanup.cleanup(mainWS)
            mainWS = vanaNormalizedWS
            del(vanaNormalizedWS)

        # Convert units from TOF to energy
        energyConvertedWS = _convertTOFToDeltaE(mainWS,
                                                wsNames,
                                                childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = energyConvertedWS
        del(energyConvertedWS)

        # KiKf conversion
        kikfCorrectedWS = _correctByKiKf(mainWS,
                                         wsNames,
                                         childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = kikfCorrectedWS
        del(kikfCorrectedWS)

        # Rebinning
        # TODO automatize binning in w. Do we need rebinning in q as well?
        params = self.getProperty(PROP_BINNING_W).value
        if params:
            rebinnedWS = _rebin(mainWS,
                                params,
                                wsNames,
                                childAlgorithmLogging)
            wsCleanup.cleanup(mainWS)
            mainWS = rebinnedWS
            del(rebinnedWS)

        # Detector efficiency correction
        efficiencyCorrectedWS = _correctByDetectorEfficiency(mainWS,
                                                             wsNames,
                                                             childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = efficiencyCorrectedWS
        del(efficiencyCorrectedWS)

        # TODO Self-shielding corrections

        self._finalize(mainWS, wsCleanup, report)

    def PyInit(self):
        # TODO Property validation.
        # Inputs
        self.declareProperty(FileProperty(PROP_INPUT_FILE,
                                          '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']))
        self.declareProperty(MatrixWorkspaceProperty(PROP_INPUT_WORKSPACE,
                                                     '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input))
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_WORKSPACE,
                             '',
                             direction=Direction.Output),
                             doc='The output of the algorithm')
        self.declareProperty(PROP_REDUCTION_TYPE,
                             REDUCTION_TYPE_SAMPLE,
                             validator=StringListValidator([REDUCTION_TYPE_SAMPLE, REDUCTION_TYPE_VANADIUM, REDUCTION_TYPE_CD, REDUCTION_TYPE_EC]),
                             direction=Direction.Input,
                             doc='Type of the reduction workflow and output')
        self.declareProperty(PROP_CLEANUP_MODE,
                             CLEANUP_DELETE,
                             validator=StringListValidator([CLEANUP_DELETE, CLEANUP_KEEP]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces')
        self.declareProperty(PROP_INITIAL_ELASTIC_PEAK_REFERENCE,
                             '',
                             direction=Direction.Input,
                             doc="Reference file or workspace for initial 'EPP' sample log entry")
        self.declareProperty(MatrixWorkspaceProperty(PROP_VANADIUM_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced vanadium workspace')
        self.declareProperty(MatrixWorkspaceProperty(PROP_EC_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced empty can workspace')
        self.declareProperty(MatrixWorkspaceProperty(PROP_CD_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced cadmium workspace')
        self.declareProperty(ITableWorkspaceProperty(PROP_EPP_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm')
        self.declareProperty(ITableWorkspaceProperty(PROP_MONITOR_EPP_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm for the monitor workspace')
        self.declareProperty(PROP_INDEX_TYPE,
                             INDEX_TYPE_WORKSPACE_INDEX,
                             direction=Direction.Input,
                             doc='Type of numbers in ' + PROP_MONITOR_INDEX + ' and ' + PROP_DETECTORS_FOR_EI_CALIBRATION + ' properties')
        self.declareProperty(PROP_MONITOR_INDEX,
                             0,
                             direction=Direction.Input,
                             doc='Index of the main monitor')
        self.declareProperty(PROP_INCIDENT_ENERGY_CALIBRATION,
                             INCIDENT_ENERGY_CALIBRATION_YES,
                             validator=StringListValidator([INCIDENT_ENERGY_CALIBRATION_YES, INCIDENT_ENERGY_CALIBRATION_YES]),
                             direction=Direction.Input,
                             doc='Enable or disable incident energy calibration on IN4 and IN6')
        self.declareProperty(PROP_DETECTORS_FOR_EI_CALIBRATION,
                             '',
                             direction=Direction.Input,
                             doc='List of detectors used for the incident energy calibration')
        self.declareProperty(MatrixWorkspaceProperty(PROP_INCIDENT_ENERGY_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='A single-valued workspace holding the calibrated incident energy')
        self.declareProperty(PROP_FLAT_BACKGROUND_SCALING,
                             1.0,
                             direction=Direction.Input,
                             doc='Flat background scaling constant')
        self.declareProperty(PROP_FLAT_BACKGROUND_WINDOW,
                             30,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) for flat background')
        self.declareProperty(MatrixWorkspaceProperty(PROP_FLAT_BACKGROUND_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Workspace from which to get flat background data')
        self.declareProperty(IntArrayProperty(PROP_USER_MASK,
                                              '',
                                              direction=Direction.Input),
                             doc='List of spectra to mask')
        self.declareProperty(PROP_DETECTOR_DIAGNOSTICS,
                             DIAGNOSTICS_YES,
                             validator=StringListValidator([DIAGNOSTICS_YES, DIAGNOSTICS_NO]),
                             direction=Direction.Input,
                             doc='If true, run detector diagnostics or apply ' + PROP_DIAGNOSTICS_WORKSPACE)
        self.declareProperty(MatrixWorkspaceProperty(PROP_DIAGNOSTICS_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Detector diagnostics workspace obtained from another reduction run.')
        self.declareProperty(PROP_NORMALISATION,
                             NORM_METHOD_MONITOR,
                             validator=StringListValidator([NORM_METHOD_MONITOR, NORM_METHOD_TIME, NORM_METHOD_OFF]),
                             direction=Direction.Input,
                             doc='Normalisation method')
        self.declareProperty(PROP_TRANSMISSION,
                             1.0,
                             direction=Direction.Input,
                             doc='Sample transmission for empty can subtraction')
        self.declareProperty(PROP_BINNING_Q,
                             '',
                             direction=Direction.Input,
                             doc='Rebinning in q')
        self.declareProperty(PROP_BINNING_W,
                             '',
                             direction=Direction.Input,
                             doc='Rebinning in w')
        # Rest of the output properties.
        self.declareProperty(ITableWorkspaceProperty(PROP_OUTPUT_DETECTOR_EPP_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE,
                                               '',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output workspace for calibrated inciden energy')
        self.declareProperty(ITableWorkspaceProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for flat background')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE,
                                               '',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output workspace for detector diagnostics')

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        fileGiven = not self.getProperty(PROP_INPUT_FILE).isDefault
        wsGiven = not self.getProperty(PROP_INPUT_WORKSPACE).isDefault
        # Validate an input exists
        if fileGiven == wsGiven:
            issues[PROP_INPUT_FILE] = 'Must give either an input file or an input workspace.'

        return issues

    def _convertToWorkspaceIndex(self, i, ws):
        indexType = self.getProperty(PROP_INDEX_TYPE).value
        if indexType == INDEX_TYPE_WORKSPACE_INDEX:
            return i
        elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
            return ws.getIndexFromSpectrumNumber(i)
        else: # INDEX_TYPE_DETECTOR_ID
            for j in range(ws.getNumberHistograms()):
                if ws.getSpectrum(j).hasDetectorID(i):
                    return j
            raise RuntimeError('No workspace index found for detector id {0}'.format(i))

    def _finalize(self, outWS, wsCleanup, report):
        self.setProperty(PROP_OUTPUT_WORKSPACE, outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

AlgorithmFactory.subscribe(DirectILLReduction)
