# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, Progress, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty, FloatBoundedValidator, Property,
                           PropertyCriterion, StringListValidator)
from mantid.simpleapi import (ComputeCalibrationCoefVan, Integration)
import numpy


class DirectILLIntegrateVanadium(DataProcessorAlgorithm):
    """A workflow algorithm which integrates the vanadium data."""

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
        return 'DirectILLIntegrateVanadium'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Integrate vanadium workspace. Part of the TOF workflow at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        progress = Progress(self, 0.0, 1.0, 3)
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        progress.report('Loading inputs')
        mainWS = self._inputWS(wsCleanup)

        progress.report('Integrating')
        mainWS = self._integrate(mainWS, wsCleanup, subalgLogging)

        self._finalize(mainWS, wsCleanup)
        progress.report('Done')

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
            doc='A workspace to be integrated.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The integrated workspace.')
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
        self.declareProperty(name=common.PROP_DWF_CORRECTION,
                             defaultValue=common.DWF_ON,
                             validator=StringListValidator([
                                 common.DWF_ON,
                                 common.DWF_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable the correction for the Debye-Waller factor for ' + common.PROP_OUTPUT_WS + '.')
        self.declareProperty(name=common.PROP_TEMPERATURE,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Vanadium temperature in Kelvin for Debye-Waller correction, ' +
                                 'overrides the default value from the sample logs.')
        self.setPropertySettings(common.PROP_TEMPERATURE, EnabledWhenProperty(common.PROP_DWF_CORRECTION,
                                                                              PropertyCriterion.IsDefault))

    def _inputWS(self, wsCleanup):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()

    def _integrate(self, mainWS, wsCleanup, subalgLogging):
        """Integrate mainWS applying Debye-Waller correction, if requested."""
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        calibrationWS = self.getPropertyValue(common.PROP_OUTPUT_WS)
        if self.getProperty(common.PROP_DWF_CORRECTION).value == common.DWF_ON:
            if not self.getProperty(common.PROP_TEMPERATURE).isDefault:
                temperature = self.getProperty(common.PROP_TEMPERATURE).value
            else:
                temperature = 293.0
                ILL_TEMPERATURE_ENTRY = 'sample.temperature'
                if mainWS.run().hasProperty(ILL_TEMPERATURE_ENTRY):
                    temperatureProperty = mainWS.run().getProperty(ILL_TEMPERATURE_ENTRY)
                    if hasattr(temperatureProperty, 'getStatistics'):
                        temperature = temperatureProperty.getStatistics().mean
                    else:
                        temperature = temperatureProperty.value
            calibrationWS = ComputeCalibrationCoefVan(VanadiumWorkspace=mainWS,
                                                      EPPTable=eppWS,
                                                      OutputWorkspace=calibrationWS,
                                                      Temperature=temperature,
                                                      EnableLogging=subalgLogging)
            wsCleanup.cleanup(mainWS)
            return calibrationWS
        # No DWF correction - integrate manually.
        # TODO revise when ComputeCalibrationCoefVan supports this option.
        size = eppWS.rowCount()
        starts = numpy.zeros(size)
        ends = numpy.zeros(size)
        for i in range(size):
            row = eppWS.row(i)
            if row['FitStatus'] == 'success':
                fwhm = 2.0 * numpy.sqrt(2.0 * numpy.log(2.0)) * row['Sigma']
                centre = row['PeakCentre']
                starts[i] = centre - 3.0 * fwhm
                ends[i] = centre + 3.0 * fwhm
        calibrationWS = Integration(InputWorkspace=mainWS,
                                    OutputWorkspace=calibrationWS,
                                    RangeLowerList=starts,
                                    RangeUpperList=ends,
                                    EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return calibrationWS


AlgorithmFactory.subscribe(DirectILLIntegrateVanadium)
