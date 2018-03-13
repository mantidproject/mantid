# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (Direction, StringListValidator)
from mantid.simpleapi import (ConvertUnits)
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_WS = 'DirectBeamWorkspace'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    REFLECTED_WS = 'ReflectedBeamWorkspace'
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

    def _convertToMomentumTransfer(self, ws):
        """Convert the X units of ws to momentum transfer."""
        qWSName = self._names.withSuffix('in_momentum_transfer')
        qWS = ConvertUnits(InputWorkspace=ws,
                           OutputWorkspace=qWSName,
                           Target='MomentumTransfer',
                           EMode='Elastic',
                           EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return qWS

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _inputWS(self):
        "Return the input workspace."
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        return ws


AlgorithmFactory.subscribe(ReflectometryILLConvertToQ)
