# -*- coding: utf-8 -*-

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction,
                        FileProperty, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty,
                        mtd, PropertyMode, WorkspaceProperty,
                        WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direct, Direction,
                           EnabledWhenProperty, FloatArrayProperty,
                           FloatBoundedValidator, IntArrayBoundedValidator,
                           IntArrayProperty, IntBoundedValidator,
                           IntMandatoryValidator, PropertyCriterion,
                           StringListValidator, UnitConversion)
from mantid.simpleapi import (AddSampleLog, BinWidthAtX,
                              CalculateFlatBackground, ClearMaskFlag,
                              CloneWorkspace, ComputeCalibrationCoefVan,
                              ConvertToConstantL2, ConvertUnits, CorrectKiKf,
                              CreateSingleValuedWorkspace, DeleteWorkspace,
                              DetectorEfficiencyCorUser, Divide,
                              ExtractMonitors, FindEPP, GetEiMonDet, Load,
                              MaskDetectors, MedianBinWidth,
                              MedianDetectorTest, MergeRuns, Minus, Multiply,
                              NormaliseToMonitor, Plus, Rebin, Scale)
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

_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = 'NoisyBkgDiagnosticsHighThreshold'
_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = 'NoisyBkgDiagnosticsLowThreshold'
_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = 'NoisyBkgDiagnosticsErrorThreshold'
_PROP_CD_WS = 'CadmiumWorkspace'
_PROP_CLEANUP_MODE = 'Cleanup'
_PROP_DIAGNOSTICS_WS = 'DiagnosticsWorkspace'
_PROP_DET_DIAGNOSTICS = 'Diagnostics'
_PROP_DETS_AT_L2 = 'DetectorsAtL2'
_PROP_EC_WS = 'EmptyContainerWorkspace'
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
_PROP_REBINNING_MODE_W = 'EnergyRebinningMode'
_PROP_REBINNING_PARAMS_Q = 'QRebinningParams'
_PROP_REBINNING_PARAMS_W = 'EnergyRebinningParams'
_PROP_REDUCTION_TYPE = 'ReductionType'
_PROP_TRANSMISSION = 'Transmission'
_PROP_SUBALG_LOGGING = 'SubalgorithmLogging'
_PROP_USER_MASK = 'MaskedDetectors'
_PROP_VANA_WS = 'VanadiumWorkspace'

_PROPGROUP_REBINNING = 'Rebinning for SofQW'

_REBIN_AUTO_ELASTIC_PEAK = 'Rebin to Bin Width at Elastic Peak'
_REBIN_AUTO_MEDIAN_BIN_WIDTH = 'Rebin to Median Bin Width'
_REBIN_MANUAL = 'Manual Rebinning'

_REDUCTION_TYPE_EC_CD = 'Empty Container/Cadmium'
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
        subalgLogging = False
        if self.getProperty(_PROP_SUBALG_LOGGING).value == \
                _SUBALG_LOGGING_ON:
            subalgLogging = True
        reductionType = self.getProperty(_PROP_REDUCTION_TYPE).value
        wsNamePrefix = self.getProperty(_PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(_PROP_CLEANUP_MODE).value
        wsNames = _NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = _IntermediateWSCleanup(cleanupMode, subalgLogging)

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

        # Get input workspace.
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        # Extract monitors to a separate workspace.
        mainWS, monWS = self._separateMons(mainWS, wsNames,
                                           wsCleanup, subalgLogging)

        # Time-independent background.
        mainWS, bkgWS = self._flatBkgDet(mainWS, wsNames,
                                         wsCleanup, subalgLogging)
        monWS = self._flatBkgMon(monWS, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanupLater(bkgWS)

        # Find elastic peak positions.
        detEPPWS = self._createEPPWSDet(mainWS, wsNames,
                                        wsCleanup, subalgLogging)
        monEPPWS = self._createEPPWSMon(monWS, wsNames,
                                        wsCleanup, subalgLogging)
        wsCleanup.cleanupLater(detEPPWS, monEPPWS)

        # Detector diagnostics, if requested.
        mainWS = self._detDiagnostics(mainWS, bkgWS, detEPPWS, wsNames,
                                      wsCleanup, report, subalgLogging)
        # Apply user mask.
        mainWS = self._applyUserMask(mainWS, wsNames,
                                     wsCleanup, subalgLogging)

        # Calibrate incident energy, if requested.
        mainWS, monWS = self._calibrateEi(mainWS, detEPPWS, monWS, monEPPWS,
                                          wsNames, wsCleanup, report,
                                          subalgLogging)
        wsCleanup.cleanupLater(monWS)

        # Normalisation to monitor/time, if requested.
        mainWS = self._normalize(mainWS, monWS, monEPPWS, wsNames,
                                 wsCleanup, subalgLogging)

        # Reduction for empty container and cadmium ends here.
        if reductionType == _REDUCTION_TYPE_EC_CD:
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with vanadium and sample reductions.

        # Empty container subtraction, if requested.
        mainWS = self._subtractEC(mainWS, wsNames, wsCleanup, subalgLogging)

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
                                          EnableLogging=subalgLogging)
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with sample reduction.

        # Vanadium normalization.
        # TODO Absolute normalization.
        mainWS = self._normalizeToVana(mainWS, wsNames, wsCleanup,
                                       subalgLogging)

        # Convert units from TOF to energy.
        mainWS = self._convertTOFToDeltaE(mainWS, wsNames, wsCleanup,
                                          subalgLogging)

        # KiKf conversion.
        mainWS = self._correctByKiKf(mainWS, wsNames,
                                     wsCleanup, subalgLogging)

        # Rebinning.
        mainWS = self._rebinInW(mainWS, wsNames, wsCleanup, report,
                                subalgLogging)

        # Detector efficiency correction.
        mainWS = self._correctByDetectorEfficiency(mainWS, wsNames,
                                                   wsCleanup, subalgLogging)

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
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))

        # Properties.
        self.declareProperty(FileProperty(name=_PROP_INPUT_FILE,
                                          defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='Input file')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
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
                                 _REDUCTION_TYPE_EC_CD]),
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
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced vanadium workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_EC_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced empty container workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_CD_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
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
        self.declareProperty(name=_PROP_REBINNING_MODE_W,
                             defaultValue=_REBIN_AUTO_ELASTIC_PEAK,
                             validator=StringListValidator([
                                 _REBIN_AUTO_ELASTIC_PEAK,
                                 _REBIN_AUTO_MEDIAN_BIN_WIDTH,
                                 _REBIN_MANUAL]),
                             direction=Direction.Input,
                             doc='Energy rebinnin mode.')
        self.setPropertySettings(_PROP_REBINNING_MODE_W, EnabledWhenProperty(
            _PROP_REDUCTION_TYPE, PropertyCriterion.IsEqualTo,
            _REDUCTION_TYPE_SAMPLE))
        self.setPropertyGroup(_PROP_REBINNING_MODE_W, _PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=_PROP_REBINNING_PARAMS_W),
                             doc='Manual energy rebinning parameters.')
        self.setPropertySettings(_PROP_REBINNING_PARAMS_W, EnabledWhenProperty(
            _PROP_REDUCTION_TYPE, PropertyCriterion.IsEqualTo,
            _REDUCTION_TYPE_SAMPLE))
        self.setPropertyGroup(_PROP_REBINNING_PARAMS_W, _PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=_PROP_REBINNING_PARAMS_Q),
                             doc='Rebinning parameters for q.')
        self.setPropertySettings(_PROP_REBINNING_PARAMS_Q, EnabledWhenProperty(
            _PROP_REDUCTION_TYPE, PropertyCriterion.IsEqualTo,
            _REDUCTION_TYPE_SAMPLE))
        self.setPropertyGroup(_PROP_REBINNING_PARAMS_Q, _PROPGROUP_REBINNING)
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
        '''
        Checks for issues with user input.
        '''
        issues = dict()

        fileGiven = not self.getProperty(_PROP_INPUT_FILE).isDefault
        wsGiven = not self.getProperty(_PROP_INPUT_WS).isDefault
        # Validate that an input exists
        if fileGiven == wsGiven:
            issues[_PROP_INPUT_FILE] = \
                'Must give either an input file or an input workspace.'
        reductionType = self.getProperty(_PROP_REDUCTION_TYPE)
        if reductionType == _REDUCTION_TYPE_SAMPLE:
            if self.getProperty(_PROP_REBINNING_PARAMS_Q):
                issues[_PROP_REBINNING_PARAMS_Q] = _PROP_REBINNING_PARAMS_Q + \
                    ' is mandatory when ' + _PROP_REDUCTION_TYPE + ' is ' + \
                    _REDUCTION_TYPE_SAMPLE + '.'
        return issues

    def _applyUserMask(self, mainWS, wsNames, wsCleanup, algorithmLogging):
        '''
        Applies user mask to a workspace.
        '''
        userMask = self.getProperty(_PROP_USER_MASK).value
        indexType = self.getProperty(_PROP_INDEX_TYPE).value
        maskedWSName = wsNames.withSuffix('masked')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=algorithmLogging)
        if indexType == _INDEX_TYPE_DET_ID:
            MaskDetectors(Workspace=maskedWS,
                          DetectorList=userMask,
                          EnableLogging=algorithmLogging)
        elif indexType == _INDEX_TYPE_SPECTRUM_NUMBER:
            MaskDetectors(Workspace=maskedWS,
                          SpectraList=userMask,
                          EnableLogging=algorithmLogging)
        elif indexType == _INDEX_TYPE_WS_INDEX:
            MaskDetectors(Workspace=maskedWS,
                          WorkspaceIndexList=userMask,
                          EnableLogging=algorithmLogging)
        else:
            raise RuntimeError('Unknown ' + _PROP_INDEX_TYPE)
        wsCleanup.cleanup(mainWS)
        return maskedWS

    def _calibrateEi(self, mainWS, detEPPWS, monWS, monEPPWS, wsNames,
                     wsCleanup, report, subalgLogging):
        '''
        Performs and applies incident energy calibration.
        '''
        eiCalibration = \
            self.getProperty(_PROP_INCIDENT_ENERGY_CALIBRATION).value
        if eiCalibration == _INCIDENT_ENERGY_CALIBRATION_YES:
            eiInWS = self.getProperty(_PROP_INCIDENT_ENERGY_WS).value
            if not eiInWS:
                eiCalibrationDets = \
                    self.getProperty(_PROP_DETS_AT_L2).value
                monIndex = self.getProperty(_PROP_MON_INDEX).value
                monIndex = self._convertToWorkspaceIndex(monIndex, monWS)
                indexType = self.getProperty(_PROP_INDEX_TYPE).value
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
                                              subalgLogging)
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
                                                    subalgLogging)
                wsCleanup.cleanup(mainWS)
                mainWS = eiCalibratedDetWS
                eiCalibratedMonWS = \
                    _applyIncidentEnergyCalibration(monWS,
                                                    _WS_CONTENT_MONS,
                                                    eiCalibrationWS,
                                                    wsNames,
                                                    report,
                                                    subalgLogging)
                wsCleanup.cleanup(monWS)
                monWS = eiCalibratedMonWS
            if not self.getProperty(
                    _PROP_OUTPUT_INCIDENT_ENERGY_WS).isDefault:
                self.setProperty(_PROP_OUTPUT_INCIDENT_ENERGY_WS,
                                 eiCalibrationWS)
            wsCleanup.cleanup(eiCalibrationWS)
        return mainWS, monWS

    def _convertTOFToDeltaE(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Converts the X units from time-of-flight to energy transfer.
        '''
        energyConvertedWSName = wsNames.withSuffix('energy_converted')
        energyConvertedWS = ConvertUnits(InputWorkspace=mainWS,
                                         OutputWorkspace=energyConvertedWSName,
                                         Target='DeltaE',
                                         EMode='Direct',
                                         EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return energyConvertedWS

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

    def _correctByDetectorEfficiency(self, mainWS, wsNames, wsCleanup,
                                     subalgLogging):
        '''
        Applies detector efficiency corrections.
        '''
        correctedWSName = wsNames.withSuffix('detector_efficiency_corrected')
        correctedWS = \
            DetectorEfficiencyCorUser(InputWorkspace=mainWS,
                                      OutputWorkspace=correctedWSName,
                                      EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return correctedWS

    def _correctByKiKf(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Applies the k_i / k_f correction.
        '''
        correctedWSName = wsNames.withSuffix('kikf')
        correctedWS = CorrectKiKf(InputWorkspace=mainWS,
                                  OutputWorkspace=correctedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return correctedWS

    def _createEPPWSDet(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Creates an EPP table for a detector workspace.
        '''
        detEPPInWS = self.getProperty(_PROP_EPP_WS).value
        if not detEPPInWS:
            detEPPWS = _findEPP(mainWS,
                                _WS_CONTENT_DETS,
                                wsNames,
                                subalgLogging)
        else:
            detEPPWS = detEPPInWS
            wsCleanup.protect(detEPPWS)
        if not self.getProperty(_PROP_OUTPUT_DET_EPP_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_DET_EPP_WS,
                             detEPPWS)
        return detEPPWS

    def _createEPPWSMon(self, monWS, wsNames, wsCleanup, subalgLogging):
        '''
        Creates an EPP table for a monitor workspace.
        '''
        monEPPInWS = self.getProperty(_PROP_MON_EPP_WS).value
        if not monEPPInWS:
            monEPPWS = _findEPP(monWS,
                                _WS_CONTENT_MONS,
                                wsNames,
                                subalgLogging)
        else:
            monEPPWS = monEPPInWS
            wsCleanup.protect(monEPPWS)
        if not self.getProperty(_PROP_OUTPUT_MON_EPP_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_MON_EPP_WS,
                             monEPPWS)
        return monEPPWS

    def _detDiagnostics(self, mainWS, bkgWS, detEPPWS, wsNames, wsCleanup,
                        report, subalgLogging):
        '''
        Performs and applies detector diagnostics.
        '''
        if self.getProperty(_PROP_DET_DIAGNOSTICS).value == \
                _DIAGNOSTICS_YES:
            diagnosticsInWS = \
                self.getProperty(_PROP_DIAGNOSTICS_WS).value
            if not diagnosticsInWS:
                lowThreshold = \
                    self.getProperty(
                        _PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = \
                    self.getProperty(
                        _PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD).value
                significanceTest = \
                    self.getProperty(
                        _PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
                peakDiagnosticsSettings = \
                    _DiagnosticsSettings(lowThreshold,
                                         highThreshold,
                                         significanceTest)
                sigmaMultiplier = \
                    self.getProperty(_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                lowThreshold = \
                    self.getProperty(_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD).value
                highThreshold = \
                    self.getProperty(
                        _PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD).value
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
                                                   subalgLogging)
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
                                                  subalgLogging)
            wsCleanup.cleanup(diagnosticsWS)
            wsCleanup.cleanup(mainWS)
            return diagnosedWS
        return mainWS

    def _finalize(self, outWS, wsCleanup, report):
        '''
        Does final cleanup, reporting and sets the output property.
        '''
        self.setProperty(_PROP_OUTPUT_WS, outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

    def _flatBkgDet(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Subtracts flat background from detector workspace.
        '''
        bkgInWS = self.getProperty(_PROP_FLAT_BKG_WS).value
        windowWidth = self.getProperty(_PROP_FLAT_BKG_WINDOW).value
        if not bkgInWS:
            bkgWS = _createFlatBkg(mainWS,
                                   _WS_CONTENT_DETS,
                                   windowWidth,
                                   wsNames,
                                   subalgLogging)
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
                                           subalgLogging)
        wsCleanup.cleanup(mainWS)
        return bkgSubtractedWS, bkgWS

    def _flatBkgMon(self, monWS, wsNames, wsCleanup, subalgLogging):
        '''
        Subtracts flat background from monitor workspace.
        '''
        windowWidth = self.getProperty(_PROP_FLAT_BKG_WINDOW).value
        monBkgWS = _createFlatBkg(monWS,
                                  _WS_CONTENT_MONS,
                                  windowWidth,
                                  wsNames,
                                  subalgLogging)
        monBkgScaling = 1
        bkgSubtractedMonWS = _subtractFlatBkg(monWS,
                                              _WS_CONTENT_MONS,
                                              monBkgWS,
                                              monBkgScaling,
                                              wsNames,
                                              wsCleanup,
                                              subalgLogging)
        wsCleanup.cleanup(monBkgWS)
        wsCleanup.cleanup(monWS)
        return bkgSubtractedMonWS

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        '''
        Returns the raw input workspace.
        '''
        inputFile = self.getProperty(_PROP_INPUT_FILE).value
        if inputFile:
            eppReference = \
                self.getProperty(_PROP_INITIAL_ELASTIC_PEAK_REFERENCE).value
            mainWS = _loadFiles(inputFile,
                                eppReference,
                                wsNames,
                                wsCleanup,
                                self.log(),
                                subalgLogging)
        elif self.getProperty(_PROP_INPUT_WS).value:
            mainWS = self.getProperty(_PROP_INPUT_WS).value
        return mainWS

    def _normalize(self, mainWS, monWS, monEPPWS, wsNames, wsCleanup,
                   subalgLogging):
        '''
        Normalizes to monitor or time.
        '''
        normalisationMethod = self.getProperty(_PROP_NORMALISATION).value
        if normalisationMethod != _NORM_METHOD_OFF:
            if normalisationMethod == _NORM_METHOD_MON:
                sigmaMultiplier = \
                    self.getProperty(_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
                monIndex = self.getProperty(_PROP_MON_INDEX).value
                monIndex = self._convertToWorkspaceIndex(monIndex, monWS)
                normalizedWS = _normalizeToMonitor(mainWS,
                                                   monWS,
                                                   monEPPWS,
                                                   sigmaMultiplier,
                                                   monIndex,
                                                   wsNames,
                                                   wsCleanup,
                                                   subalgLogging)
            elif normalisationMethod == _NORM_METHOD_TIME:
                normalizedWS = _normalizeToTime(mainWS,
                                                wsNames,
                                                wsCleanup,
                                                subalgLogging)
            else:
                raise RuntimeError('Unknonwn normalisation method ' +
                                   normalisationMethod)
            wsCleanup.cleanup(mainWS)
            return normalizedWS
        return mainWS

    def _normalizeToVana(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Normalizes to vanadium workspace.
        '''
        vanaWS = self.getProperty(_PROP_VANA_WS).value
        if vanaWS:
            vanaNormalizedWSName = wsNames.withSuffix('vanadium_normalized')
            vanaNormalizedWS = Divide(LHSWorkspace=mainWS,
                                      RHSWorkspace=vanaWS,
                                      OutputWorkspace=vanaNormalizedWSName,
                                      EnableLogging=subalgLogging)
            wsCleanup.cleanup(mainWS)
            return vanaNormalizedWS
        return mainWS

    def _rebinInW(self, mainWS, wsNames, wsCleanup, report,
                  subalgLogging):
        '''
        Rebins the horizontal axis of a workspace.
        '''
        mode = self.getProperty(_PROP_REBINNING_MODE_W).value
        if mode == _REBIN_AUTO_ELASTIC_PEAK:
            binWidth = BinWidthAtX(InputWorkspace=mainWS,
                                   X=0.0,
                                   Rounding='10^n')
            params = [binWidth]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == _REBIN_AUTO_MEDIAN_BIN_WIDTH:
            binWidth = MedianBinWidth(InputWorkspace=mainWS,
                                      Rounding='10^n')
            params = [binWidth]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == _REBIN_MANUAL:
            params = self.getProperty(_PROP_REBINNING_PARAMS_W).value
        else:
            raise RuntimeError('Unknown ' + _PROP_REBINNING_MODE_W)
        rebinnedWS = _rebin(mainWS, params, wsNames, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return rebinnedWS

    def _separateMons(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Extracts monitors to a separate workspace.
        '''
        detWSName = wsNames.withSuffix('extracted_detectors')
        monWSName = wsNames.withSuffix('extracted_monitors')
        detWS, monWS = ExtractMonitors(InputWorkspace=mainWS,
                                       DetectorWorkspace=detWSName,
                                       MonitorWorkspace=monWSName,
                                       EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return detWS, monWS

    def _subtractEC(self, mainWS, wsNames, wsCleanup, subalgLogging):
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
                                                   subalgLogging)
            else:
                ecSubtractedWS = _subtractEC(mainWS,
                                             ecInWS,
                                             transmission,
                                             wsNames,
                                             wsCleanup,
                                             subalgLogging)
            wsCleanup.cleanup(mainWS)
            return ecSubtractedWS
        return mainWS

AlgorithmFactory.subscribe(DirectILLReduction)
