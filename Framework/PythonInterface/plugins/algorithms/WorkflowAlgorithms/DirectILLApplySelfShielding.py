# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, MatrixWorkspaceProperty,
                        Progress, PropertyMode,  WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatBoundedValidator, StringListValidator)
from mantid.simpleapi import (CloneWorkspace, CreateSingleValuedWorkspace, Divide, Minus, Multiply)


def _subtractEC(ws, ecWS, ecScaling, wsNames, wsCleanup, algorithmLogging):
    """Subtract empty container."""
    # out = in - ecScaling * EC
    scalingWSName = wsNames.withSuffix('ecScaling')
    scalingWS = CreateSingleValuedWorkspace(OutputWorkspace=scalingWSName,
                                            DataValue=ecScaling,
                                            EnableLogging=algorithmLogging)
    scaledECWSName = wsNames.withSuffix('scaled_EC')
    scaledECWS = Multiply(LHSWorkspace=ecWS,
                          RHSWorkspace=scalingWS,
                          OutputWorkspace=scaledECWSName,
                          EnableLogging=algorithmLogging)
    ecSubtractedWSName = wsNames.withSuffix('EC_subtracted')
    ecSubtractedWS = Minus(LHSWorkspace=ws,
                           RHSWorkspace=scaledECWS,
                           OutputWorkspace=ecSubtractedWSName,
                           EnableLogging=algorithmLogging)
    wsCleanup.cleanup(scalingWS)
    wsCleanup.cleanup(scaledECWS)
    return ecSubtractedWS


class DirectILLApplySelfShielding(DataProcessorAlgorithm):
    """A data reduction workflow algorithm for the direct geometry TOF spectrometers at ILL."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return [ "DirectILLReduction" ]

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLApplySelfShielding'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Applies empty container subtraction and self-shielding corrections.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        progress = Progress(self, 0.0, 1.0, 4)
        subalgLogging = False
        if self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON:
            subalgLogging = True
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        progress.report('Loading inputs')
        mainWS = self._inputWS(wsCleanup)

        progress.report('Applying self shielding corrections')
        mainWS, applied = self._applyCorrections(mainWS, wsNames, wsCleanup, subalgLogging)

        progress.report('Subtracting EC')
        mainWS, subtracted = self._subtractEC(mainWS, wsNames, wsCleanup, subalgLogging)

        if not applied and not subtracted:
            mainWS = self._cloneOnly(mainWS, wsNames, wsCleanup, subalgLogging)

        self._finalize(mainWS, wsCleanup)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input),
            doc='A workspace to which to apply the corrections.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The corrected workspace.')
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
            name=common.PROP_EC_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='An empty container workspace for subtraction from the input workspace.')
        self.declareProperty(name=common.PROP_EC_SCALING,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='A multiplier (transmission, if no self ' +
                                 'shielding is applied) for the empty container.')
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_SELF_SHIELDING_CORRECTION_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='A workspace containing the self shielding correction factors.')

    def _cloneOnly(self, mainWS, wsNames, wsCleanup, subalgLogging):
        cloneWSName = wsNames.withSuffix('cloned_only')
        cloneWS = CloneWorkspace(InputWorkspace=mainWS,
                                 OutputWorkspace=cloneWSName)
        return cloneWS

    def _applyCorrections(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Applies self shielding corrections to a workspace, if corrections exist."""
        if self.getProperty(common.PROP_SELF_SHIELDING_CORRECTION_WS).isDefault:
            return mainWS, False
        correctionWS = self.getProperty(common.PROP_SELF_SHIELDING_CORRECTION_WS).value
        correctedWSName = wsNames.withSuffix('self_shielding_corrected')
        correctedWS = Divide(LHSWorkspace=mainWS,
                             RHSWorkspace=correctionWS,
                             OutputWorkspace=correctedWSName,
                             EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return correctedWS, True

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()

    def _inputWS(self, wsCleanup):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _subtractEC(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Subtract the empty container workspace without self-shielding corrections."""
        if self.getProperty(common.PROP_EC_WS).isDefault:
            return mainWS, False
        ecWS = self.getProperty(common.PROP_EC_WS).value
        ecScaling = self.getProperty(common.PROP_EC_SCALING).value
        subtractedWS = _subtractEC(mainWS, ecWS, ecScaling, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return subtractedWS, True


AlgorithmFactory.subscribe(DirectILLApplySelfShielding)
