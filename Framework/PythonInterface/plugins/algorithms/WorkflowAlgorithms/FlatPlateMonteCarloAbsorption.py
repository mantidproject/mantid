from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import SetBeam, SetSample, MonteCarloAbsorption, GroupWorkspaces
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        PropertyMode, Progress, WorkspaceGroupProperty)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           Direction, logger, FloatBoundedValidator, MaterialBuilder, CompositeValidator)


class FlatPlateMonteCarloAbsorption(DataProcessorAlgorithm):

    # Sample variables
    _input_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_height = None
    _sample_width = None
    _sample_thickness = None
    _sample_angle = None

    _beam_height = None
    _beam_width = None

    _events = None
    _interpolation = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates absorption corrections for a flat sample shape."

    def PyInit(self):
        # Sample
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='Input workspace')
        self.declareProperty(name='ChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Chemical formula for the sample')
        self.declareProperty(name='DensityType', defaultValue = 'Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc = 'Use of Mass density or Number density')
        self.declareProperty(name='Density', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3)')
        self.declareProperty(name='Height', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample height')
        self.declareProperty(name='Width', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample width')
        self.declareProperty(name='Thickness', defaultValue=0.5,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample thickness')
        self.declareProperty(name='Center', defaultValue=0.,
                             doc='Sample angle')
        self.declareProperty(name='Angle', defaultValue=0.0,
                             doc='Sample angle')

        # Beam size
        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')
        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        # Monte Carlo
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')
        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(['Linear', 'CSpline']),
                             doc='Type of interpolation')

        # Output
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', 'Corrections', direction=Direction.Output),
                             doc='The corrections workspace')

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
                  Geometry={'Shape': 'FlatPlate',
                            'Width': self._sample_width,
                            'Height': self._sample_height,
                            'Thick': self._sample_thickness,
							'Center': [0.,0.,self._sample_center],
                            'Angle': self._sample_angle},
                  Material=sample_mat_list)

        prog.report('Calculating sample corrections')
        MonteCarloAbsorption(InputWorkspace=self._input_ws_name,
                             OutputWorkspace=self._output_ws,
                             EventsPerPoint=self._events,
                             NumberOfWavelengthPoints=self._number_wavelengths,
                             Interpolation=self._interpolation)

        prog.report('Recording sample logs')
        sample_logs = [('sample_shape', 'flatplate'),
                       ('sample_height', self._sample_height),
                       ('sample_width', self._sample_width),
                       ('sample_thickness', self._sample_thickness),
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
        self._sample_height = self.getProperty('Height').value
        self._sample_width = self.getProperty('Width').value
        self._sample_thickness = self.getProperty('Thickness').value
        self._sample_center = self.getProperty('Center').value
        self._sample_angle = self.getProperty('Angle').value

        self._number_wavelengths = self.getProperty('NumberOfWavelengthPoints').value
        self._events = self.getProperty('EventsPerPoint').value
        self._interpolation = self.getProperty('Interpolation').value

        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

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
AlgorithmFactory.subscribe(FlatPlateMonteCarloAbsorption)
