# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode)
from mantid.kernel import (Direction, StringListValidator)
from mantid.simpleapi import (ConvertUnits, Divide, RebinToWorkspace)
import ReflectometryILL_common as common


class Prop:
    CLEANUP = 'Cleanup'
    DIRECT_BEAM_WS = 'DirectBeamWorkspace'
    INPUT_WS = 'InputWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    SUBALG_LOGGING = 'SubalgorithmLogging'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class ReflectometryILLReduction(DataProcessorAlgorithm):

    def category(self):
        """Return algorithm's categories."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLReduction'

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

        ws = self._reflectivity(ws)

        ws = self._convertToMomentumTransfer(ws)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        self.declareProperty(MatrixWorkspaceProperty(Prop.INPUT_WS, defaultValue='',
                                                     direction=Direction.Input),
                             doc='A prepared input workspace.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS, defaultValue='',
                                                     direction=Direction.Output),
                             doc='The reduced output workspace')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disalbe child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=common.WSCleanup.ON,
                             validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.DIRECT_BEAM_WS, defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A beam position table from a direct beam measurement.')

    def validateInputs(self):
        """Return a dictionary mapping invalid properties to error messages."""
        issues = dict()
        ws = self.getProperty(Prop.INPUT_WS).value
        if ws.getNumberHistograms() != 1:
            issues[Prop.INPUT_WS] = 'The workspace should contain only a single histogram.'
        if not self.getProperty(Prop.DIRECT_BEAM_WS).isDefault:
            ws = self.getProperty(Prop.DIRECT_BEAM_WS).value
            if ws.getNumberHistograms() != 1:
                issues[Prop.DIRECT_BEAM_WS] = 'The workspace should contain only a single histogram.'
        return issues

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

    def _reflectivity(self, ws):
        "Divide ws by the direct beam."
        if self.getProperty(Prop.DIRECT_BEAM_WS).isDefault:
            return ws
        directWS = self.getProperty(Prop.DIRECT_BEAM_WS).value
        rebinnedDirectWSName = self._names.withSuffix('rebinned')
        rebinnedDirectWS = RebinToWorkspace(WorkspaceToRebin = directWS,
                                            WorkspaceToMatch = ws,
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

AlgorithmFactory.subscribe(ReflectometryILLReduction)
