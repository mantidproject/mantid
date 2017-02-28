# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        MatrixWorkspaceProperty, PropertyMode, WorkspaceGroupProperty, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty, FloatBoundedValidator, IntBoundedValidator,
                           Property, PropertyCriterion, StringListValidator)
from mantid.simpleapi import (ConvertUnits, FlatPlatePaalmanPingsCorrection)


def _selfShieldingCorrectionsCylinder(ws, ecWS, sampleChemicalFormula,
                                      sampleNumberDensity,
                                      containerChemicalFormula,
                                      containerNumberDensity, sampleInnerR,
                                      sampleOuterR, containerOuterR, beamWidth,
                                      beamHeight, stepSize, numberWavelengths,
                                      wsNames, algorithmLogging):
    """Calculate the self-shielding corrections workspace for cylindrical sample."""
    from mantid.simpleapi import CylinderPaalmanPingsCorrection
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections_with_ec')
    selfShieldingWS = CylinderPaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleInnerRadius=sampleInnerR,
        SampleOuterRadius=sampleOuterR,
        CanWorkspace=ecWS,
        CanChemicalFormula=containerChemicalFormula,
        CanDensityType='Number Density',
        CanDensity=containerNumberDensity,
        CanOuterRadius=containerOuterR,
        BeamHeight=beamHeight,
        BeamWidth=beamWidth,
        StepSize=stepSize,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsCylinderNoEC(ws, sampleChemicalFormula,
                                          sampleNumberDensity,
                                          sampleInnerR, sampleOuterR,
                                          beamWidth, beamHeight, stepSize,
                                          numberWavelengths, wsNames,
                                          algorithmLogging):
    """Calculate the self-shielding corrections workspace for cylindrical sample without using empty container data."""
    from mantid.simpleapi import CylinderPaalmanPingsCorrection
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections')
    selfShieldingWS = CylinderPaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleInnerRadius=sampleInnerR,
        SampleOuterRadius=sampleOuterR,
        BeamHeight=beamHeight,
        BeamWidth=beamWidth,
        StepSize=stepSize,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsSlab(ws, ecWS, sampleChemicalFormula,
                                  sampleNumberDensity,
                                  containerChemicalFormula,
                                  containerNumberDensity, sampleThickness,
                                  sampleAngle, containerFrontThickness,
                                  containerBackThickness, numberWavelengths,
                                  wsNames, algorithmLogging):
    """Calculate the self-shielding corrections workspace for flat plate sample."""
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections_with_ec')
    selfShieldingWS = FlatPlatePaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleThickness=sampleThickness,
        SampleAngle=sampleAngle,
        CanWorkspace=ecWS,
        CanChemicalFormula=containerChemicalFormula,
        CanDensityType='Number Density',
        CanDensity=containerNumberDensity,
        CanFrontThickness=containerFrontThickness,
        CanBackThickness=containerBackThickness,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


def _selfShieldingCorrectionsSlabNoEC(ws, sampleChemicalFormula,
                                      sampleNumberDensity, sampleThickness,
                                      sampleAngle, numberWavelengths, wsNames,
                                      algorithmLogging):
    """Calculate the self-shielding corrections workspace for flat plate sample without using empty container data."""
    selfShieldingWSName = \
        wsNames.withSuffix('self_shielding_corrections')
    selfShieldingWS = FlatPlatePaalmanPingsCorrection(
        SampleWorkspace=ws,
        SampleChemicalFormula=sampleChemicalFormula,
        SampleDensityType='Number Density',
        SampleDensity=sampleNumberDensity,
        SampleThickness=sampleThickness,
        SampleAngle=sampleAngle,
        NumberWavelengths=numberWavelengths,
        Emode='Direct',
        OutputWorkspace=selfShieldingWSName,
        EnableLogging=algorithmLogging)
    return selfShieldingWS


class DirectILLSelfShielding(DataProcessorAlgorithm):
    """A workflow algorithm for self-shielding corrections."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

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
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        # Get input workspace.
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        # Self shielding and empty container subtraction, if requested.
        correctionWS = self._selfShielding(mainWS, wsNames, wsCleanup, subalgLogging)

        self._finalize(correctionWS, wsCleanup)

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=0)
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            optional=PropertyMode.Optional,
            direction=Direction.Input),
            doc='Input workspace.')
        self.declareProperty(WorkspaceGroupProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output corrections workspace.')
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
        self.declareProperty(name=common.PROP_SELF_SHIELDING_STEP_SIZE,
                             defaultValue=0.002,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Step size for self shielding simulation.')
        self.declareProperty(name=common.PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS,
                             defaultValue=10,
                             validator=positiveInt,
                             direction=Direction.Input,
                             doc='Number of wavelengths for self shielding ' +
                                 'simulation')
        self.declareProperty(name=common.PROP_SAMPLE_SHAPE,
                             defaultValue=common.SAMPLE_SHAPE_SLAB,
                             validator=StringListValidator([
                                 common.SAMPLE_SHAPE_SLAB,
                                 common.SAMPLE_SHAPE_CYLINDER]),
                             direction=Direction.Input,
                             doc='The shape of the sample and its container.')
        self.declareProperty(name=common.PROP_SAMPLE_CHEMICAL_FORMULA,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Chemical formula for the sample material.')
        self.setPropertyGroup(common.PROP_SAMPLE_CHEMICAL_FORMULA,
                              common.PROPGROUP_SAMPLE)
        self.declareProperty(name=common.PROP_SAMPLE_NUMBER_DENSITY,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Number density of the sample material.')
        self.setPropertyGroup(common.PROP_SAMPLE_NUMBER_DENSITY,
                              common.PROPGROUP_SAMPLE)
        self.declareProperty(name=common.PROP_CONTAINER_CHEMICAL_FORMULA,
                             defaultValue='',
                             direction=Direction.Input,
                             doc='Chemical formula for the container ' +
                                 'material.')
        self.setPropertyGroup(common.PROP_CONTAINER_CHEMICAL_FORMULA,
                              common.PROPGROUP_CONTAINER)
        self.declareProperty(name=common.PROP_CONTAINER_NUMBER_DENSITY,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Number density of the container material.')
        self.setPropertyGroup(common.PROP_CONTAINER_NUMBER_DENSITY,
                              common.PROPGROUP_CONTAINER)
        self.declareProperty(name=common.PROP_SAMPLE_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Sample thickness.')
        self.setPropertyGroup(common.PROP_SAMPLE_THICKNESS,
                              common.PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=common.PROP_CONTAINER_FRONT_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Container front face thickness.')
        self.setPropertyGroup(common.PROP_CONTAINER_FRONT_THICKNESS,
                              common.PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=common.PROP_CONTAINER_BACK_THICKNESS,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Container back face thickness.')
        self.setPropertyGroup(common.PROP_CONTAINER_BACK_THICKNESS,
                              common.PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=common.PROP_SAMPLE_ANGLE,
                             defaultValue=0.0,
                             direction=Direction.Input,
                             doc='Sample rotation angle.')
        self.setPropertyGroup(common.PROP_SAMPLE_ANGLE,
                              common.PROPGROUP_SLAB_CONTAINER)
        self.declareProperty(name=common.PROP_SAMPLE_INNER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Inner radius of the sample.')
        self.setPropertyGroup(common.PROP_SAMPLE_INNER_RADIUS,
                              common.PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=common.PROP_SAMPLE_OUTER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Outer radius of the sample.')
        self.setPropertyGroup(common.PROP_SAMPLE_OUTER_RADIUS,
                              common.PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=common.PROP_CONTAINER_OUTER_RADIUS,
                             defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             doc='Outer radius of the container.')
        self.setPropertyGroup(common.PROP_CONTAINER_OUTER_RADIUS,
                              common.PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=common.PROP_BEAM_WIDTH,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Width of the neutron beam.')
        self.setPropertyGroup(common.PROP_BEAM_WIDTH,
                              common.PROPGROUP_CYLINDER_CONTAINER)
        self.declareProperty(name=common.PROP_BEAM_HEIGHT,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Height of the neutron beam.')
        self.setPropertyGroup(common.PROP_BEAM_HEIGHT,
                              common.PROPGROUP_CYLINDER_CONTAINER)

    def validateInputs(self):
        """Check for issues with user input."""
        return dict()

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.finalCleanup()

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _selfShielding(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Return the self shielding corrections."""
        ecWS = self.getProperty(common.PROP_EC_WS).value
        wavelengthWSName = wsNames.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(InputWorkspace=mainWS,
                                    OutputWorkspace=wavelengthWSName,
                                    Target='Wavelength',
                                    EMode='Direct',
                                    EnableLogging=subalgLogging)
        sampleShape = self.getProperty(common.PROP_SAMPLE_SHAPE).value
        if ecWS:
            wavelengthECWSName = wsNames.withSuffix('ec_in_wavelength')
            wavelengthECWS = \
                ConvertUnits(InputWorkspace=ecWS,
                             OutputWorkspace=wavelengthECWSName,
                             Target='Wavelength',
                             EMode='Direct',
                             EnableLogging=subalgLogging)
            sampleChemicalFormula = self.getProperty(common.PROP_SAMPLE_CHEMICAL_FORMULA).value
            sampleNumberDensity = self.getProperty(common.PROP_SAMPLE_NUMBER_DENSITY).value
            containerChemicalFormula = self.getProperty(common.PROP_CONTAINER_CHEMICAL_FORMULA).value
            containerNumberDensity = self.getProperty(common.PROP_CONTAINER_NUMBER_DENSITY).value
            stepSize = self.getProperty(common.PROP_SELF_SHIELDING_STEP_SIZE).value
            numberWavelengths = self.getProperty(common.PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS).value
            if sampleShape == common.SAMPLE_SHAPE_CYLINDER:
                sampleInnerR = self.getProperty(common.PROP_SAMPLE_INNER_RADIUS).value
                sampleOuterR = self.getProperty(common.PROP_SAMPLE_OUTER_RADIUS).value
                containerOuterR = self.getProperty(common.PROP_CONTAINER_OUTER_RADIUS).value
                beamWidth = self.getProperty(common.PROP_BEAM_WIDTH).value
                beamHeight = self.getProperty(common.PROP_BEAM_HEIGHT).value
                selfShieldingWS = \
                    _selfShieldingCorrectionsCylinder(
                        wavelengthWS, wavelengthECWS,
                        sampleChemicalFormula, sampleNumberDensity,
                        containerChemicalFormula,
                        containerNumberDensity, sampleInnerR,
                        sampleOuterR, containerOuterR, beamWidth,
                        beamHeight, stepSize, numberWavelengths,
                        wsNames, subalgLogging)
            else:
                # Slab container.
                sampleThickness = self.getProperty(common.PROP_SAMPLE_THICKNESS).value
                containerFrontThickness = self.getProperty(common.PROP_CONTAINER_FRONT_THICKNESS).value
                containerBackThickness = self.getProperty(common.PROP_CONTAINER_BACK_THICKNESS).value
                angle = self.getProperty(common.PROP_SAMPLE_ANGLE).value
                selfShieldingWS = \
                    _selfShieldingCorrectionsSlab(
                        wavelengthWS, wavelengthECWS,
                        sampleChemicalFormula, sampleNumberDensity,
                        containerChemicalFormula,
                        containerNumberDensity, sampleThickness, angle,
                        containerFrontThickness,
                        containerBackThickness, numberWavelengths,
                        wsNames, subalgLogging)
            wsCleanup.cleanup(wavelengthECWS)
        else:  # No ecWS.
            sampleChemicalFormula = self.getProperty(common.PROP_SAMPLE_CHEMICAL_FORMULA).value
            sampleNumberDensity = self.getProperty(common.PROP_SAMPLE_NUMBER_DENSITY).value
            stepSize = self.getProperty(common.PROP_SELF_SHIELDING_STEP_SIZE).value
            numberWavelengths = self.getProperty(common.PROP_SELF_SHIELDING_NUMBER_WAVELENGTHS).value
            if sampleShape == common.SAMPLE_SHAPE_CYLINDER:
                sampleInnerR = self.getProperty(common.PROP_SAMPLE_INNER_RADIUS).value
                sampleOuterR = self.getProperty(common.PROP_SAMPLE_OUTER_RADIUS).value
                beamWidth = self.getProperty(common.PROP_BEAM_WIDTH).value
                beamHeight = self.getProperty(common.PROP_BEAM_HEIGHT).value
                selfShieldingWS = \
                    _selfShieldingCorrectionsCylinderNoEC(
                        wavelengthWS, sampleChemicalFormula,
                        sampleNumberDensity, sampleInnerR, sampleOuterR,
                        beamWidth, beamHeight, stepSize, numberWavelengths,
                        wsNames, subalgLogging)
            else:
                # Slab container.
                sampleThickness = \
                    self.getProperty(common.PROP_SAMPLE_THICKNESS).value
                angle = self.getProperty(common.PROP_SAMPLE_ANGLE).value
                selfShieldingWS = \
                    _selfShieldingCorrectionsSlabNoEC(
                        wavelengthWS, sampleChemicalFormula,
                        sampleNumberDensity, sampleThickness, angle,
                        numberWavelengths, wsNames, subalgLogging)
        wsCleanup.cleanup(wavelengthWS)
        return selfShieldingWS


AlgorithmFactory.subscribe(DirectILLSelfShielding)
