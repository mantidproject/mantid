from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as s_api
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator, Progress)
from mantid.kernel import (VisibleWhenProperty, PropertyCriterion, StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator, LogicOperator)


class CalculateMonteCarloAbsorption(DataProcessorAlgorithm):
    # General variables
    _unit = None
    _emode = None
    _efixed = None
    _general_kwargs = None
    _container_kwargs = None
    _shape = None

    # Sample variables
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None

    # Container variables
    _can_ws_name = None
    _can_chemical_formula = None
    _can_density_type = None
    _can_density = None

    # Output workspaces
    _ass_ws = None
    _acc_ws = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates indirect absorption corrections for a given sample shape."

    def PyInit(self):
        # General properties

        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')
        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        self.setPropertyGroup('BeamHeight', 'Beam Options')
        self.setPropertyGroup('BeamWidth', 'Beam Options')

        # Monte Carlo options
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')
        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(
                                 ['Linear', 'CSpline']),
                             doc='Type of interpolation')

        self.setPropertyGroup('NumberOfWavelengthPoints', 'Monte Carlo Options')
        self.setPropertyGroup('EventsPerPoint', 'Monte Carlo Options')
        self.setPropertyGroup('Interpolation', 'Monte Carlo Options')

        # Sample options
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Sample Workspace')
        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             doc='Chemical formula for the sample material')
        self.declareProperty(name='SampleDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Sample density type')
        self.declareProperty(name='SampleDensity', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample density')

        self.setPropertyGroup('SampleWorkspace', 'Sample Options')
        self.setPropertyGroup('SampleChemicalFormula', 'Sample Options')
        self.setPropertyGroup('SampleDensityType', 'Sample Options')
        self.setPropertyGroup('SampleDensity', 'Sample Options')

        # Container options
        self.declareProperty(MatrixWorkspaceProperty('ContainerWorkspace', '', direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Container Workspace')

        containerCondition = VisibleWhenProperty('ContainerWorkspace', PropertyCriterion.IsNotDefault)

        self.declareProperty(name='ContainerChemicalFormula', defaultValue='',
                             doc='Chemical formula for the container material')
        self.declareProperty(name='ContainerDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Container density type')
        self.declareProperty(name='ContainerDensity', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container density')

        self.setPropertyGroup('ContainerWorkspace', 'Container Options')
        self.setPropertyGroup('ContainerChemicalFormula', 'Container Options')
        self.setPropertyGroup('ContainerDensityType', 'Container Options')
        self.setPropertyGroup('ContainerDensity', 'Container Options')

        self.setPropertySettings('ContainerChemicalFormula', containerCondition)
        self.setPropertySettings('ContainerDensityType', containerCondition)
        self.setPropertySettings('ContainerDensity', containerCondition)

        # Shape options
        self.declareProperty(name='Shape', defaultValue='FlatPlate',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Geometric shape of the sample environment')

        flatPlateCondition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'FlatPlate')
        cylinderCondition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Cylinder')
        annulusCondition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Annulus')

        # height is common to all, and should be the same for sample and container
        self.declareProperty('Height', defaultValue=0.0, validator=FloatBoundedValidator(0.0),
                             doc='Height of the sample environment (cm)')

        self.setPropertyGroup('Shape', 'Shape Options')
        self.setPropertyGroup('Height', 'Shape Options')

        # ---------------------------Sample---------------------------
        # Flat Plate
        self.declareProperty(name='SampleWidth', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the sample environment (cm)')
        self.declareProperty(name='SampleThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Thickness of the sample environment (cm)')
        self.declareProperty(name='SampleCenter', defaultValue=0.0,
                             doc='Center of the sample environment')
        self.declareProperty(name='SampleAngle', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Angle of the sample environment with respect to the beam (degrees)')

        self.setPropertySettings('SampleWidth', flatPlateCondition)
        self.setPropertySettings('SampleThickness', flatPlateCondition)
        self.setPropertySettings('SampleCenter', flatPlateCondition)
        self.setPropertySettings('SampleAngle', flatPlateCondition)

        self.setPropertyGroup('SampleWidth', 'Sample Shape Options')
        self.setPropertyGroup('SampleThickness', 'Sample Shape Options')
        self.setPropertyGroup('SampleCenter', 'Sample Shape Options')
        self.setPropertyGroup('SampleAngle', 'Sample Shape Options')

        # Cylinder
        self.declareProperty(name='SampleRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Radius of the sample environment (cm)')

        self.setPropertySettings('SampleRadius', cylinderCondition)
        self.setPropertyGroup('SampleRadius', 'Sample Shape Options')

        # Annulus
        self.declareProperty(name='SampleInnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the sample environment (cm)')
        self.declareProperty(name='SampleOuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the sample environment (cm)')

        self.setPropertySettings('SampleInnerRadius', annulusCondition)
        self.setPropertySettings('SampleOuterRadius', annulusCondition)

        self.setPropertyGroup('SampleInnerRadius', 'Sample Shape Options')
        self.setPropertyGroup('SampleOuterRadius', 'Sample Shape Options')

        # ---------------------------Container---------------------------
        # Flat Plate
        self.declareProperty(name='ContainerWidth', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the container environment (cm)')
        self.declareProperty(name='ContainerThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Thickness of the container environment (cm)')
        self.declareProperty(name='ContainerCenter', defaultValue=0.0,
                             doc='Center of the container environment')
        self.declareProperty(name='ContainerAngle', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Angle of the container environment with respect to the beam (degrees)')

        containerFlatPlateCondition = VisibleWhenProperty(containerCondition, flatPlateCondition, LogicOperator.And)

        self.setPropertySettings('ContainerWidth', containerFlatPlateCondition)
        self.setPropertySettings('ContainerThickness', containerFlatPlateCondition)
        self.setPropertySettings('ContainerCenter', containerFlatPlateCondition)
        self.setPropertySettings('ContainerAngle', containerFlatPlateCondition)

        self.setPropertyGroup('ContainerWidth', 'Container Shape Options')
        self.setPropertyGroup('ContainerThickness', 'Container Shape Options')
        self.setPropertyGroup('ContainerCenter', 'Container Shape Options')
        self.setPropertyGroup('ContainerAngle', 'Container Shape Options')

        # Both cylinder and annulus have an annulus container

        notFlatPlateCondition = VisibleWhenProperty('Shape', PropertyCriterion.IsNotEqualTo, 'FlatPlate')

        containerNFPCondition = VisibleWhenProperty(containerCondition, notFlatPlateCondition, LogicOperator.And)

        self.declareProperty(name='ContainerInnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the container environment (cm)')
        self.declareProperty(name='ContainerOuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the container environment (cm)')

        self.setPropertySettings('ContainerInnerRadius', containerNFPCondition)
        self.setPropertySettings('ContainerOuterRadius', containerNFPCondition)

        self.setPropertyGroup('ContainerInnerRadius', 'Container Shape Options')
        self.setPropertyGroup('ContainerOuterRadius', 'Container Shape Options')

        # output
        self.declareProperty(WorkspaceGroupProperty(name='CorrectionsWorkspace',
                                                    defaultValue='corrections',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='Name of the workspace group to save correction factors')
        self.setPropertyGroup('CorrectionsWorkspace', 'Output Options')

    def PyExec(self):

        # set up progress reporting
        self.prog = Progress(self, 0, 1, 10)
        self._setup()

        pass

    def _setup(self):

        # The beam properties and monte carlo properties are simply passed on to the
        # SimpleShapeMonteCarloAbsorptionCorrection algorithm so they are being put into
        # a dictionary for simplicity

        self._general_kwargs = {'BeamHeight': self.getProperty('BeamHeight').value,
                                'BeamWidth': self.getProperty('BeamWidth').value,
                                'NumberOfWavelengthPoints': self.getProperty('NumberOfWavelengthPoints').value,
                                'EventsPerPoint': self.getProperty('EventsPerPoint').value,
                                'Interpolation': self.getProperty('Interpolation').value}

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._container_ws_name = self.getPropertyValue('ContainerWorkspace')
        self._shape = self.getProperty('Shape').value

        self._sample_kwargs = {'ChemicalFormula': self.getPropertyValue('SampleChemicalFormula'),
                               'DensityType': self.getPropertyValue('SampleDensityType'),
                               'Density': self.getProperty('SampleDensity').value,
                               'Shape': self._shape,
                               'Height': self.getProperty('Height').value}

        if self._container_ws_name:
            self._container_kwargs = {'ChemicalFormula': self.getPropertyValue('ContainerChemicalFormula'),
                                      'DensityType': self.getPropertyValue('ContainerDensityType'),
                                      'Density': self.getProperty('ContainerDensity').value,
                                      'Height': self.getProperty('Height').value}

        if self._shape == 'FlatPlate':

            self._sample_kwargs['Width'] = self.getProperty('SampleWidth').value
            self._sample_kwargs['Thickness'] = self.getProperty('SampleThickness').value
            self._sample_kwargs['Angle'] = self.getProperty('SampleAngle').value
            self._sample_kwargs['Center'] = self.getProperty('SampleCenter').value

        if self._shape == 'Cylinder':
            self._sample_kwargs['Radius'] = self.getProperty('SampleRadius').value

        if self._shape == 'Annulus':
            self._sample_kwargs['InnerRadius'] = self.getProperty('SampleInnerRadius').value
            self._sample_kwargs['OuterRadius'] = self.getProperty('SampleOuterRadius').value

            if self._container_ws_name:
                if self._shape == 'FlatPlate':
                    self._container_kwargs['Width'] = self.getProperty('ContainerWidth').value
                    self._container_kwargs['Thickness'] = self.getProperty('ContainerThickness').value
                    self._container_kwargs['Angle'] = self.getProperty('ContainerAngle').value
                    self._container_kwargs['Center'] = self.getProperty('ContainerCenter').value
                    self._container_kwargs['Shape'] = 'FlatPlate'
                else:
                    self._container_kwargs['InnerRadius'] = self.getProperty('ContainerInnerRadius').value
                    self._container_kwargs['OuterRadius'] = self.getProperty('ContainerOuterRadius').value
                    self._container_kwargs['Shape'] = 'Annulus'
                
# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateMonteCarloAbsorption)
