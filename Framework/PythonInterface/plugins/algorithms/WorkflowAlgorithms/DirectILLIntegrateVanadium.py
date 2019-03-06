# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
import ILL_utilities as utils
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, Progress, PropertyMode, WorkspaceProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty, FloatBoundedValidator, Property,
                           PropertyCriterion, StringListValidator)
from mantid.simpleapi import (ComputeCalibrationCoefVan, MaskDetectorsIf)


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
        progress = Progress(self, 0.0, 1.0, 4)
        self._subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)

        progress.report('Loading inputs')
        mainWS = self._inputWS()

        progress.report('Integrating')
        mainWS = self._integrate(mainWS)

        progress.report('Masking zeros')
        mainWS = self._maskZeros(mainWS)

        self._finalize(mainWS)
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
                             defaultValue=utils.Cleanup.ON,
                             validator=StringListValidator([
                                 utils.Cleanup.ON,
                                 utils.Cleanup.OFF]),
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

    def _inputWS(self):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        self._cleanup.protect(mainWS)
        return mainWS

    def _finalize(self, outWS):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        self._cleanup.cleanup(outWS)
        self._cleanup.finalCleanup()

    def _integrate(self, mainWS):
        """Integrate mainWS applying Debye-Waller correction, if requested."""
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        calibrationWS = self.getProperty(common.PROP_OUTPUT_WS).value
        dwfEnabled = self.getProperty(common.PROP_DWF_CORRECTION).value == common.DWF_ON
        temperature = self.getProperty(common.PROP_TEMPERATURE).value
        calibrationWS = ComputeCalibrationCoefVan(VanadiumWorkspace=mainWS,
                                                  EPPTable=eppWS,
                                                  OutputWorkspace=calibrationWS,
                                                  Temperature=temperature,
                                                  EnableDWF=dwfEnabled,
                                                  EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(mainWS)
        return calibrationWS

    def _maskZeros(self, mainWS):
        """Mask zero integrals in mainWS."""
        mainWS = MaskDetectorsIf(InputWorkspace=mainWS,
                                 OutputWorkspace=mainWS,
                                 Mode='SelectIf',
                                 Operator='Equal',
                                 Value=0.,
                                 EnableLogging=self._subalgLogging)
        return mainWS


AlgorithmFactory.subscribe(DirectILLIntegrateVanadium)
