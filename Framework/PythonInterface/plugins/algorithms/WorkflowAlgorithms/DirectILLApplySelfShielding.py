# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, MatrixWorkspaceProperty,
                        Progress, PropertyMode, WorkspaceGroupProperty, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatBoundedValidator, StringListValidator)
from mantid.simpleapi import (ApplyPaalmanPingsCorrection, ConvertUnits, CreateSingleValuedWorkspace, Minus, Multiply)


def _applySelfShieldingCorrections(ws, ecWS, ecScaling, correctionsWS, wsNames,
                                   algorithmLogging):
    """Apply a self-shielding corrections workspace."""
    correctedWSName = wsNames.withSuffix('self_shielding_applied')
    correctedWS = ApplyPaalmanPingsCorrection(
        SampleWorkspace=ws,
        CorrectionsWorkspace=correctionsWS,
        CanWorkspace=ecWS,
        CanScaleFactor=ecScaling,
        OutputWorkspace=correctedWSName,
        RebinCanToSample=False,
        EnableLogging=algorithmLogging)
    return correctedWS


def _applySelfShieldingCorrectionsNoEC(ws, correctionsWS, wsNames,
                                       algorithmLogging):
    """Apply a self-shielding corrections workspace without subtracting an empty container."""
    correctedWSName = wsNames.withSuffix('self_shielding_applied')
    correctedWS = ApplyPaalmanPingsCorrection(
        SampleWorkspace=ws,
        CorrectionsWorkspace=correctionsWS,
        OutputWorkspace=correctedWSName,
        RebinCanToSample=False,
        EnableLogging=algorithmLogging)
    return correctedWS


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
        return 'Workflow\\Inelastic'

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
        progress = Progress(self, 0.0, 1.0, 3)
        subalgLogging = False
        if self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON:
            subalgLogging = True
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        progress.report('Loading inputs')
        mainWS = self._inputWS(wsCleanup)
        ecWS = self.getProperty(common.PROP_EC_WS).value
        selfShieldingWS = self.getProperty(common.PROP_SELF_SHIELDING_CORRECTION_WS).value
        if ecWS and not selfShieldingWS:
            progress.setNumSteps(3)
            progress.report('Subtracting container')
            mainWS = self._subtractEC(mainWS, ecWS, wsNames, wsCleanup, subalgLogging)
            self._finalize(mainWS, wsCleanup)
            progress.report('Done')
            return
        # With Paalman-Pings corrections.
        progress.report('Applying corrections')
        wavelengthWSName = wsNames.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(InputWorkspace=mainWS,
                                    OutputWorkspace=wavelengthWSName,
                                    Target='Wavelength',
                                    EMode='Direct',
                                    EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        if ecWS:
            ecScaling = self.getProperty(common.PROP_EC_SCALING).value
            wavelengthECWSName = wsNames.withSuffix('ec_in_wavelength')
            wavelengthECWS = \
                ConvertUnits(InputWorkspace=ecWS,
                             OutputWorkspace=wavelengthECWSName,
                             Target='Wavelength',
                             EMode='Direct',
                             EnableLogging=subalgLogging)
            correctedWS = _applySelfShieldingCorrections(wavelengthWS, wavelengthECWS, ecScaling, selfShieldingWS, wsNames, subalgLogging)
            wsCleanup.cleanup(wavelengthECWS)
        else:  # No ecWS
            correctedWS = _applySelfShieldingCorrectionsNoEC(wavelengthWS, selfShieldingWS, wsNames, subalgLogging)
        wsCleanup.cleanup(wavelengthWS)
        correctedWS = ConvertUnits(InputWorkspace=correctedWS,
                                   Target='TOF',
                                   EMode='Direct',
                                   EnableLogging=subalgLogging)
        self._finalize(correctedWS, wsCleanup)
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
            doc='Input workspace.')
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
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_EC_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Reduced empty container workspace.')
        self.declareProperty(name=common.PROP_EC_SCALING,
                             defaultValue=1.0,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Scaling factor (transmission, if no self ' +
                                 'shielding is applied) for empty container.')
        self.declareProperty(WorkspaceGroupProperty(
            name=common.PROP_SELF_SHIELDING_CORRECTION_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='A workspace containing self shielding correction factors.')

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

    def _subtractEC(self, mainWS, ecInWS, wsNames, wsCleanup, subalgLogging):
        """Subtract the empty container workspace without self-shielding corrections."""
        ecScaling = self.getProperty(common.PROP_EC_SCALING).value
        ecSubtractedWS = _subtractEC(mainWS, ecInWS, ecScaling, wsNames, wsCleanup, subalgLogging)
        wsCleanup.cleanup(mainWS)
        return ecSubtractedWS


AlgorithmFactory.subscribe(DirectILLApplySelfShielding)
