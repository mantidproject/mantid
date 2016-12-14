# -*- coding: utf-8 -*-

import collections
import glob
from mantid.api import AlgorithmFactory, AlgorithmManagerImpl, AnalysisDataServiceImpl, DataProcessorAlgorithm, FileAction, FileProperty, ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, PropertyMode,  WorkspaceProperty
from mantid.kernel import CompositeValidator, Direct, Direction, FloatBoundedValidator, IntArrayBoundedValidator, IntArrayProperty, IntBoundedValidator, IntMandatoryValidator, StringListValidator, StringMandatoryValidator, UnitConversion
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground, ClearMaskFlag,\
                             CloneWorkspace, ComputeCalibrationCoefVan, ConvertToConstantL2,\
                             ConvertUnits, CorrectKiKf, CreateSingleValuedWorkspace, CreateWorkspace, DeleteWorkspace, DetectorEfficiencyCorUser, Divide, ExtractMonitors, ExtractSpectra, \
                             FindDetectorsOutsideLimits, FindEPP, GetEiMonDet, GroupWorkspaces, Integration, Load,\
                             MaskDetectors, MedianDetectorTest, MergeRuns, Minus, Multiply, NormaliseToMonitor, Plus, Rebin, Scale
import numpy
from os import path

CLEANUP_DELETE = 'Delete Intermediate Workspaces'
CLEANUP_KEEP   = 'Keep Intermediate Workspaces'

DIAGNOSTICS_NO    = 'No Detector Diagnostics'
DIAGNOSTICS_YES   = 'Diagnose Detectors'

INCIDENT_ENERGY_CALIBRATION_NO  = 'No Incident Energy Calibration'
INCIDENT_ENERGY_CALIBRATION_YES = 'Calibrate Incident Energy'

INDEX_TYPE_DETECTOR_ID     = 'Detector ID'
INDEX_TYPE_WORKSPACE_INDEX = 'Workspace Index'
INDEX_TYPE_SPECTRUM_NUMBER = 'Spectrum Number'

NORM_METHOD_MONITOR = 'Monitor'
NORM_METHOD_OFF     = 'No Normalisation'
NORM_METHOD_TIME    = 'Acquisition Time'

SUBALG_LOGGING_OFF = 'No Subalgorithm Logging'
SUBALG_LOGGING_ON  = 'Allow Subalgorithm Logging'

PROP_BINNING_Q                          = 'QBinning'
PROP_BINNING_W                          = 'WBinning'
PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD     = 'NoisyBkgDiagnosticsHighThreshold'
PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD      = 'NoisyBkgDiagnosticsLowThreshold'
PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST  = 'NoisyBkgDiagnosticsErrorThreshold'
PROP_CD_WORKSPACE                       = 'CadmiumWorkspace'
PROP_CLEANUP_MODE                       = 'Cleanup'
PROP_DIAGNOSTICS_WORKSPACE              = 'DiagnosticsWorkspace'
PROP_DETECTOR_DIAGNOSTICS               = 'Diagnostics'
PROP_DETECTORS_AT_L2                    = 'DetectorsAtL2'
PROP_EC_WORKSPACE                       = 'EmptyCanWorkspace'
PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER      = 'ElasticPeakWidthInSigmas'
PROP_EPP_WORKSPACE                      = 'EPPWorkspace'
PROP_FLAT_BKG_SCALING                   = 'FlatBkgScaling'
PROP_FLAT_BKG_WINDOW                    = 'FlatBkgAveragingWindow'
PROP_FLAT_BKG_WORKSPACE                 = 'FlatBkgWorkspace'
PROP_INCIDENT_ENERGY_CALIBRATION        = 'IncidentEnergyCalibration'
PROP_INCIDENT_ENERGY_WORKSPACE          = 'IncidentEnergyWorkspace'
PROP_INDEX_TYPE                         = 'IndexType'
PROP_INITIAL_ELASTIC_PEAK_REFERENCE     = 'InitialElasticPeakReference'
PROP_INPUT_FILE                         = 'InputFile'
PROP_INPUT_WORKSPACE                    = 'InputWorkspace'
PROP_MONITOR_EPP_WORKSPACE              = 'MonitorEPPWorkspace'
PROP_MONITOR_INDEX                      = 'Monitor'
PROP_NORMALISATION                      = 'Normalisation'
PROP_OUTPUT_DIAGNOSTICS_WORKSPACE       = 'OutputDiagnosticsWorkspace'
PROP_OUTPUT_DETECTOR_EPP_WORKSPACE      = 'OutputEPPWorkspace'
PROP_OUTPUT_FLAT_BKG_WORKSPACE          = 'OutputFlatBkgWorkspace'
PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE   = 'OutputIncidentEnergyWorkspace'
PROP_OUTPUT_MONITOR_EPP_WORKSPACE       = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_WORKSPACE                   = 'OutputWorkspace'
PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD    = 'ElasticPeakDiagnosticsHighThreshold'
PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD     = 'ElasticPeakDiagnosticsLowThreshold'
PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = 'ElasticPeakDiagnosticsErrorThreshold'
PROP_REDUCTION_TYPE                     = 'ReductionType'
PROP_TRANSMISSION                       = 'Transmission'
PROP_SUBALGORITHM_LOGGING               = 'SubalgorithmLogging'
PROP_USER_MASK                          = 'MaskedDetectors'
PROP_VANADIUM_WORKSPACE                 = 'VanadiumWorkspace'

REDUCTION_TYPE_CD = 'Empty Container/Cadmium'
REDUCTION_TYPE_EC = REDUCTION_TYPE_CD
REDUCTION_TYPE_SAMPLE = 'Sample'
REDUCTION_TYPE_VANADIUM = 'Vanadium'

WS_CONTENT_DETECTORS = 0
WS_CONTENT_MONITORS = 1

class DiagnosticsSettings:
    '''
    Holds settings for MedianDetectorTest.
    '''
    def __init__(self, lowThreshold, highThreshold, significanceTest):
        self.lowThreshold = lowThreshold
        self.highThreshold = highThreshold
        self.significanceTest = significanceTest

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

def _createFlatBkg(ws, wsType, windowWidth, wsNames, algorithmLogging):
    '''
    Returns a flat background workspace.
    '''
    if wsType == WS_CONTENT_DETECTORS:
        bkgWSName = wsNames.withSuffix('flat_bkg_for_detectors')
    else:
        bkgWSName = wsNames.withSuffix('flat_bkg_for_monitors')
    bkgWS = CalculateFlatBackground(InputWorkspace=ws,
                                    OutputWorkspace=bkgWSName,
                                    Mode='Moving Average',
                                    OutputMode='Return Background',
                                    SkipMonitors=False,
                                    NullifyNegativeValues=False,
                                    AveragingWindowWidth=windowWidth,
                                    EnableLogging=algorithmLogging)
    return bkgWS

def _subtractFlatBkg(ws, wsType, bkgWorkspace, bkgScaling, wsNames, wsCleanup, algorithmLogging):
    '''
    Subtracts a scaled flat background from a workspace.
    '''
    if wsType == WS_CONTENT_DETECTORS:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_detectors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_detectors_scaled')
    else:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_monitors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_monitors_scaled')
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

def _reportSinglePeakDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of zero count diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass # Nothing to report.
    elif diagnose == 1:
        report.notice('Workspace index {0} did not pass elastic peak diagnostics.'.format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad for ' +
            'masking by elastic peakd diagnostics for unknown reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.notice(('Workspace index {0} has been marked as an outlier ' +
            'and excluded from the peak median intensity calculation in ' +
            'elastic peak diagnostics.').format(wsIndex))

def _reportSingleBkgDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of background diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass # Nothing to report.
    elif diagnose == 1:
        report.notice('Workspace index {0} did not pass noisy background diagnostics.'.format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad ' +
            'by noisy background diagnostics for unknown reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.notice(('Workspace index {0} has been marked as an outlier ' +
            'and excluded from the background median calculation in ' +
            'noisy background diagnostics.').format(wsIndex))

def _nominalTOF(ws):
     ei = ws.getRun().getLogData('Ei').value
     l1 = ws.getRun().getLogData('L1').value

def _reportDiagnostics(report, peakDiagnostics, bkgDiagnostics):
    '''
    Parses the mask workspaces and fills in the report accordingly.
    '''
    for i in range(peakDiagnostics.getNumberHistograms()):
        _reportSinglePeakDiagnostics(report,
                                     peakDiagnostics,
                                     i)
        _reportSingleBkgDiagnostics(report,
                                    bkgDiagnostics,
                                    i)

def _medianEPP(eppWS, eppIndices):
    centres = list()
    sigmas = list()
    for rowIndex in eppIndices:
        eppRow = eppWS.row(rowIndex)
        if eppRow['FitStatus'] == 'success':
            centres.append(eppRow['PeakCentre'])
            sigmas.append(eppRow['Sigma'])
    if len(centres) == 0:
        raise RuntimeError('No successes in EPP table.')
    median = numpy.median(numpy.array(centres))
    sigma = numpy.median(numpy.array(sigmas))
    return median, sigma
    

def _diagnoseDetectors(ws, bkgWS, eppWS, eppIndices, peakSettings, sigmaMultiplier, noisyBkgSettings, wsNames, wsCleanup, report, algorithmLogging):
    '''
    Returns a diagnostics workspace.
    '''
    # 1. Diagnose elastic peak region.
    constantL2WSName = wsNames.withSuffix('constant_L2')
    constantL2WS = ConvertToConstantL2(InputWorkspace=ws,
                                       OutputWorkspace=constantL2WSName,
                                       EnableLogging=algorithmLogging)
    medianEPPCentre, medianEPPSigma = _medianEPP(eppWS, eppIndices)
    elasticPeakDiagnosticsWSName = wsNames.withSuffix('diagnostics_elastic_peak')
    integrationBegin = medianEPPCentre - sigmaMultiplier * medianEPPSigma
    integrationEnd = medianEPPCentre + sigmaMultiplier * medianEPPSigma
    elasticPeakDiagnostics, nFailures = MedianDetectorTest(InputWorkspace=constantL2WS,
                                                           OutputWorkspace=elasticPeakDiagnosticsWSName,
                                                           SignificanceTest=peakSettings.significanceTest,
                                                           LowThreshold=peakSettings.lowThreshold,
                                                           HighThreshold=peakSettings.highThreshold,
                                                           CorrectForSolidAngle=True,
                                                           RangeLower=integrationBegin,
                                                           RangeUpper=integrationEnd,
                                                           EnableLogging=algorithmLogging)
    # 2. Diagnose backgrounds.
    noisyBkgWSName = wsNames.withSuffix('diagnostics_noisy_bkg')
    noisyBkgDiagnostics, nFailures = MedianDetectorTest(InputWorkspace=bkgWS,
                                                        OutputWorkspace=noisyBkgWSName,
                                                        SignificanceTest=noisyBkgSettings.significanceTest,
                                                        LowThreshold=noisyBkgSettings.lowThreshold,
                                                        HighThreshold=noisyBkgSettings.highThreshold,
                                                        LowOutlier=0.0,
                                                        EnableLogging=algorithmLogging)
    _reportDiagnostics(report, elasticPeakDiagnostics, noisyBkgDiagnostics)
    ClearMaskFlag(Workspace=noisyBkgDiagnostics,
                  EnableLogging=algorithmLogging)
    combinedDiagnosticsWSName = wsNames.withSuffix('diagnostics')
    diagnosticsWS = Plus(LHSWorkspace=elasticPeakDiagnostics,
                         RHSWorkspace=noisyBkgDiagnostics,
                         OutputWorkspace=combinedDiagnosticsWSName,
                         EnableLogging=algorithmLogging)
    wsCleanup.cleanup(constantL2WS)
    wsCleanup.cleanup(elasticPeakDiagnostics)
    wsCleanup.cleanup(noisyBkgDiagnostics)
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
        # TODO Remove the IndexType hack when the GetEiMonDet pull request
        #      is merged.
        energy = GetEiMonDet(DetectorWorkspace=detWorkspace,
                             DetectorEPPTable=detEPPWorkspace,
                             IndexType=indexType.replace(' ', ''),
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

def _normalizeToMonitor(ws, monWS, monEPPWS, sigmaMultiplier, monIndex, wsNames, wsCleanup, algorithmLogging):
    '''
    Normalizes to monitor counts.
    '''
    normalizedWSName = wsNames.withSuffix('normalized_to_monitor')
    normalizationFactorWsName = wsNames.withSuffix('normalization_factor_monitor')
    eppRow = monEPPWS.row(monIndex)
    sigma = eppRow['Sigma']
    centre = eppRow['PeakCentre']
    begin = centre - sigmaMultiplier * sigma
    end = centre + sigmaMultiplier * sigma
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
    Subtracts cadmium corrected emtpy container.
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
    Subtracts empty container.
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
        if self.getProperty(PROP_SUBALGORITHM_LOGGING).value == SUBALG_LOGGING_ON:
            childAlgorithmLogging = True
        reductionType = self.getProperty(PROP_REDUCTION_TYPE).value
        wsNamePrefix = self.getProperty(PROP_OUTPUT_WORKSPACE).valueAsStr
        cleanupMode = self.getProperty(PROP_CLEANUP_MODE).value
        wsNames = NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = IntermediateWsCleanup(cleanupMode, childAlgorithmLogging)
        indexType = self.getProperty(PROP_INDEX_TYPE).value

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

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
        detWS, monWS = _extractMonitorWs(mainWS,
                                         wsNames,
                                         childAlgorithmLogging)
        monIndex = self.getProperty(PROP_MONITOR_INDEX).value
        monIndex = self._convertToWorkspaceIndex(monIndex,
                                                 monWS)
        wsCleanup.cleanup(mainWS)
        mainWS = detWS
        del(detWS)
        wsCleanup.cleanupLater(monWS)

        # Time-independent background
        bkgInWS = self.getProperty(PROP_FLAT_BKG_WORKSPACE).value
        windowWidth = self.getProperty(PROP_FLAT_BKG_WINDOW).value
        if not bkgInWS:
            bkgWS = _createFlatBkg(mainWS,
                                          WS_CONTENT_DETECTORS,
                                          windowWidth,
                                          wsNames,
                                          childAlgorithmLogging)
        else:
            bkgWS = bkgInWS
            wsCleanup.protect(bkgWS)
        if not self.getProperty(PROP_OUTPUT_FLAT_BKG_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_FLAT_BKG_WORKSPACE, bkgWS)
        bkgScaling = self.getProperty(PROP_FLAT_BKG_SCALING).value
        bkgSubtractedWS = _subtractFlatBkg(mainWS,
                                                  WS_CONTENT_DETECTORS,
                                                  bkgWS,
                                                  bkgScaling,
                                                  wsNames,
                                                  wsCleanup,
                                                  childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = bkgSubtractedWS
        del(bkgSubtractedWS)
        wsCleanup.cleanupLater(bkgWS)
        # Monitor time-independent background.
        monBkgWS = _createFlatBkg(monWS,
                                         WS_CONTENT_MONITORS,
                                         windowWidth,
                                         wsNames,
                                         childAlgorithmLogging)
        monBkgScaling = 1
        bkgSubtractedMonWS = _subtractFlatBkg(monWS,
                                                     WS_CONTENT_MONITORS,
                                                     monBkgWS,
                                                     monBkgScaling,
                                                     wsNames,
                                                     wsCleanup,
                                                     childAlgorithmLogging)
        wsCleanup.cleanup(monWS)
        monWS = bkgSubtractedMonWS
        del(bkgSubtractedMonWS)

        # Find elastic peak positions for detectors.
        detEPPInWS = self.getProperty(PROP_EPP_WORKSPACE).value
        if not detEPPInWS:
            detEPPWS = _findEPP(mainWS,
                                WS_CONTENT_DETECTORS,
                                wsNames,
                                childAlgorithmLogging)
        else:
            detEPPWS = detInWS
            wsCleanup.protect(detEPPWS)
        if not self.getProperty(PROP_OUTPUT_DETECTOR_EPP_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_DETECTOR_EPP_WORKSPACE,
                             detEPPWS)
        # Elastic peaks for monitors
        monEPPInWS = self.getProperty(PROP_MONITOR_EPP_WORKSPACE).value
        if not monEPPInWS:
            monEPPWS = _findEPP(monWS,
                                WS_CONTENT_MONITORS,
                                wsNames,
                                childAlgorithmLogging)
        else:
            monEPPWS = monInWS
            wsCleanup.protect(monEPPWS)
        if not self.getProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE).isDefault:
            self.setProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE,
                             monEPPWS)
        wsCleanup.cleanupLater(detEPPWS, monEPPWS)

        # Detector diagnostics, if requested.
        if self.getProperty(PROP_DETECTOR_DIAGNOSTICS).value == DIAGNOSTICS_YES:
            diagnosticsInWS = self.getProperty(PROP_DIAGNOSTICS_WORKSPACE).value
            if not diagnosticsInWS:
                lowThreshold = self.getProperty(PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = self.getProperty(PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD).value
                significanceTest = self.getProperty(PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
                peakDiagnosticsSettings = DiagnosticsSettings(lowThreshold,
                                                              highThreshold,
                                                              significanceTest)
                sigmaMultiplier = self.getProperty(PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                lowThreshold = self.getProperty(PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = self.getProperty(PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD).value
                significanceTest = self.getProperty(PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
                bkgDiagnosticsSettings = DiagnosticsSettings(lowThreshold,
                                                             highThreshold,
                                                             significanceTest)
                detectorsAtL2 = self.getProperty(PROP_DETECTORS_AT_L2).value
                diagnosticsWS = _diagnoseDetectors(mainWS,
                                                   bkgWS,
                                                   detEPPWS,
                                                   detectorsAtL2,
                                                   peakDiagnosticsSettings,
                                                   sigmaMultiplier,
                                                   bkgDiagnosticsSettings,
                                                   wsNames,
                                                   wsCleanup,
                                                   report,
                                                   childAlgorithmLogging)
            else:
                diagnosticsWS = diagnosticsInWS
                wsCleanup.protect(diagnosticsWS)
            if not self.getProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE).isDefault:
                self.setProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE, diagnosticsWS)
            diagnosedWS = _maskDiagnosedDetectors(mainWS,
                                                  diagnosticsWS,
                                                  wsNames,
                                                  childAlgorithmLogging)
            wsCleanup.cleanup(diagnosticsWS)
            del(diagnosticsWS)
            wsCleanup.cleanup(mainWS)
            mainWS = diagnosedWS
            del(diagnosedWS)
        # Apply user mask.
        userMask = self.getProperty(PROP_USER_MASK).value
        maskedWS = _applyUserMask(mainWS, 
                                  userMask,
                                  indexType,
                                  wsNames,
                                  childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = maskedWS
        del(maskedWS)

        # Get calibrated incident energy
        eiCalibration = self.getProperty(PROP_INCIDENT_ENERGY_CALIBRATION).value
        if eiCalibration == INCIDENT_ENERGY_CALIBRATION_YES:
            eiInWS = self.getProperty(PROP_INCIDENT_ENERGY_WORKSPACE).value
            if not eiInWS:
                eiCalibrationDets = self.getProperty(PROP_DETECTORS_AT_L2).value
                eiCalibrationWS = _calibratedIncidentEnergy(mainWS,
                                                            detEPPWS,
                                                            monWS,
                                                            monEPPWS,
                                                            indexType,
                                                            eiCalibrationDets,
                                                            monIndex,
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
                eiCalibratedMonWS = _applyIncidentEnergyCalibration(monWS,
                                                                    WS_CONTENT_MONITORS,
                                                                    eiCalibrationWS,
                                                                    wsNames,
                                                                    report,
                                                                    childAlgorithmLogging)
                wsCleanup.cleanup(monWS)
                monWS = eiCalibratedMonWS
                del(eiCalibratedMonWS)
            if not self.getProperty(PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE).isDefault:
                self.setProperty(PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE, eiCalibrationWS)
            wsCleanup.cleanup(eiCalibrationWS)
            del(eiCalibrationWS)

        # Normalisation to monitor/time
        normalisationMethod = self.getProperty(PROP_NORMALISATION).value
        if normalisationMethod != NORM_METHOD_OFF:
            if normalisationMethod == NORM_METHOD_MONITOR:
                sigmaMultiplier = self.getProperty(PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                normalizedWS = _normalizeToMonitor(mainWS,
                                                   monWS,
                                                   monEPPWS,
                                                   sigmaMultiplier,
                                                   monIndex,
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

        # Reduction for empty container and cadmium ends here.
        if reductionType == REDUCTION_TYPE_CD or reductionType == REDUCTION_TYPE_EC:
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with vanadium and sample reductions.

        # Empty container subtraction
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
                                               EPPTable=detEPPWS,
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
        # Validators.
        greaterThanUnityFloat = FloatBoundedValidator(lower=1)
        mandatoryPositiveInt = CompositeValidator()
        mandatoryPositiveInt.add(IntMandatoryValidator())
        mandatoryPositiveInt.add(IntBoundedValidator(lower=0))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveIntArray = IntArrayBoundedValidator()
        positiveIntArray.setLower(0)
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(FileProperty(name=PROP_INPUT_FILE,
                                          defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='Input file')
        # TODO We may want an input monitor workspace here as well.
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_INPUT_WORKSPACE,
                                                     defaultValue='',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='Input workspace')
        self.declareProperty(WorkspaceProperty(name=PROP_OUTPUT_WORKSPACE,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output of the algorithm')
        self.declareProperty(name=PROP_REDUCTION_TYPE,
                             defaultValue=REDUCTION_TYPE_SAMPLE,
                             validator=StringListValidator([REDUCTION_TYPE_SAMPLE, REDUCTION_TYPE_VANADIUM, REDUCTION_TYPE_CD, REDUCTION_TYPE_EC]),
                             direction=Direction.Input,
                             doc='Type of the reduction workflow and output')
        self.declareProperty(name=PROP_CLEANUP_MODE,
                             defaultValue=CLEANUP_DELETE,
                             validator=StringListValidator([CLEANUP_DELETE, CLEANUP_KEEP]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces')
        self.declareProperty(name=PROP_SUBALGORITHM_LOGGING,
                             defaultValue=SUBALG_LOGGING_OFF,
                             validator=StringListValidator([SUBALG_LOGGING_OFF, SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to print in the logs.')
        self.declareProperty(name=PROP_INITIAL_ELASTIC_PEAK_REFERENCE,
                             defaultValue='',
                             direction=Direction.Input,
                             doc="Reference file or workspace for initial 'EPP' sample log entry")
        self.declareProperty(name=PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the elastic peak in multiples of 'Sigma' in the EPP table.")
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_VANADIUM_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Reduced vanadium workspace')
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_EC_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Reduced empty container workspace')
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_CD_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Reduced cadmium workspace')
        self.declareProperty(ITableWorkspaceProperty(name=PROP_EPP_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm')
        self.declareProperty(ITableWorkspaceProperty(name=PROP_MONITOR_EPP_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm for the monitor workspace')
        self.declareProperty(name=PROP_INDEX_TYPE,
                             defaultValue=INDEX_TYPE_WORKSPACE_INDEX,
                             validator=StringListValidator([INDEX_TYPE_WORKSPACE_INDEX, INDEX_TYPE_SPECTRUM_NUMBER, INDEX_TYPE_DETECTOR_ID]),
                             direction=Direction.Input,
                             doc='Type of numbers in ' + PROP_MONITOR_INDEX + ' and ' + PROP_DETECTORS_AT_L2 + ' properties')
        self.declareProperty(name=PROP_MONITOR_INDEX,
                             defaultValue=0,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Index of the main monitor')
        self.declareProperty(name=PROP_INCIDENT_ENERGY_CALIBRATION,
                             defaultValue=INCIDENT_ENERGY_CALIBRATION_YES,
                             validator=StringListValidator([INCIDENT_ENERGY_CALIBRATION_YES, INCIDENT_ENERGY_CALIBRATION_YES]),
                             direction=Direction.Input,
                             doc='Enable or disable incident energy calibration on IN4 and IN6')
        # TODO This is actually mandatory if Ei calibration or
        #      diagnostics are enabled.
        self.declareProperty(IntArrayProperty(name=PROP_DETECTORS_AT_L2,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of detectors with the instruments nominal L2 distance.')
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_INCIDENT_ENERGY_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A single-valued workspace holding the calibrated incident energy')
        self.declareProperty(name=PROP_FLAT_BKG_SCALING,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Flat background scaling constant')
        self.declareProperty(name=PROP_FLAT_BKG_WINDOW,
                             defaultValue=30,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) for flat background')
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_FLAT_BKG_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace from which to get flat background data')
        self.declareProperty(IntArrayProperty(name=PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask')
        self.declareProperty(name=PROP_DETECTOR_DIAGNOSTICS,
                             defaultValue=DIAGNOSTICS_YES,
                             validator=StringListValidator([DIAGNOSTICS_YES, DIAGNOSTICS_NO]),
                             direction=Direction.Input,
                             doc='If true, run detector diagnostics or apply ' + PROP_DIAGNOSTICS_WORKSPACE)
        self.declareProperty(MatrixWorkspaceProperty(name=PROP_DIAGNOSTICS_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Detector diagnostics workspace obtained from another reduction run.')
        self.declareProperty(name=PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit used in elastic peak diagnostics.')
        self.declareProperty(name=PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.0,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit used in elastic peak diagnostics.')
        self.declareProperty(name=PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance test in the elastic peak diagnostics.')
        self.declareProperty(name=PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit used in noisy background diagnostics.')
        self.declareProperty(name=PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=33.3,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit used in noisy background diagnostics.')
        self.declareProperty(name=PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance test in the noisy background diagnostics.')
        self.declareProperty(name=PROP_NORMALISATION,
                             defaultValue=NORM_METHOD_MONITOR,
                             validator=StringListValidator([NORM_METHOD_MONITOR, NORM_METHOD_TIME, NORM_METHOD_OFF]),
                             direction=Direction.Input,
                             doc='Normalisation method')
        self.declareProperty(name=PROP_TRANSMISSION,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Sample transmission for empty container subtraction')
        self.declareProperty(name=PROP_BINNING_Q,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Rebinning in q')
        self.declareProperty(name=PROP_BINNING_W,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Rebinning in w')
        # Rest of the output properties.
        self.declareProperty(ITableWorkspaceProperty(name=PROP_OUTPUT_DETECTOR_EPP_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(WorkspaceProperty(name=PROP_OUTPUT_INCIDENT_ENERGY_WORKSPACE,
                                               defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output workspace for calibrated inciden energy')
        self.declareProperty(ITableWorkspaceProperty(name=PROP_OUTPUT_MONITOR_EPP_WORKSPACE,
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(WorkspaceProperty(name=PROP_OUTPUT_FLAT_BKG_WORKSPACE,
                                               defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output workspace for flat background')
        self.declareProperty(WorkspaceProperty(name=PROP_OUTPUT_DIAGNOSTICS_WORKSPACE,
                                               defaultValue='',
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
        # Validate that an input exists
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
