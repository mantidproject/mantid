# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import ILL_utilities as utils
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (Direction, FloatBoundedValidator, Property, StringListValidator)
from mantid.simpleapi import (ConvertToPointData, CreateWorkspace, Divide, GroupToXResolution, Multiply,
                              ReflectometryMomentumTransfer)
import ReflectometryILL_common as common
import scipy.constants as constants


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_FOREGROUND_WS = 'DirectForegroundWorkspace'
    GROUPING_FRACTION = 'GroupingQFraction'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    SUBALG_LOGGING = 'SubalgorithmLogging'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class ReflectometryILLConvertToQ(DataProcessorAlgorithm):

    def category(self):
        """Return algorithm's categories."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLConvertToQ'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Converts a reflectivity workspace from wavelength to momentum transfer.'

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryILLPolarizationCor', 'ReflectometryILLPreprocess', 'ReflectometryILLSumForeground',
                'ReflectometryMomentumTransfer']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(wsPrefix, cleanupMode)

        ws, directWS = self._inputWS()

        ws = self._correctForChopperOpenings(ws, directWS)
        ws = self._convertToMomentumTransfer(ws)

        sumInLambda = self._sumType(ws.run()) == 'SumInLambda'
        if sumInLambda:
            directWS = self._sameQAndDQ(ws, directWS, 'direct_')
        ws = self._toPointData(ws)
        ws = self._groupPoints(ws)

        if sumInLambda:
            directWS = self._toPointData(directWS, 'direct_')
            directWS = self._groupPoints(directWS, 'direct_')
            ws = self._divideByDirect(ws, directWS)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        positiveFloat = FloatBoundedValidator(lower=0., exclusive=True)
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.INPUT_WS,
                defaultValue='',
                direction=Direction.Input,
                validator=WorkspaceUnitValidator('Wavelength')),
            doc='A reflectivity workspace in wavelength to be converted to Q.')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output),
            doc='The input workspace in momentum transfer.')
        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc='Enable or disable child algorithm logging.')
        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=utils.Cleanup.ON,
            validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
            doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.DIRECT_FOREGROUND_WS,
                defaultValue='',
                direction=Direction.Input,
                validator=WorkspaceUnitValidator('Wavelength')),
            doc='Summed direct beam workspace.')
        self.declareProperty(
            Prop.GROUPING_FRACTION,
            defaultValue=Property.EMPTY_DBL,
            validator=positiveFloat,
            doc='If set, group the output by steps of this fraction multiplied by Q resolution')

    def validateInputs(self):
        """Validate the input properties."""
        issues = dict()
        inputWS = self.getProperty(Prop.INPUT_WS).value
        if inputWS.getNumberHistograms() != 1:
            issues[Prop.INPUT_WS] = 'The workspace should have only a single histogram. Was foreground summation forgotten?'
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        if directWS.getNumberHistograms() != 1:
            issues[Prop.DIRECT_FOREGROUND_WS] = 'The workspace should have only a single histogram. Was foreground summation forgotten?'
        run = inputWS.run()
        if not run.hasProperty(common.SampleLogs.SUM_TYPE):
            issues[Prop.INPUT_WS] = "'" + common.SampleLogs.SUM_TYPE + "' entry missing in sample logs"
        else:
            sumType = run.getProperty(common.SampleLogs.SUM_TYPE).value
            if sumType not in ['SumInLambda', 'SumInQ']:
                issues[Prop.INPUT_WS] = "Unknown sum type in sample logs: '" + sumType + "'. Allowed values: 'SumInLambda' or 'SumInQ'."
            else:
                if sumType == 'SumInLambda':
                    if directWS.blocksize() != inputWS.blocksize():
                        issues[Prop.DIRECT_FOREGROUND_WS] = 'Number of bins does not match with InputWorkspace.'
                    directXs = directWS.readX(0)
                    inputXs = inputWS.readX(0)
                    if directXs[0] != inputXs[0] or directXs[-1] != inputXs[-1]:
                        issues[Prop.DIRECT_FOREGROUND_WS] = 'Binning does not match with InputWorkspace.'
        return issues

    def _convertToMomentumTransfer(self, ws):
        """Convert the X units of ws to momentum transfer."""
        logs = ws.run()
        reflectedForeground = self._foreground(logs)
        instrumentName = common.instrumentName(ws)
        sumType = logs.getProperty(common.SampleLogs.SUM_TYPE).value
        pixelSize = common.pixelSize(instrumentName)
        detResolution = common.detectorResolution()
        chopperSpeed = common.chopperSpeed(logs, instrumentName)
        chopperOpening = common.chopperOpeningAngle(logs, instrumentName)
        chopperRadius = 0.36 if instrumentName == 'D17' else 0.305
        chopperPairDist = common.chopperPairDistance(logs, instrumentName)
        tofBinWidth = self._TOFChannelWidth(logs)
        qWSName = self._names.withSuffix('in_momentum_transfer')
        qWS = ReflectometryMomentumTransfer(
            InputWorkspace=ws,
            OutputWorkspace=qWSName,
            SummationType=sumType,
            ReflectedForeground=reflectedForeground,
            PixelSize=pixelSize,
            DetectorResolution=detResolution,
            ChopperSpeed=chopperSpeed,
            ChopperOpening=chopperOpening,
            ChopperRadius=chopperRadius,
            ChopperPairDistance=chopperPairDist,
            FirstSlitName='slit2',
            FirstSlitSizeSampleLog=common.SampleLogs.SLIT2WIDTH,
            SecondSlitName='slit3',
            SecondSlitSizeSampleLog=common.SampleLogs.SLIT3WIDTH,
            TOFChannelWidth=tofBinWidth,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return qWS

    def _correctForChopperOpenings(self, ws, directWS):
        """Correct reflectivity values if chopper openings between RB and DB differ."""

        def opening(instrumentName, logs, Xs):
            chopperGap = common.chopperPairDistance(logs, instrumentName)
            chopperPeriod = 60. / common.chopperSpeed(logs, instrumentName)
            openingAngle = common.chopperOpeningAngle(logs, instrumentName)
            return chopperGap * constants.m_n / constants.h / chopperPeriod * Xs * 1e-10 + openingAngle / 360.

        instrumentName = common.instrumentName(ws)
        Xbins = ws.readX(0)
        Xs = (Xbins[:-1] + Xbins[1:]) / 2.
        reflectedOpening = opening(instrumentName, ws.run(), Xs)
        directOpening = opening(instrumentName, directWS.run(), Xs)
        corFactorWSName = self._names.withSuffix('chopper_opening_correction_factors')
        corFactorWS = CreateWorkspace(
            OutputWorkspace=corFactorWSName,
            DataX=Xbins,
            DataY=directOpening / reflectedOpening,
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

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _divideByDirect(self, ws, directWS):
        """Divide ws by the direct beam."""
        reflectivityWSName = self._names.withSuffix('reflectivity')
        reflectivityWS = Divide(
            LHSWorkspace=ws,
            RHSWorkspace=directWS,
            OutputWorkspace=reflectivityWSName,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(directWS)
        reflectivityWS.setYUnit('Reflectivity')
        reflectivityWS.setYUnitLabel('Reflectivity')
        # The X error data is lost in Divide.
        reflectivityWS.setDx(0, ws.readDx(0))
        self._cleanup.cleanup(ws)
        return reflectivityWS

    def _foreground(self, sampleLogs):
        """Return a [start, end] list defining the foreground workspace indices."""
        start = sampleLogs.getProperty(common.SampleLogs.FOREGROUND_START).value
        end = sampleLogs.getProperty(common.SampleLogs.FOREGROUND_END).value
        return [start, end]

    def _groupPoints(self, ws, extraLabel=''):
        """Group bins by Q resolution."""
        if self.getProperty(Prop.GROUPING_FRACTION).isDefault:
            return ws
        qFraction = self.getProperty(Prop.GROUPING_FRACTION).value
        groupedWSName = self._names.withSuffix(extraLabel + 'grouped')
        groupedWS = GroupToXResolution(
            InputWorkspace=ws,
            OutputWorkspace=groupedWSName,
            FractionOfDx=qFraction,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return groupedWS

    def _inputWS(self):
        """Return the input workspace."""
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        self._cleanup.protect(directWS)
        return ws, directWS

    def _sameQAndDQ(self, ws, directWS, extraLabel=''):
        """Create a new workspace with Y and E from directWS and X and DX data from ws."""
        qWSName = self._names.withSuffix(extraLabel + 'in_momentum_transfer')
        qWS = CreateWorkspace(
            OutputWorkspace=qWSName,
            DataX=ws.readX(0),
            DataY=directWS.readY(0)[::-1],  # Invert data because wavelength is inversely proportional to Q.
            DataE=directWS.readE(0)[::-1],
            Dx=ws.readDx(0),
            UnitX=ws.getAxis(0).getUnit().unitID(),
            ParentWorkspace=directWS,
            EnableLogging=self._subalgLogging)
        return qWS

    def _sumType(self, logs):
        """Return the sum type applied to ws."""
        return logs.getProperty(common.SampleLogs.SUM_TYPE).value

    def _TOFChannelWidth(self, sampleLogs):
        """Return the time of flight bin width."""
        return sampleLogs.getProperty('PSD.time_of_flight_0').value

    def _toPointData(self, ws, extraLabel=''):
        """Convert ws from binned to point data."""
        pointWSName = self._names.withSuffix(extraLabel + 'as_points')
        pointWS = ConvertToPointData(
            InputWorkspace=ws,
            OutputWorkspace=pointWSName,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return pointWS


AlgorithmFactory.subscribe(ReflectometryILLConvertToQ)
