# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        MatrixWorkspaceProperty, Progress, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatArrayProperty, FloatBoundedValidator, Property, RebinParamsValidator,
                           StringListValidator)
from mantid.simpleapi import (BinWidthAtX, CloneWorkspace, ConvertSpectrumAxis, ConvertToDistribution, ConvertUnits, CorrectKiKf,
                              DetectorEfficiencyCorUser, Divide, GenerateGroupingPowder, GroupDetectors, MaskDetectors, Rebin, Scale,
                              SofQWNormalisedPolygon, Transpose)
import math
import numpy
import os
from scipy import constants
import tempfile


def _absoluteUnits(ws, vanaWS, wsNames, wsCleanup, report, algorithmLogging):
    """Scales ws by an absolute units factor."""
    sampleMaterial = ws.sample().getMaterial()
    sampleNumberDensity = sampleMaterial.numberDensity
    vanaMaterial = vanaWS.sample().getMaterial()
    vanaNumberDensity = vanaMaterial.numberDensity
    vanaCrossSection = vanaMaterial.totalScatterXSection()
    factor = vanaNumberDensity / sampleNumberDensity * vanaCrossSection
    if factor <= 0 or math.isnan(factor) or math.isinf(factor):
        raise RuntimeError('Invalid absolute units normalisation factor: {}'.format(factor))
    report.notice('Absolute units scaling factor: {}'.format(factor))
    scaledWSName = wsNames.withSuffix('absolute_units')
    scaledWS = Scale(InputWorkspace=ws,
                     OutputWorkspace=scaledWSName,
                     Factor=factor,
                     EnableLogging=algorithmLogging)
    wsCleanup.cleanup(ws)
    return scaledWS


def _createDetectorGroups(ws):
    """Find workspace indices with (almost) same theta and group them. Masked
    detectors are ignored.
    """
    numHistograms = ws.getNumberHistograms()
    groups = list()
    detectorGrouped = numHistograms * [False]
    spectrumInfo = ws.spectrumInfo()
    twoThetas = numpy.empty(numHistograms)
    for i in range(numHistograms):
        det = ws.getDetector(i)
        twoThetas[i] = ws.detectorTwoTheta(det)
    for i in range(numHistograms):
        if spectrumInfo.isMasked(i) or detectorGrouped[i]:
            continue
        twoTheta1 = twoThetas[i]
        currentGroup = [ws.getDetector(i).getID()]
        twoThetaDiff = numpy.abs(twoThetas[i + 1:] - twoTheta1)
        equalTwoThetas = numpy.flatnonzero(twoThetaDiff < 0.01 / 180.0 * constants.pi)
        for j in (i + 1) + equalTwoThetas:
            if spectrumInfo.isMasked(int(j)):
                continue
            currentGroup.append(ws.getDetector(int(j)).getID())
            detectorGrouped[j] = True
        groups.append(currentGroup)
    return groups


def _detectorGroupsToXml(groups, instrument):
    """Create grouping pattern XML tree from detector groups."""
    from xml.etree import ElementTree
    rootElement = ElementTree.Element('detector-grouping', {'instrument': 'IN5'})
    for group in groups:
        name = str(group.pop())
        groupElement = ElementTree.SubElement(rootElement, 'group', {'name': name})
        detIds = name
        for detId in group:
            detIds += ',' + str(detId)
        ElementTree.SubElement(groupElement, 'detids', {'val': detIds})
    return rootElement


def _defaultEnergyBinning(ws, algorithmLogging):
    """Create common (but nonequidistant) binning for a DeltaE workspace."""
    xs = ws.extractX()
    minXIndex = numpy.nanargmin(xs[:, 0])
    dx = BinWidthAtX(InputWorkspace=ws,
                     X=0.0,
                     EnableLogging=algorithmLogging)
    lastX = numpy.max(xs[:, -1])
    binCount = ws.blocksize()
    borders = list()
    templateXs = xs[minXIndex, :]
    currentX = numpy.nan
    for i in range(binCount):
        currentX = templateXs[i]
        borders.append(currentX)
        if currentX > 0:
            break
    i = 1
    equalBinStart = borders[-1]
    while currentX < lastX:
        currentX = equalBinStart + i * dx
        borders.append(currentX)
        i += 1
    borders[-1] = lastX
    return numpy.array(borders)


def _deltaQ(ws):
    """Estimate a q bin width for a S(theta, w) workspace."""
    deltaTheta = _medianDeltaTheta(ws)
    wavelength = ws.run().getProperty('wavelength').value
    return 2.0 * constants.pi / wavelength * deltaTheta


def _parseHybridBinningTokens(rebinning):
    """Return a list of rebinning ranges for given hybrid param string."""
    tokens = rebinning.split(',')
    paramGroups = list()
    currentGroup = list()
    for tokenIndex in range(len(tokens)):
        token = tokens[tokenIndex].strip()
        if token == 'a':
            if currentGroup:
                paramGroups.append(currentGroup)
                currentGroup = list()
            # Don't add consecutive empty lists into paramGroups.
            if not paramGroups or (paramGroups and paramGroups[-1]):
                # Empty list in paramGroups means automatic binning
                paramGroups.append(list())
        else:
            try:
                value = float(token)
            except ValueError:
                raise RuntimeError('Unknown token in ' + common.PROP_REBINNING_W + ": '" + token + "'.")
            currentGroup.append(value)
            if tokenIndex == len(tokens) - 1:
                paramGroups.append(currentGroup)
    return paramGroups


def _paramGroupsToEdges(groups):
    """Convert param groups to groups of bin edges."""
    edgeGroups = list()
    for params in groups:
        if not params:
            # Empty list in edgeGroups means automatic binning.
            edgeGroups.append(list())
        else:
            edges = list()
            beginX = params.pop(0)
            while params:
                if len(params) < 2:
                    raise RuntimeError('Error in ' + common.PROP_REBINNING_W
                                       + ': not enough numbers to form the binning.')
                dx = params.pop(0)
                endX = params.pop(0)
                x = beginX
                index = 1
                while x < endX:
                    edges.append(x)
                    x = beginX + index * dx
                    index += 1
                beginX = endX
            edges.append(beginX)
            edgeGroups.append(edges)
    return edgeGroups


def _mergeEdges(edges, edgeGroups, minX, maxX):
    """Merge edges and edgeGroups into a single list of bin edges."""
    mergedEdges = list()
    for groupIndex in range(len(edgeGroups)):
        currentGroup = edgeGroups[groupIndex]
        if not currentGroup:
            edgeBegin = edgeGroups[groupIndex - 1][-1] if groupIndex > 0 else edges[0]
            edgeEnd = edgeGroups[groupIndex + 1][0] if groupIndex < len(edgeGroups) - 1 else edges[-1]
            begin = numpy.searchsorted(edges, edgeBegin) + 1
            if begin < len(edges):
                e = edges[begin]
                index = begin
                while e < edgeEnd and index < len(edges):
                    if minX < e and e < maxX:
                        mergedEdges.append(e)
                    index += 1
                    e = edges[index]
        else:
            # Pick the edges from the groups.
            for e in currentGroup:
                if minX < e and e < maxX:
                    mergedEdges.append(e)
    return mergedEdges


def _hybridEnergyBinning(ws, rebinning, algorithmLogging):
    """Parse rebinning parameters retuning an array of bin boundaries."""
    paramGroups = _parseHybridBinningTokens(rebinning)
    autoEdges = _defaultEnergyBinning(ws, algorithmLogging)
    # Check limits for the full X range.
    globalBeginX = float('-inf')
    if len(paramGroups[0]) == 1:
        globalBeginX = paramGroups.pop(0)[0]
    elif len(paramGroups[0]) == 2:
        paramGroups[0].insert(0, autoEdges[0])
    globalEndX = float('inf')
    if len(paramGroups[-1]) == 1:
        globalEndX = paramGroups.pop(-1)[0]
    elif len(paramGroups[-1]) == 2:
        paramGroups[-1].append(autoEdges[-1])
    # Build bin edges from user given binning params
    userEdges = _paramGroupsToEdges(paramGroups)
    # Merge user and automatic edges
    mergedEdges = list()
    if not math.isinf(globalBeginX):
        mergedEdges.append(globalBeginX)
    mergedEdges += _mergeEdges(autoEdges, userEdges, globalBeginX, globalEndX)
    if not math.isinf(globalEndX):
        mergedEdges.append(globalEndX)
    return numpy.array(mergedEdges)


def _medianDeltaTheta(ws):
    """Calculate the median theta spacing for a S(theta, w) workspace."""
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
    dThetas = numpy.abs(numpy.diff(thetas))
    return numpy.median(dThetas[dThetas > numpy.deg2rad(0.1)])


def _minMaxQ(ws):
    """Estimate the start and end q bins for a S(theta, w) workspace."""
    Ei = ws.run().getProperty('Ei').value * 1e-3 * constants.e  # in Joules
    xs = ws.readX(0)
    minW = xs[0] * 1e-3 * constants.e  # in Joules
    maxEf = Ei - minW
    # In Ånströms
    maxQ = numpy.sqrt(2.0 * constants.m_n / constants.hbar**2 *
                      (Ei + maxEf - 2 * numpy.sqrt(Ei * maxEf) * -1.0)) * 1e-10
    minQ = 0.0
    return (minQ, maxQ)


def _rebin(ws, params, wsNames, algorithmLogging):
    """Rebin a workspace."""
    rebinnedWSName = wsNames.withSuffix('rebinned')
    rebinnedWS = Rebin(InputWorkspace=ws,
                       OutputWorkspace=rebinnedWSName,
                       Params=params,
                       EnableLogging=algorithmLogging)
    return rebinnedWS


def _writeXml(element, filename):
    """Write XML element to a file."""
    from xml.etree import ElementTree
    with open(filename, mode='wb') as outFile:
        ElementTree.ElementTree(element).write(outFile)


class DirectILLReduction(DataProcessorAlgorithm):
    """A data reduction workflow algorithm for the direct geometry TOF spectrometers at ILL."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return [ "DirectILLApplySelfShielding","DirectILLCollectData",
                 "DirectILLDiagnostics","DirectILLIntegrateVanadium","DirectILLSelfShielding" ]

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLReduction'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        progress = Progress(self, 0.0, 1.0, 9)
        report = common.Report()
        subalgLogging = False
        if self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON:
            subalgLogging = True
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

        # Get input workspace.
        progress.report('Loading inputs')
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        progress.report('Applying diagnostics')
        mainWS = self._applyDiagnostics(mainWS, wsNames, wsCleanup, subalgLogging)

        # Vanadium normalization.
        progress.report('Normalising to vanadium')
        mainWS = self._normalizeToVana(mainWS, wsNames, wsCleanup, report, subalgLogging)

        # Convert units from TOF to energy.
        progress.report('Converting to energy')
        mainWS = self._convertTOFToDeltaE(mainWS, wsNames, wsCleanup, subalgLogging)

        # KiKf conversion.
        mainWS = self._correctByKiKf(mainWS, wsNames, wsCleanup, subalgLogging)

        # Rebinning.
        progress.report('Rebinning in energy')
        mainWS = self._rebinInW(mainWS, wsNames, wsCleanup, subalgLogging)

        # Divide the energy transfer workspace by bin widths.
        mainWS = self._convertToDistribution(mainWS, wsNames, wsCleanup, subalgLogging)

        # Detector efficiency correction.
        progress.report('Correcting detector efficiency')
        mainWS = self._correctByDetectorEfficiency(mainWS, wsNames, wsCleanup, subalgLogging)

        progress.report('Grouping detectors')
        mainWS = self._groupDetectors(mainWS, wsNames, wsCleanup, report, subalgLogging)

        self._outputWSConvertedToTheta(mainWS, wsNames, wsCleanup, subalgLogging)

        progress.report('Converting to q')
        mainWS = self._sOfQW(mainWS, wsNames, wsCleanup, report, subalgLogging)
        mainWS = self._transpose(mainWS, wsNames, wsCleanup, subalgLogging)
        self._finalize(mainWS, wsCleanup, report)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        PROPGROUP_REBINNING = 'Rebinning for SofQW'
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(0., exclusive=True)
        validRebinParams = RebinParamsValidator(AllowEmpty=True)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input),
            doc='A workspace to reduce.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The reduced S(Q, DeltaE) workspace.')
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
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_VANA_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='An integrated vanadium workspace.')
        self.declareProperty(name=common.PROP_ABSOLUTE_UNITS,
                             defaultValue=common.ABSOLUTE_UNITS_OFF,
                             validator=StringListValidator([
                                 common.ABSOLUTE_UNITS_OFF,
                                 common.ABSOLUTE_UNITS_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable normalisation to absolute units.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Detector diagnostics workspace for masking.')
        self.declareProperty(name=common.PROP_GROUPING_ANGLE_STEP,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             doc='A scattering angle step to which to group detectors, in degrees.')
        self.declareProperty(FloatArrayProperty(name=common.PROP_REBINNING_PARAMS_W, validator=validRebinParams),
                             doc='Manual energy rebinning parameters.')
        self.setPropertyGroup(common.PROP_REBINNING_PARAMS_W, PROPGROUP_REBINNING)
        self.declareProperty(name=common.PROP_REBINNING_W,
                             defaultValue='',
                             doc='Energy rebinning when mixing manual and automatic binning parameters.')
        self.declareProperty(FloatArrayProperty(name=common.PROP_BINNING_PARAMS_Q, validator=validRebinParams),
                             doc='Manual q rebinning parameters.')
        self.setPropertyGroup(common.PROP_BINNING_PARAMS_Q, PROPGROUP_REBINNING)
        self.declareProperty(name=common.PROP_TRANSPOSE_SAMPLE_OUTPUT,
                             defaultValue=common.TRANSPOSING_ON,
                             validator=StringListValidator([
                                 common.TRANSPOSING_ON,
                                 common.TRANSPOSING_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable ' + common.PROP_OUTPUT_WS + ' transposing.')
        self.declareProperty(WorkspaceProperty(
            name=common.PROP_OUTPUT_THETA_W_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output workspace for reduced S(theta, DeltaE).')
        self.setPropertyGroup(common.PROP_OUTPUT_THETA_W_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        issues = dict()
        eBinParamProp = self.getProperty(common.PROP_REBINNING_PARAMS_W)
        eBinProp = self.getProperty(common.PROP_REBINNING_W)
        if not eBinParamProp.isDefault and not eBinProp.isDefault:
            issues[common.PROP_REBINNING_W] = 'Cannot be specified at the same time with ' + common.PROP_REBINNING_PARAMS_W + '.'
        return issues

    def _applyDiagnostics(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Mask workspace according to diagnostics."""
        if self.getProperty(common.PROP_DIAGNOSTICS_WS).isDefault:
            return mainWS
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        maskedWSName = wsNames.withSuffix('diagnostics_applied')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        MaskDetectors(Workspace=maskedWS,
                      MaskedWorkspace=diagnosticsWS,
                      EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return maskedWS

    def _convertToDistribution(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Convert the workspace into a distribution."""
        distributionWSName = wsNames.withSuffix('as_distribution')
        distributionWS = CloneWorkspace(InputWorkspace=mainWS,
                                        OutputWorkspace=distributionWSName,
                                        EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        ConvertToDistribution(Workspace=distributionWS,
                              EnableLogging=subalgLogging)
        return distributionWS

    def _convertTOFToDeltaE(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Convert the X axis units from time-of-flight to energy transfer."""
        energyConvertedWSName = wsNames.withSuffix('energy_converted')
        energyConvertedWS = ConvertUnits(InputWorkspace=mainWS,
                                         OutputWorkspace=energyConvertedWSName,
                                         Target='DeltaE',
                                         EMode='Direct',
                                         EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return energyConvertedWS

    def _correctByDetectorEfficiency(self, mainWS, wsNames, wsCleanup,
                                     subalgLogging):
        """Apply detector efficiency corrections."""
        correctedWSName = wsNames.withSuffix('detector_efficiency_corrected')
        correctedWS = \
            DetectorEfficiencyCorUser(InputWorkspace=mainWS,
                                      OutputWorkspace=correctedWSName,
                                      EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return correctedWS

    def _correctByKiKf(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Apply the k_i / k_f correction."""
        correctedWSName = wsNames.withSuffix('kikf')
        correctedWS = CorrectKiKf(InputWorkspace=mainWS,
                                  OutputWorkspace=correctedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return correctedWS

    def _finalize(self, outWS, wsCleanup, report):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

    def _groupDetectors(self, mainWS, wsNames, wsCleanup, report, subalgLogging):
        """Group detectors with similar thetas."""
        instrument = mainWS.getInstrument()
        fileHandle, path = tempfile.mkstemp(suffix='.xml', prefix='grouping-{}-'.format(instrument.getName()))
        # We don't need the handle, just the path.
        os.close(fileHandle)
        angleStepProperty = self.getProperty(common.PROP_GROUPING_ANGLE_STEP)
        if angleStepProperty.isDefault:
            if instrument.hasParameter('natural-angle-step'):
                angleStep = instrument.getNumberParameter('natural-angle-step', recursive=False)[0]
                report.notice('Using grouping angle step of {} degrees from the IPF.'.format(angleStep))
            else:
                angleStep = 0.01
                report.notice('Using the default grouping angle step of {} degrees.'.format(angleStep))
        else:
            angleStep = angleStepProperty.value
        GenerateGroupingPowder(InputWorkspace=mainWS,
                               AngleStep=angleStep,
                               GroupingFilename=path,
                               GenerateParFile=False,
                               EnableLogging=subalgLogging)
        try:
            groupedWSName = wsNames.withSuffix('grouped_detectors')
            groupedWS = GroupDetectors(InputWorkspace=mainWS,
                                       OutputWorkspace=groupedWSName,
                                       MapFile=path,
                                       KeepUngroupedSpectra=False,
                                       Behaviour='Average',
                                       EnableLogging=subalgLogging)
            wsCleanup.cleanup(mainWS)
            return groupedWS
        finally:
            os.remove(path)

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _normalizeToVana(self, mainWS, wsNames, wsCleanup, report, subalgLogging):
        """Normalize to vanadium workspace."""
        if self.getProperty(common.PROP_VANA_WS).isDefault:
            return mainWS
        vanaWS = self.getProperty(common.PROP_VANA_WS).value
        vanaNormalizedWSName = wsNames.withSuffix('vanadium_normalized')
        vanaNormalizedWS = Divide(LHSWorkspace=mainWS,
                                  RHSWorkspace=vanaWS,
                                  OutputWorkspace=vanaNormalizedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        if self.getProperty(common.PROP_ABSOLUTE_UNITS).value == common.ABSOLUTE_UNITS_ON:
            vanaNormalizedWS = _absoluteUnits(vanaNormalizedWS, vanaWS, wsNames, wsCleanup, report,
                                              subalgLogging)
        return vanaNormalizedWS

    def _outputWSConvertedToTheta(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """
        If requested, convert the spectrum axis to theta and save the result
        into the proper output property.
        """
        if not self.getProperty(common.PROP_OUTPUT_THETA_W_WS).isDefault:
            thetaWSName = wsNames.withSuffix('in_theta_energy_for_output')
            thetaWS = ConvertSpectrumAxis(InputWorkspace=mainWS,
                                          OutputWorkspace=thetaWSName,
                                          Target='Theta',
                                          EMode='Direct',
                                          EnableLogging=subalgLogging)
            self.setProperty(common.PROP_OUTPUT_THETA_W_WS, thetaWS)
            wsCleanup.cleanup(thetaWS)

    def _rebinInW(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Rebin the horizontal axis of a workspace."""
        eRebinParams = self.getProperty(common.PROP_REBINNING_PARAMS_W)
        eRebin = self.getProperty(common.PROP_REBINNING_W)
        if eRebinParams.isDefault:
            if eRebin.isDefault:
                binBorders = _defaultEnergyBinning(mainWS, subalgLogging)
            else:
                binBorders = _hybridEnergyBinning(mainWS, eRebin.value, subalgLogging)
            params = list()
            binWidths = numpy.diff(binBorders)
            for start, width in zip(binBorders[:-1], binWidths):
                params.append(start)
                params.append(width)
            params.append(binBorders[-1])
        else:
            params = self.getProperty(common.PROP_REBINNING_PARAMS_W).value
        rebinnedWS = _rebin(mainWS, params, wsNames, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return rebinnedWS

    def _sOfQW(self, mainWS, wsNames, wsCleanup, report, subalgLogging):
        """Run the SofQWNormalisedPolygon algorithm."""
        sOfQWWSName = wsNames.withSuffix('sofqw')
        if self.getProperty(common.PROP_BINNING_PARAMS_Q).isDefault:
            qMin, qMax = _minMaxQ(mainWS)
            dq = _deltaQ(mainWS)
            e = numpy.ceil(-numpy.log10(dq)) + 1
            dq = (5. * ((dq*10**e) // 5 + 1.))*10**-e
            params = [qMin, dq, qMax]
            report.notice('Binned momentum transfer axis to bin width {0} A-1.'.format(dq))
        else:
            params = self.getProperty(common.PROP_BINNING_PARAMS_Q).value
            if len(params) == 1:
                qMin, qMax = _minMaxQ(mainWS)
                params = [qMin, params[0], qMax]
        Ei = mainWS.run().getLogData('Ei').value
        sOfQWWS = SofQWNormalisedPolygon(InputWorkspace=mainWS,
                                         OutputWorkspace=sOfQWWSName,
                                         QAxisBinning=params,
                                         EMode='Direct',
                                         EFixed=Ei,
                                         ReplaceNaNs=False,
                                         EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return sOfQWWS

    def _transpose(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Transpose the final output workspace."""
        transposing = self.getProperty(common.PROP_TRANSPOSE_SAMPLE_OUTPUT).value
        if transposing == common.TRANSPOSING_OFF:
            return mainWS
        transposedWSName = wsNames.withSuffix('transposed')
        transposedWS = Transpose(InputWorkspace=mainWS,
                                 OutputWorkspace=transposedWSName,
                                 EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return transposedWS


AlgorithmFactory.subscribe(DirectILLReduction)
