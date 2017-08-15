from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as s_api
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator, Progress, mtd)
from mantid.kernel import (VisibleWhenProperty, PropertyCriterion, StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator, LogicOperator)


class CalculateMonteCarloAbsorption(DataProcessorAlgorithm):
    # General variables
    _emode = None
    _efixed = None
    _general_kwargs = None
    _shape = None
    _height = None

    # Sample variables
    _sample_angle = None
    _sample_center = None
    _sample_chemical_formula = None
    _sample_density = None
    _sample_density_type = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _sample_radius = None
    _sample_thickness = None
    _sample_unit = None
    _sample_width = None
    _sample_ws = None
    _sample_ws_name = None

    # Container variables
    _container_angle = None
    _container_center = None
    _container_chemical_formula = None
    _container_density = None
    _container_density_type = None
    _container_inner_radius = None
    _container_outer_radius = None
    _container_thickness = None
    _container_width = None
    _container_ws_name = None

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
        self.declareProperty(name='ContainerFrontThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Front thickness of the container environment (cm)')
        self.declareProperty(name='ContainerBackThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Back thickness of the container environment (cm)')

        containerFlatPlateCondition = VisibleWhenProperty(containerCondition, flatPlateCondition, LogicOperator.And)

        self.setPropertySettings('ContainerFrontThickness', containerFlatPlateCondition)
        self.setPropertySettings('ContainerBackThickness', containerFlatPlateCondition)

        self.setPropertyGroup('ContainerFrontThickness', 'Container Shape Options')
        self.setPropertyGroup('ContainerBackThickness', 'Container Shape Options')

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
        prog = Progress(self, 0, 1, 10)

        prog.report('Converting to wavelength')
        sample_wave_ws = self._convert_to_wavelength(self._sample_ws_name, '__sample_wave')

        prog.report('Calculating sample absorption factors')

        sample_kwargs = dict()
        sample_kwargs.update(self._general_kwargs)
        sample_kwargs['ChemicalFormula'] = self._sample_chemical_formula
        sample_kwargs['DensityType'] = self._sample_density_type
        sample_kwargs['Density'] = self._sample_density
        sample_kwargs['Height'] = self._height
        sample_kwargs['Shape'] = self._shape

        if self._shape == 'FlatPlate':
            sample_kwargs['Width'] = self._sample_width
            sample_kwargs['Thickness'] = self._sample_thickness
            sample_kwargs['Angle'] = self._sample_angle
            sample_kwargs['Center'] = self._sample_center

        if self._shape == 'Cylinder':
            sample_kwargs['Radius'] = self._sample_radius

        if self._shape == 'Annulus':
            sample_kwargs['InnerRadius'] = self._sample_inner_radius
            sample_kwargs['OuterRadius'] = self._sample_outer_radius

        s_api.SimpleShapeMonteCarloAbsorption(InputWorkspace=sample_wave_ws,
                                              OutputWorkspace=self._ass_ws,
                                              **sample_kwargs)

        self._ass_ws = self._convert_from_wavelength(self._ass_ws, self._ass_ws)

        if self._container_ws_name:

            prog.report('Calculating container absorption factors')

            container_wave_1 = self._convert_to_wavelength(self._container_ws_name, '__container_wave_1')
            container_wave_2 = self._clone_ws(container_wave_1)

            container_kwargs = dict()
            container_kwargs.update(self._general_kwargs)
            container_kwargs['ChemicalFormula'] = self._container_chemical_formula
            container_kwargs['DensityType'] = self._container_density_type
            container_kwargs['Density'] = self._container_density
            container_kwargs['Height'] = self._height
            container_kwargs['Shape'] = self._shape

            if self._shape == 'FlatPlate':
                container_kwargs['Width'] = self._sample_width
                container_kwargs['Angle'] = self._sample_angle
                offset_front = 0.5 * (self._container_front_thickness + self._sample_thickness)
                container_kwargs['Thickness'] = self._container_front_thickness
                container_kwargs['Center'] = -offset_front

                s_api.SimpleShapeMonteCarloAbsorption(InputWorkspace=container_wave_1,
                                                      OutputWorkspace='_acc_1',
                                                      **container_kwargs)

                offset_back = 0.5 * (self._container_back_thickness + self._sample_thickness)
                container_kwargs['Thickness'] = self._container_back_thickness
                container_kwargs['Center'] = offset_back

                s_api.SimpleShapeMonteCarloAbsorption(InputWorkspace=container_wave_2,
                                                      OutputWorkspace='_acc_2',
                                                      **container_kwargs)

                self._acc_ws = self._multiply('_acc_1', '_acc_2', self._acc_ws)
                mtd['_acc_1'].delete()
                mtd['_acc_2'].delete()

            if self._shape == 'Cylinder':
                container_kwargs['InnerRadius'] = self._container_inner_radius
                container_kwargs['OuterRadius'] = self._container_outer_radius
                container_kwargs['Shape'] = 'Annulus'

                s_api.SimpleShapeMonteCarloAbsorption(InputWorkspace=container_wave_1,
                                                      OutputWorkspace=self._acc_ws,
                                                      **container_kwargs)

            if self._shape == 'Annulus':
                container_kwargs['InnerRadius'] = self._container_inner_radius
                container_kwargs['OuterRadius'] = self._sample_inner_radius

            self._acc_ws = self._convert_from_wavelength(self._acc_ws, self._acc_ws)

            sample_wave_ws.delete()

            # mtd.addOrReplace(self._output_ws + '_ass', self._ass_ws)
            mtd.addOrReplace(self._output_ws + '_acc', self._acc_ws)

            self._output_ws = self._group_ws([self._ass_ws, self._acc_ws], self._output_ws)

        self.setProperty('CorrectionsWorkspace', self._output_ws)

    def _setup(self):

        # The beam properties and monte carlo properties are simply passed straight on to the
        # SimpleShapeMonteCarloAbsorptionCorrection algorithm so they are being put into
        # a dictionary for simplicity

        self._general_kwargs = {'BeamHeight': self.getProperty('BeamHeight').value,
                                'BeamWidth': self.getProperty('BeamWidth').value,
                                'NumberOfWavelengthPoints': self.getProperty('NumberOfWavelengthPoints').value,
                                'EventsPerPoint': self.getProperty('EventsPerPoint').value,
                                'Interpolation': self.getProperty('Interpolation').value}

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._sample_ws = mtd[self._sample_ws_name]
        self._container_ws_name = self.getPropertyValue('ContainerWorkspace')
        self._shape = self.getProperty('Shape').value
        self._height = self.getProperty('Height').value

        self._sample_unit = self._sample_ws.getAxis(0).getUnit().unitID()
        logger.information('Input X-unit is {}'.format(self._sample_unit))
        if self._sample_unit == 'dSpacing':
            self._emode = 'Elastic'
        else:
            self._emode = str(self._sample_ws.getEMode())
        if self._emode == 'Indirect' or 'Direct':
            self._efixed = self._get_efixed()

        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_density_type = self.getPropertyValue('SampleDensityType')
        self._sample_density = self.getProperty('SampleDensity').value

        if self._container_ws_name:
            self._container_chemical_formula = self.getPropertyValue('ContainerChemicalFormula')
            self._container_density_type = self.getPropertyValue('ContainerDensityType')
            self._container_density = self.getProperty('ContainerDensity').value

        if self._shape == 'FlatPlate':
            self._sample_width = self.getProperty('SampleWidth').value
            self._sample_thickness = self.getProperty('SampleThickness').value
            self._sample_angle = self.getProperty('SampleAngle').value
            self._sample_center = self.getProperty('SampleCenter').value

        if self._shape == 'Cylinder':
            self._sample_radius = self.getProperty('SampleRadius').value

        if self._shape == 'Annulus':
            self._sample_inner_radius = self.getProperty('SampleInnerRadius').value
            self._sample_outer_radius = self.getProperty('SampleOuterRadius').value

        if self._container_ws_name:
            if self._shape == 'FlatPlate':
                self._container_front_thickness = self.getProperty('ContainerFrontThickness').value
                self._container_back_thickness = self.getProperty('ContainerBackThickness').value

            else:
                self._container_inner_radius = self.getProperty('ContainerInnerRadius').value
                self._container_outer_radius = self.getProperty('ContainerOuterRadius').value

        self._output_ws = self.getPropertyValue('CorrectionsWorkspace')
        self._ass_ws = self._output_ws + '_ass'
        self._acc_ws = self._output_ws + '_acc'

    def validateInputs(self):

        self._setup()
        issues = dict()

        if self._shape == 'Annulus':
            if self._sample_inner_radius >= self._sample_outer_radius:
                issues['SampleOuterRadius'] = 'Must be greater than SampleInnerRadius'

        if self._container_ws_name:
            container_unit = mtd[self._container_ws_name].getAxis(0).getUnit().unitID()
            if container_unit != self._sample_unit:
                raise ValueError('Sample and Container units must be the same!')

            if self._shape == 'Cylinder':
                if self._container_inner_radius <= self._sample_radius:
                    issues['ContainerInnerRadius'] = 'Must be greater than SampleRadius'
                if self._container_outer_radius <= self._container_inner_radius:
                    issues['ContainerOuterRadius'] = 'Must be greater than ContainerInnerRadius'

            if self._shape == 'Annulus':
                if self._container_inner_radius >= self._sample_inner_radius:
                    issues['ContainerInnerRadius'] = 'Must be less than SampleInnerRadius'
                if self._container_outer_radius <= self._sample_outer_radius:
                    issues['ContainerOuterRadius'] = 'Must be greater than SampleOuterRadius'

        return issues

    def _get_efixed(self):
        """
        Returns the efixed value relating to the sample workspace
        """
        inst = self._sample_ws.getInstrument()

        if inst.hasParameter('Efixed'):
            return inst.getNumberParameter('Efixed')[0]

        if inst.hasParameter('analyser'):
            analyser_comp = inst.getComponentByName(inst.getStringParameter('analyser')[0])

            if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
                return analyser_comp.getNumberParameter('EFixed')[0]

        raise ValueError('No Efixed parameter found')

    # ------------------------------- Converting to/from wavelength -------------------------------

    def _convert_to_wavelength(self, input_ws, output_ws):

        if self._sample_unit == 'Wavelength':
            return self._clone_ws(input_ws, output_ws)

        else:
            convert_unit_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)
            convert_unit_alg.setProperty("InputWorkspace", input_ws)
            convert_unit_alg.setProperty("OutputWorkspace", output_ws)
            convert_unit_alg.setProperty("Target", 'Wavelength')
            convert_unit_alg.setProperty("EMode", self._emode)
            if self._emode == 'Indirect':
                convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            return convert_unit_alg.getProperty("OutputWorkspace").value

    def _convert_from_wavelength(self, input_ws, output_ws):

        convert_unit_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)

        if self._sample_unit != 'Wavelength':
            convert_unit_alg.setProperty("InputWorkspace", input_ws)
            convert_unit_alg.setProperty("OutputWorkspace", output_ws)
            convert_unit_alg.setProperty("Target", self._unit)
            convert_unit_alg.setProperty("EMode", self._emode)
            if self._emode == 'Indirect':
                convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            return convert_unit_alg.getProperty("OutputWorkspace").value

        else:
            return input_ws

    # ------------------------------- Child algorithms -------------------------------

    def _clone_ws(self, input_ws, output_ws='_clone'):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.setProperty("OutputWorkspace", output_ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _multiply(self, lhs_ws, rhs_ws, output_ws):
        multiply_alg = self.createChildAlgorithm("Multiply", enableLogging=False)
        multiply_alg.setProperty("LHSWorkspace", lhs_ws)
        multiply_alg.setProperty("RHSWorkspace", rhs_ws)
        multiply_alg.setProperty("OutputWorkspace", output_ws)
        multiply_alg.execute()
        return multiply_alg.getProperty('OutputWorkspace').value

    def _group_ws(self, input_ws, output_ws):
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", input_ws)
        group_alg.setProperty("OutputWorkspace", output_ws)
        group_alg.execute()
        return group_alg.getProperty("OutputWorkspace").value


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateMonteCarloAbsorption)
