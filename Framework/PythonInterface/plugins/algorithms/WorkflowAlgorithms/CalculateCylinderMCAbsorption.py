from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (mtd, CloneWorkspace, DeleteWorkspace, GroupWorkspaces, ConvertUnits,
                              Multiply, AddSampleLogMultiple,
                              CylinderMonteCarloAbsorption, AnnulusMonteCarloAbsorption)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator, Progress)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator)


class CalculateCylinderMCAbsorption(DataProcessorAlgorithm):
    # Sample variables
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_radius = None
    _sample_height = None

    # Container variables
    _can_ws_name = None
    _can_chemical_formula = None
    _can_density_type = None
    _can_density = None
    _can_radius = None

    _beam_height = None
    _beam_width = None

    _unit = None
    _emode = None
    _efixed = None
    _number_wavelengths = None
    _events = None
    _abs_ws = None
    _ass_ws = None
    _acc_ws = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates indirect absorption corrections for a flat sample shape."

    def PyInit(self):
        # Sample
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     direction=Direction.Input),
                             doc='Sample workspace')
        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Chemical formula for the sample')
        self.declareProperty(name='SampleDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Sample density type.')
        self.declareProperty(name='SampleDensity', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample number density')
        self.declareProperty(name='SampleRadius', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample radius')
        self.declareProperty(name='SampleHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample height')

        # Container
        self.declareProperty(MatrixWorkspaceProperty('ContainerWorkspace', '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='Container workspace')
        self.declareProperty(name='ContainerChemicalFormula', defaultValue='',
                             doc='Chemical formula for the Container')
        self.declareProperty(name='ContainerDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Container density type.')
        self.declareProperty(name='ContainerDensity', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container number density')
        self.declareProperty(name='ContainerInnerRadius', defaultValue=0.2,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container inner radius')
        self.declareProperty(name='ContainerOuterRadius', defaultValue=0.2,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container outer radius')

        # Beam size
        self.declareProperty(name='BeamHeight', defaultValue='',
                             doc='Height of the beam (cm)')
        self.declareProperty(name='BeamWidth', defaultValue='',
                             doc='Width of the beam (cm)')

        # Monte Carlo
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')

        # Output
        self.declareProperty(WorkspaceGroupProperty('CorrectionsWorkspace', 'Corrections',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The workspace group to save correction factors')

    def PyExec(self):

        # Set up progress reporting
        n_prog_reports = 2
        if self._can_ws_name is not None:
            n_prog_reports += 1
        prog = Progress(self, 0.0, 1.0, n_prog_reports)

        sample_wave_ws = '__sam_wave'
        self._convert_to_wavelength(self._sample_ws_name, sample_wave_ws)

        prog.report('Calculating sample corrections')
        CylinderMonteCarloAbsorption(InputWorkspace=sample_wave_ws,
                                     OutputWorkspace=self._ass_ws,
                                     ChemicalFormula=self._sample_chemical_formula,
                                     DensityType=self._sample_density_type,
                                     Density=self._sample_density,
                                     Radius=self._sample_radius,
                                     BeamHeight=self._beam_height,
                                     BeamWidth=self._beam_width,
                                     EventsPerPoint=self._events,
                                     NumberOfWavelengthPoints=self._number_wavelengths,
                                     Interpolation=self._interpolation)
        self._convert_from_wavelength(self._ass_ws, self._ass_ws)
        group = self._ass_ws

        if self._can_ws_name is not None:
            can_wave_ws = '__can_wave'
            self._convert_to_wavelength(self._can_ws_name, can_wave_ws)

            prog.report('Calculating container corrections')
            AnnulusMonteCarloAbsorption(InputWorkspace=can_wave_ws,
                                        OutputWorkspace=self._acc_ws,
                                        ChemicalFormula=self._can_chemical_formula,
                                        DensityType=self._can_density_type,
                                        Density=self._can_density,
                                        Height=self._sample_height,
                                        InnerRadius=self._can_inner_radius,
                                        OuterRadius=self._can_outer_radius,
                                        BeamHeight=self._beam_height,
                                        BeamWidth=self._beam_width,
                                        EventsPerPoint=self._events,
                                        NumberOfWavelengthPoints=self._number_wavelengths,
                                        Interpolation=self._interpolation)

            self._convert_from_wavelength(self._acc_ws, self._acc_ws)
            group += ',' + self._acc_ws
            self._delete_ws(can_wave_ws)

        self._delete_ws(sample_wave_ws)

        prog.report('Recording sample logs')
        sample_log_workspaces = [self._ass_ws]
        sample_logs = [('sample_shape', 'cylinder'),
                       ('sample_filename', self._sample_ws_name),
                       ('sample_height', self._sample_height),
                       ('sample_radius', self._sample_radius)]

        if self._can_ws_name is not None:
            sample_logs.append(('container_filename', self._can_ws_name))
            sample_log_workspaces.append(self._acc_ws)
            sample_logs.append(('container_inner_radius', self._can_inner_radius))
            sample_logs.append(('container_outer_radius', self._can_outer_radius))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        for ws_name in sample_log_workspaces:
            self._add_sample_log_mult(ws_name, log_names, log_values)

        # Output the Ass workspace
        self._group_ws(group, self._abs_ws)
        self.setProperty('CorrectionsWorkspace', self._abs_ws)

    def _setup(self):
        """
        Get algorithm properties.
        """

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._unit = mtd[self._sample_ws_name].getAxis(0).getUnit().unitID()
        logger.information('Input X-unit is %s' % self._unit)
        if self._unit == 'dSpacing':
            self._emode = 'Elastic'
        else:
            self._emode = str(mtd[self._sample_ws_name].getEMode())
        if self._emode == 'Indirect' or 'Direct':
            self._efixed = self._get_Efixed()

        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_density_type = self.getProperty('SampleDensityType').value
        self._sample_density = self.getProperty('SampleDensity').value
        sample_height = self.getProperty('SampleHeight').value
        if sample_height == '':
            self._sample_height = self._beam_height
        else:
            self._sample_height = sample_height
        self._sample_radius = self.getProperty('SampleRadius').value

        self._can_ws_name = self.getPropertyValue('ContainerWorkspace')
        if self._can_ws_name == '':
            self._can_ws_name = None
        self._can_chemical_formula = self.getPropertyValue('ContainerChemicalFormula')
        self._can_density_type = self.getProperty('ContainerDensityType').value
        self._can_density = self.getProperty('ContainerDensity').value
        self._can_inner_radius = self.getProperty('ContainerInnerRadius').value
        self._can_outer_radius = self.getProperty('ContainerOuterRadius').value

        self._number_wavelengths = self.getProperty('NumberOfWavelengthPoints').value
        self._events = self.getProperty('EventsPerPoint').value
        self._interpolation = 'CSpline'

        self._abs_ws = self.getPropertyValue('CorrectionsWorkspace')
        self._ass_ws = self._abs_ws + '_ass'
        self._acc_ws = self._abs_ws + '_acc'

    def validateInputs(self):
        """
        Validate algorithm options.
        """

        self._setup()
        issues = dict()

        if self._can_ws_name is not None:
            can_unit = mtd[self._can_ws_name].getAxis(0).getUnit().unitID()
            if can_unit != self._unit:
                raise ValueError('Sample and container unit NOT the same')

            if self._can_inner_radius <= self._sample_radius:
                issues['ContainerInnerRadius'] = 'Must be greater than SampleRadius'
            if self._can_outer_radius <= self._can_inner_radius:
                issues['ContainerOuterRadius'] = 'Must be greater than ContainerInnerRadius'

            if self._can_chemical_formula == '':
                issues['ContainerChemicalFormula'] = 'Must be set to use container corrections'

        return issues

    def _get_Efixed(self):
        inst = mtd[self._sample_ws_name].getInstrument()

        if inst.hasParameter('Efixed'):
            return inst.getNumberParameter('EFixed')[0]

        if inst.hasParameter('analyser'):
            analyser_name = inst.getStringParameter('analyser')[0]
            analyser_comp = inst.getComponentByName(analyser_name)

            if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
                return analyser_comp.getNumberParameter('EFixed')[0]

        raise ValueError('No Efixed parameter found')

    def _convert_to_wavelength(self, input_ws, output_ws):
        convert_unit_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)

        if self._unit == 'Wavelength':
            self._clone_ws(input_ws, output_ws)
        else:
            convert_unit_alg.setProperty("InputWorkspace", input_ws)
            convert_unit_alg.setProperty("OutputWorkspace", output_ws)
            convert_unit_alg.setProperty("Target", 'Wavelength')
            convert_unit_alg.setProperty("EMode", self._emode)
            if self._emode == 'Indirect':
                convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            mtd.addOrReplace(output_ws, convert_unit_alg.getProperty("OutputWorkspace").value)

    def _convert_from_wavelength(self, input_ws, output_ws):
        convert_unit_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)

        if self._unit != 'Wavelength':
            convert_unit_alg.setProperty("InputWorkspace", input_ws)
            convert_unit_alg.setProperty("OutputWorkspace", output_ws)
            convert_unit_alg.setProperty("Target", self._unit)
            convert_unit_alg.setProperty("EMode", self._emode)
            if self._emode == 'Indirect':
                convert_unit_alg.setProperty("EFixed", self._efixed)
            convert_unit_alg.execute()
            mtd.addOrReplace(output_ws, convert_unit_alg.getProperty("OutputWorkspace").value)

    def _clone_ws(self, input_ws, output_ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.setProperty("OutputWorkspace", output_ws)
        clone_alg.execute()
        mtd.addOrReplace(output_ws, clone_alg.getProperty("OutputWorkspace").value)

    def _delete_ws(self, input_ws):
        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", input_ws)
        delete_alg.execute()

    def _group_ws(self, input_ws, output_ws):
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", input_ws)
        group_alg.setProperty("OutputWorkspace", output_ws)
        group_alg.execute()
        mtd.addOrReplace(output_ws, group_alg.getProperty("OutputWorkspace").value)

    def _add_sample_log_mult(self, input_ws, log_names, log_values):
        sample_log_mult_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        sample_log_mult_alg.setProperty("Workspace", input_ws)
        sample_log_mult_alg.setProperty("LogNames", log_names)
        sample_log_mult_alg.setProperty("LogValues", log_values)
        sample_log_mult_alg.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateCylinderMCAbsorption)
