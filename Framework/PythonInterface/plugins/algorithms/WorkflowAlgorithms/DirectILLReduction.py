# -*- coding: utf-8 -*-
from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction,
                        FileProperty, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty,
                        mtd, PropertyMode, WorkspaceProperty,
                        WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direct, Direction,
                           EnabledWhenProperty, FloatArrayProperty,
                           FloatBoundedValidator, IntArrayBoundedValidator,
                           IntArrayProperty, IntBoundedValidator,
                           IntMandatoryValidator, Property, PropertyCriterion,
                           StringArrayProperty, StringListValidator,
                           UnitConversion)
from mantid.simpleapi import (AddSampleLog, ApplyPaalmanPingsCorrection,
                              BinWidthAtX, CalculateFlatBackground,
                              ClearMaskFlag, CloneWorkspace,
                              ComputeCalibrationCoefVan, ConvertSpectrumAxis,
                              ConvertToConstantL2, ConvertToPointData,
                              ConvertUnits, CorrectKiKf, CorrectTOFAxis,
                              CreateEmptyTableWorkspace,
                              CreateSingleValuedWorkspace, DeleteWorkspace,
                              DetectorEfficiencyCorUser, Divide,
                              ExtractMonitors, FindEPP,
                              FlatPlatePaalmanPingsCorrection, GetEiMonDet,
                              GroupDetectors, Integration, Load, MaskDetectors,
                              MedianBinWidth, MedianDetectorTest, MergeRuns,
                              Minus, Multiply, NormaliseToMonitor, Plus, Rebin,
                              Scale, SofQWNormalisedPolygon, SolidAngle,
                              Transpose)
import numpy
from scipy import constants

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

_PROP_BEAM_HEIGHT = 'BeamHeight'
_PROP_BEAM_WIDTH = 'BeamWidth'
_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = 'NoisyBkgDiagnosticsHighThreshold'
_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = 'NoisyBkgDiagnosticsLowThreshold'
_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = 'NoisyBkgDiagnosticsErrorThreshold'
_PROP_CLEANUP_MODE = 'Cleanup'
_PROP_CONTAINER_BACK_THICKNESS = 'ContainerBackThickness'
_PROP_CONTAINER_CHEMICAL_FORMULA = 'ContainerChemicalFormula'
_PROP_CONTAINER_FRONT_THICKNESS = 'ContainerFronThickness'
_PROP_CONTAINER_NUMBER_DENSITY = 'ContainerNumberDensity'
_PROP_CONTAINER_OUTER_RADIUS = 'ContainerOuterRadius'
_PROP_DIAGNOSTICS_WS = 'DiagnosticsWorkspace'
_PROP_DET_DIAGNOSTICS = 'Diagnostics'
_PROP_DETS_AT_L2 = 'DetectorsAtL2'
_PROP_EC_SCALING_FACTOR = 'EmptyContainerScalingFactor'
_PROP_EC_WS = 'EmptyContainerWorkspace'
_PROP_ELASTIC_BIN_INDEX = 'ElasticBinIndex'
_PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER = 'ElasticPeakWidthInSigmas'
_PROP_EPP_WS = 'EPPWorkspace'
_PROP_FLAT_BKG_SCALING = 'FlatBkgScaling'
_PROP_FLAT_BKG_WINDOW = 'FlatBkgAveragingWindow'
_PROP_FLAT_BKG_WS = 'FlatBkgWorkspace'
_PROP_INCIDENT_ENERGY_CALIBRATION = 'IncidentEnergyCalibration'
_PROP_INCIDENT_ENERGY_WS = 'IncidentEnergyWorkspace'
_PROP_INDEX_TYPE = 'IndexType'
_PROP_INPUT_FILE = 'InputFile'
_PROP_INPUT_WS = 'InputWorkspace'
_PROP_MON_EPP_WS = 'MonitorEPPWorkspace'
_PROP_MON_INDEX = 'Monitor'
_PROP_NORMALISATION = 'Normalisation'
_PROP_OUTPUT_DET_EPP_WS = 'OutputEPPWorkspace'
_PROP_OUTPUT_DIAGNOSTICS_WS = 'OutputDiagnosticsWorkspace'
_PROP_OUTPUT_DIAGNOSTICS_REPORT_WS = 'OutputDiagnosticsReportWorkspace'
_PROP_OUTPUT_FLAT_BKG_WS = 'OutputFlatBkgWorkspace'
_PROP_OUTPUT_INCIDENT_ENERGY_WS = 'OutputIncidentEnergyWorkspace'
_PROP_OUTPUT_MON_EPP_WS = 'OutputMonitorEPPWorkspace'
_PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS = \
    'OutputSelfShieldingCorrectionWorkspace'
_PROP_OUTPUT_THETA_W_WS = 'OutputSofThetaEnergyWorkspace'
_PROP_OUTPUT_WS = 'OutputWorkspace'
_PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD = 'ElasticPeakDiagnosticsHighThreshold'
_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD = 'ElasticPeakDiagnosticsLowThreshold'
_PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = \
    'ElasticPeakDiagnosticsErrorThreshold'
_PROP_REBINNING_MODE_Q = 'QRebinningMode'
_PROP_REBINNING_MODE_W = 'EnergyRebinningMode'
_PROP_REBINNING_PARAMS_Q = 'QRebinningParams'
_PROP_REBINNING_PARAMS_W = 'EnergyRebinningParams'
_PROP_REDUCTION_TYPE = 'ReductionType'
_PROP_REFERENCE_TOF_AXIS_WS = 'ReferenceTOFAxisWorkspace'
_PROP_SAMPLE_ANGLE = 'SampleAngle'
_PROP_SAMPLE_CHEMICAL_FORMULA = 'SampleChemicalFormula'
_PROP_SAMPLE_NUMBER_DENSITY = 'SampleNumberDensity'
_PROP_SAMPLE_INNER_RADIUS = 'SampleInnerRadius'
_PROP_SAMPLE_OUTER_RADIUS = 'SampleOuterRadius'
_PROP_SAMPLE_SHAPE = 'SampleShape'
_PROP_SAMPLE_THICKNESS = 'SampleThickness'
_PROP_SELF_SHIELDING_CORRECTION = 'SelfShieldingCorrection'
_PROP_SELF_SHIELDING_CORRECTION_WS = 'SelfShieldingCorrectionWorkspace'
_PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS = 'SelfShieldingNumberWavelengths'
_PROP_SELF_SHIELDING_STEP_SIZE = 'SelfShieldingStepSize'
_PROP_SUBALG_LOGGING = 'SubalgorithmLogging'
_PROP_TRANSPOSE_SAMPLE_OUTPUT = 'Transposing'
_PROP_USER_MASK = 'MaskedDetectors'
_PROP_USER_MASK_COMPONENTS = 'MaskedComponents'
_PROP_VANA_WS = 'VanadiumWorkspace'

_PROPGROUP_BEAM = 'Incident neutron beam properties'
_PROPGROUP_CONTAINER = 'Container Material Properties'
_PROPGROUP_CYLINDER_CONTAINER = 'Cylindrical Container Properties'
_PROPGROUP_DET_DIAGNOSTICS = 'Detector Diagnostics and Masking'
_PROPGROUP_EC = 'Empty Container Subtraction'
_PROPGROUP_FLAT_BKG = 'Flat Time-Independent Background'
_PROPGROUP_OPTIONAL_OUTPUT = 'Optional Output'
_PROPGROUP_REBINNING = 'Rebinning for SofQW'
_PROPGROUP_SAMPLE = 'Sample Material Properties'
_PROPGROUP_SLAB_CONTAINER = 'Flat Container Properties'
_PROPGROUP_TOF_AXIS_CORRECTION = 'TOF Axis Correction for Elastic Channel'

_REBIN_AUTO_ELASTIC_PEAK = 'Rebin to Bin Width at Elastic Peak'
_REBIN_AUTO_MEDIAN_BIN_WIDTH = 'Rebin to Median Bin Width'
_REBIN_AUTO_Q = 'Rebin to Median 2Theta'
_REBIN_MANUAL_Q = 'Manual q Rebinning'
_REBIN_MANUAL_W = 'Manual Energy Rebinning'

_REDUCTION_TYPE_EC = 'Empty Container'
_REDUCTION_TYPE_SAMPLE = 'Sample'
_REDUCTION_TYPE_VANA = 'Vanadium'

_SAMPLE_SHAPE_CYLINDER = 'Cylindrical Sample'
_SAMPLE_SHAPE_SLAB = 'Flat Plate Sample'

_SELF_SHIELDING_CORRECTION_OFF = 'No Self Shielding Correction'
_SELF_SHIELDING_CORRECTION_ON = 'Apply Self Shielding Correction'

_SUBALG_LOGGING_OFF = 'No Subalgorithm Logging'
_SUBALG_LOGGING_ON = 'Allow Subalgorithm Logging'

_TRANSPOSING_OFF = 'No Sample Output Transposing'
_TRANSPOSING_ON = 'Transpose Sample Output'

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


def _createDetectorGroups(ws):
    '''
    Finds detectors with (almost) same theta and groups them. Masked detectors
    are ignored.
    '''
    numHistograms = ws.getNumberHistograms()
    assignedDets = list()
    groups = list()
    for i in range(numHistograms):
        if i in assignedDets:
            continue
        det1 = ws.getDetector(i)
        if det1.isMasked():
            continue
        currentGroup = [det1.getID()]
        twoTheta1 = ws.detectorTwoTheta(det1)
        for j in range(i + 1, numHistograms):
            if j in assignedDets:
                continue
            det2 = ws.getDetector(j)
            if det2.isMasked():
                continue
            twoTheta2 = ws.detectorTwoTheta(det2)
            if abs(twoTheta1 - twoTheta2) < 0.01 / 180.0 * constants.pi:
                currentGroup.append(det2.getID())
                assignedDets.append(j)
        groups.append(currentGroup)
    return groups


def _createDiagnosticsReportTable(elasticIntensityWS, bkgWS, diagnosticsWS,
                                  reportWSName, algorithmLogging):
    '''
    Creates a table workspace containing information used for detector
    diagnostics.
    '''
    PLOT_TYPE_X = 1
    PLOT_TYPE_Y = 2
    reportWS = CreateEmptyTableWorkspace(OutputWorkspace=reportWSName,
                                         EnableLogging=algorithmLogging)
    reportWS.addColumn('int', 'WorkspaceIndex', PLOT_TYPE_X)
    reportWS.addColumn('double', 'ElasticIntensity', PLOT_TYPE_Y)
    reportWS.addColumn('double', 'FlatBkg', PLOT_TYPE_Y)
    reportWS.addColumn('int', 'Diagnosed', PLOT_TYPE_Y)
    for i in range(elasticIntensityWS.getNumberHistograms()):
        elasticIntensity = elasticIntensityWS.readY(i)[0]
        bkg = bkgWS.readY(i)[0]
        diagnosed = int(diagnosticsWS.readY(i)[0])
        row = (i, elasticIntensity, bkg, diagnosed)
        reportWS.addRow(row)


def _groupsToGroupingPattern(groups):
    '''
    Returns a grouping pattern suitable for the GroupDetectors algorithm.
    '''
    pattern = ''
    for group in groups:
        for index in group:
            pattern += str(index) + '+'
        pattern = pattern[:-1] + ','
    return pattern[:-1]


def _loadFiles(inputFilename, wsNames, wsCleanup, algorithmLogging):
    '''
    Loads files specified by filenames, merging them into a single
    workspace.
    '''
    # TODO Explore ways of loading data file-by-file and merging pairwise.
    #      Should save some memory (IN5!).
    rawWSName = wsNames.withSuffix('raw')
    rawWS = Load(Filename=inputFilename,
                 OutputWorkspace=rawWSName,
                 EnableLogging=algorithmLogging)
    mergedWSName = wsNames.withSuffix('merged')
    mergedWS = MergeRuns(InputWorkspaces=rawWS,
                         OutputWorkspace=mergedWSName,
                         EnableLogging=algorithmLogging)
    wsCleanup.cleanup(rawWS)
    return mergedWS


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


def _medianDeltaTheta(ws):
    '''
    Calculates the median theta spacing for S(theta, w) workspace.
    '''
    thetas = list()
    spectrumInfo = ws.spectrumInfo()
    for i in range(ws.getNumberHistograms()):
        if (not spectrumInfo.isMasked(i) and spectrumInfo.hasDetectors(i) and
                not spectrumInfo.isMonitor(i)):
            det = ws.getDetector(i)
            twoTheta = ws.detectorTwoTheta(det)
            thetas.append(twoTheta)
    if not thetas:
        raise RuntimeError('No usable detectors for median DTheta ' +
                           'calculation.')
    dThetas = numpy.diff(thetas)
    return numpy.median(dThetas[dThetas > numpy.radians(0.1)])


def _minMaxQ(ws):
    '''
    Estimates the start and end q bins for S(theta, w) workspace.
    '''
    Ei = ws.run().getProperty('Ei').value * 1e-3 * constants.e  # in Joules
    xs = ws.readX(0)
    minW = xs[0] * 1e-3 * constants.e  # in Joules
    maxEf = Ei - minW
    # In Ånströms
    maxQ = numpy.sqrt(2.0 * constants.m_n / constants.hbar**2 *
                     (Ei + maxEf - 2 * numpy.sqrt(Ei * maxEf) * -1.0)) * 1e-10
    minQ = 0.0
    return (minQ, maxQ)


def _deltaQ(ws):
    '''
    Estimates a q bin width for S(theta, w) workspace.
    '''
    deltaTheta = _medianDeltaTheta(ws)
    wavelength = ws.run().getProperty('wavelength').value
    return 2.0 * constants.pi / wavelength * deltaTheta


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
                       wsCleanup, report, reportWSName, algorithmLogging):
    '''
    Returns a diagnostics workspace.
    '''
    # 1. Diagnose elastic peak region.
    constantL2WSName = wsNames.withSuffix('constant_L2')
    constantL2WS = ConvertToConstantL2(InputWorkspace=ws,
                                       OutputWorkspace=constantL2WSName,
                                       EnableLogging=algorithmLogging)
    medianEPPCentre, medianEPPSigma = _medianEPP(eppWS, eppIndices)
    integrationBegin = medianEPPCentre - sigmaMultiplier * medianEPPSigma
    integrationEnd = medianEPPCentre + sigmaMultiplier * medianEPPSigma
    integratedElasticPeaksWSName = \
        wsNames.withSuffix('integrated_elastic_peak')
    integratedElasticPeaksWS = \
        Integration(InputWorkspace=constantL2WS,
                    OutputWorkspace=integratedElasticPeaksWSName,
                    RangeLower=integrationBegin,
                    RangeUpper=integrationEnd,
                    IncludePartialBins=True,
                    EnableLogging=algorithmLogging)
    solidAngleWSName = wsNames.withSuffix('detector_solid_angles')
    solidAngleWS = SolidAngle(InputWorkspace=ws,
                              OutputWorkspace=solidAngleWSName,
                              EnableLogging=algorithmLogging)
    solidAngleCorrectedElasticPeaksWSName = \
        wsNames.withSuffix('solid_angle_corrected_elastic_peak')
    solidAngleCorrectedElasticPeaksWS = \
        Divide(LHSWorkspace=integratedElasticPeaksWS,
               RHSWorkspace=solidAngleWS,
               OutputWorkspace=solidAngleCorrectedElasticPeaksWSName,
               EnableLogging=algorithmLogging)
    elasticPeakDiagnosticsWSName = \
        wsNames.withSuffix('diagnostics_elastic_peak')
    elasticPeakDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=solidAngleCorrectedElasticPeaksWS,
                           OutputWorkspace=elasticPeakDiagnosticsWSName,
                           SignificanceTest=peakSettings.significanceTest,
                           LowThreshold=peakSettings.lowThreshold,
                           HighThreshold=peakSettings.highThreshold,
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
    ClearMaskFlag(Workspace=elasticPeakDiagnostics,
                  EnableLogging=algorithmLogging)
    ClearMaskFlag(Workspace=noisyBkgDiagnostics,
                  EnableLogging=algorithmLogging)
    combinedDiagnosticsWSName = wsNames.withSuffix('diagnostics')
    diagnosticsWS = Plus(LHSWorkspace=elasticPeakDiagnostics,
                         RHSWorkspace=noisyBkgDiagnostics,
                         OutputWorkspace=combinedDiagnosticsWSName,
                         EnableLogging=algorithmLogging)
    if reportWSName:
        _createDiagnosticsReportTable(solidAngleCorrectedElasticPeaksWS,
                                      bkgWS,
                                      diagnosticsWS,
                                      reportWSName,
                                      algorithmLogging)
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
                              monEPPWorkspace, eiCalibrationDets,
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
        energy = GetEiMonDet(DetectorWorkspace=detWorkspace,
                             DetectorEPPTable=detEPPWorkspace,
                             IndexType='Workspace Index',
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


def _applySelfShieldingCorrections(ws, ecWS, ecScaling, correctionsWS, wsNames,
                                   algorithmLogging):
    '''
    Applies a self-shielding corrections workspace.
    '''
    correctedWSName = wsNames.withSuffix('self_shielding_applied')
    correctedWS = ApplyPaalmanPingsCorrection(
        SampleWorkspace=ws,
        CorrectionsWorkspace=correctionsWS,
        CanWorkspace=ecWS,
        CanScaleFactor=ecScaling,
        OutputWorkspace=correctedWSName,
        EnableLogging=algorithmLogging)
    return correctedWS


def _applySelfShieldingCorrectionsNoEC(ws, correctionsWS, wsNames,
                                       algorithmLogging):
    '''
    Applies a self-shielding corrections workspace without subtracting an empty
    container.
    '''
    correctedWSName = wsNames.withSuffix('self_shielding_applied')
    correctedWS = ApplyPaalmanPingsCorrection(
        SampleWorkspace=ws,
        CorrectionsWorkspace=correctionsWS,
        OutputWorkspace=correctedWSName,
        EnableLogging=algorithmLogging)
    return correctedWS


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


def _selfShieldingCorrectionsCylinder(ws, ecWS, sampleChemicalFormula,
                                      sampleNumberDensity,
                                      containerChemicalFormula,
                                      containerNumberDensity, sampleInnerR,
                                      sampleOuterR, containerOuterR, beamWidth,
                                      beamHeight, stepSize, numberWavelengths,
                                      wsNames, algorithmLogging):
    '''
    Calculates the self-shielding corrections workspace for cylindrical sample.
    '''
    from mantid.simpleapi import CylinderPaalmanPingsCorrection
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections_with_ec')
    selfShieldingWS = CylinderPaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleInnerRadius=sampleInnerR,
        SampleOuterRadius=sampleOuterR,
        CanWorkspace=ecWS,
        CanChemicalFormula=containerChemicalFormula,
        CanDensityType='Number Density',
        CanDensity=containerNumberDensity,
        CanOuterRadius=containerOuterR,
        BeamHeight=beamHeight,
        BeamWidth=beamWidth,
        StepSize=stepSize,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsCylinderNoEC(ws, sampleChemicalFormula,
                                          sampleNumberDensity,
                                          sampleInnerR, sampleOuterR,
                                          beamWidth, beamHeight, stepSize,
                                          numberWavelengths, wsNames,
                                          algorithmLogging):
    '''
    Calculates the self-shielding corrections workspace for cylindrical sample
    without using empty container data.
    '''
    from mantid.simpleapi import CylinderPaalmanPingsCorrection
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections')
    selfShieldingWS = CylinderPaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleInnerRadius=sampleInnerR,
        SampleOuterRadius=sampleOuterR,
        BeamHeight=beamHeight,
        BeamWidth=beamWidth,
        StepSize=stepSize,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsSlab(ws, ecWS, sampleChemicalFormula,
                                  sampleNumberDensity,
                                  containerChemicalFormula,
                                  containerNumberDensity, sampleThickness,
                                  sampleAngle, containerFrontThickness,
                                  containerBackThickness, numberWavelengths,
                                  wsNames, algorithmLogging):
    '''
    Calculates the self-shielding corrections workspace for flat plate sample.
    '''
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections_with_ec')
    selfShieldingWS = FlatPlatePaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleThickness=sampleThickness,
        SampleAngle=sampleAngle,
        CanWorkspace=ecWS,
        CanChemicalFormula=containerChemicalFormula,
        CanDensityType='NumberDensity',
        CanDensity=containerNumberDensity,
        CanFrontThickness=containerFrontThickness,
        CanBackThickness=containerBackThickness,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsSlabNoEC(ws, sampleChemicalFormula,
                                      sampleNumberDensity, sampleThickness,
                                      sampleAngle, numberWavelengths, wsNames,
                                      algorithmLogging):
    '''
    Calculates the self-shielding corrections workspace for flat plate sample
    without using empty container data.
    '''
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections')
    selfShieldingWS = FlatPlatePaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleThickness=sampleThickness,
        SampleAngle=sampleAngle,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _subtractEC(ws, ecWS, ecScaling, wsNames, wsCleanup, algorithmLogging):
    '''
    Subtracts empty container.
    '''
    # out = in - ecScaling * EC
    scalingWSName = wsNames.withSuffix('ecScaling')
    scalingWS = \
        CreateSingleValuedWorkspace(OutputWorkspace=scalingWSName,
                                    DataValue=ecScaling,
                                    EnableLogging=algorithmLogging)
    correctedECWSName = wsNames.withSuffix('scaled_EC')
    correctedECWS = Multiply(LHSWorkspace=ecWS,
                             RHSWorkspace=scalingWS,
                             OutputWorkspace=correctedECWSName,
                             EnableLogging=algorithmLogging)
    tofAdjustedECWSName = wsNames.withSuffix('scaled_EC_TOF_corrected')
    tofAdjustedECWS = CorrectTOFAxis(InputWorkspace=correctedECWS,
                                     OutputWorkspace=tofAdjustedECWSName,
                                     ReferenceWorkspace=ws,
                                     IndexType='Workspace Index',
                                     ReferenceSpectra='0',
                                     EnableLogging=algorithmLogging)
    ecSubtractedWSName = wsNames.withSuffix('EC_subtracted')
    ecSubtractedWS = Minus(LHSWorkspace=ws,
                           RHSWorkspace=tofAdjustedECWS,
                           OutputWorkspace=ecSubtractedWSName,
                           EnableLogging=algorithmLogging)
    wsCleanup.cleanup(scalingWS)
    wsCleanup.cleanup(correctedECWS)
    wsCleanup.cleanup(tofAdjustedECWS)
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

        # Apply user mask.
        mainWS = self._applyUserMask(mainWS, wsNames,
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

        # Calibrate incident energy, if requested.
        mainWS, monWS = self._calibrateEi(mainWS, detEPPWS, monWS, monEPPWS,
                                          wsNames, wsCleanup, report,
                                          subalgLogging)
        wsCleanup.cleanupLater(monWS)

        # Normalisation to monitor/time, if requested.
        mainWS = self._normalize(mainWS, monWS, monEPPWS, wsNames,
                                 wsCleanup, subalgLogging)

        # Reduction for empty container and cadmium ends here.
        if reductionType == _REDUCTION_TYPE_EC:
            self._finalize(mainWS, wsCleanup, report)
            return

        # Continuing with vanadium and sample reductions.

        # Save vanadium's original TOF for later restoration.
        vanaXs = None
        if reductionType == _REDUCTION_TYPE_VANA:
            vanaXs = numpy.array(mainWS.readX(0))

        mainWS = self._correctTOFAxis(mainWS, wsNames, wsCleanup,
                                      subalgLogging)

        # Self shielding and empty container subtraction, if requested.
        mainWS = self._selfShieldingAndEC(mainWS, wsNames, wsCleanup,
                                          subalgLogging)

        # Reduction for vanadium ends here.
        if reductionType == _REDUCTION_TYPE_VANA:
            # We output an integrated vanadium, ready to be used for
            # normalization.
            # Restore vanadium's original TOF axis. Otherwise, the EPP table
            # would be incorrect.
            for i in range(mainWS.getNumberHistograms()):
                numpy.copyto(mainWS.dataX(i), vanaXs)
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

        mainWS = self._groupDetectors(mainWS, wsNames, wsCleanup,
                                      subalgLogging)

        self._outputWSConvertedToTheta(mainWS, wsNames, wsCleanup,
                                       subalgLogging)

        mainWS = self._sOfQW(mainWS, wsNames, wsCleanup, subalgLogging)
        mainWS = self._transpose(mainWS, wsNames, wsCleanup, subalgLogging)
        self._finalize(mainWS, wsCleanup, report)

    def PyInit(self):
        # Validators.
        greaterThanUnityFloat = FloatBoundedValidator(lower=1)
        mandatoryPositiveInt = CompositeValidator()
        mandatoryPositiveInt.add(IntMandatoryValidator())
        mandatoryPositiveInt.add(IntBoundedValidator(lower=0))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=0)
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
        self.declareProperty(name=_PROP_TRANSPOSE_SAMPLE_OUTPUT,
                             defaultValue=_TRANSPOSING_ON,
                             validator=StringListValidator([
                                 _TRANSPOSING_ON,
                                 _TRANSPOSING_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable ' + _PROP_OUTPUT_WS +
                                 ' transposing for ' + _REDUCTION_TYPE_SAMPLE +
                                 ' reductions.')
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
        self.setPropertyGroup(_PROP_FLAT_BKG_SCALING, _PROPGROUP_FLAT_BKG)
        self.declareProperty(name=_PROP_FLAT_BKG_WINDOW,
                             defaultValue=30,
                             validator=mandatoryPositiveInt,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) ' +
                                 'for flat background.')
        self.setPropertyGroup(_PROP_FLAT_BKG_WINDOW, _PROPGROUP_FLAT_BKG)
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Workspace from which to get flat background data.')
        self.setPropertyGroup(_PROP_FLAT_BKG_WS, _PROPGROUP_FLAT_BKG)
        self.declareProperty(IntArrayProperty(name=_PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask.')
        self.setPropertyGroup(_PROP_USER_MASK, _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(
            StringArrayProperty(name=_PROP_USER_MASK_COMPONENTS,
                                values='',
                                direction=Direction.Input),
            doc='List of instrument components to mask.')
        self.setPropertyGroup(_PROP_USER_MASK_COMPONENTS,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_DET_DIAGNOSTICS,
                             defaultValue=_DIAGNOSTICS_YES,
                             validator=StringListValidator([
                                 _DIAGNOSTICS_YES,
                                 _DIAGNOSTICS_NO]),
                             direction=Direction.Input,
                             doc='If true, run detector diagnostics or ' +
                                 'apply ' + _PROP_DIAGNOSTICS_WS + '.')
        self.setPropertyGroup(_PROP_DET_DIAGNOSTICS,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Detector diagnostics workspace obtained from another ' +
                'reduction run.')
        self.setPropertyGroup(_PROP_DET_DIAGNOSTICS,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(_PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.0,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(_PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the elastic peak diagnostics.')
        self.setPropertyGroup(_PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(_PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=33.3,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(_PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the noisy background diagnostics.')
        self.setPropertyGroup(_PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              _PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=_PROP_NORMALISATION,
                             defaultValue=_NORM_METHOD_MON,
                             validator=StringListValidator([
                                 _NORM_METHOD_MON,
                                 _NORM_METHOD_TIME,
                                 _NORM_METHOD_OFF]),
                             direction=Direction.Input,
                             doc='Normalisation method.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_EC_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced empty container workspace.')
        self._enabledByNonECReduction(_PROP_EC_WS)
        self.setPropertyGroup(_PROP_EC_WS,
                              _PROPGROUP_EC)
        self.declareProperty(name=_PROP_EC_SCALING_FACTOR,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Scaling factor (transmission, if no self ' +
                                 'shielding is applied) for empty container.')
        self._enabledByNonECReduction(_PROP_EC_SCALING_FACTOR)
        self.setPropertyGroup(_PROP_EC_SCALING_FACTOR,
                              _PROPGROUP_EC)
        self.declareProperty(name=_PROP_SELF_SHIELDING_CORRECTION,
                             defaultValue=_SELF_SHIELDING_CORRECTION_OFF,
                             validator=StringListValidator([
                                 _SELF_SHIELDING_CORRECTION_OFF,
                                 _SELF_SHIELDING_CORRECTION_ON]),
                             direction=Direction.Input,
                             doc='Enable of disable self shielding ' +
                                 'correction.')
        self._enabledByNonECReduction(_PROP_SELF_SHIELDING_CORRECTION)
        self.declareProperty(MatrixWorkspaceProperty(
            name=_PROP_SELF_SHIELDING_CORRECTION_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='A workspace containing self shielding correction factors.')
        self._enabledBySelfShieldingCorrection(
            _PROP_SELF_SHIELDING_CORRECTION_WS)
        self.setPropertySettings(_PROP_SELF_SHIELDING_CORRECTION_WS,
                                 EnabledWhenProperty(
                                     _PROP_SELF_SHIELDING_CORRECTION,
                                     PropertyCriterion.IsEqualTo,
                                     _SELF_SHIELDING_CORRECTION_ON))
        self.declareProperty(name=_PROP_SELF_SHIELDING_STEP_SIZE,
                             defaultValue=0.002,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Step size for self shielding simulation.')
        self._enabledBySelfShieldingCorrection(_PROP_SELF_SHIELDING_STEP_SIZE)
        self.declareProperty(name=_PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS,
                             defaultValue=10,
                             validator=positiveInt,
                             direction=Direction.Input,
                             doc='Number of wavelengths for self shielding ' +
                                 'simulation')
        self._enabledBySelfShieldingCorrection(
            _PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS)
        self.declareProperty(name=_PROP_SAMPLE_SHAPE,
                             defaultValue=_SAMPLE_SHAPE_SLAB,
                             validator=StringListValidator([
                                 _SAMPLE_SHAPE_SLAB,
                                 _SAMPLE_SHAPE_CYLINDER]),
                             direction=Direction.Input,
                             doc='The shape of the sample and its container.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_SHAPE)
        self.declareProperty(name=_PROP_SAMPLE_CHEMICAL_FORMULA,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Chemical formula for the sample material.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_CHEMICAL_FORMULA)
        self.setPropertyGroup(_PROP_SAMPLE_CHEMICAL_FORMULA,
                              _PROPGROUP_SAMPLE)
        self.declareProperty(name=_PROP_SAMPLE_NUMBER_DENSITY,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Number density of the sample material.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_NUMBER_DENSITY)
        self.setPropertyGroup(_PROP_SAMPLE_NUMBER_DENSITY,
                              _PROPGROUP_SAMPLE)
        self.declareProperty(name=_PROP_CONTAINER_CHEMICAL_FORMULA,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Chemical formula for the container ' +
                                 'material.')
        self._enabledBySelfShieldingCorrection(
            _PROP_CONTAINER_CHEMICAL_FORMULA)
        self.setPropertySettings(_PROP_CONTAINER_CHEMICAL_FORMULA,
                                 EnabledWhenProperty(
                                     _PROP_SELF_SHIELDING_CORRECTION,
                                     PropertyCriterion.IsEqualTo,
                                     _SELF_SHIELDING_CORRECTION_ON))
        self.setPropertyGroup(_PROP_CONTAINER_CHEMICAL_FORMULA,
                              _PROPGROUP_CONTAINER)
        self.declareProperty(name=_PROP_CONTAINER_NUMBER_DENSITY,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Number density of the container material.')
        self._enabledBySelfShieldingCorrection(_PROP_CONTAINER_NUMBER_DENSITY)
        self.setPropertyGroup(_PROP_CONTAINER_NUMBER_DENSITY,
                              _PROPGROUP_CONTAINER)
        self.declareProperty(name=_PROP_SAMPLE_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Sample thickness.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_THICKNESS)
        self.setPropertyGroup(_PROP_SAMPLE_THICKNESS,
                              _PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=_PROP_CONTAINER_FRONT_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Container front face thickness.')
        self._enabledBySelfShieldingCorrection(_PROP_CONTAINER_FRONT_THICKNESS)
        self.setPropertyGroup(_PROP_CONTAINER_FRONT_THICKNESS,
                              _PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=_PROP_CONTAINER_BACK_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Container back face thickness.')
        self._enabledBySelfShieldingCorrection(_PROP_CONTAINER_BACK_THICKNESS)
        self.setPropertyGroup(_PROP_CONTAINER_BACK_THICKNESS,
                              _PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=_PROP_SAMPLE_ANGLE,
                             defaultValue=0.0,
                             direction=Direction.Input,
                             doc='Sample rotation angle.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_ANGLE)
        self.setPropertyGroup(_PROP_SAMPLE_ANGLE,
                              _PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=_PROP_SAMPLE_INNER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Inner radius of the sample.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_INNER_RADIUS)
        self.setPropertyGroup(_PROP_SAMPLE_INNER_RADIUS,
                              _PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=_PROP_SAMPLE_OUTER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Outer radius of the sample.')
        self._enabledBySelfShieldingCorrection(_PROP_SAMPLE_OUTER_RADIUS)
        self.setPropertyGroup(_PROP_SAMPLE_OUTER_RADIUS,
                              _PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=_PROP_CONTAINER_OUTER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Outer radius of the container.')
        self._enabledBySelfShieldingCorrection(_PROP_CONTAINER_OUTER_RADIUS)
        self.setPropertyGroup(_PROP_CONTAINER_OUTER_RADIUS,
                              _PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=_PROP_BEAM_WIDTH,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Width of the neutron beam.')
        self._enabledBySelfShieldingCorrection(_PROP_BEAM_WIDTH)
        self.setPropertyGroup(_PROP_BEAM_WIDTH,
                              _PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=_PROP_BEAM_HEIGHT,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Height of the neutron beam.')
        self._enabledBySelfShieldingCorrection(_PROP_BEAM_HEIGHT)
        self.setPropertyGroup(_PROP_BEAM_HEIGHT,
                              _PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(MatrixWorkspaceProperty(
                             name=_PROP_REFERENCE_TOF_AXIS_WS,
                             defaultValue='',
                             validator=WorkspaceUnitValidator('TOF'),
                             optional=PropertyMode.Optional,
                             direction=Direction.Input),
                             doc='Workspace from which to copy the TOF axis.')
        self._enabledBySampleReduction(_PROP_REFERENCE_TOF_AXIS_WS)
        self.setPropertyGroup(_PROP_REFERENCE_TOF_AXIS_WS,
                              _PROPGROUP_TOF_AXIS_CORRECTION)
        self.declareProperty(name=_PROP_ELASTIC_BIN_INDEX,
                             defaultValue=Property.EMPTY_INT,
                             validator=IntBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc='Bin index of the elastic channel.')
        self._enabledBySampleReduction(_PROP_ELASTIC_BIN_INDEX)
        self.setPropertyGroup(_PROP_ELASTIC_BIN_INDEX,
                              _PROPGROUP_TOF_AXIS_CORRECTION)
        self.declareProperty(name=_PROP_REBINNING_MODE_W,
                             defaultValue=_REBIN_AUTO_ELASTIC_PEAK,
                             validator=StringListValidator([
                                 _REBIN_AUTO_ELASTIC_PEAK,
                                 _REBIN_AUTO_MEDIAN_BIN_WIDTH,
                                 _REBIN_MANUAL_W]),
                             direction=Direction.Input,
                             doc='Energy rebinnin mode.')
        self._enabledBySampleReduction(_PROP_REBINNING_MODE_W)
        self.setPropertyGroup(_PROP_REBINNING_MODE_W, _PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=_PROP_REBINNING_PARAMS_W),
                             doc='Manual energy rebinning parameters.')
        self._enabledBySampleReduction(_PROP_REBINNING_PARAMS_W)
        self.setPropertyGroup(_PROP_REBINNING_PARAMS_W, _PROPGROUP_REBINNING)
        self.declareProperty(name=_PROP_REBINNING_MODE_Q,
                             defaultValue=_REBIN_AUTO_Q,
                             validator=StringListValidator([
                                 _REBIN_AUTO_Q,
                                 _REBIN_MANUAL_Q]),
                             direction=Direction.Input,
                             doc='q rebinning mode.')
        self._enabledBySampleReduction(_PROP_REBINNING_MODE_Q)
        self.setPropertyGroup(_PROP_REBINNING_MODE_Q, _PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=_PROP_REBINNING_PARAMS_Q),
                             doc='Manual q rebinning parameters.')
        self._enabledBySampleReduction(_PROP_REBINNING_PARAMS_Q)
        self.setPropertyGroup(_PROP_REBINNING_PARAMS_Q, _PROPGROUP_REBINNING)
        # Rest of the output properties.
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_OUTPUT_DET_EPP_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for elastic peak positions.')
        self.setPropertyGroup(_PROP_OUTPUT_DET_EPP_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_OUTPUT_MON_EPP_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for monitor elastic peak positions.')
        self.setPropertyGroup(_PROP_OUTPUT_MON_EPP_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_INCIDENT_ENERGY_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for calibrated inciden energy.')
        self.setPropertyGroup(_PROP_OUTPUT_INCIDENT_ENERGY_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for flat background.')
        self.setPropertyGroup(_PROP_OUTPUT_FLAT_BKG_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for detector diagnostics.')
        self.setPropertyGroup(_PROP_OUTPUT_DIAGNOSTICS_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(ITableWorkspaceProperty(
            name=_PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output table workspace for detector diagnostics reporting.')
        self.setPropertyGroup(_PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for self shielding corrections.')
        self._enabledByNonECReduction(
            _PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS)
        self.setPropertyGroup(_PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(WorkspaceProperty(
            name=_PROP_OUTPUT_THETA_W_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for reduced S(theta, DeltaE).')
        self._enabledBySampleReduction(_PROP_OUTPUT_THETA_W_WS)
        self.setPropertyGroup(_PROP_OUTPUT_THETA_W_WS,
                              _PROPGROUP_OPTIONAL_OUTPUT)

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
        reductionType = self.getProperty(_PROP_REDUCTION_TYPE).value
        if reductionType in [_REDUCTION_TYPE_SAMPLE, _REDUCTION_TYPE_VANA]:
            if self.getProperty(_PROP_DETS_AT_L2).isDefault:
                issues[_PROP_DETS_AT_L2] = \
                    'Detectors at L2 distance have to be specified when ' + \
                    _PROP_REDUCTION_TYPE + ' is ' + _REDUCTION_TYPE_VANA + \
                    ' or ' + _REDUCTION_TYPE_SAMPLE + '.'
        if reductionType == _REDUCTION_TYPE_SAMPLE:
            if (self.getProperty(_PROP_REBINNING_MODE_W).value ==
                    _REBIN_MANUAL_W and
                    self.getProperty(_PROP_REBINNING_PARAMS_W).isDefault):
                issues[_PROP_REBINNING_PARAMS_W] = \
                    'Energy rebinning parameters must be given in manual ' + \
                    'rebinning mode'
            if (self.getProperty(_PROP_REBINNING_MODE_Q).value ==
                    _REBIN_MANUAL_Q and
                    self.getProperty(_PROP_REBINNING_PARAMS_Q).isDefault):
                issues[_PROP_REBINNING_PARAMS_Q] = \
                    'q rebinning parameters must be given in manual ' + \
                    'rebinning mode.'
        selfShielding = \
            self.getProperty(_PROP_SELF_SHIELDING_CORRECTION).value
        if selfShielding == _SELF_SHIELDING_CORRECTION_ON:
            if self.getProperty(_PROP_SAMPLE_CHEMICAL_FORMULA).isDefault:
                issues[_PROP_SAMPLE_CHEMICAL_FORMULA] = 'Chemical formula ' + \
                    'must be specified.'
            if self.getProperty(_PROP_SAMPLE_NUMBER_DENSITY).isDefault:
                issues[_PROP_SAMPLE_NUMBER_DENSITY] = 'Number density ' + \
                    'must be specified.'
            if not self.getProperty(_PROP_EC_WS).isDefault:
                if self.getProperty(
                        _PROP_CONTAINER_CHEMICAL_FORMULA).isDefault:
                    issues[_PROP_CONTAINER_CHEMICAL_FORMULA] = 'Chemical ' + \
                        'formula must be specified.'
                if self.getProperty(_PROP_CONTAINER_NUMBER_DENSITY).isDefault:
                    issues[_PROP_CONTAINER_NUMBER_DENSITY] = 'Number ' + \
                        'density must be specified.'
            sampleShape = self.getProperty(_PROP_SAMPLE_SHAPE).value
            if sampleShape == _SAMPLE_SHAPE_CYLINDER:
                if self.getProperty(_PROP_SAMPLE_INNER_RADIUS).isDefault:
                    issues[_PROP_SAMPLE_INNER_RADIUS] = 'Inner radius ' + \
                        'must be specified.'
                if self.getProperty(_PROP_SAMPLE_OUTER_RADIUS).isDefault:
                    issues[_PROP_SAMPLE_OUTER_RADIUS] = 'Outer radius ' + \
                        'must be specified.'
                if self.getProperty(_PROP_BEAM_HEIGHT).isDefault:
                    issues[_PROP_BEAM_HEIGHT] = 'Beam height must be ' + \
                        'specified.'
                if self.getProperty(_PROP_BEAM_WIDTH).isDefault:
                    issues[_PROP_BEAM_WIDTH] = 'Beam widtht must be specified.'
                if not self.getProperty(_PROP_EC_WS).isDefault:
                    if self.getProperty(
                            _PROP_CONTAINER_OUTER_RADIUS).isDefault:
                        issues[_PROP_CONTAINER_OUTER_RADIUS] = 'Outer ' + \
                            'radius must be specified.'
            else:
                if self.getProperty(_PROP_SAMPLE_THICKNESS).isDefault:
                    issues[_PROP_SAMPLE_THICKNESS] = 'Thickness must be ' + \
                        'specified.'
                if self.getProperty(_PROP_SAMPLE_ANGLE).isDefault:
                    issues[_PROP_SAMPLE_ANGLE] = 'Sample angle must be ' + \
                        'specified.'
                if not self.getProperty(_PROP_EC_WS).isDefault:
                    if self.getProperty(
                            _PROP_CONTAINER_BACK_THICKNESS).isDefault:
                        issues[_PROP_CONTAINER_BACK_THICKNESS] = \
                            'Thickness must be specified.'
                    if self.getProperty(
                            _PROP_CONTAINER_FRONT_THICKNESS).isDefault:
                        issues[_PROP_CONTAINER_FRONT_THICKNESS] = \
                            'Thickness must be specified.'
        return issues

    def _applyUserMask(self, mainWS, wsNames, wsCleanup, algorithmLogging):
        '''
        Applies user mask to a workspace.
        '''
        userMask = self.getProperty(_PROP_USER_MASK).value
        indexType = self.getProperty(_PROP_INDEX_TYPE).value
        maskComponents = self.getProperty(_PROP_USER_MASK_COMPONENTS).value
        maskedWSName = wsNames.withSuffix('masked')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=algorithmLogging)
        if indexType == _INDEX_TYPE_DET_ID:
            MaskDetectors(Workspace=maskedWS,
                          DetectorList=userMask,
                          ComponentList=maskComponents,
                          EnableLogging=algorithmLogging)
        elif indexType == _INDEX_TYPE_SPECTRUM_NUMBER:
            MaskDetectors(Workspace=maskedWS,
                          SpectraList=userMask,
                          ComponentList=maskComponents,
                          EnableLogging=algorithmLogging)
        elif indexType == _INDEX_TYPE_WS_INDEX:
            MaskDetectors(Workspace=maskedWS,
                          WorkspaceIndexList=userMask,
                          ComponentList=maskComponents,
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
                eiCalibrationDets = \
                    self._convertListToWorkspaceIndices(eiCalibrationDets,
                                                        mainWS)
                monIndex = self.getProperty(_PROP_MON_INDEX).value
                monIndex = self._convertToWorkspaceIndex(monIndex, monWS)
                eiCalibrationWS = \
                    _calibratedIncidentEnergy(mainWS,
                                              detEPPWS,
                                              monWS,
                                              monEPPWS,
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

    def _convertListToWorkspaceIndices(self, indices, ws):
        '''
        Converts a list of spectrum nubmers/detector IDs to workspace indices.
        '''
        return [self._convertToWorkspaceIndex(i, ws) for i in indices]

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

    def _correctTOFAxis(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Adjusts the TOF axis to get the elastic channel correct.
        '''
        correctedWSName = wsNames.withSuffix('tof_axis_corrected')
        referenceWS = self.getProperty(_PROP_REFERENCE_TOF_AXIS_WS).value
        if referenceWS:
            correctedWS = CorrectTOFAxis(InputWorkspace=mainWS,
                                         OutputWorkspace=correctedWSName,
                                         ReferenceWorkspace=referenceWS,
                                         EnableLogging=subalgLogging)
            wsCleanup.cleanup(mainWS)
            return correctedWS
        if not self.getProperty(_PROP_ELASTIC_BIN_INDEX).isDefault:
            index = self.getProperty(_PROP_ELASTIC_BIN_INDEX).value
        else:
            if not mainWS.run().hasProperty('Detector.elasticpeak'):
                self.log().warning('No ' + _PROP_REFERENCE_TOF_AXIS_WS +
                                   ' or ' + _PROP_ELASTIC_BIN_INDEX +
                                   ' given. TOF axis will not be adjusted.')
                return mainWS
            index = mainWS.run().getLogData('Detector.elasticpeak').value
        detectorsAtL2 = self.getProperty(_PROP_DETS_AT_L2).value
        detectorsAtL2 = self._convertListToWorkspaceIndices(detectorsAtL2,
                                                            mainWS)
        correctedWS = CorrectTOFAxis(InputWorkspace=mainWS,
                                     OutputWorkspace=correctedWSName,
                                     IndexType='Workspace Index',
                                     ReferenceSpectra=detectorsAtL2,
                                     ElasticBinIndex=index,
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
                detectorsAtL2 = \
                    self._convertListToWorkspaceIndices(detectorsAtL2, mainWS)
                diagnosticsReportWSName = \
                    self.getProperty(
                        _PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).valueAsStr
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
                                                   diagnosticsReportWSName,
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

    def _enabledByNonECReduction(self, prop):
        '''
        Enables a property if reduction type is Vanadium or Sample.
        '''
        self.setPropertySettings(prop,
                                 EnabledWhenProperty(
                                     _PROP_REDUCTION_TYPE,
                                     PropertyCriterion.IsNotEqualTo,
                                     _REDUCTION_TYPE_EC))

    def _enabledBySampleReduction(self, prop):
        '''
        Enables a property if reduction type is Sample.
        '''
        self.setPropertySettings(prop,
                                 EnabledWhenProperty(
                                     _PROP_REDUCTION_TYPE,
                                     PropertyCriterion.IsEqualTo,
                                     _REDUCTION_TYPE_SAMPLE))

    def _enabledBySelfShieldingCorrection(self, prop):
        '''
        Enables a property if self-shielding corrections are enabled.
        '''
        self.setPropertySettings(prop,
                                 EnabledWhenProperty(
                                     _PROP_SELF_SHIELDING_CORRECTION,
                                     PropertyCriterion.IsEqualTo,
                                     _SELF_SHIELDING_CORRECTION_ON))

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

    def _groupDetectors(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Groups detectors with similar thetas.
        '''
        groups = _createDetectorGroups(mainWS)
        groupingPattern = _groupsToGroupingPattern(groups)
        groupedWSName = wsNames.withSuffix('grouped_detectors')
        groupedWS = GroupDetectors(InputWorkspace=mainWS,
                                   OutputWorkspace=groupedWSName,
                                   GroupingPattern=groupingPattern,
                                   KeepUngroupedSpectra=False,
                                   Behaviour='Average',
                                   EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return groupedWS

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        '''
        Returns the raw input workspace.
        '''
        inputFile = self.getProperty(_PROP_INPUT_FILE).value
        if inputFile:
            mainWS = _loadFiles(inputFile,
                                wsNames,
                                wsCleanup,
                                subalgLogging)
        elif self.getProperty(_PROP_INPUT_WS).value:
            mainWS = self.getProperty(_PROP_INPUT_WS).value
            wsCleanup.protect(mainWS)
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

    def _outputWSConvertedToTheta(self, mainWS, wsNames, wsCleanup,
                                  subalgLogging):
        '''
        If requested, converts the spectrum axis to theta and save the result
        into the proper output property.
        '''
        thetaWSName = self.getProperty(_PROP_OUTPUT_THETA_W_WS).valueAsStr
        if thetaWSName:
            thetaWSName = self.getProperty(_PROP_OUTPUT_THETA_W_WS).value
            thetaWS = ConvertSpectrumAxis(InputWorkspace=mainWS,
                                          OutputWorkspace=thetaWSName,
                                          Target='Theta',
                                          EMode='Direct',
                                          EnableLogging=subalgLogging)
            self.setProperty(_PROP_OUTPUT_THETA_W_WS, thetaWS)

    def _rebinInW(self, mainWS, wsNames, wsCleanup, report,
                  subalgLogging):
        '''
        Rebins the horizontal axis of a workspace.
        '''
        mode = self.getProperty(_PROP_REBINNING_MODE_W).value
        if mode == _REBIN_AUTO_ELASTIC_PEAK:
            binWidth = BinWidthAtX(InputWorkspace=mainWS,
                                   X=0.0,
                                   Rounding='10^n',
                                   EnableLogging=subalgLogging)
            params = [binWidth]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == _REBIN_AUTO_MEDIAN_BIN_WIDTH:
            binWidth = MedianBinWidth(InputWorkspace=mainWS,
                                      Rounding='10^n',
                                      EnableLogging=subalgLogging)
            params = [binWidth]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == _REBIN_MANUAL_W:
            params = self.getProperty(_PROP_REBINNING_PARAMS_W).value
        else:
            raise RuntimeError('Unknown ' + _PROP_REBINNING_MODE_W)
        rebinnedWS = _rebin(mainWS, params, wsNames, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return rebinnedWS

    def _selfShieldingAndEC(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Calculates and applies self shielding corrections and empty can
        subtraction.
        '''
        ecWS = self.getProperty(_PROP_EC_WS).value
        selfShielding = self.getProperty(_PROP_SELF_SHIELDING_CORRECTION).value
        if not ecWS and selfShielding == _SELF_SHIELDING_CORRECTION_OFF:
            # No EC, no self-shielding -> nothing to do.
            return mainWS
        elif ecWS and selfShielding == _SELF_SHIELDING_CORRECTION_OFF:
            # Use simple EC subtraction only.
            return self._subtractEC(mainWS, ecWS, wsNames, wsCleanup,
                                    subalgLogging)
        wavelengthWSName = wsNames.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(InputWorkspace=mainWS,
                                    OutputWorkspace=wavelengthWSName,
                                    Target='Wavelength',
                                    EMode='Direct',
                                    EnableLogging=subalgLogging)
        selfShieldingWS = \
            self.getProperty(_PROP_SELF_SHIELDING_CORRECTION_WS).value
        sampleShape = self.getProperty(_PROP_SAMPLE_SHAPE).value
        if ecWS:
            ecScaling = self.getProperty(_PROP_EC_SCALING_FACTOR).value
            tofCorrectedECWSName = wsNames.withSuffix('ec_tof_corrected')
            tofCorrectedECWS = CorrectTOFAxis(
                InputWorkspace=ecWS,
                OutputWorkspace=tofCorrectedECWSName,
                ReferenceWorkspace=mainWS,
                IndexType='Workspace Index',
                ReferenceSpectra='0',
                EnableLogging=subalgLogging)
            wavelengthECWSName = wsNames.withSuffix('ec_in_wavelength')
            wavelengthECWS = \
                ConvertUnits(InputWorkspace=tofCorrectedECWS,
                             OutputWorkspace=wavelengthECWSName,
                             Target='Wavelength',
                             EMode='Direct',
                             EnableLogging=subalgLogging)
            wsCleanup.cleanup(tofCorrectedECWS)
            if not selfShieldingWS:
                sampleChemicalFormula = \
                    self.getProperty(_PROP_SAMPLE_CHEMICAL_FORMULA).value
                sampleNumberDensity = \
                    self.getProperty(_PROP_SAMPLE_NUMBER_DENSITY).value
                containerChemicalFormula = \
                    self.getProperty(
                        _PROP_CONTAINER_CHEMICAL_FORMULA).value
                containerNumberDensity = \
                    self.getProperty(_PROP_CONTAINER_NUMBER_DENSITY).value
                stepSize = \
                    self.getProperty(_PROP_SELF_SHIELDING_STEP_SIZE).value
                numberWavelengths = \
                    self.getProperty(
                        _PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS).value
                if sampleShape == _SAMPLE_SHAPE_CYLINDER:
                    sampleInnerR = \
                        self.getProperty(_PROP_SAMPLE_INNER_RADIUS).value
                    sampleOuterR = \
                        self.getProperty(_PROP_SAMPLE_OUTER_RADIUS).value
                    containerOuterR = \
                        self.getProperty(
                            _PROP_CONTAINER_OUTER_RADIUS).value
                    beamWidth = self.getProperty(_PROP_BEAM_WIDTH).value
                    beamHeight = self.getProperty(_PROP_BEAM_HEIGHT).value
                    selfShieldingWS = \
                        _selfShieldingCorrectionsCylinder(
                            wavelengthWS, wavelengthECWS,
                            sampleChemicalFormula, sampleNumberDensity,
                            containerChemicalFormula,
                            containerNumberDensity, sampleInnerR,
                            sampleOuterR, containerOuterR, beamWidth,
                            beamHeight, stepSize, numberWavelengths,
                            wsNames, subalgLogging)
                else:
                    # Slab container.
                    sampleThickness = \
                        self.getProperty(_PROP_SAMPLE_THICKNESS).value
                    containerFrontThickness = \
                        self.getProperty(
                            _PROP_CONTAINER_FRONT_THICKNESS).value
                    containerBackThickness = \
                        self.getProperty(
                            _PROP_CONTAINER_BACK_THICKNESS).value
                    angle = self.getProperty(_PROP_SAMPLE_ANGLE).value
                    selfShieldingWS = \
                        _selfShieldingCorrectionsSlab(
                            wavelengthWS, wavelengthECWS,
                            sampleChemicalFormula, sampleNumberDensity,
                            containerChemicalFormula,
                            containerNumberDensity, sampleThickness, angle,
                            containerFrontThickness,
                            containerBackThickness, numberWavelengths,
                            wsNames, subalgLogging)
            correctedWS = _applySelfShieldingCorrections(wavelengthWS,
                                                         wavelengthECWS,
                                                         ecScaling,
                                                         selfShieldingWS,
                                                         wsNames,
                                                         subalgLogging)
            wsCleanup.cleanup(wavelengthECWS)
        else:  # No ecWS.
            if not selfShieldingWS:
                sampleChemicalFormula = \
                    self.getProperty(_PROP_SAMPLE_CHEMICAL_FORMULA).value
                sampleNumberDensity = \
                    self.getProperty(_PROP_SAMPLE_NUMBER_DENSITY).value
                stepSize = \
                    self.getProperty(_PROP_SELF_SHIELDING_STEP_SIZE).value
                numberWavelengths = \
                    self.getProperty(
                        _PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS).value
                if sampleShape == _SAMPLE_SHAPE_CYLINDER:
                    sampleInnerR = \
                        self.getProperty(_PROP_SAMPLE_INNER_RADIUS).value
                    sampleOuterR = \
                        self.getProperty(_PROP_SAMPLE_OUTER_RADIUS).value
                    beamWidth = self.getProperty(_PROP_BEAM_WIDTH).value
                    beamHeight = self.getProperty(_PROP_BEAM_HEIGHT).value
                    selfShieldingWS = \
                        _selfShieldingCorrectionsCylinderNoEC(
                            wavelengthWS, sampleChemicalFormula,
                            sampleNumberDensity, sampleInnerR, sampleOuterR,
                            beamWidth, beamHeight, stepSize, numberWavelengths,
                            wsNames, subalgLogging)
                else:
                    # Slab container.
                    sampleThickness = \
                        self.getProperty(_PROP_SAMPLE_THICKNESS).value
                    angle = self.getProperty(_PROP_SAMPLE_ANGLE).value
                    selfShieldingWS = \
                        _selfShieldingCorrectionsSlabNoEC(
                            wavelengthWS, sampleChemicalFormula,
                            sampleNumberDensity, sampleThickness, angle,
                            numberWavelengths, wsNames, subalgLogging)
            correctedWS = _applySelfShieldingCorrectionsNoEC(wavelengthWS,
                                                             selfShieldingWS,
                                                             wsNames,
                                                             subalgLogging)
        correctedWS = ConvertUnits(InputWorkspace=correctedWS,
                                   OutputWorkspace=correctedWS,
                                   Target='TOF',
                                   EMode='Direct',
                                   EnableLogging=subalgLogging)
        if not self.getProperty(
                _PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS).isDefault:
            self.setProperty(_PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS,
                             selfShieldingWS)
        wsCleanup.cleanup(mainWS)
        wsCleanup.cleanup(wavelengthWS)
        return correctedWS

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

    def _sOfQW(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Runs the SofQWNormalisedPolygon algorithm.
        '''
        sOfQWWSName = wsNames.withSuffix('sofqw')
        qRebinningMode = self.getProperty(_PROP_REBINNING_MODE_Q).value
        if qRebinningMode == _REBIN_AUTO_Q:
            qMin, qMax = _minMaxQ(mainWS)
            dq = _deltaQ(mainWS)
            qBinning = '{0}, {1}, {2}'.format(qMin, dq, qMax)
        else:
            qBinning = self.getProperty(_PROP_REBINNING_PARAMS_Q).value
        Ei = mainWS.run().getLogData('Ei').value
        sOfQWWS = SofQWNormalisedPolygon(InputWorkspace=mainWS,
                                         OutputWorkspace=sOfQWWSName,
                                         QAxisBinning=qBinning,
                                         EMode='Direct',
                                         EFixed=Ei,
                                         ReplaceNaNs=False,
                                         EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return sOfQWWS

    def _subtractEC(self, mainWS, ecInWS, wsNames, wsCleanup, subalgLogging):
        '''
        Subtracts the empty container workspace.
        '''
        ecScaling = self.getProperty(_PROP_EC_SCALING_FACTOR).value
        ecSubtractedWS = _subtractEC(mainWS,
                                     ecInWS,
                                     ecScaling,
                                     wsNames,
                                     wsCleanup,
                                     subalgLogging)
        wsCleanup.cleanup(mainWS)
        return ecSubtractedWS

    def _transpose(self, mainWS, wsNames, wsCleanup, subalgLogging):
        '''
        Transposes the final output workspace.
        '''
        transposing = self.getProperty(_PROP_TRANSPOSE_SAMPLE_OUTPUT).value
        if transposing == _TRANSPOSING_OFF:
            return mainWS
        pointDataWSName = wsNames.withSuffix('point_data_converted')
        pointDataWS = ConvertToPointData(InputWorkspace=mainWS,
                                         OutputWorkspace=pointDataWSName,
                                         EnableLogging=subalgLogging)
        transposedWSName = wsNames.withSuffix('transposed')
        transposedWS = Transpose(InputWorkspace=pointDataWS,
                                 OutputWorkspace=transposedWSName,
                                 EnableLogging=subalgLogging)
        wsCleanup.cleanup(pointDataWS)
        wsCleanup.cleanup(mainWS)
        return transposedWS

AlgorithmFactory.subscribe(DirectILLReduction)
