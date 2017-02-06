# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILLReduction_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, MatrixWorkspaceProperty,
                        PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, StringListValidator)
from mantid.simpleapi import CloneWorkspace, MaskDetectors


class DirectILLApplyDiagnostics(DataProcessorAlgorithm):
    """A workflow algorithm which applies diagnostics perfomed by DirectILLDetectorDiagnostics."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLApplyDiagnostics'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Apply detector diagnostics to a workspace. Part of the TOF workflow at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        mainWS = self._inputWS(wsCleanup)
        maskedWSName = wsNames.withSuffix('diagnostics_applied')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        MaskDetectors(Workspace=maskedWS,
                      MaskedWorkspace=diagnosticsWS,
                      EnableLogging=subalgLogging)
        self.setProperty(common.PROP_OUTPUT_WS, maskedWS)

    def PyInit(self):
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))

        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Mandatory),
            doc='Detector diagnostics workspace obtained from another ' +
                'reduction run.')
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

    def _inputWS(self, wsCleanup):
        """Return the input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS


AlgorithmFactory.subscribe(DirectILLApplyDiagnostics)
