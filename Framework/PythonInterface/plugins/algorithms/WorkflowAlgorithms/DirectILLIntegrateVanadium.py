# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import CompositeValidator, Direction, FloatBoundedValidator, Property, StringListValidator
from mantid.simpleapi import ComputeCalibrationCoefVan

class DirectILLIntegrateVanadium(DataProcessorAlgorithm):
    """A workflow algorithm which integrates the vanadium data."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLIntegrateVanadium'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Integrate vanadium workspace. Part of the TOF workflow at ILL.'

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

        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        if not self.getProperty(common.PROP_TEMPERATURE).isDefault:
            temperature = self.getProperty(common.PROP_TEMPERATURE).value
        else:
            temperature = 293.0
            MLZ_TEMPERATURE_ENTRY = 'temperature'
            ILL_TEMPERATURE_ENTRY = 'sample.temperature'
            if mainWS.run().hasProperty(MLZ_TEMPERATURE_ENTRY):
                temperature = mainWS.run().getProperty(MLZ_TEMPERATURE_ENTRY).value
            elif mainWS.run().hasProperty(ILL_TEMPERATURE_ENTRY):
                temperature = mainWS.run().getProperty(ILL_TEMPERATURE_ENTRY).value
        calibrationWS = self.getPropertyValue(common.PROP_OUTPUT_WS)
        calibrationWS = ComputeCalibrationCoefVan(VanadiumWorkspace=mainWS,
                                                  EPPTable=eppWS,
                                                  OutputWorkspace=calibrationWS,
                                                  Temperature=temperature,
                                                  EnableLogging=subalgLogging)
        self.setProperty(common.PROP_OUTPUT_WS, calibrationWS)

    def PyInit(self):
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)

        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Mandatory,
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
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Mandatory),
            doc='Table workspace containing results from the FindEPP algorithm.')
        self.declareProperty(name=common.PROP_TEMPERATURE,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Experimental temperature (Vanadium ' +
                                 'reduction type only), in Kelvins.')

AlgorithmFactory.subscribe(DirectILLIntegrateVanadium)
