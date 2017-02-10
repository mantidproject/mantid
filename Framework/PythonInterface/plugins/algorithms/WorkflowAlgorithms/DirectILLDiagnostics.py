# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty, PropertyMode, WorkspaceProperty, 
                        WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatBoundedValidator, IntArrayBoundedValidator,
                           IntArrayProperty, StringArrayProperty, StringListValidator)
from mantid.simpleapi import (ClearMaskFlag, CloneWorkspace, ConvertToConstantL2, CreateEmptyTableWorkspace, Divide,
                              ExtractMask, Integration, MaskDetectors, MedianDetectorTest, Plus, SolidAngle)


class _DiagnosticsSettings:
    """A class to hold settings for MedianDetectorTest."""

    def __init__(self, lowThreshold, highThreshold, significanceTest):
        """Initialize an instance of the class."""
        self.lowThreshold = lowThreshold
        self.highThreshold = highThreshold
        self.significanceTest = significanceTest


def _createDiagnosticsReportTable(elasticIntensityWS, bkgWS, diagnosticsWS,
                                  reportWSName, algorithmLogging):
    """Return a table workspace containing information used for detector diagnostics."""
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


def _extractUserMask(ws, wsNames, algorithmLogging):
    """Extracts user specified mask from a workspace"""
    maskWSName = wsNames.withSuffix('user_mask_extracted')
    maskWS, detectorIDs = ExtractMask(InputWorkspace=ws,
                                      OutputWorkspace=maskWSName,
                                      EnableLogging=algorithmLogging)
    return maskWS


def _diagnoseDetectors(ws, bkgWS, eppWS, eppIndices, peakSettings,
                       sigmaMultiplier, noisyBkgSettings, wsNames,
                       wsCleanup, report, reportWSName, algorithmLogging):
    """Return a diagnostics workspace."""
    # 1. Diagnose elastic peak region.
    constantL2WSName = wsNames.withSuffix('constant_L2')
    constantL2WS = ConvertToConstantL2(InputWorkspace=ws,
                                       OutputWorkspace=constantL2WSName,
                                       EnableLogging=algorithmLogging)
    medianEPPCentre, medianEPPSigma = common.medianEPP(eppWS, eppIndices)
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
    ClearMaskFlag(Workspace=elasticPeakDiagnostics,
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
    userMask = _extractUserMask(ws, wsNames, algorithmLogging)
    combinedDiagnosticsWSName = wsNames.withSuffix('diagnostics')
    diagnosticsWS = Plus(LHSWorkspace=elasticPeakDiagnostics,
                         RHSWorkspace=noisyBkgDiagnostics,
                         OutputWorkspace=combinedDiagnosticsWSName,
                         EnableLogging=algorithmLogging)
    diagnosticsWS = Plus(LHSWorkspace=diagnosticsWS,
                         RHSWorkspace=userMask,
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
    wsCleanup.cleanup(userMask)
    return diagnosticsWS



def _maskDiagnosedDetectors(ws, diagnosticsWS, wsNames, algorithmLogging):
    """Mask detectors according to diagnostics."""
    maskedWSName = wsNames.withSuffix('diagnostics_applied')
    maskedWS = CloneWorkspace(InputWorkspace=ws,
                              OutputWorkspace=maskedWSName,
                              EnableLogging=algorithmLogging)
    MaskDetectors(Workspace=maskedWS,
                  MaskedWorkspace=diagnosticsWS,
                  EnableLogging=algorithmLogging)
    return maskedWS


def _reportDiagnostics(report, peakDiagnostics, bkgDiagnostics):
    """Parse the mask workspaces and fills in the report accordingly."""
    for i in range(peakDiagnostics.getNumberHistograms()):
        _reportSinglePeakDiagnostics(report, peakDiagnostics, i)
        _reportSingleBkgDiagnostics(report, bkgDiagnostics, i)


def _reportSingleBkgDiagnostics(report, diagnostics, wsIndex):
    """Report the result of background diagnostics for a single diagnose."""
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


def _reportSinglePeakDiagnostics(report, diagnostics, wsIndex):
    """Report the result of zero count diagnostics for a single diagnose."""
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


class DirectILLDiagnostics(DataProcessorAlgorithm):
    """A workflow algorithm for detector diagnostics."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLDiagnostics'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Perform detector diagnostics and masking for the direct geometry TOF spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        report = common.Report()
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        # Get input workspace.
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        # Apply user mask.
        mainWS = self._applyUserMask(mainWS, wsNames,
                                     wsCleanup, subalgLogging)

        # Detector diagnostics, if requested.
        mainWS = self._detDiagnostics(mainWS, wsNames, wsCleanup, report, subalgLogging)

        self._finalize(mainWS, wsCleanup)

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        greaterThanUnityFloat = FloatBoundedValidator(lower=1)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveIntArray = IntArrayBoundedValidator()
        positiveIntArray.setLower(0)
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='A diagnostics mask workspace.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=common.CLEANUP_ON,
                             validator=StringListValidator([
                                 common.CLEANUP_ON,
                                 common.CLEANUP_OFF]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=common.PROP_SUBALG_LOGGING,
                             defaultValue=common.SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 common.SUBALG_LOGGING_OFF,
                                 common.SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to ' +
                                 'print in the logs.')
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Table workspace containing results from the FindEPP ' +
                'algorithm.')
        self.declareProperty(name=common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the elastic peak in multiples " +
                                 " of 'Sigma' in the EPP table.")
        self.declareProperty(IntArrayProperty(name=common.PROP_DETS_AT_L2,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of detectors with the instruments ' +
                                 'nominal L2 distance.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_FLAT_BKG_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Workspace from which to get flat background data.')
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.0,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=33.3,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              common.PROPGROUP_DET_DIAGNOSTICS)
        self.declareProperty(IntArrayProperty(name=common.PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask.')
        self.declareProperty(
            StringArrayProperty(name=common.PROP_USER_MASK_COMPONENTS,
                                values='',
                                direction=Direction.Input),
            doc='List of instrument components to mask.')
        # Rest of the output properties
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output table workspace for detector diagnostics reporting.')
        self.setPropertyGroup(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        # TODO implement this.
        return dict()

    def _applyUserMask(self, mainWS, wsNames, wsCleanup, algorithmLogging):
        """Apply user mask to a workspace."""
        userMask = self.getProperty(common.PROP_USER_MASK).value
        maskComponents = self.getProperty(common.PROP_USER_MASK_COMPONENTS).value
        maskedWSName = wsNames.withSuffix('masked')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=algorithmLogging)
        MaskDetectors(Workspace=maskedWS,
                      DetectorList=userMask,
                      ComponentList=maskComponents,
                      EnableLogging=algorithmLogging)
        return maskedWS

    def _detDiagnostics(self, mainWS, wsNames, wsCleanup, report, subalgLogging):
        """Perform and apply detector diagnostics."""
        lowThreshold = self.getProperty(common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD).value
        highThreshold = self.getProperty(common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD).value
        significanceTest = self.getProperty(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
        peakDiagnosticsSettings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
        sigmaMultiplier = self.getProperty(common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
        lowThreshold = self.getProperty(common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD).value
        highThreshold = self.getProperty(common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD).value
        significanceTest = self.getProperty(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
        bkgDiagnosticsSettings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
        detectorsAtL2 = self.getProperty(common.PROP_DETS_AT_L2).value
        detectorsAtL2 = common.convertListToWorkspaceIndices(detectorsAtL2, mainWS)
        diagnosticsReportWSName = self.getProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).valueAsStr
        bkgWS = self.getProperty(common.PROP_FLAT_BKG_WS).value
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        diagnosticsWS = _diagnoseDetectors(mainWS, bkgWS, eppWS, detectorsAtL2, peakDiagnosticsSettings,
                                           sigmaMultiplier, bkgDiagnosticsSettings, wsNames, wsCleanup,
                                           report, diagnosticsReportWSName, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return diagnosticsWS

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.finalCleanup()

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS


AlgorithmFactory.subscribe(DirectILLDiagnostics)
