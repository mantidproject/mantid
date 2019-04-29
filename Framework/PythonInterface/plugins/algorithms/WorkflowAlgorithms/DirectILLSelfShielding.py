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
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        MatrixWorkspaceProperty, PropertyMode, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty, IntBoundedValidator,
                           Property, PropertyCriterion, StringListValidator)
from mantid.simpleapi import (ConvertUnits, MonteCarloAbsorption)


class DirectILLSelfShielding(DataProcessorAlgorithm):
    """A workflow algorithm for self-shielding corrections."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return [ 'DirectILLApplySelfShielding', 'DirectILLReduction' ]

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLSelfShielding'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Calculates self-shielding correction for the direct geometry TOF spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        self._names = utils.NameSource(wsNamePrefix, cleanupMode)
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)

        # Get input workspace.
        mainWS = self._inputWS()

        # Self shielding and empty container subtraction, if requested.
        correctionWS = self._selfShielding(mainWS)

        self._finalize(correctionWS)

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        PROPGROUP_SIMULATION_INSTRUMENT = 'Simulation Instrument Settings'
        greaterThanOneInt = IntBoundedValidator(lower=2)
        greaterThanTwoInt = IntBoundedValidator(lower=3)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='A workspace for which to simulate the self shielding.')
        self.declareProperty(MatrixWorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='A workspace containing the self shielding correction factors.')
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
        self.declareProperty(name=common.PROP_SIMULATION_INSTRUMENT,
                             defaultValue=common.SIMULATION_INSTRUMEN_SPARSE,
                             validator=StringListValidator([
                                 common.SIMULATION_INSTRUMEN_SPARSE,
                                 common.SIMULATION_INSTRUMENT_FULL]),
                             direction=Direction.Input,
                             doc='Select if the simulation should be performed on full or approximated instrument.')
        self.setPropertyGroup(common.PROP_SIMULATION_INSTRUMENT, PROPGROUP_SIMULATION_INSTRUMENT)
        self.declareProperty(name=common.PROP_SPARSE_INSTRUMENT_ROWS,
                             defaultValue=5,
                             validator=greaterThanTwoInt,
                             direction=Direction.Input,
                             doc='Number of detector rows in sparse simulation instrument.')
        self.setPropertyGroup(common.PROP_SPARSE_INSTRUMENT_ROWS, PROPGROUP_SIMULATION_INSTRUMENT)
        self.setPropertySettings(common.PROP_SPARSE_INSTRUMENT_ROWS, EnabledWhenProperty(common.PROP_SIMULATION_INSTRUMENT,
                                 PropertyCriterion.IsEqualTo, common.SIMULATION_INSTRUMEN_SPARSE))
        self.declareProperty(name=common.PROP_SPARSE_INSTRUMENT_COLUMNS,
                             defaultValue=20,
                             validator=greaterThanOneInt,
                             direction=Direction.Input,
                             doc='Number of detector columns in sparse simulation instrument.')
        self.setPropertyGroup(common.PROP_SPARSE_INSTRUMENT_COLUMNS, PROPGROUP_SIMULATION_INSTRUMENT)
        self.setPropertySettings(common.PROP_SPARSE_INSTRUMENT_COLUMNS, EnabledWhenProperty(common.PROP_SIMULATION_INSTRUMENT,
                                 PropertyCriterion.IsEqualTo, common.SIMULATION_INSTRUMEN_SPARSE))
        self.declareProperty(name=common.PROP_NUMBER_OF_SIMULATION_WAVELENGTHS,
                             defaultValue=Property.EMPTY_INT,
                             validator=greaterThanTwoInt,
                             direction=Direction.Input,
                             doc='Number of wavelength points where the simulation is performed (default: all).')

    def validateInputs(self):
        """Check for issues with user input."""
        return dict()

    def _finalize(self, outWS):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        self._cleanup.cleanup(outWS)
        self._cleanup.finalCleanup()

    def _inputWS(self):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        self._cleanup.protect(mainWS)
        return mainWS

    def _selfShielding(self, mainWS):
        """Return the self shielding corrections."""
        wavelengthWSName = self._names.withSuffix('input_in_wavelength')
        wavelengthWS = ConvertUnits(InputWorkspace=mainWS,
                                    OutputWorkspace=wavelengthWSName,
                                    Target='Wavelength',
                                    EMode='Direct',
                                    EnableLogging=self._subalgLogging)
        wavelengthPoints = self.getProperty(common.PROP_NUMBER_OF_SIMULATION_WAVELENGTHS).value
        correctionWSName = self._names.withSuffix('correction')
        useFullInstrument = self.getProperty(common.PROP_SIMULATION_INSTRUMENT).value == common.SIMULATION_INSTRUMENT_FULL
        if useFullInstrument:
            correctionWS = MonteCarloAbsorption(InputWorkspace=wavelengthWS,
                                                OutputWorkspace=correctionWSName,
                                                SparseInstrument=False,
                                                NumberOfWavelengthPoints=wavelengthPoints,
                                                Interpolation='CSpline',
                                                EnableLogging=self._subalgLogging)
        else:
            rows = self.getProperty(common.PROP_SPARSE_INSTRUMENT_ROWS).value
            columns = self.getProperty(common.PROP_SPARSE_INSTRUMENT_COLUMNS).value
            correctionWS = MonteCarloAbsorption(InputWorkspace=wavelengthWS,
                                                OutputWorkspace=correctionWSName,
                                                SparseInstrument=True,
                                                NumberOfDetectorRows=rows,
                                                NumberOfDetectorColumns=columns,
                                                NumberOfWavelengthPoints=wavelengthPoints,
                                                Interpolation='CSpline',
                                                EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(wavelengthWS)
        correctionWS = ConvertUnits(InputWorkspace=correctionWS,
                                    OutputWorkspace=correctionWSName,
                                    Target='TOF',
                                    EMode='Direct',
                                    EnableLogging=self._subalgLogging)
        return correctionWS


AlgorithmFactory.subscribe(DirectILLSelfShielding)
