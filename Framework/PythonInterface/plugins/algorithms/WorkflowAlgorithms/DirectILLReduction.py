# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, Progress, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatArrayProperty, FloatBoundedValidator, IntBoundedValidator, Property,
                           StringListValidator)
from mantid.simpleapi import (BinWidthAtX, CloneWorkspace, ConvertToPointData, ConvertUnits, CorrectKiKf, DetectorEfficiencyCorUser,
                              Divide, GroupDetectors, MaskDetectors, MedianBinWidth, Rebin, SofQWNormalisedPolygon, Transpose)
import numpy
from scipy import constants


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
        if spectrumInfo.isMasked(i):
            twoThetas[i] = numpy.nan
        else:
            twoThetas[i] = ws.detectorTwoTheta(det)
    for i in range(numHistograms):
        if detectorGrouped[i]:
            continue
        twoTheta1 = twoThetas[i]
        if numpy.isnan(twoTheta1):
            continue
        currentGroup = [ws.getDetector(i).getID()]
        twoThetaDiff = numpy.abs(twoThetas[i + 1:] - twoTheta1)
        equalTwoThetas = numpy.flatnonzero(twoThetaDiff < 0.01 / 180.0 * constants.pi)
        for j in (i + 1) + equalTwoThetas:
            currentGroup.append(ws.getDetector(j).getID())
            detectorGrouped[j] = True
        groups.append(currentGroup)
    return groups


def _deltaQ(ws):
    """Estimate a q bin width for a S(theta, w) workspace."""
    deltaTheta = _medianDeltaTheta(ws)
    wavelength = ws.run().getProperty('wavelength').value
    return 2.0 * constants.pi / wavelength * deltaTheta


def _detectorGroupsToXml(groups, instrument):
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
    dThetas = numpy.diff(thetas)
    return numpy.median(dThetas[dThetas > numpy.radians(0.1)])


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
    from xml.etree import ElementTree
    with open(filename, mode='w') as outFile:
        ElementTree.ElementTree(element).write(outFile)


class DirectILLReduction(DataProcessorAlgorithm):
    """A data reduction workflow algorithm for the direct geometry TOF spectrometers at ILL."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

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
        # TODO Absolute normalization.
        progress.report('Normalising to vanadium')
        mainWS = self._normalizeToVana(mainWS, wsNames, wsCleanup,
                                       subalgLogging)

        # Convert units from TOF to energy.
        progress.report('Converting to energy')
        mainWS = self._convertTOFToDeltaE(mainWS, wsNames, wsCleanup,
                                          subalgLogging)

        # KiKf conversion.
        mainWS = self._correctByKiKf(mainWS, wsNames,
                                     wsCleanup, subalgLogging)

        # Rebinning.
        progress.report('Rebinning in energy')
        mainWS = self._rebinInW(mainWS, wsNames, wsCleanup, report,
                                subalgLogging)

        # Detector efficiency correction.
        progress.report('Correcting detector efficiency')
        mainWS = self._correctByDetectorEfficiency(mainWS, wsNames,
                                                   wsCleanup, subalgLogging)

        progress.report('Grouping detectors')
        mainWS = self._groupDetectors(mainWS, wsNames, wsCleanup,
                                      subalgLogging)

        self._outputWSConvertedToTheta(mainWS, wsNames, wsCleanup,
                                       subalgLogging)

        progress.report('Converting to q')
        mainWS = self._sOfQW(mainWS, wsNames, wsCleanup, subalgLogging)
        mainWS = self._transpose(mainWS, wsNames, wsCleanup, subalgLogging)
        self._finalize(mainWS, wsCleanup, report)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=0)

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
                             doc='The output of the algorithm.')
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
            doc='Reduced vanadium workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Mandatory),
            doc='Detector diagnostics workspace obtained from another ' +
                'reduction run.')
        self.declareProperty(name=common.PROP_REBINNING_MODE_W,
                             defaultValue=common.REBIN_AUTO_ELASTIC_PEAK,
                             validator=StringListValidator([
                                 common.REBIN_AUTO_ELASTIC_PEAK,
                                 common.REBIN_AUTO_MEDIAN_BIN_WIDTH,
                                 common.REBIN_MANUAL_W]),
                             direction=Direction.Input,
                             doc='Energy rebinnin mode.')
        self.setPropertyGroup(common.PROP_REBINNING_MODE_W, common.PROPGROUP_REBINNING)
        self.declareProperty(name=common.PROP_BIN_COUNT_LIMIT_W,
                             defaultValue=Property.EMPTY_INT,
                             validator=positiveInt,
                             direction=Direction.Input,
                             doc='Maximum number of energy bins for automatic rebinning '
                                 '(default: 10 times the number of TOF bins).')
        self.declareProperty(FloatArrayProperty(name=common.PROP_REBINNING_PARAMS_W),
                             doc='Manual energy rebinning parameters.')
        self.setPropertyGroup(common.PROP_REBINNING_PARAMS_W, common.PROPGROUP_REBINNING)
        self.declareProperty(name=common.PROP_BINNING_MODE_Q,
                             defaultValue=common.REBIN_AUTO_Q,
                             validator=StringListValidator([
                                 common.REBIN_AUTO_Q,
                                 common.REBIN_MANUAL_Q]),
                             direction=Direction.Input,
                             doc='q rebinning mode.')
        self.setPropertyGroup(common.PROP_BINNING_MODE_Q, common.PROPGROUP_REBINNING)
        self.declareProperty(FloatArrayProperty(name=common.PROP_BINNING_PARAMS_Q),
                             doc='Manual q rebinning parameters.')
        self.setPropertyGroup(common.PROP_BINNING_PARAMS_Q, common.PROPGROUP_REBINNING)
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
        # TODO
        return dict()

    def _applyDiagnostics(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Mask workspace according to diagnostics."""
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        if not diagnosticsWS:
            return mainWS
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

    def _binWidthLimit(self, ws):
        if self.getProperty(common.PROP_BIN_COUNT_LIMIT_W).isDefault:
            countLimit = 10 * ws.blocksize()
        else:
            countLimit = self.getProperty(common.PROP_BIN_COUNT_LIMIT_W).value
        xs = ws.dataX(0)
        return (xs[-1] - xs[0]) / float(countLimit)

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

    def _groupDetectors(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Group detectors with similar thetas."""
        import os
        import tempfile
        groups = _createDetectorGroups(mainWS)
        instrumentName = mainWS.run().getProperty('instrument.name').value
        groupsXml = _detectorGroupsToXml(groups, instrumentName)
        fileHandle, path = tempfile.mkstemp(suffix='.xml', prefix='grouping-{}-'.format(instrumentName))
        _writeXml(groupsXml, path)
        groupedWSName = wsNames.withSuffix('grouped_detectors')
        groupedWS = GroupDetectors(InputWorkspace=mainWS,
                                   OutputWorkspace=groupedWSName,
                                   MapFile=path,
                                   KeepUngroupedSpectra=False,
                                   Behaviour='Average',
                                   EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        os.remove(path)
        return groupedWS

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS


    def _normalizeToVana(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Normalize to vanadium workspace."""
        vanaWS = self.getProperty(common.PROP_VANA_WS).value
        # TODO integrate vana, ComputeCalibrationCoefVan
        vanaNormalizedWSName = wsNames.withSuffix('vanadium_normalized')
        vanaNormalizedWS = Divide(LHSWorkspace=mainWS,
                                  RHSWorkspace=vanaWS,
                                  OutputWorkspace=vanaNormalizedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return vanaNormalizedWS

    def _outputWSConvertedToTheta(self, mainWS, wsNames, wsCleanup,
                                  subalgLogging):
        """If requested, convert the spectrum axis to theta and save the result
        into the proper output property.
        """
        thetaWSName = self.getProperty(common.PROP_OUTPUT_THETA_W_WS).valueAsStr
        if thetaWSName:
            thetaWSName = self.getProperty(common.PROP_OUTPUT_THETA_W_WS).value
            thetaWS = ConvertSpectrumAxis(InputWorkspace=mainWS,
                                          OutputWorkspace=thetaWSName,
                                          Target='Theta',
                                          EMode='Direct',
                                          EnableLogging=subalgLogging)
            self.setProperty(common.PROP_OUTPUT_THETA_W_WS, thetaWS)

    def _rebinInW(self, mainWS, wsNames, wsCleanup, report,
                  subalgLogging):
        """Rebin the horizontal axis of a workspace."""
        mode = self.getProperty(common.PROP_REBINNING_MODE_W).value
        if mode == common.REBIN_AUTO_ELASTIC_PEAK:
            binWidth = BinWidthAtX(InputWorkspace=mainWS,
                                   X=0.0,
                                   Rounding='10^n',
                                   EnableLogging=subalgLogging)
            binWidthLimit = self._binWidthLimit(mainWS)
            params = [max(binWidth, binWidthLimit)]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == common.REBIN_AUTO_MEDIAN_BIN_WIDTH:
            binWidth = MedianBinWidth(InputWorkspace=mainWS,
                                      Rounding='10^n',
                                      EnableLogging=subalgLogging)
            binWidthLimit = self._binWidthLimit(mainWS)
            params = [max(binWidth, binWidthLimit)]
            report.notice('Rebinned energy axis to bin width {}.'
                          .format(binWidth))
        elif mode == common.REBIN_MANUAL_W:
            params = self.getProperty(common.PROP_REBINNING_PARAMS_W).value
        else:
            raise RuntimeError('Unknown ' + common.PROP_REBINNING_MODE_W)
        rebinnedWS = _rebin(mainWS, params, wsNames, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return rebinnedWS

    def _sOfQW(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Run the SofQWNormalisedPolygon algorithm."""
        sOfQWWSName = wsNames.withSuffix('sofqw')
        qRebinningMode = self.getProperty(common.PROP_BINNING_MODE_Q).value
        if qRebinningMode == common.REBIN_AUTO_Q:
            qMin, qMax = _minMaxQ(mainWS)
            dq = _deltaQ(mainWS)
            qBinning = '{0}, {1}, {2}'.format(qMin, dq, qMax)
        else:
            qBinning = self.getProperty(common.PROP_BINNING_PARAMS_Q).value
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

    def _transpose(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Transpose the final output workspace."""
        transposing = self.getProperty(common.PROP_TRANSPOSE_SAMPLE_OUTPUT).value
        if transposing == common.TRANSPOSING_OFF:
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
