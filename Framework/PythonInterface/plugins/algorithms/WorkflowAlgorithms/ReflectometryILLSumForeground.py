# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatArrayBoundedValidator, FloatArrayLengthValidator, FloatArrayProperty,
                           IntArrayBoundedValidator, IntArrayLengthValidator, IntArrayProperty, Property, StringListValidator)
from mantid.simpleapi import (AddSampleLog, ConvertToDistribution, CreateWorkspace, CropWorkspace, Divide, ExtractSingleSpectrum,
                              Multiply, RebinToWorkspace, ReflectometrySumInQ)
import numpy
import ReflectometryILL_common as common
from scipy import constants


class Prop:
    CLEANUP = 'Cleanup'
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

        sumType = self._sumType()
        if sumType == SumType.IN_LAMBDA:
            ws = self._sumForegroundInLambda(ws)
            ws = self._divideByDirect(ws)
            self._addSumTypeToLogs(ws, SumType.IN_LAMBDA)
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
        twoNonnegativeFloats = CompositeValidator()
        twoNonnegativeFloats.add(FloatArrayLengthValidator(length=2))
        twoNonnegativeFloats.add(nonnegativeFloatArray)

        self.declareProperty(MatrixWorkspaceProperty(Prop.INPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='An input workspace (units wavelength) to be integrated.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='The integrated foreground divided by the summed direct beam.')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disable child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=common.WSCleanup.ON,
                             validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(Prop.SUM_TYPE,
                             defaultValue=SumType.IN_LAMBDA,
                             validator=StringListValidator([SumType.IN_LAMBDA, SumType.IN_Q]),
                             doc='Type of summation to perform.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.DIRECT_FOREGROUND_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='Summed direct beam workspace if output in reflectivity is required.')
        self.declareProperty(IntArrayProperty(Prop.FOREGROUND_INDICES,
                                              values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                                              validator=threeNonnegativeInts),
                             doc='A three element array of foreground start, centre and end workspace indices.')
        self.declareProperty(FloatArrayProperty(Prop.WAVELENGTH_RANGE,
                                                values=[0, Property.EMPTY_DBL],
                                                validator=twoNonnegativeFloats),
                             doc='The wavelength bounds when summing in Q.')

    def validateInputs(self):
        """Validate the algorithm's input properties."""
        issues = dict()
        if self.getProperty(Prop.SUM_TYPE).value == SumType.IN_Q:
            if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'Direct foreground workspace is needed for summing in Q.'
        if not self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
            if directWS.getNumberHistograms() != 1:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'The workspace has histograms != 1. Was foreground summation forgotten?'
        wRange = self.getProperty(Prop.WAVELENGTH_RANGE).value
        if len(wRange) == 2 and wRange[0] >= wRange[1]:
            issues[Prop.WAVELENGTH_RANGE] = 'Upper limit is smaller than the lower limit.'
        if len(wRange) > 2:
            issues[Prop.WAVELENGTH_RANGE] = 'The range should be in the form [min] or [min, max].'
        return issues

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
        if self.getProperty(Prop.WAVELENGTH_RANGE).isDefault:
            return ws
        if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            self.log().warning('Output is not cropped to given WavelengthRange when summing the direct beam.')
            return ws
        wRange = self.getProperty(Prop.WAVELENGTH_RANGE).value
        rangeProp = {'XMin': wRange[0]}
        if len(wRange) == 2:
            rangeProp['XMax'] = wRange[1]
        croppedWSName = self._names.withSuffix('cropped')
        croppedWS = CropWorkspace(InputWorkspace=ws,
                                  OutputWorkspace=croppedWSName,
                                  EnableLogging=self._subalgLogging,
                                  **rangeProp)
        self._cleanup.cleanup(ws)
        return croppedWS

    def _checkIfFlatSample(self):
        """Returns true if sample is deemed 'flat' for SumInQ."""
        # TODO: add real calculations here from COSMOS
        # Now we always return False because this is what is in a reference dataset.
        return False

    def _correctForChopperOpenings(self, ws, directWS):
        """Correct reflectivity values if chopper openings between RB and DB differ."""
        def opening(instrumentName, logs, Xs):
            chopperGap = common.chopperPairDistance(logs, instrumentName)
            chopperPeriod = 60. / common.chopperSpeed(logs, instrumentName)
            openingAngle = common.chopperOpeningAngle(logs, instrumentName)
            return chopperGap * constants.m_n / constants.h / chopperPeriod * Xs + openingAngle / 360.
        instrumentName = ws.getInstrument().getName()
        Xs = ws.readX(0)
        if ws.isHistogramData():
            Xs = (Xs[:-1] + Xs[1:]) / 2.
        reflectedOpening = opening(instrumentName, ws.run(), Xs)
        directOpening = opening(instrumentName, directWS.run(), Xs)
        corFactorWSName = self._names.withSuffix('chopper_opening_correction_factors')
        corFactorWS = CreateWorkspace(
            OutputWorkspace=corFactorWSName,
            DataX=ws.readX(0),
            DataY=reflectedOpening / directOpening,
            UnitX=ws.getAxis(0).getUnit().unitID(),
            ParentWorkspace=ws,
            EnableLogging=self._subalgLogging)
        correctedWSName = self._names.withSuffix('corrected_by_chopper_opening')
        correctedWS = Multiply(
            LHSWorkspace=ws,
            RHSWorkspace=corFactorWS,
            OutputWorkspace=correctedWSName,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(corFactorWS)
        self._cleanup.cleanup(ws)
        return correctedWS

    def _divideByDirect(self, ws):
        "Divide ws by the direct beam."
        if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            return ws
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        rebinnedWSName = self._names.withSuffix('rebinned')
        rebinnedWS = RebinToWorkspace(WorkspaceToRebin=ws,
                                      WorkspaceToMatch=directWS,
                                      OutputWorkspace=rebinnedWSName,
                                      EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        reflectivityWSName = self._names.withSuffix('reflectivity')
        reflectivityWS = Divide(LHSWorkspace=rebinnedWS,
                                RHSWorkspace=directWS,
                                OutputWorkspace=reflectivityWSName,
                                EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(rebinnedWS)
        reflectivityWS = self._correctForChopperOpenings(reflectivityWS, directWS)
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
        "Return the input workspace."
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        return ws

    def _sumForegroundInLambda(self, ws):
        """Sum the foreground region into a single histogram."""
        foreground = self._foregroundIndices(ws)
        sumIndices = [i for i in range(foreground[0], foreground[2] + 1)]
        beamPosIndex = foreground[1]
        foregroundWSName = self._names.withSuffix('foreground_grouped')
        foregroundWS = ExtractSingleSpectrum(InputWorkspace=ws,
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
            ys = ws.readY(i)
            foregroundYs += ys
            es = ws.readE(i)
            foregroundEs += es**2
        numpy.sqrt(foregroundEs, out=foregroundEs)
        self._cleanup.cleanup(ws)
        return foregroundWS

    def _sumForegroundInQ(self, ws):
        """Sum the foreground region into a single histogram using the coherent method."""
        foreground = self._foregroundIndices(ws)
        sumIndices = [i for i in range(foreground[0], foreground[2] + 1)]
        beamPosIndex = foreground[1]
        isFlatSample = self._checkIfFlatSample()
        sumWSName = self._names.withSuffix('summed_in_Q')
        sumWS = ReflectometrySumInQ(
            InputWorkspace=ws,
            OutputWorkspace=sumWSName,
            InputWorkspaceIndexSet=sumIndices,
            BeamCentre=beamPosIndex,
            FlatSample=isFlatSample,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return sumWS

    def _sumType(self):
        return self.getProperty(Prop.SUM_TYPE).value


AlgorithmFactory.subscribe(ReflectometryILLSumForeground)
