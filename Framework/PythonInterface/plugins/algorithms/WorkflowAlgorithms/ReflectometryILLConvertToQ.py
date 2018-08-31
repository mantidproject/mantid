# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, WorkspaceUnitValidator)
from mantid.kernel import (Direction, FloatBoundedValidator, Property, StringListValidator)
from mantid.simpleapi import (ConvertToPointData, CreateWorkspace, Divide, ReflectometryMomentumTransfer)
import numpy
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_WS = 'DirectBeamWorkspace'
    DIRECT_FOREGROUND_WS = 'DirectForegroundWorkspace'
    GROUPING_FRACTION = 'GroupingQFraction'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    REFLECTED_WS = 'ReflectedBeamWorkspace'
    POLARIZED = 'Polarized'
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
        return ['ReflectometryILLPolarizationCor', 'ReflectometryILLPreprocess', 'ReflectometryILLSumForeground']

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

        ws, directWS = self._inputWS()

        ws = self._correctForChopperOpenings(ws, directWS)
        ws = self._convertToMomentumTransfer(ws)
        if directWS is not None:
            directWS = self._sameQAndDQ(ws, directWS, 'direct_')
        ws = self._toPointData(ws)
        ws = self._groupPoints(ws)

        if directWS is not None:
            directWS = self._toPointData(directWS, 'direct_')
            directWS = self._groupPoints(directWS, 'direct_')
            ws = self._divideByDirect(ws, directWS)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        positiveFloat = FloatBoundedValidator(lower=0., exclusive=True)
        self.declareProperty(MatrixWorkspaceProperty(Prop.INPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='A reflectivity workspace in wavelength to be converted to Q.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='The input workspace in momentum transfer.')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disable child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=common.WSCleanup.ON,
                             validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.REFLECTED_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='A non-summed reflected beam workspace, needed for Q resolution calculation.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.DIRECT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='A non-summed direct beam workspace, needed for Q resolution calculation.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.DIRECT_FOREGROUND_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional,
                                                     validator=WorkspaceUnitValidator('Wavelength')),
                             doc='Summed direct beam workspace if output in reflectivity is required.')
        self.declareProperty(Prop.POLARIZED,
                             defaultValue=False,
                             doc='True if input workspace has been corrected for polarization efficiencies.')
        self.declareProperty(Prop.GROUPING_FRACTION,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             doc='If set, group the output by steps of this fraction multiplied by Q resolution')

    def validateInputs(self):
        """Validate the input properties."""
        issues = dict()
        inputWS = self.getProperty(Prop.INPUT_WS).value
        if inputWS.getNumberHistograms() != 1:
            issues[Prop.INPUT_WS] = 'The workspace should have only a single histogram. Was foreground summation forgotten?'
        if not self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            directWS = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
            if directWS.getNumberHistograms() != 1:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'The workspace should have only a single histogram. Was foreground summation forgotten?'
            if directWS.blocksize() != inputWS.blocksize():
                issues[Prop.DIRECT_FOREGROUND_WS] = 'Number of bins does not match with InputWorkspace.'
            directXs = directWS.readX(0)
            inputXs = inputWS.readX(0)
            if directXs[0] != inputXs[0] or directXs[-1] != inputXs[-1]:
                issues[Prop.DIRECT_FOREGROUND_WS] = 'Binning does not match with InputWorkspace.'
        return issues

    def _convertToMomentumTransfer(self, ws):
        """Convert the X units of ws to momentum transfer."""
        reflectedWS = self.getProperty(Prop.REFLECTED_WS).value
        reflectedForeground = self._foreground(reflectedWS.run())
        directWS = self.getProperty(Prop.DIRECT_WS).value
        directForeground = self._foreground(directWS.run())
        logs = ws.run()
        instrumentName = ws.getInstrument().getName()
        if instrumentName != 'D17' and instrumentName != 'FIGARO':
            raise RuntimeError('Unrecognized instrument {}. Only D17 and FIGARO are supported.'.format(instrumentName))
        sumType = logs.getProperty(common.SampleLogs.SUM_TYPE).value
        polarized = self.getProperty(Prop.POLARIZED).value
        pixelSize = 0.001195 if instrumentName == 'D17' else 0.0012
        detResolution = common.detectorResolution()
        chopperSpeed = common.chopperSpeed(logs, instrumentName)
        chopperOpening = common.chopperOpeningAngle(logs, instrumentName)
        chopperRadius = 0.36 if instrumentName == 'D17' else 0.305
        chopperPairDist = common.chopperPairDistance(logs, instrumentName)
        slit1SizeLog = common.slitSizeLogEntry(instrumentName, 1)
        slit2SizeLog = common.slitSizeLogEntry(instrumentName, 2)
        tofBinWidth = self._TOFChannelWidth(logs, instrumentName)
        qWSName = self._names.withSuffix('in_momentum_transfer')
        qWS = ReflectometryMomentumTransfer(
            InputWorkspace=ws,
            OutputWorkspace=qWSName,
            ReflectedBeamWorkspace=reflectedWS,
            ReflectedForeground=reflectedForeground,
            DirectBeamWorkspace=directWS,
            DirectForeground=directForeground,
            SummationType=sumType,
            Polarized=polarized,
            PixelSize=pixelSize,
            DetectorResolution=detResolution,
            ChopperSpeed=chopperSpeed,
            ChopperOpening=chopperOpening,
            ChopperRadius=chopperRadius,
            ChopperPairDistance=chopperPairDist,
            Slit1Name='slit2',
            Slit1SizeSampleLog=slit1SizeLog,
            Slit2Name='slit3',
            Slit2SizeSampleLog=slit2SizeLog,
            TOFChannelWidth=tofBinWidth,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return qWS

    def _correctForChopperOpenings(self, ws, directWS):
        """Corrects ws for different chopper opening angles."""
        correctedWS = common.correctForChopperOpenings(ws, directWS, self._names, self._cleanup, self._subalgLogging)
        return correctedWS

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _divideByDirect(self, ws, directWS):
        """Divide ws by the direct beam."""
        reflectivityWSName = self._names.withSuffix('reflectivity')
        reflectivityWS = Divide(LHSWorkspace=ws,
                                RHSWorkspace=directWS,
                                OutputWorkspace=reflectivityWSName,
                                EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        self._cleanup.cleanup(directWS)
        reflectivityWS.setYUnit('Reflectivity')
        reflectivityWS.setYUnitLabel('Reflectivity')
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
        xs = ws.readX(0)
        ys = ws.readY(0)
        es = ws.readE(0)
        dxs = ws.readDx(0)
        if numpy.any(dxs <= 0.):
            raise RuntimeError('Cannot proceed: the momentum transfer workspace contains nonpositive Q resolutions.')
        index = 0
        start = xs[index]
        groupedXs = list()
        groupedYs = list()
        groupedEs = list()
        groupedDxs = list()

        while True:
            width = qFraction * dxs[index]
            end = xs[index] + width
            pick = numpy.logical_and(xs >= start, xs < end)
            pickedXs = xs[pick]
            if len(pickedXs) > 0:
                groupedXs.append(numpy.mean(pickedXs))
                groupedYs.append(numpy.mean(ys[pick]))
                pickedEs = es[pick]
                groupedEs.append(numpy.sqrt(numpy.dot(pickedEs, pickedEs)) / len(pickedEs))
                groupWidth = pickedXs[-1] - pickedXs[0]
                groupedDxs.append(numpy.sqrt(dxs[index]**2 + (0.68 * groupWidth)**2))
            start = end
            if start > xs[-1]:
                break
            index = numpy.nonzero(xs > start)[0][0]
        groupedWSName = self._names.withSuffix(extraLabel + 'grouped')
        groupedWS = CreateWorkspace(
            OutputWorkspace=groupedWSName,
            DataX=groupedXs,
            DataY=groupedYs,
            DataE=groupedEs,
            Dx=groupedDxs,
            UnitX=ws.getAxis(0).getUnit().unitID(),
            ParentWorkspace=ws,
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return groupedWS

    def _inputWS(self):
        """Return the input workspace."""
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        directWS = None
        if not self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
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

    def _TOFChannelWidth(self, sampleLogs, instrumentName):
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
