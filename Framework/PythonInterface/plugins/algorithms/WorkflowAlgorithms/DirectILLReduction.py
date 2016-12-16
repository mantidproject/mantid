# -*- coding: utf-8 -*-

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, FileAction,\
    FileProperty, ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd,\
    PropertyMode, WorkspaceProperty
from mantid.kernel import CompositeValidator, Direct, Direction,\
    FloatBoundedValidator, IntArrayBoundedValidator, IntArrayProperty,\
    IntBoundedValidator, IntMandatoryValidator, StringListValidator,\
    UnitConversion
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground,\
    ClearMaskFlag, CloneWorkspace, ComputeCalibrationCoefVan,\
    ConvertToConstantL2, ConvertUnits, CorrectKiKf,\
    CreateSingleValuedWorkspace, DeleteWorkspace, DetectorEfficiencyCorUser,\
    Divide, ExtractMonitors, FindEPP, GetEiMonDet, Load, MaskDetectors,\
    MedianDetectorTest, MergeRuns, Minus, Multiply, NormaliseToMonitor,\
    Plus, Rebin, Scale
import numpy
from os import path

_CLEANUP_DELETE = 'Delete Intermediate Workspaces'
_CLEANUP_KEEP = 'Keep Intermediate Workspaces'

_DIAGNOSTICS_NO = 'No Detector Diagnostics'
_DIAGNOSTICS_YES = 'Diagnose Detectors'

_INCIDENT_ENERGY_CALIBRATION_NO = 'No Incident Energy Calibration'
_INCIDENT_ENERGY_CALIBRATION_YES = 'Calibrate Incident Energy'

_INDEX_TYPE_DET_ID = 'Detector ID'
_INDEX_TYPE_WS_INDEX = 'Workspace Index'
_INDEX_TYPE_SPECTRUM_NUMBER = 'Spectrum Number'

_NORM_METHOD_MON = 'Monitor'
_NORM_METHOD_OFF = 'No Normalisation'
_NORM_METHOD_TIME = 'Acquisition Time'

_SUBALG_LOGGING_OFF = 'No Subalgorithm Logging'
_SUBALG_LOGGING_ON = 'Allow Subalgorithm Logging'

_PROP_BINNING_Q = 'QBinning'
_PROP_BINNING_W = 'WBinning'
_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = 'NoisyBkgDiagnosticsHighThreshold'
_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = 'NoisyBkgDiagnosticsLowThreshold'
_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = 'NoisyBkgDiagnosticsErrorThreshold'
_PROP_CD_WS = 'CadmiumWorkspace'
_PROP_CLEANUP_MODE = 'Cleanup'
_PROP_DIAGNOSTICS_WS = 'DiagnosticsWorkspace'
_PROP_DET_DIAGNOSTICS = 'Diagnostics'
_PROP_DETS_AT_L2 = 'DetectorsAtL2'
_PROP_EC_WS = 'EmptyCanWorkspace'
_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER = 'ElasticPeakWidthInSigmas'
_PROP_EPP_WS = 'EPPWorkspace'
_PROP_FLAT_BKG_SCALING = 'FlatBkgScaling'
_PROP_FLAT_BKG_WINDOW = 'FlatBkgAveragingWindow'
_PROP_FLAT_BKG_WS = 'FlatBkgWorkspace'
_PROP_INCIDENT_ENERGY_CALIBRATION = 'IncidentEnergyCalibration'
_PROP_INCIDENT_ENERGY_WS = 'IncidentEnergyWorkspace'
_PROP_INDEX_TYPE = 'IndexType'
_PROP_INITIAL_ELASTIC_PEAK_REFERENCE = 'InitialElasticPeakReference'
_PROP_INPUT_FILE = 'InputFile'
_PROP_INPUT_WS = 'InputWorkspace'
_PROP_MON_EPP_WS = 'MonitorEPPWorkspace'
_PROP_MON_INDEX = 'Monitor'
_PROP_NORMALISATION = 'Normalisation'
_PROP_OUTPUT_DET_EPP_WS = 'OutputEPPWorkspace'
_PROP_OUTPUT_DIAGNOSTICS_WS = 'OutputDiagnosticsWorkspace'
_PROP_OUTPUT_FLAT_BKG_WS = 'OutputFlatBkgWorkspace'
_PROP_OUTPUT_INCIDENT_ENERGY_WS = 'OutputIncidentEnergyWorkspace'
_PROP_OUTPUT_MON_EPP_WS = 'OutputMonitorEPPWorkspace'
_PROP_OUTPUT_WS = 'OutputWorkspace'
_PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD = 'ElasticPeakDiagnosticsHighThreshold'
_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD = 'ElasticPeakDiagnosticsLowThreshold'
_PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = \
    'ElasticPeakDiagnosticsErrorThreshold'
_PROP_REDUCTION_TYPE = 'ReductionType'
_PROP_TRANSMISSION = 'Transmission'
_PROP_SUBALG_LOGGING = 'SubalgorithmLogging'
_PROP_USER_MASK = 'MaskedDetectors'
_PROP_VANA_WS = 'VanadiumWorkspace'

_REDUCTION_TYPE_CD = 'Empty Container/Cadmium'
_REDUCTION_TYPE_EC = _REDUCTION_TYPE_CD
_REDUCTION_TYPE_SAMPLE = 'Sample'
_REDUCTION_TYPE_VANA = 'Vanadium'

_WS_CONTENT_DETS = 0
_WS_CONTENT_MONS = 1


class _DiagnosticsSettings:
    '''
    Holds settings for MedianDetectorTest.
    '''
    def __init__(self, lowThreshold, highThreshold, significanceTest):
        self.lowThreshold = lowThreshold
        self.highThreshold = highThreshold
        self.significanceTest = significanceTest


class _IntermediateWSCleanup:
    '''
    Manages intermediate workspace cleanup.
    '''
    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == _CLEANUP_DELETE
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


class _NameSource:
    '''
    Provides names for intermediate workspaces.
    '''
    def __init__(self, prefix, cleanupMode):
        '''
        Initializes an instance of the class.
        '''
        self._names = set()
        self._prefix = prefix
        if cleanupMode == _CLEANUP_DELETE:
            self._prefix = '__' + prefix

    def withSuffix(self, suffix):
        '''
        Returns a workspace name with given suffix applied.
        '''
        return self._prefix + '_' + suffix


class _Report:
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
        self._entries.append(_Report._Entry(text, _Report._LEVEL_ERROR))

    def notice(self, text):
        '''
        Adds an information entry to this report.
        '''
        self._entries.append(_Report._Entry(text, _Report._LEVEL_NOTICE))

    def warning(self, text):
        '''
        Adds a warning entry to this report.
        '''
        self._entries.append(_Report._Entry(text, _Report._LEVEL_WARNING))

    def toLog(self, log):
        '''
        Writes this report to a log.
        '''
        for entry in self._entries:
            loggingLevel = entry.level()
            if loggingLevel == _Report._LEVEL_NOTICE:
                log.notice(entry.contents())
            elif loggingLevel == _Report._LEVEL_WARNING:
                log.warning(entry.contents())
            elif loggingLevel == _Report._LEVEL_ERROR:
                log.error(entry.contents())


def _loadFiles(inputFilename, eppReference, wsNames, wsCleanup, log,
               algorithmLogging):
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
                break
        if referenceFilename:
            referenceFilename = path.join(head, referenceFilename)
            log.information('Using ' + referenceFilename +
                            ' as initial EPP reference.')
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
    if wsType == _WS_CONTENT_DETS:
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


def _subtractFlatBkg(ws, wsType, bkgWorkspace, bkgScaling, wsNames,
                     wsCleanup, algorithmLogging):
    '''
    Subtracts a scaled flat background from a workspace.
    '''
    if wsType == _WS_CONTENT_DETS:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_detectors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_detectors_scaled')
    else:
        subtractedWSName = wsNames.withSuffix('flat_bkg_subtracted_monitors')
        scaledBkgWSName = wsNames.withSuffix('flat_bkg_for_monitors_scaled')
    Scale(InputWorkspace=bkgWorkspace,
          OutputWorkspace=scaledBkgWSName,
          Factor=bkgScaling,
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
    if wsType == _WS_CONTENT_DETS:
        eppWSName = wsNames.withSuffix('epp_detectors')
    else:
        eppWSName = wsNames.withSuffix('epp_monitors')
    eppWS = FindEPP(InputWorkspace=ws,
                    OutputWorkspace=eppWSName,
                    EnableLogging=algorithmLogging)
    return eppWS


def _reportSinglePeakDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of zero count diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass  # Nothing to report.
    elif diagnose == 1:
        report.notice(('Workspace index {0} did not pass elastic peak ' +
                      'diagnostics.').format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad for ' +
                      'masking by elastic peakd diagnostics for unknown ' +
                      'reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.notice(('Workspace index {0} has been marked as an outlier ' +
                       'and excluded from the peak median intensity ' +
                       'calculation in elastic peak diagnostics.')
                      .format(wsIndex))


def _reportSingleBkgDiagnostics(report, diagnostics, wsIndex):
    '''
    Reports the result of background diagnostics for a single diagnose.
    '''
    diagnose = diagnostics.readY(wsIndex)[0]
    if diagnose == 0:
        pass  # Nothing to report.
    elif diagnose == 1:
        report.notice(('Workspace index {0} did not pass noisy background ' +
                       'diagnostics.').format(wsIndex))
    else:
        report.error(('Workspace index {0} has been marked as bad ' +
                      'by noisy background diagnostics for unknown ' +
                      'reasons.').format(wsIndex))
    det = diagnostics.getDetector(wsIndex)
    if det and det.isMasked():
        report.notice(('Workspace index {0} has been marked as an outlier ' +
                       'and excluded from the background median calculation ' +
                       'in noisy background diagnostics.').format(wsIndex))


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
    '''
    Calculates the median 'PeakCentre' for given workpsace indices.
    '''
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


def _diagnoseDetectors(ws, bkgWS, eppWS, eppIndices, peakSettings,
                       sigmaMultiplier, noisyBkgSettings, wsNames,
                       wsCleanup, report, algorithmLogging):
    '''
    Returns a diagnostics workspace.
    '''
    # 1. Diagnose elastic peak region.
    constantL2WSName = wsNames.withSuffix('constant_L2')
    constantL2WS = ConvertToConstantL2(InputWorkspace=ws,
                                       OutputWorkspace=constantL2WSName,
                                       EnableLogging=algorithmLogging)
    medianEPPCentre, medianEPPSigma = _medianEPP(eppWS, eppIndices)
    elasticPeakDiagnosticsWSName = \
        wsNames.withSuffix('diagnostics_elastic_peak')
    integrationBegin = medianEPPCentre - sigmaMultiplier * medianEPPSigma
    integrationEnd = medianEPPCentre + sigmaMultiplier * medianEPPSigma
    elasticPeakDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=constantL2WS,
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
    noisyBkgDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=bkgWS,
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
    maskedWorkspace = CloneWorkspace(InputWorkspace=ws,
                                     OutputWorkspace=maskedWSName,
                                     EnableLogging=algorithmLogging)
    if indexType == _INDEX_TYPE_DET_ID:
        MaskDetectors(Workspace=maskedWorkspace,
                      DetectorList=mask,
                      EnableLogging=algorithmLogging)
    elif indexType == _INDEX_TYPE_SPECTRUM_NUMBER:
        MaskDetectors(Workspace=maskedWorkspace,
                      SpectraList=mask,
                      EnableLogging=algorithmLogging)
    elif indexType == _INDEX_TYPE_WS_INDEX:
        MaskDetectors(Workspace=maskedWorkspace,
                      WorkspaceIndexList=mask,
                      EnableLogging=algorithmLogging)
    else:
        raise RuntimeError('Unknown ' + _PROP_INDEX_TYPE)
    return maskedWorkspace


def _calibratedIncidentEnergy(detWorkspace, detEPPWorkspace, monWorkspace,
                              monEPPWorkspace, indexType, eiCalibrationDets,
                              eiCalibrationMon, wsNames, log,
                              algorithmLogging):
    '''
    Returns the calibrated incident energy.
    '''
    instrument = detWorkspace.getInstrument().getName()
    eiWorkspace = None
    if instrument in ['IN4', 'IN6']:
        pulseInterval = \
            detWorkspace.getRun().getLogData('pulse_interval').value
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
        eiWorkspace = \
            CreateSingleValuedWorkspace(OutputWorkspace=eiWSName,
                                        DataValue=energy,
                                        EnableLogging=algorithmLogging)
    else:
        log.error('Instrument ' + instrument + ' not supported for ' +
                  'incident energy calibration')
    return eiWorkspace


def _applyIncidentEnergyCalibration(ws, wsType, eiWS, wsNames, report,
                                    algorithmLogging):
    '''
    Updates incident energy and wavelength in the sample logs.
    '''
    originalEnergy = ws.getRun().getLogData('Ei').value
    originalWavelength = ws.getRun().getLogData('wavelength').value
    energy = eiWS.readY(0)[0]
    wavelength = UnitConversion.run('Energy', 'Wavelength', energy, 0,
                                    0, 0, Direct, 5)
    if wsType == _WS_CONTENT_DETS:
        calibratedWSName = \
            wsNames.withSuffix('incident_energy_calibrated_detectors')
    elif wsType == _WS_CONTENT_MONS:
        calibratedWSName = \
            wsNames.withSuffix('incident_energy_calibrated_monitors')
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
    report.notice('Original wavelength: {} new wavelength {}.'
                  .format(originalWavelength, wavelength))
    return calibratedWS


def _normalizeToMonitor(ws, monWS, monEPPWS, sigmaMultiplier, monIndex,
                        wsNames, wsCleanup, algorithmLogging):
    '''
    Normalizes to monitor counts.
    '''
    normalizedWSName = wsNames.withSuffix('normalized_to_monitor')
    normalizationFactorWsName = \
        wsNames.withSuffix('normalization_factor_monitor')
    eppRow = monEPPWS.row(monIndex)
    sigma = eppRow['Sigma']
    centre = eppRow['PeakCentre']
    begin = centre - sigmaMultiplier * sigma
    end = centre + sigmaMultiplier * sigma
    normalizedWS, normalizationFactorWS = \
        NormaliseToMonitor(InputWorkspace=ws,
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
    time = ws.getLogData('actual_time').value
    normalizationFactorWS = \
        CreateSingleValuedWorkspace(OutputWorkspace=normalizationFactorWsName,
                                    DataValue=time,
                                    EnableLogging=algorithmLogging)
    normalizedWS = Divide(LHSWorkspace=ws,
                          RHSWorkspace=normalizationFactorWS,
                          OutputWorkspace=normalizedWSName,
                          EnableLogging=algorithmLogging)
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS


def _subtractECWithCd(ws, ecWS, cdWS, transmission, wsNames, wsCleanup,
                      algorithmLogging):
    '''
    Subtracts cadmium corrected emtpy container.
    '''
    # out = (in - Cd) / transmission - (EC - Cd)
    transmissionWSName = wsNames.withSuffix('transmission')
    transmissionWS = \
        CreateSingleValuedWorkspace(OutputWorkspace=transmissionWSName,
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
    correctedCdSubtractedWSName = \
        wsNames.withSuffix('transmission_corrected_Cd_subtracted')
    correctedCdSubtractedWS = \
        Divide(LHSWorkspace=cdSubtractedWS,
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
    transmissionWS = \
        CreateSingleValuedWorkspace(OutputWorkspace=transmissionWSName,
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


def _normalizeToVana(ws, vanaWS, wsNames, algorithmLogging):
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
    energyConvertedWS = ConvertUnits(InputWorkspace=ws,
                                     OutputWorkspace=energyConvertedWSName,
                                     Target='DeltaE',
                                     EMode='Direct',
                                     EnableLogging=algorithmLogging)
    return energyConvertedWS


def _rebin(ws, params, wsNames, algorithmLogging):
    '''
    Rebins a workspace.
    '''
    rebinnedWSName = wsNames.withSuffix('rebinned')
    rebinnedWS = Rebin(InputWorkspace=ws,
                       OutputWorkspace=rebinnedWSName,
                       Params=params,
                       EnableLogging=algorithmLogging)
    return rebinnedWS


def _correctByKiKf(ws, wsNames, algorithmLogging):
    '''
    Applies the k_i / k_f correction.
    '''
    correctedWSName = wsNames.withSuffix('kikf')
    correctedWS = CorrectKiKf(InputWorkspace=ws,
                              OutputWorkspace=correctedWSName,
                              EnableLogging=algorithmLogging)
    return correctedWS


def _correctByDetectorEfficiency(ws, wsNames, algorithmLogging):
    '''
    Applies detector efficiency corrections.
    '''
    correctedWSName = wsNames.withSuffix('detector_efficiency_corrected')
    correctedWS = DetectorEfficiencyCorUser(InputWorkspace=ws,
                                            OutputWorkspace=correctedWSName,
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
        return 'Data reduction workflow for the direct geometry ' + \
               'time-of-flight spectrometers at ILL.'

    def version(self):
        return 1

    def PyExec(self):
        report = _Report()
        childAlgorithmLogging = False
        if self.getProperty(_PROP_SUBALG_LOGGING).value == \
                _SUBALG_LOGGING_ON:
            childAlgorithmLogging = True
        reductionType = self.getProperty(_PROP_REDUCTION_TYPE).value
        wsNamePrefix = self.getProperty(_PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(_PROP_CLEANUP_MODE).value
        wsNames = _NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = _IntermediateWSCleanup(cleanupMode, childAlgorithmLogging)
        indexType = self.getProperty(_PROP_INDEX_TYPE).value

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

        # Init workspace.
        inputFile = self.getProperty(_PROP_INPUT_FILE).value
        if inputFile:
            eppReference = \
                self.getProperty(_PROP_INITIAL_ELASTIC_PEAK_REFERENCE).value
            mainWS = _loadFiles(inputFile,
                                eppReference,
                                wsNames,
                                wsCleanup,
                                self.log(),
                                childAlgorithmLogging)
        elif self.getProperty(_PROP_INPUT_WS).value:
            mainWS = self.getProperty(_PROP_INPUT_WS).value

        # Extract monitors to a separate workspace
        detWS, monWS = _extractMonitorWs(mainWS,
                                         wsNames,
                                         childAlgorithmLogging)
        monIndex = self.getProperty(_PROP_MON_INDEX).value
        monIndex = self._convertToWorkspaceIndex(monIndex,
                                                 monWS)
        wsCleanup.cleanup(mainWS)
        mainWS = detWS
        del(detWS)
        wsCleanup.cleanupLater(monWS)

        # Time-independent background
        bkgInWS = self.getProperty(_PROP_FLAT_BKG_WS).value
        windowWidth = self.getProperty(_PROP_FLAT_BKG_WINDOW).value
        if not bkgInWS:
            bkgWS = _createFlatBkg(mainWS,
                                   _WS_CONTENT_DETS,
                                   windowWidth,
                                   wsNames,
                                   childAlgorithmLogging)
        else:
            bkgWS = bkgInWS
            wsCleanup.protect(bkgWS)
        if not self.getProperty(_PROP_OUTPUT_FLAT_BKG_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_FLAT_BKG_WS, bkgWS)
        bkgScaling = self.getProperty(_PROP_FLAT_BKG_SCALING).value
        bkgSubtractedWS = _subtractFlatBkg(mainWS,
                                           _WS_CONTENT_DETS,
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
                                  _WS_CONTENT_MONS,
                                  windowWidth,
                                  wsNames,
                                  childAlgorithmLogging)
        monBkgScaling = 1
        bkgSubtractedMonWS = _subtractFlatBkg(monWS,
                                              _WS_CONTENT_MONS,
                                              monBkgWS,
                                              monBkgScaling,
                                              wsNames,
                                              wsCleanup,
                                              childAlgorithmLogging)
        wsCleanup.cleanup(monWS)
        monWS = bkgSubtractedMonWS
        del(bkgSubtractedMonWS)

        # Find elastic peak positions for detectors.
        detEPPInWS = self.getProperty(_PROP_EPP_WS).value
        if not detEPPInWS:
            detEPPWS = _findEPP(mainWS,
                                _WS_CONTENT_DETS,
                                wsNames,
                                childAlgorithmLogging)
        else:
            detEPPWS = detEPPInWS
            wsCleanup.protect(detEPPWS)
        if not self.getProperty(_PROP_OUTPUT_DET_EPP_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_DET_EPP_WS,
                             detEPPWS)
        # Elastic peaks for monitors
        monEPPInWS = self.getProperty(_PROP_MON_EPP_WS).value
        if not monEPPInWS:
            monEPPWS = _findEPP(monWS,
                                _WS_CONTENT_MONS,
                                wsNames,
                                childAlgorithmLogging)
        else:
            monEPPWS = monEPPInWS
            wsCleanup.protect(monEPPWS)
        if not self.getProperty(_PROP_OUTPUT_MON_EPP_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_MON_EPP_WS,
                             monEPPWS)
        wsCleanup.cleanupLater(detEPPWS, monEPPWS)

        # Detector diagnostics, if requested.
        if self.getProperty(_PROP_DET_DIAGNOSTICS).value == \
                _DIAGNOSTICS_YES:
            diagnosticsInWS = \
                self.getProperty(_PROP_DIAGNOSTICS_WS).value
            if not diagnosticsInWS:
                lowThreshold = \
                    self.getProperty(_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = \
                    self.getProperty(
                        _PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD).value
                significanceTest = \
                    self.getProperty(
                        _PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
                peakDiagnosticsSettings = _DiagnosticsSettings(lowThreshold,
                                                              highThreshold,
                                                              significanceTest)
                sigmaMultiplier = \
                    self.getProperty(_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                lowThreshold = \
                    self.getProperty(_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = \
                    self.getProperty(_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD).value
                significanceTest = \
                    self.getProperty(
                        _PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
                bkgDiagnosticsSettings = _DiagnosticsSettings(lowThreshold,
                                                             highThreshold,
                                                             significanceTest)
                detectorsAtL2 = self.getProperty(_PROP_DETS_AT_L2).value
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
            if not self.getProperty(
                    _PROP_OUTPUT_DIAGNOSTICS_WS).isDefault:
                self.setProperty(_PROP_OUTPUT_DIAGNOSTICS_WS,
                                 diagnosticsWS)
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
        userMask = self.getProperty(_PROP_USER_MASK).value
        maskedWS = _applyUserMask(mainWS,
                                  userMask,
                                  indexType,
                                  wsNames,
                                  childAlgorithmLogging)
        wsCleanup.cleanup(mainWS)
        mainWS = maskedWS
        del(maskedWS)

        # Get calibrated incident energy
        eiCalibration = \
            self.getProperty(_PROP_INCIDENT_ENERGY_CALIBRATION).value
        if eiCalibration == _INCIDENT_ENERGY_CALIBRATION_YES:
            eiInWS = self.getProperty(_PROP_INCIDENT_ENERGY_WS).value
            if not eiInWS:
                eiCalibrationDets = \
                    self.getProperty(_PROP_DETS_AT_L2).value
                eiCalibrationWS = \
                    _calibratedIncidentEnergy(mainWS,
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
                eiCalibratedDetWS = \
                    _applyIncidentEnergyCalibration(mainWS,
                                                    _WS_CONTENT_DETS,
                                                    eiCalibrationWS,
                                                    wsNames,
                                                    report,
                                                    childAlgorithmLogging)
                wsCleanup.cleanup(mainWS)
                mainWS = eiCalibratedDetWS
                del(eiCalibratedDetWS)
                eiCalibratedMonWS = \
                    _applyIncidentEnergyCalibration(monWS,
                                                    _WS_CONTENT_MONS,
                                                    eiCalibrationWS,
                                                    wsNames,
                                                    report,
                                                    childAlgorithmLogging)
                wsCleanup.cleanup(monWS)
                monWS = eiCalibratedMonWS
                del(eiCalibratedMonWS)
            if not self.getProperty(
                    _PROP_OUTPUT_INCIDENT_ENERGY_WS).isDefault:
                self.setProperty(_PROP_OUTPUT_INCIDENT_ENERGY_WS,
                                 eiCalibrationWS)
            wsCleanup.cleanup(eiCalibrationWS)
            del(eiCalibrationWS)

        # Normalisation to monitor/time
        normalisationMethod = self.getProperty(_PROP_NORMALISATION).value
        if normalisationMethod != _NORM_METHOD_OFF:
            if normalisationMethod == _NORM_METHOD_MON:
                sigmaMultiplier = \
                    self.getProperty(_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                normalizedWS = _normalizeToMonitor(mainWS,
                                                   monWS,
                                                   monEPPWS,
                                                   sigmaMultiplier,
                                                   monIndex,
                                                   wsNames,
                                                   wsCleanup,
                                                   childAlgorithmLogging)
            elif normalisationMethod == _NORM_METHOD_TIME:
                normalizedWS = _normalizeToTime(mainWS,
                                                wsNames,
                                                wsCleanup,
                                                childAlgorithmLogging)
            else:
                raise RuntimeError('Unknonwn normalisation method ' +
                                   normalisationMethod)
            wsCleanup.cleanup(mainWS)
            mainWS = normalizedWS
            del(normalizedWS)

        # Reduction for empty container and cadmium ends here.
        if reductionType == _REDUCTION_TYPE_CD or reductionType == \
                _REDUCTION_TYPE_EC:
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with vanadium and sample reductions.

        # Empty container subtraction
        ecInWS = self.getProperty(_PROP_EC_WS).value
        if ecInWS:
            cdInWS = self.getProperty(_PROP_CD_WS).value
            transmission = self.getProperty(_PROP_TRANSMISSION).value
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
        if reductionType == _REDUCTION_TYPE_VANA:
            # We output an integrated vanadium, ready to be used for
            # normalization.
            outWS = self.getPropertyValue(_PROP_OUTPUT_WS)
            # TODO For the time being, we may just want to integrate
            # the vanadium data as `ComputeCalibrationCoef` does not do
            # the best possible Debye-Waller correction.
            mainWS = \
                ComputeCalibrationCoefVan(VanadiumWorkspace=mainWS,
                                          EPPTable=detEPPWS,
                                          OutputWorkspace=outWS,
                                          EnableLogging=childAlgorithmLogging)
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with sample reduction.

        # Vanadium normalization.
        # TODO Absolute normalization.
        vanaWS = self.getProperty(_PROP_VANA_WS).value
        if vanaWS:
            vanaNormalizedWS = _normalizeToVana(mainWS,
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
        params = self.getProperty(_PROP_BINNING_W).value
        if params:
            rebinnedWS = _rebin(mainWS,
                                params,
                                wsNames,
                                childAlgorithmLogging)
            wsCleanup.cleanup(mainWS)
            mainWS = rebinnedWS
            del(rebinnedWS)

        # Detector efficiency correction
        efficiencyCorrectedWS = \
            _correctByDetectorEfficiency(mainWS,
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
        self.declareProperty(FileProperty(name=_PROP_INPUT_FILE,
                                          defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='Input file')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_INPUT_WS,
            defaultValue='',
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(WorkspaceProperty(name=_PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output of the algorithm.')
        self.declareProperty(name=_PROP_REDUCTION_TYPE,
                             defaultValue=_REDUCTION_TYPE_SAMPLE,
                             validator=StringListValidator([
                                 _REDUCTION_TYPE_SAMPLE,
                                 _REDUCTION_TYPE_VANA,
                                 _REDUCTION_TYPE_CD,
                                 _REDUCTION_TYPE_EC]),
                             direction=Direction.Input,
                             doc='Type of the reduction workflow and output.')
        self.declareProperty(name=_PROP_CLEANUP_MODE,
                             defaultValue=_CLEANUP_DELETE,
                             validator=StringListValidator([
                                 _CLEANUP_DELETE,
                                 _CLEANUP_KEEP]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=_PROP_SUBALG_LOGGING,
                             defaultValue=_SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 _SUBALG_LOGGING_OFF,
                                 _SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to ' +
                                 'print in the logs.')
        self.declareProperty(name=_PROP_INITIAL_ELASTIC_PEAK_REFERENCE,
                             defaultValue='',
                             direction=Direction.Input,
                             doc="Reference file or workspace for " +
                                 "initial 'EPP' sample log entry.")
        self.declareProperty(name=_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the elastic peak in multiples " +
                                 " of 'Sigma' in the EPP table.")
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_VANA_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced vanadium workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_EC_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced empty container workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_CD_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced cadmium workspace.')
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Table workspace containing results from the FindEPP ' +
                'algorithm.')
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_MON_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Table workspace containing results from the FindEPP ' +
                'algorithm for the monitor workspace.')
        self.declareProperty(name=_PROP_INDEX_TYPE,
                             defaultValue=_INDEX_TYPE_WS_INDEX,
                             validator=StringListValidator([
                                 _INDEX_TYPE_WS_INDEX,
                                 _INDEX_TYPE_SPECTRUM_NUMBER,
                                 _INDEX_TYPE_DET_ID]),
                             direction=Direction.Input,
                             doc='Type of numbers in ' +
                                 _PROP_MON_INDEX +
                                 ' and ' + _PROP_DETS_AT_L2 +
                                 ' properties.')
        self.declareProperty(name=_PROP_MON_INDEX,
                             defaultValue=0,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Index of the main monitor.')
        self.declareProperty(name=_PROP_INCIDENT_ENERGY_CALIBRATION,
                             defaultValue=_INCIDENT_ENERGY_CALIBRATION_YES,
                             validator=StringListValidator([
                                 _INCIDENT_ENERGY_CALIBRATION_YES,
                                 _INCIDENT_ENERGY_CALIBRATION_NO]),
                             direction=Direction.Input,
                             doc='Enable or disable incident energy ' +
                                 'calibration on IN4 and IN6.')
        # TODO This is actually mandatory if Ei calibration or
        #      diagnostics are enabled.
        self.declareProperty(IntArrayProperty(name=_PROP_DETS_AT_L2,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of detectors with the instruments ' +
                                 'nominal L2 distance.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_INCIDENT_ENERGY_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='A single-valued workspace holding the calibrated ' +
                'incident energy.')
        self.declareProperty(name=_PROP_FLAT_BKG_SCALING,
                             defaultValue=1.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Flat background scaling constant')
        self.declareProperty(name=_PROP_FLAT_BKG_WINDOW,
                             defaultValue=30,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) ' +
                                 'for flat background.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Workspace from which to get flat background data.')
        self.declareProperty(IntArrayProperty(name=_PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask.')
        self.declareProperty(name=_PROP_DET_DIAGNOSTICS,
                             defaultValue=_DIAGNOSTICS_YES,
                             validator=StringListValidator([
                                 _DIAGNOSTICS_YES,
                                 _DIAGNOSTICS_NO]),
                             direction=Direction.Input,
                             doc='If true, run detector diagnostics or ' +
                                 'apply ' + _PROP_DIAGNOSTICS_WS + '.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Detector diagnostics workspace obtained from another ' +
                'reduction run.')
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.0,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the elastic peak diagnostics.')
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=33.3,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the noisy background diagnostics.')
        self.declareProperty(name=_PROP_NORMALISATION,
                             defaultValue=_NORM_METHOD_MON,
                             validator=StringListValidator([
                                 _NORM_METHOD_MON,
                                 _NORM_METHOD_TIME,
                                 _NORM_METHOD_OFF]),
                             direction=Direction.Input,
                             doc='Normalisation method.')
        self.declareProperty(name=_PROP_TRANSMISSION,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Sample transmission for empty container ' +
                                 'subtraction.')
        self.declareProperty(name=_PROP_BINNING_Q,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Rebinning in q.')
        self.declareProperty(name=_PROP_BINNING_W,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Rebinning in w.')
        # Rest of the output properties.
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_OUTPUT_DET_EPP_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for elastic peak positions.')
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_INCIDENT_ENERGY_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for calibrated inciden energy.')
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_OUTPUT_MON_EPP_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for elastic peak positions.')
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for flat background.')
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for detector diagnostics.')

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        fileGiven = not self.getProperty(_PROP_INPUT_FILE).isDefault
        wsGiven = not self.getProperty(_PROP_INPUT_WS).isDefault
        # Validate that an input exists
        if fileGiven == wsGiven:
            issues[_PROP_INPUT_FILE] = \
                'Must give either an input file or an input workspace.'
        return issues

    def _convertToWorkspaceIndex(self, i, ws):
        '''
        Converts given number to workspace index.
        '''
        indexType = self.getProperty(_PROP_INDEX_TYPE).value
        if indexType == _INDEX_TYPE_WS_INDEX:
            return i
        elif indexType == _INDEX_TYPE_SPECTRUM_NUMBER:
            return ws.getIndexFromSpectrumNumber(i)
        else:  # _INDEX_TYPE_DET_ID
            for j in range(ws.getNumberHistograms()):
                if ws.getSpectrum(j).hasDetectorID(i):
                    return j
            raise RuntimeError('No workspace index found for detector id ' +
                               '{0}'.format(i))

    def _finalize(self, outWS, wsCleanup, report):
        '''
        Does final cleanup, reporting and sets the output property.
        '''
        self.setProperty(_PROP_OUTPUT_WS, outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

AlgorithmFactory.subscribe(DirectILLReduction)
