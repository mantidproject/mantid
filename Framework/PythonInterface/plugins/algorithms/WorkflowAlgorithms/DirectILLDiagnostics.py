# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
import ILL_utilities as utils
import mantid
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, Progress, PropertyMode,
                        WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatBoundedValidator, IntArrayBoundedValidator,
                           IntArrayProperty, Property, StringArrayProperty, StringListValidator)
from mantid.simpleapi import (ClearMaskFlag, CreateEmptyTableWorkspace, Divide, ExtractMask, Integration, LoadMask, MaskDetectors,
                              MedianDetectorTest, Plus, Scale, SolidAngle)
import numpy
import os.path

_PLOT_TYPE_X = 1
_PLOT_TYPE_Y = 2


class _DiagnosticsSettings:
    """A class to hold settings for MedianDetectorTest."""

    def __init__(self, lowThreshold, highThreshold, significanceTest):
        """Initialize an instance of the class."""
        self.lowThreshold = lowThreshold
        self.highThreshold = highThreshold
        self.significanceTest = significanceTest


def _beamStopRanges(ws):
    """Parse beam stop ranges from the instrument parameters of ws."""
    instrument = ws.getInstrument()
    definition = instrument.getStringParameter('beam_stop_diagnostics_spectra')[0]
    ranges = definition.split(',')
    begins = list()
    ends = list()
    for r in ranges:
        (begin, end) = r.split('-')
        begin = int(begin)
        end = int(end)
        if (begin >= end):
            raise RuntimeError("While parsing 'beam_stop_diagnostics_spectra' instrument parameter: "
                               + "begin spectrum number greater than the end number.")
        begin = common.convertToWorkspaceIndex(begin, ws, common.INDEX_TYPE_SPECTRUM_NUMBER)
        end = common.convertToWorkspaceIndex(end, ws, common.INDEX_TYPE_SPECTRUM_NUMBER)
        begins.append(begin)
        ends.append(end)
    return (begins, ends)


def _bkgDiagnostics(bkgWS, noisyBkgSettings, wsNames, algorithmLogging):
    """Diagnose noisy flat backgrounds and return a mask workspace."""
    noisyBkgWSName = wsNames.withSuffix('diagnostics_noisy_bkg')
    noisyBkgDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=bkgWS,
                           OutputWorkspace=noisyBkgWSName,
                           SignificanceTest=noisyBkgSettings.significanceTest,
                           LowThreshold=noisyBkgSettings.lowThreshold,
                           HighThreshold=noisyBkgSettings.highThreshold,
                           LowOutlier=0.0,
                           EnableLogging=algorithmLogging)
    ClearMaskFlag(Workspace=noisyBkgDiagnostics,
                  EnableLogging=algorithmLogging)
    return noisyBkgDiagnostics


def _createDiagnosticsReportTable(reportWSName, numberHistograms, algorithmLogging):
    """Return a table workspace for detector diagnostics reporting."""
    if mtd.doesExist(reportWSName):
        reportWS = mtd[reportWSName]
    else:
        reportWS = CreateEmptyTableWorkspace(OutputWorkspace=reportWSName,
                                             EnableLogging=algorithmLogging)
    existingColumnNames = reportWS.getColumnNames()
    if 'WorkspaceIndex' not in existingColumnNames:
        reportWS.addColumn('int', 'WorkspaceIndex', _PLOT_TYPE_X)
    reportWS.setRowCount(numberHistograms)
    for i in range(numberHistograms):
        reportWS.setCell('WorkspaceIndex', i, i)
    return reportWS


def _createMaskWS(ws, name, algorithmLogging):
    """Return a single bin workspace with same number of histograms as ws."""
    extractResult = ExtractMask(InputWorkspace=ws,
                                OutputWorkspace=name,
                                EnableLogging=algorithmLogging)
    maskWS = Scale(InputWorkspace=extractResult.OutputWorkspace,
                   Factor=0.,
                   OutputWorkspace=name,
                   EnableLogging=algorithmLogging)
    return maskWS


def _elasticPeakDiagnostics(ws, peakSettings, wsNames, algorithmLogging):
    """Diagnose elastic peaks and return a mask workspace"""
    elasticPeakDiagnosticsWSName = wsNames.withSuffix('diagnostics_elastic_peak')
    elasticPeakDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=ws,
                           OutputWorkspace=elasticPeakDiagnosticsWSName,
                           SignificanceTest=peakSettings.significanceTest,
                           LowThreshold=peakSettings.lowThreshold,
                           HighThreshold=peakSettings.highThreshold,
                           EnableLogging=algorithmLogging)
    ClearMaskFlag(Workspace=elasticPeakDiagnostics,
                  EnableLogging=algorithmLogging)
    return elasticPeakDiagnostics


def _integrateBkgs(ws, eppWS, sigmaMultiplier, wsNames, wsCleanup, algorithmLogging):
    """Return a workspace integrated around flat background areas."""
    histogramCount = ws.getNumberHistograms()
    binMatrix = ws.extractX()
    leftBegins = binMatrix[:, 0]
    leftEnds = numpy.empty(histogramCount)
    rightBegins = numpy.empty(histogramCount)
    rightEnds = binMatrix[:, -1]
    for i in range(histogramCount):
        eppRow = eppWS.row(i)
        if eppRow['FitStatus'] != 'success':
            leftBegins[i] = 0
            leftEnds[i] = 0
            rightBegins[i] = 0
            rightEnds[i] = 0
            continue
        peakCentre = eppRow['PeakCentre']
        sigma = eppRow['Sigma']
        leftEnds[i] = peakCentre - sigmaMultiplier * sigma
        if leftBegins[i] > leftEnds[i]:
            leftBegins[i] = leftEnds[i]
        rightBegins[i] = peakCentre + sigmaMultiplier * sigma
        if rightBegins[i] > rightEnds[i]:
            rightBegins[i] = rightEnds[i]
    leftWSName =  wsNames.withSuffix('integrated_left_bkgs')
    leftWS = Integration(InputWorkspace=ws,
                         OutputWorkspace=leftWSName,
                         RangeLowerList=leftBegins,
                         RangeUpperList=leftEnds,
                         EnableLogging=algorithmLogging)
    rightWSName = wsNames.withSuffix('integrated_right_bkgs')
    rightWS = Integration(InputWorkspace=ws,
                          OutputWorkspace=rightWSName,
                          RangeLowerList=rightBegins,
                          RangeUpperList=rightEnds,
                          EnableLogging=algorithmLogging)
    sumWSName = wsNames.withSuffix('integrated_bkgs_sum')
    sumWS = Plus(LHSWorkspace=leftWS,
                 RHSWorkspace=rightWS,
                 OutputWorkspace=sumWSName,
                 EnableLogging=algorithmLogging)
    wsCleanup.cleanup(leftWS)
    wsCleanup.cleanup(rightWS)
    return sumWS


def _integrateElasticPeaks(ws, eppWS, sigmaMultiplier, wsNames, wsCleanup, algorithmLogging):
    """Return a workspace integrated around the elastic peak."""
    histogramCount = ws.getNumberHistograms()
    integrationBegins = numpy.empty(histogramCount)
    integrationEnds = numpy.empty(histogramCount)
    for i in range(histogramCount):
        eppRow = eppWS.row(i)
        if eppRow['FitStatus'] != 'success':
            integrationBegins[i] = 0
            integrationEnds[i] = 0
            continue
        peakCentre = eppRow['PeakCentre']
        sigma = eppRow['Sigma']
        integrationBegins[i] = peakCentre - sigmaMultiplier * sigma
        integrationEnds[i] = peakCentre + sigmaMultiplier * sigma
    integratedElasticPeaksWSName = \
        wsNames.withSuffix('integrated_elastic_peak')
    integratedElasticPeaksWS = \
        Integration(InputWorkspace=ws,
                    OutputWorkspace=integratedElasticPeaksWSName,
                    IncludePartialBins=True,
                    RangeLowerList=integrationBegins,
                    RangeUpperList=integrationEnds,
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
    wsCleanup.cleanup(integratedElasticPeaksWS)
    wsCleanup.cleanup(solidAngleWS)
    return solidAngleCorrectedElasticPeaksWS


def _maskedListToStr(masked):
    """Return a string presentation of a list of spectrum numbers."""
    if not masked:
        return 'None'

    def addBlock(string, begin, end):
        if blockBegin == blockEnd:
            string += str(blockEnd) + ', '
        else:
            string += str(blockBegin) + '-' + str(blockEnd) + ', '
        return string

    string = str()
    blockBegin = None
    blockEnd = None
    maxMasked = max(masked)
    for i in sorted(masked):
        if blockBegin is None:
            blockBegin = i
            blockEnd = i
        if i == maxMasked:
            if i > blockEnd + 1:
                string = addBlock(string, blockBegin, blockEnd)
                string += str(i)
            else:
                string += str(blockBegin) + '-' + str(i)
            break
        if i > blockEnd + 1:
            string = addBlock(string, blockBegin, blockEnd)
            blockBegin = i
        blockEnd = i
    return string


def _reportBkgDiagnostics(reportWS, bkgWS, diagnosticsWS):
    """Return masked spectrum numbers and add background diagnostics information to a report workspace."""
    return _reportDiagnostics(reportWS, bkgWS, diagnosticsWS, 'FlatBkg', 'BkgDiagnosed')


def _reportBeamStopMask(reportWS, maskWS):
    """Return masked spectrum numbers and add default mask information to a report workspace."""
    return _reportMasking(reportWS, maskWS, 'BeamStopMask')


def _reportDefaultMask(reportWS, maskWS):
    """Return masked spectrum numbers and add default mask information to a report workspace."""
    return _reportMasking(reportWS, maskWS, 'DefaultMask')


def _reportDiagnostics(reportWS, dataWS, diagnosticsWS, dataColumn, diagnosedColumn):
    """Return masked spectrum numbers and add diagnostics information to a report workspace."""
    if reportWS is not None:
        existingColumnNames = reportWS.getColumnNames()
        if dataColumn not in existingColumnNames:
            reportWS.addColumn('double', dataColumn, _PLOT_TYPE_Y)
        if diagnosedColumn not in existingColumnNames:
            reportWS.addColumn('double', diagnosedColumn, _PLOT_TYPE_Y)
    maskedSpectra = set()
    for i in range(dataWS.getNumberHistograms()):
        diagnosed = int(diagnosticsWS.readY(i)[0])
        if reportWS is not None:
            y = dataWS.readY(i)[0]
            reportWS.setCell(dataColumn, i, y)
            reportWS.setCell(diagnosedColumn, i, diagnosed)
        if diagnosed != 0:
            maskedSpectra.add(dataWS.getSpectrum(i).getSpectrumNo())
    return maskedSpectra


def _reportMasking(reportWS, maskWS, maskedColumn):
    """Return masked spectrum numbers and add masking information to a report workspace."""
    if reportWS is not None:
        existingColumnNames = reportWS.getColumnNames()
        if maskedColumn not in existingColumnNames:
            reportWS.addColumn('double', maskedColumn, _PLOT_TYPE_Y)
    maskedSpectra = set()
    for i in range(maskWS.getNumberHistograms()):
        masked = int(maskWS.readY(i)[0])
        if reportWS is not None:
            reportWS.setCell(maskedColumn, i, masked)
        if masked != 0:
            maskedSpectra.add(maskWS.getSpectrum(i).getSpectrumNo())
    return maskedSpectra


def _reportPeakDiagnostics(reportWS, peakIntensityWS, diagnosticsWS):
    """Return masked spectrum numbers and add elastic peak diagnostics information to a report workspace."""
    return _reportDiagnostics(reportWS, peakIntensityWS, diagnosticsWS, 'ElasticIntensity', 'IntensityDiagnosed')


class DirectILLDiagnostics(DataProcessorAlgorithm):
    """A workflow algorithm for detector diagnostics."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return [ "DirectILLReduction" ]

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
        progress = Progress(self, 0.0, 1.0, 7)
        self._report = utils.Report()
        self._subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        self._names = utils.NameSource(wsNamePrefix, cleanupMode)
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)

        progress.report('Loading inputs')
        mainWS = self._inputWS()

        maskWSName = self._names.withSuffix('combined_mask')
        maskWS = _createMaskWS(mainWS, maskWSName, self._subalgLogging)
        self._cleanup.cleanupLater(maskWS)

        reportWS = None
        if not self.getProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).isDefault:
            reportWSName = self.getProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).valueAsStr
            reportWS = _createDiagnosticsReportTable(reportWSName, mainWS.getNumberHistograms(), self._subalgLogging)

        progress.report('Loading default mask')
        defaultMaskWS = self._defaultMask(mainWS)
        defaultMaskedSpectra = set()
        if defaultMaskWS is not None:
            defaultMaskedSpectra = _reportDefaultMask(reportWS, defaultMaskWS)
            maskWS = Plus(LHSWorkspace=maskWS,
                          RHSWorkspace=defaultMaskWS,
                          EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(defaultMaskWS)

        progress.report('User-defined mask')
        userMaskWS = self._userMask(mainWS)
        maskWS = Plus(LHSWorkspace=maskWS,
                      RHSWorkspace=userMaskWS,
                      EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(userMaskWS)

        beamStopMaskedSpectra = set()
        if self._beamStopDiagnosticsEnabled(mainWS):
            progress.report('Diagnosing beam stop')
            beamStopMaskWS = self._beamStopDiagnostics(mainWS, maskWS)
            beamStopMaskedSpectra = _reportBeamStopMask(reportWS, beamStopMaskWS)
            maskWS = Plus(LHSWorkspace=maskWS,
                          RHSWorkspace=beamStopMaskWS,
                          EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(beamStopMaskWS)

        bkgMaskedSpectra = set()
        if self._bkgDiagnosticsEnabled(mainWS):
            progress.report('Diagnosing backgrounds')
            bkgMaskWS, bkgWS = self._bkgDiagnostics(mainWS)
            bkgMaskedSpectra = _reportBkgDiagnostics(reportWS, bkgWS, bkgMaskWS)
            maskWS = Plus(LHSWorkspace=maskWS,
                          RHSWorkspace=bkgMaskWS,
                          EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(bkgMaskWS)
            self._cleanup.cleanup(bkgWS)

        peakMaskedSpectra = set()
        if self._peakDiagnosticsEnabled(mainWS):
            progress.report('Diagnosing peaks')
            peakMaskWS, peakIntensityWS = self._peakDiagnostics(mainWS)
            peakMaskedSpectra = _reportPeakDiagnostics(reportWS, peakIntensityWS, peakMaskWS)
            maskWS = Plus(LHSWorkspace=maskWS,
                          RHSWorkspace=peakMaskWS,
                          EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(peakMaskWS)
            self._cleanup.cleanup(peakIntensityWS)

        self._outputReports(reportWS, defaultMaskedSpectra, beamStopMaskedSpectra,
                            peakMaskedSpectra, bkgMaskedSpectra)

        self._finalize(maskWS)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        PROPGROUP_BEAM_STOP_DIAGNOSTICS = 'Beam Stop Diagnostics'
        PROPGROUP_BKG_DIAGNOSTICS = 'Background Diagnostics'
        PROPGROUP_PEAK_DIAGNOSTICS = 'Elastic Peak Diagnostics'
        PROPGROUP_USER_MASK = 'Additional Masking'
        greaterThanUnityFloat = FloatBoundedValidator(lower=1)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveIntArray = IntArrayBoundedValidator(lower=0)
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input),
            doc="A 'raw' workspace from DirectILLCollectData to calculate the diagnostics from.")
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='A diagnostics mask workspace.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=utils.Cleanup.ON,
                             validator=StringListValidator([
                                 utils.Cleanup.ON,
                                 utils.Cleanup.OFF]),
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
            doc='Table workspace containing results from the FindEPP algorithm.')
        self.declareProperty(name=common.PROP_ELASTIC_PEAK_DIAGNOSTICS,
                             defaultValue=common.ELASTIC_PEAK_DIAGNOSTICS_AUTO,
                             validator=StringListValidator([
                                 common.ELASTIC_PEAK_DIAGNOSTICS_AUTO,
                                 common.ELASTIC_PEAK_DIAGNOSTICS_ON,
                                 common.ELASTIC_PEAK_DIAGNOSTICS_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_ELASTIC_PEAK_DIAGNOSTICS,
                              PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Integration half width of the elastic peak in multiples " +
                                 " of 'Sigma' in the EPP table.")
        self.setPropertyGroup(common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                              PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                              PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=Property.EMPTY_DBL,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                              PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='To fail the elastic peak diagnostics, the intensity must also exceed ' +
                                 'this number of error bars with respect to the median intensity.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS,
                             defaultValue=common.BKG_DIAGNOSTICS_AUTO,
                             validator=StringListValidator([
                                 common.BKG_DIAGNOSTICS_AUTO,
                                 common.BKG_DIAGNOSTICS_ON,
                                 common.BKG_DIAGNOSTICS_OFF]),
                             direction=Direction.Input,
                             doc='Control the background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS, PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_SIGMA_MULTIPLIER,
                             defaultValue=10.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the range excluded from background integration around " +
                                 "the elastic peaks in multiplies of 'Sigma' in the EPP table")
        self.setPropertyGroup(common.PROP_BKG_SIGMA_MULTIPLIER,
                              PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                              PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=Property.EMPTY_DBL,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                              PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='To fail the background diagnostics, the background level must also exceed ' +
                                 'this number of error bars with respect to the median level.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BEAM_STOP_DIAGNOSTICS,
                             defaultValue=common.BEAM_STOP_DIAGNOSTICS_AUTO,
                             validator=StringListValidator([
                                 common.BEAM_STOP_DIAGNOSTICS_AUTO,
                                 common.BEAM_STOP_DIAGNOSTICS_ON,
                                 common.BEAM_STOP_DIAGNOSTICS_OFF]),
                             direction=Direction.Input,
                             doc='Control the beam stop diagnostics.')
        self.setPropertyGroup(common.PROP_BEAM_STOP_DIAGNOSTICS, PROPGROUP_BEAM_STOP_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BEAM_STOP_THRESHOLD,
                             defaultValue=0.67,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for the lower acceptance limit for beam stop diagnostics.')
        self.setPropertyGroup(common.PROP_BEAM_STOP_THRESHOLD, PROPGROUP_BEAM_STOP_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_DEFAULT_MASK,
                             defaultValue=common.DEFAULT_MASK_ON,
                             validator=StringListValidator([
                                 common.DEFAULT_MASK_ON,
                                 common.DEFAULT_MASK_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable instrument specific default mask.')
        self.declareProperty(IntArrayProperty(name=common.PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask.')
        self.setPropertyGroup(common.PROP_USER_MASK, PROPGROUP_USER_MASK)
        self.declareProperty(
            StringArrayProperty(name=common.PROP_USER_MASK_COMPONENTS,
                                values='',
                                direction=Direction.Input),
            doc='List of instrument components to mask.')
        self.setPropertyGroup(common.PROP_USER_MASK_COMPONENTS, PROPGROUP_USER_MASK)
        # Rest of the output properties
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output table workspace for detector diagnostics reporting.')
        self.setPropertyGroup(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(name=common.PROP_OUTPUT_DIAGNOSTICS_REPORT,
                             defaultValue='',
                             direction=Direction.Output,
                             doc='Diagnostics report as a string.')
        self.setPropertyGroup(common.PROP_OUTPUT_DIAGNOSTICS_REPORT,
                              common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        issues = dict()
        if self.getProperty(common.PROP_EPP_WS).isDefault:
            if self.getProperty(common.PROP_ELASTIC_PEAK_DIAGNOSTICS).value == common.ELASTIC_PEAK_DIAGNOSTICS_ON:
                issues[common.PROP_EPP_WS] = 'An EPP table is needed for elastic peak diagnostics.'
            if self.getProperty(common.PROP_BKG_DIAGNOSTICS).value == common.BKG_DIAGNOSTICS_ON:
                issues[common.PROP_EPP_WS] = 'An EPP table is needed for background diagnostics.'
        for propName in [common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD, common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD]:
            prop = self.getProperty(propName)
            if not prop.isDefault:
                if prop.value >= 1.:
                    issues[propName] = 'The low threshold cannot equal or exceed 1.'
        return issues

    def _beamStopDiagnostics(self, mainWS, maskWS):
        """Diagnose beam stop and return a mask workspace."""
        import operator

        def thresholdIndex(ws, maskWS, begin, end, threshold):
            """Return an index where the threshold is crossed."""
            step = -1 if begin < end else 1
            comp = operator.ge if begin < end else operator.le
            Ys = numpy.empty(abs(end - begin))
            i = end + step
            indexShift = -min(begin, end + 1)
            while comp(i, begin):
                index = i + indexShift
                if maskWS.readY(i) == 0:
                    Ys[index] = numpy.sum(ws.readY(i))
                else:
                    Ys[index] = 0.
                i += step
            maxIndex = int(numpy.argmax(Ys))
            thresholdVal = threshold * Ys[maxIndex]
            i = maxIndex - indexShift + step
            while comp(i, begin):
                index = i + indexShift
                if Ys[index] < thresholdVal:
                    return i
                i += step
            return i
        beamStopDiagnosticsWSName = self._names.withSuffix('beam_stop_diagnostics')
        beamStopDiagnosticsWS = _createMaskWS(mainWS, beamStopDiagnosticsWSName, self._subalgLogging)
        threshold = self.getProperty(common.PROP_BEAM_STOP_THRESHOLD).value
        beginIndices, endIndices = _beamStopRanges(mainWS)
        for i in range(len(beginIndices)):
            begin = beginIndices[i]
            end = endIndices[i]
            mid = int((end + begin) / 2)
            lowerIdx = thresholdIndex(mainWS, maskWS, mid, begin, threshold)
            upperIdx = thresholdIndex(mainWS, maskWS, mid, end, threshold)
            if lowerIdx != upperIdx:
                for j in range(upperIdx - lowerIdx + 1):
                    ys = beamStopDiagnosticsWS.dataY(lowerIdx + j)
                    ys[0] = 1.0
        return beamStopDiagnosticsWS

    def _beamStopDiagnosticsEnabled(self, mainWS):
        """Return true if beam stop diagnostics are enabled, false otherwise."""
        beamStopDiagnostics = self.getProperty(common.PROP_BEAM_STOP_DIAGNOSTICS).value
        if beamStopDiagnostics == common.BEAM_STOP_DIAGNOSTICS_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter('beam_stop_diagnostics_spectra'):
                return True
            return False
        elif beamStopDiagnostics == common.BEAM_STOP_DIAGNOSTICS_ON:
            instrument = mainWS.getInstrument()
            if not instrument.hasParameter('beam_stop_diagnostics_spectra'):
                self._report.error("'beam_stop_diagnostics_spectra' missing from instrument parameters. "
                                   + "Beam stop diagnostics disabled.")
                return False
            return True
        return False

    def _bkgDiagnostics(self, mainWS):
        """Perform background diagnostics."""
        if self.getProperty(common.PROP_EPP_WS).isDefault:
            # With the AUTO option validateInputs might let missing EPPWorkspace pass.
            raise RuntimeError('Missing ' + common.PROP_EPP_WS + '. Elastic peak positions are needed for background diagnostics.')
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        sigmaMultiplier = self.getProperty(common.PROP_BKG_SIGMA_MULTIPLIER).value
        integratedBkgs = _integrateBkgs(mainWS, eppWS, sigmaMultiplier, self._names, self._cleanup, self._subalgLogging)
        lowThreshold = self._bkgDiagnosticsLowThreshold(mainWS)
        highThreshold = self._bkgDiagnosticsHighThreshold(mainWS)
        significanceTest = self._bkgDiagnosticsSignificanceTest(mainWS)
        settings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
        bkgDiagnosticsWS = _bkgDiagnostics(integratedBkgs, settings, self._names, self._subalgLogging)
        return (bkgDiagnosticsWS, integratedBkgs)

    def _bkgDiagnosticsEnabled(self, mainWS):
        """Return true if background diagnostics are enabled, false otherwise."""
        bkgDiagnostics = self.getProperty(common.PROP_BKG_DIAGNOSTICS).value
        if bkgDiagnostics == common.BKG_DIAGNOSTICS_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter('enable_background_diagnostics'):
                enabled = instrument.getBoolParameter('enable_background_diagnostics')[0]
                if not enabled:
                    self._report.notice('Background diagnostics disable by the IPF.')
                    return False
            self._report.notice('Background diagnostics enabled.')
            return True
        return bkgDiagnostics == common.BKG_DIAGNOSTICS_ON

    def _bkgDiagnosticsHighThreshold(self, ws):
        """Return a suitable value for the high threshold."""
        return self._value(ws, common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD, 'background_diagnostics_high_threshold', 3.3)

    def _bkgDiagnosticsLowThreshold(self, ws):
        """Return a suitable value for the low threshold."""
        return self._value(ws, common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD, 'background_diagnostics_low_threshold', 0.1)

    def _bkgDiagnosticsSignificanceTest(self, ws):
        """Return a suitable value for the significance test."""
        return self._value(ws, common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST, 'background_diagnostics_significance_test', 3.3)

    def _defaultMask(self, mainWS):
        """Load instrument specific default mask or return None if not available."""
        option = self.getProperty(common.PROP_DEFAULT_MASK).value
        if option == common.DEFAULT_MASK_OFF:
            return None
        instrument = mainWS.getInstrument()
        instrumentName = instrument.getName()
        if not instrument.hasParameter('Workflow.MaskFile'):
            self._report.notice('No default mask available for ' + instrumentName + '.')
            return None
        maskFilename = instrument.getStringParameter('Workflow.MaskFile')[0]
        maskFile = os.path.join(mantid.config.getInstrumentDirectory(), 'masks', maskFilename)
        defaultMaskWSName = self._names.withSuffix('default_mask')
        defaultMaskWS = LoadMask(Instrument=instrumentName,
                                 InputFile=maskFile,
                                 RefWorkspace=mainWS,
                                 OutputWorkspace=defaultMaskWSName,
                                 EnableLogging=self._subalgLogging)
        self._report.notice('Default mask loaded from ' + maskFilename)
        return defaultMaskWS

    def _finalize(self, outWS):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        self._cleanup.cleanup(outWS)
        self._cleanup.finalCleanup()
        self._report.toLog(self.log())

    def _inputWS(self):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        self._cleanup.protect(mainWS)
        return mainWS

    def _outputReports(self, reportWS, defaultMaskedSpectra, directBeamMaskedSpectra,
                       peakMaskedSpectra, bkgMaskedSpectra):
        """Set the optional output report properties."""
        if reportWS is not None:
            self.setProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS, reportWS)
        hardMasking = defaultMaskedSpectra | directBeamMaskedSpectra
        report = 'Spectra masked by default mask file:\n'
        report += _maskedListToStr(defaultMaskedSpectra)
        report += '\nSpectra masked by beam stop diagnostics:\n'
        report += _maskedListToStr(directBeamMaskedSpectra)
        report += '\nAdditional spectra marked as bad by elastic peak diagnostics:\n'
        report += _maskedListToStr(peakMaskedSpectra - hardMasking)
        report += '\nAdditional spectra marked as bad by flat background diagnostics:\n'
        report += _maskedListToStr(bkgMaskedSpectra - hardMasking)
        self.setProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT, report)

    def _peakDiagnostics(self, mainWS):
        """Perform elastic peak diagnostics."""
        if self.getProperty(common.PROP_EPP_WS).isDefault:
            # With the AUTO option validateInputs might let missing EPPWorkspace pass.
            raise RuntimeError('Missing ' + common.PROP_EPP_WS + '. Elastic peak positions are needed for peak diagnostics.')
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        sigmaMultiplier = self.getProperty(common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
        integratedPeaksWS = _integrateElasticPeaks(mainWS, eppWS, sigmaMultiplier, self._names, self._cleanup, self._subalgLogging)
        lowThreshold = self._peakDiagnosticsLowThreshold(mainWS)
        highThreshold = self._peakDiagnosticsHighThreshold(mainWS)
        significanceTest = self._peakDiagnosticsSignificanceTest(mainWS)
        settings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
        peakDiagnosticsWS = _elasticPeakDiagnostics(integratedPeaksWS, settings, self._names, self._subalgLogging)
        return (peakDiagnosticsWS, integratedPeaksWS)

    def _peakDiagnosticsEnabled(self, mainWS):
        """Return true if elastic peak diagnostics are enabled, false otherwise."""
        peakDiagnostics = self.getProperty(common.PROP_ELASTIC_PEAK_DIAGNOSTICS).value
        if peakDiagnostics == common.ELASTIC_PEAK_DIAGNOSTICS_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter('enable_elastic_peak_diagnostics'):
                enabled = instrument.getBoolParameter('enable_elastic_peak_diagnostics')[0]
                if not enabled:
                    self._report.notice('Elastic peak diagnostics disabled by the IPF.')
                    return False
            self._report.notice('Elastic peak diagnostics enabled.')
            return True
        return peakDiagnostics == common.ELASTIC_PEAK_DIAGNOSTICS_ON

    def _peakDiagnosticsHighThreshold(self, ws):
        """Return a suitable value for the high threshold."""
        return self._value(ws, common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD, 'elastic_peak_diagnostics_high_threshold', 3.)

    def _peakDiagnosticsLowThreshold(self, ws):
        """Return a suitable value for the low threshold."""
        return self._value(ws, common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD, 'elastic_peak_diagnostics_low_threshold', 0.1)

    def _peakDiagnosticsSignificanceTest(self, ws):
        """Return a suitable value for the significance test."""
        return self._value(ws, common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST, 'elastic_peak_diagnostics_significance_test', 3.3)

    def _userMask(self, mainWS):
        """Return combined masked spectra and components."""
        userMask = self.getProperty(common.PROP_USER_MASK).value
        maskComponents = self.getProperty(common.PROP_USER_MASK_COMPONENTS).value
        maskWSName = self._names.withSuffix('user_mask')
        maskWS = _createMaskWS(mainWS, maskWSName, self._subalgLogging)
        MaskDetectors(Workspace=maskWS,
                      DetectorList=userMask,
                      ComponentList=maskComponents,
                      EnableLogging=self._subalgLogging)
        extractResult = ExtractMask(InputWorkspace=maskWS,
                                    OutputWorkspace=maskWSName,
                                    EnableLogging=self._subalgLogging)
        return extractResult.OutputWorkspace

    def _value(self, ws, propertyName, instrumentParameterName, defaultValue):
        """Return a suitable value either from a property, the IPF or the supplied defaultValue."""
        prop = self.getProperty(propertyName)
        if prop.isDefault:
            instrument = ws.getInstrument()
            if instrument.hasParameter(instrumentParameterName):
                return instrument.getNumberParameter(instrumentParameterName)[0]
            return defaultValue
        return prop.value


AlgorithmFactory.subscribe(DirectILLDiagnostics)
