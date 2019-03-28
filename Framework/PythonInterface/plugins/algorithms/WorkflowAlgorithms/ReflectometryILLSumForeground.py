# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode,
                        WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatArrayBoundedValidator, FloatArrayProperty,
                           IntArrayBoundedValidator, IntArrayLengthValidator, IntArrayProperty, Property,
                           StringListValidator)
from mantid.simpleapi import (AddSampleLog, CropWorkspace, Divide, ExtractSingleSpectrum, RebinToWorkspace,
                              ReflectometryBeamStatistics, ReflectometrySumInQ)
import numpy
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_WS = 'DirectLineWorkspace'
    DIRECT_FOREGROUND_WS = 'DirectForegroundWorkspace'
    FOREGROUND_INDICES = 'Foreground'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    SUM_TYPE = 'SummationType'
    WAVELENGTH_RANGE = 'WavelengthRange'


class SumType:
    IN_LAMBDA = 'SumInLambda'
    IN_Q = 'SumInQ'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class ReflectometryILLSumForeground(DataProcessorAlgorithm):

    def category(self):
        """Return algorithm's categories."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLSumForeground'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Sums foreground pixels in selected summation mode, optionally converting to reflectivity.'

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryILLConvertToQ', 'ReflectometryILLPolarizationCor', 'ReflectometryILLPreprocess']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = common.WSCleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = common.WSNameSource(wsPrefix, cleanupMode)

        ws = self._inputWS()
        processReflected = not self._directOnly()
        if processReflected:
            self._addBeamStatisticsToLogs(ws)

        sumType = self._sumType()
        if sumType == SumType.IN_LAMBDA:
            ws = self._sumForegroundInLambda(ws)
            self._addSumTypeToLogs(ws, SumType.IN_LAMBDA)
            if processReflected:
                ws = self._rebinToDirect(ws)
        else:
            ws = self._divideByDirect(ws)
            ws = self._sumForegroundInQ(ws)
            self._addSumTypeToLogs(ws, SumType.IN_Q)
        ws = self._applyWavelengthRange(ws)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        threeNonnegativeInts = CompositeValidator()
        threeNonnegativeInts.add(IntArrayLengthValidator(3))
        nonnegativeInts = IntArrayBoundedValidator()
        nonnegativeInts.setLower(0)
        threeNonnegativeInts.add(nonnegativeInts)
        nonnegativeFloatArray = FloatArrayBoundedValidator()
        nonnegativeFloatArray.setLower(0.)
        inWavelength = WorkspaceUnitValidator('Wavelength')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.INPUT_WS,
                defaultValue='',
                direction=Direction.Input,
                validator=inWavelength),
            doc='A reflected beam workspace (units wavelength).')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output),
            doc='The summed foreground workspace.')
        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc='Enable or disable child algorithm logging.')
        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=common.WSCleanup.ON,
            validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
            doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(
            Prop.SUM_TYPE,
            defaultValue=SumType.IN_LAMBDA,
            validator=StringListValidator([SumType.IN_LAMBDA, SumType.IN_Q]),
            doc='Type of summation to perform.')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.DIRECT_FOREGROUND_WS,
                defaultValue='',
                direction=Direction.Input,
                optional=PropertyMode.Optional,
                validator=inWavelength),
            doc='Summed direct beam workspace (units wavelength).')
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=threeNonnegativeInts),
            doc='A three element array of foreground start, centre and end workspace indices.')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.DIRECT_WS,
                defaultValue='',
                direction=Direction.Input,
                optional=PropertyMode.Optional,
                validator=inWavelength),
            doc='The (not summed) direct beam workspace (units wavelength).')
        self.declareProperty(
            FloatArrayProperty(
                Prop.WAVELENGTH_RANGE,
                values=[0.],
                validator=nonnegativeFloatArray),
            doc='The wavelength bounds.')

    def validateInputs(self):
        """Validate the algorithm's input properties."""
        issues = dict()
        if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            if self.getProperty(Prop.SUM_TYPE).value == SumType.IN_Q:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'Direct foreground workspace is needed for summing in Q.'
        else:
            directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
            if directWS.getNumberHistograms() != 1:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'The workspace should have only a single histogram. Was foreground summation forgotten?'
            if self.getProperty(Prop.DIRECT_WS).isDefault:
                issues[Prop.DIRECT_WS] = 'The direct beam workspace is needed for processing the reflected workspace.'
        wRange = self.getProperty(Prop.WAVELENGTH_RANGE).value
        if len(wRange) == 2 and wRange[0] >= wRange[1]:
            issues[Prop.WAVELENGTH_RANGE] = 'Upper limit is smaller than the lower limit.'
        if len(wRange) > 2:
            issues[Prop.WAVELENGTH_RANGE] = 'The range should be in the form [min] or [min, max].'
        return issues

    def _addBeamStatisticsToLogs(self, ws):
        """Calculate beam statistics and add the results to the sample logs."""
        reflectedForeground = self._foregroundIndices(ws)
        directWS = self.getProperty(Prop.DIRECT_WS).value
        directForeground = self._foregroundIndices(directWS)
        instrumentName = common.instrumentName(ws)
        pixelSize = common.pixelSize(instrumentName)
        detResolution = common.detectorResolution()
        firstSlitSizeLog = common.slitSizeLogEntry(instrumentName, 1)
        secondSlitSizeLog = common.slitSizeLogEntry(instrumentName, 2)
        ReflectometryBeamStatistics(
            ReflectedBeamWorkspace=ws,
            ReflectedForeground=reflectedForeground,
            DirectLineWorkspace=directWS,
            DirectForeground=directForeground,
            PixelSize=pixelSize,
            DetectorResolution=detResolution,
            FirstSlitName='slit2',
            FirstSlitSizeSampleLog=firstSlitSizeLog,
            SecondSlitName='slit3',
            SecondSlitSizeSampleLog=secondSlitSizeLog,
            EnableLogging=self._subalgLogging)

    def _addSumTypeToLogs(self, ws, sumType):
        """Add a sum type entry to sample logs."""
        AddSampleLog(
            Workspace=ws,
            LogName=common.SampleLogs.SUM_TYPE,
            LogText=sumType,
            LogType='String',
            EnableLogging=self._subalgLogging)

    def _applyWavelengthRange(self, ws):
        """Cut wavelengths outside the wavelength range from a TOF workspace."""
        wRange = self.getProperty(Prop.WAVELENGTH_RANGE).value
        rangeProp = {'XMin': wRange[0]}
        if len(wRange) == 2:
            rangeProp['XMax'] = wRange[1]
        croppedWSName = self._names.withSuffix('cropped')
        croppedWS = CropWorkspace(
            InputWorkspace=ws,
            OutputWorkspace=croppedWSName,
            EnableLogging=self._subalgLogging,
            **rangeProp)
        self._cleanup.cleanup(ws)
        return croppedWS

    def _directOnly(self):
        """Return true if only the direct beam should be processed."""
        return self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault

    def _divideByDirect(self, ws):
        """Divide ws by the direct beam."""
        ws = self._rebinToDirect(ws)
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        reflectivityWSName = self._names.withSuffix('reflectivity')
        reflectivityWS = Divide(
            LHSWorkspace=ws,
            RHSWorkspace=directWS,
            OutputWorkspace=reflectivityWSName,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        reflectivityWS = common.correctForChopperOpenings(reflectivityWS, directWS, self._names, self._cleanup, self._subalgLogging)
        reflectivityWS.setYUnit('Reflectivity')
        reflectivityWS.setYUnitLabel('Reflectivity')
        return reflectivityWS

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _foregroundIndices(self, ws):
        """Return a three-element list of foreground start, center and end workspace indices."""
        foregroundProp = self.getProperty(Prop.FOREGROUND_INDICES)
        if not foregroundProp.isDefault:
            return foregroundProp.value
        logs = ws.run()
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_START):
            raise RuntimeError("The sample logs are missing the '" + common.SampleLogs.FOREGROUND_START + "' entry.")
        start = logs.getProperty(common.SampleLogs.FOREGROUND_START).value
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_CENTRE):
            raise RuntimeError("The sample logs are missing the '" + common.SampleLogs.FOREGROUND_CENTRE + "' entry.")
        centre = logs.getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_END):
            raise RuntimeError("The sample logs are missing the '" + common.SampleLogs.FOREGROUND_END + "' entry.")
        end = logs.getProperty(common.SampleLogs.FOREGROUND_END).value
        return [start, centre, end]

    def _inputWS(self):
        """Return the input workspaces."""
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        return ws

    def _rebinToDirect(self, ws):
        """Rebin ws to direct foreground."""
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        rebinnedWSName = self._names.withSuffix('rebinned')
        rebinnedWS = RebinToWorkspace(
            WorkspaceToRebin=ws,
            WorkspaceToMatch=directWS,
            OutputWorkspace=rebinnedWSName,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return rebinnedWS

    def _sumForegroundInLambda(self, ws):
        """Sum the foreground region into a single histogram."""
        foreground = self._foregroundIndices(ws)
        sumIndices = [i for i in range(foreground[0], foreground[2] + 1)]
        beamPosIndex = foreground[1]
        foregroundWSName = self._names.withSuffix('foreground_grouped')
        foregroundWS = ExtractSingleSpectrum(
            InputWorkspace=ws,
            OutputWorkspace=foregroundWSName,
            WorkspaceIndex=beamPosIndex,
            EnableLogging=self._subalgLogging)
        maxIndex = ws.getNumberHistograms() - 1
        foregroundYs = foregroundWS.dataY(0)
        foregroundEs = foregroundWS.dataE(0)
        numpy.square(foregroundEs, out=foregroundEs)
        for i in sumIndices:
            if i == beamPosIndex:
                continue
            if i < 0 or i > maxIndex:
                self.log().warning('Foreground partially out of the workspace.')
            addeeWSName = self._names.withSuffix('foreground_addee')
            addeeWS = ExtractSingleSpectrum(
                InputWorkspace=ws,
                OutputWorkspace=addeeWSName,
                WorkspaceIndex=i,
                EnableLogging=self._subalgLogging)
            addeeWS = RebinToWorkspace(
                WorkspaceToRebin=addeeWS,
                WorkspaceToMatch=foregroundWS,
                OutputWorkspace=addeeWSName,
                EnableLogging=self._subalgLogging)
            ys = addeeWS.readY(0)
            foregroundYs += ys
            es = addeeWS.readE(0)
            foregroundEs += es**2
            self._cleanup.cleanup(addeeWS)
        self._cleanup.cleanup(ws)
        numpy.sqrt(foregroundEs, out=foregroundEs)
        return foregroundWS

    def _sumForegroundInQ(self, ws):
        """Sum the foreground region into a single histogram using the coherent method."""
        foreground = self._foregroundIndices(ws)
        sumIndices = [i for i in range(foreground[0], foreground[2] + 1)]
        linePosition = ws.run().getProperty(common.SampleLogs.LINE_POSITION).value
        isFlatSample = not ws.run().getProperty('beam_stats.bent_sample').value
        sumWSName = self._names.withSuffix('summed_in_Q')
        sumWS = ReflectometrySumInQ(
            InputWorkspace=ws,
            OutputWorkspace=sumWSName,
            InputWorkspaceIndexSet=sumIndices,
            BeamCentre=linePosition,
            FlatSample=isFlatSample,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return sumWS

    def _sumType(self):
        return self.getProperty(Prop.SUM_TYPE).value


AlgorithmFactory.subscribe(ReflectometryILLSumForeground)
