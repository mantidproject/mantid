from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import SetBeam, SetSample, MonteCarloAbsorption, GroupWorkspaces
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, PropertyMode, Progress, mtd)
from mantid.kernel import (StringMandatoryValidator, Direction, logger, FloatBoundedValidator,
                           IntBoundedValidator, StringListValidator)


class CylinderMonteCarloAbsorption(DataProcessorAlgorithm):
    # Sample variables
    _input_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_height = None
    _sample_radius = None

    _beam_height = None
    _beam_width = None

    _events = None
    _interpolation = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates absorption corrections for a cylinder sample shape."

    def PyInit(self):
        # Sample options
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='Input workspace.')
        self.declareProperty(name='ChemicalFormula', defaultValue='', validator=StringMandatoryValidator(),
                             doc='Chemical formula')
        self.declareProperty(name='DensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass Density or Number density')
        self.declareProperty(name='Density', defaultValue=0.1,
                             doc='Mass Density (g/cm^3) or Number density (atoms/Angstrom^3)')
        self.declareProperty(name='Radius', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Radius')
        self.declareProperty(name='Height', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height')

        # Beam size
        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')
        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        # Monte Carlo options
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='EventsPerPoint', defaultValue=5000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')
        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(['Linear', 'CSpline']),
                             doc='Type of interpolation')

        # Output options
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrected workspace.')

    def PyExec(self):

        # Set up progress reporting
        prog = Progress(self, 0.0, 1.0, 2)

        SetBeam(self._input_ws_name,
                Geometry={'Shape': 'Slit',
                          'Width': self._beam_width,
                          'Height': self._beam_height})

        if self._sample_density_type == 'Mass Density':
            sample_mat_list = {'ChemicalFormula': self._sample_chemical_formula,
                               'SampleMassDensity': self._sample_density}
        if self._sample_density_type == 'Number Density':
            sample_mat_list = {'ChemicalFormula': self._sample_chemical_formula,
                               'SampleNumberDensity': self._sample_density}

        SetSample(self._input_ws_name,
                  Geometry={'Shape': 'Cylinder',
                            'Height': self._sample_height,
                            'Radius': self._sample_radius,
                            'Center': [0., 0., 0.]},
                  Material=sample_mat_list)

        prog.report('Calculating sample corrections')
        MonteCarloAbsorption(InputWorkspace=self._input_ws_name,
                             OutputWorkspace=self._output_ws,
                             EventsPerPoint=self._events,
                             NumberOfWavelengthPoints=self._number_wavelengths,
                             Interpolation=self._interpolation)

        # Record sample logs
        prog.report('Recording sample logs')
        sample_logs = [('sample_shape', 'cylinder'),
                       ('sample_radius', self._sample_radius),
                       ('sample_height', self._sample_height),
                       ('beam_height', self._beam_height),
                       ('beam_width', self._beam_width)]

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        add_sample_log_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        add_sample_log_alg.setProperty("Workspace", self._output_ws)
        add_sample_log_alg.setProperty("LogNames", log_names)
        add_sample_log_alg.setProperty("LogValues", log_values)
        add_sample_log_alg.execute()

        self.setProperty('OutputWorkspace', self._output_ws)

    def _setup(self):
        """
        Get algorithm properties.
        """

        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._sample_chemical_formula = self.getPropertyValue('ChemicalFormula')
        self._sample_density_type = self.getPropertyValue('DensityType')
        self._sample_density = self.getProperty('Density').value
        self._sample_radius = self.getProperty('Radius').value
        self._sample_height = self.getProperty('Height').value

        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

        self._number_wavelengths = self.getProperty('NumberOfWavelengthPoints').value
        self._events = self.getProperty('EventsPerPoint').value
        self._interpolation = self.getProperty('Interpolation').value

        self._output_ws = self.getPropertyValue('OutputWorkspace')

    def validateInputs(self):
        """
        Validate algorithm options.
        """

        self._setup()
        issues = dict()

        if self._input_ws_name == '':
            issues['InputWorkspace'] = 'Input workspace must be defined'
        if self._sample_chemical_formula == '':
            issues['ChemicalFormula'] = 'Sample chemical formula must be defined'

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CylinderMonteCarloAbsorption)
