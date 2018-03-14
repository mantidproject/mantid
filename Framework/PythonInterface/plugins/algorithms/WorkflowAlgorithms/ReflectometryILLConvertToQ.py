# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (Direction, StringListValidator)
from mantid.simpleapi import (ConvertFromDistribution, ConvertToDistribution, ReflectometryQResolution)
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_WS = 'DirectBeamWorkspace'
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

        ws = self._convertToMomentumTransfer(ws)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
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
        self.declareProperty(Prop.POLARIZED,
                             defaultValue=False,
                             doc='True if input workspace has been corrected for polarization efficiencies.')

    def _chopperOpening(self, sampleLogs, instrumentName):
        """Return the chopper opening angle."""
        if instrumentName == 'D17':
            chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper1_phase_average').value
            if chopper1Phase > 360.:
                # Workaround for broken old D17 NeXus files.
                chopper1Phase = sampleLogs.getProperty('VirtualChopper.chopper2_speed_average').value
            chopper2Phase = sampleLogs.getProperty('VirtualChopper.chopper2_phase_average').value
            openoffset = sampleLogs.getProperty('VirtualChopper.open_offset').value
            return 45. - (chopper2Phase - chopper1Phase) - openoffset
        else:
            firstChopper = sampleLogs.getProperty('ChopperSettings.firstChopper').value
            secondChopper = sampleLogs.getProperty('ChopperSettings.secondChopper').value
            phase1Entry = 'CH{}.phase'.format(firstChopper)
            phase2Entry = 'CH{}.phase'.format(secondChopper)
            chopper1Phase = sampleLogs.getProperty(phase1Entry).value
            chopper2Phase = sampleLogs.getProperty(phase2Entry).value
            opentoffset = sampleLogs.getProperty('CollAngle.openOffset').value
            return 45. - (chopper2Phase - chopper1Phase) - openoffset

    def _chopperPairDistance(self, sampleLogs, instrumentName):
        """Return the gap between the two choppers."""
        if instrumentName == 'D17':
            return sampleLogs.getProperty('Distance.ChopperGap').value * 1e-2
        else:
            return sampleLogs.getProperty('ChopperSettings.distSeparationChopperPair').value * 1e-2

    def _chopperSpeed(self, sampleLogs, instrumentName):
        """Return the chopper speed."""
        if instrumentName == 'D17':
            return sampleLogs.getProperty('VirtualChopper.chopper1_speed_average').value
        else:
            firstChopper = sampleLogs.getProperty('ChopperSettings.firstChopper').value
            speedEntry = 'CH{}.rotation_speed'.format(firstChopper)
            return sampleLogs.getProperty(speedEntry).value

    def _convertToMomentumTransfer(self, ws):
        """Convert the X units of ws to momentum transfer."""
        reflectedWS = self.getProperty(Prop.REFLECTED_WS).value
        reflectedForeground = self._foreground(reflectedWS.run())
        directWS = self.getProperty(Prop.DIRECT_WS).value
        directForeground = self._foreground(directWS.run())
        logs = ws.run()
        instrumentName = ws.getInstrument().getName()
        if instrumentName != 'D17' and instrumentName != 'Figaro':
            raise RuntimeError('Unrecognized instrument {}. Only D17 and Figaro are supported.'.format(instrumentName))
        sumType = logs.getProperty(common.SampleLogs.SUM_TYPE).value
        polarized = self.getProperty(Prop.POLARIZED).value
        pixelSize = 0.001195 if instrumentName == 'D17' else 0.0012
        detResolution = 0.0022
        chopperSpeed = self._chopperSpeed(logs, instrumentName)
        chopperOpening = self._chopperOpening(logs, instrumentName)
        chopperRadius = 0.36 if instrumentName == 'D17' else 0.305
        chopperPairDist = self._chopperPairDistance(logs, instrumentName)
        slit1SizeLog = 'VirtualSlitAxis.s2w_actual_width' if instrumentName == 'D17' else 'VirtualSlitAxis.S2H_actual_height'
        slit2SizeLog = 'VirtualSlitAxis.s3w_actual_width' if instrumentName == 'D17' else 'VirtualSlitAxis.S3H_actual_height'
        tofBinWidth = self._TOFChannelWidth(logs, instrumentName)
        qWSName = self._names.withSuffix('in_momentum_transfer')
        qWS = ReflectometryQResolution(
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

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _foreground(self, sampleLogs):
        """Return a [start, end] list defining the foreground workspace indices."""
        start = sampleLogs.getProperty(common.SampleLogs.FOREGROUND_START).value
        end = sampleLogs.getProperty(common.SampleLogs.FOREGROUND_END).value
        return [start, end]

    def _inputWS(self):
        "Return the input workspace."
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        return ws

    def _TOFChannelWidth(self, sampleLogs, instrumentName):
        """Return the time of flight bin width."""
        return sampleLogs.getProperty('PSD.time_of_flight_0').value

AlgorithmFactory.subscribe(ReflectometryILLConvertToQ)
