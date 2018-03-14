# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, IntArrayBoundedValidator, IntArrayLengthValidator, IntArrayProperty, 
                           Property, StringListValidator)
from mantid.simpleapi import (AddSampleLog, ConvertToDistribution, Divide, ExtractSingleSpectrum, RebinToWorkspace)
import numpy
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_FOREGROUND_WS = 'DirectForegroundWorkspace'
    FOREGROUND_INDICES = 'Foreground'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    SUM_TYPE = 'SummationType'


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
            ws = self._reflectivity(ws)
        else:
            raise RuntimeError('Summation in Q is not yet supported.')

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        threeNonnegativeInts = CompositeValidator()
        threeNonnegativeInts.add(IntArrayLengthValidator(3))
        nonnegativeInts = IntArrayBoundedValidator()
        nonnegativeInts.setLower(0)
        threeNonnegativeInts.add(nonnegativeInts)
        
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
                             doc='Summed direct beam workspace is output in reflectivity is required.')
        self.declareProperty(IntArrayProperty(Prop.FOREGROUND_INDICES,
                                              values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                                              validator=threeNonnegativeInts),
                             doc='A three element array of foreground start, centre and end workspace indices.')

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

    def _reflectivity(self, ws):
        "Divide ws by the direct beam."
        if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            return ws
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        rebinnedDirectWSName = self._names.withSuffix('rebinned')
        rebinnedDirectWS = RebinToWorkspace(WorkspaceToRebin=directWS,
                                            WorkspaceToMatch=ws,
                                            OutputWorkspace=rebinnedDirectWSName,
                                            EnableLogging=self._subalgLogging)
        reflectivityWSName = self._names.withSuffix('reflectivity')
        reflectivityWS = Divide(LHSWorkspace=ws,
                                RHSWorkspace=rebinnedDirectWS,
                                OutputWorkspace=reflectivityWSName,
                                EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(rebinnedDirectWS)
        self._cleanup.cleanup(ws)
        return reflectivityWS

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
        AddSampleLog(
            Workspace=foregroundWS,
            LogName=common.SampleLogs.SUM_TYPE,
            LogText=SumType.IN_LAMBDA,
            LogType='String',
            EnableLogging=self._subalgLogging)
        ConvertToDistribution(Workspace=foregroundWS,
                              EnableLogging=self._subalgLogging)
        return foregroundWS

    def _sumType(self):
        return self.getProperty(Prop.SUM_TYPE).value


AlgorithmFactory.subscribe(ReflectometryILLSumForeground)
