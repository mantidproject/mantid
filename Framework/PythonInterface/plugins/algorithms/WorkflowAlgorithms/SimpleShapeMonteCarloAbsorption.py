from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
import os.path

from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress, SpectraAxis,
                        Transpose, mtd)
from mantid.kernel import (VisibleWhenProperty, EnabledWhenProperty, PropertyCriterion, logger, config,
                           StringListValidator, IntBoundedValidator, FloatBoundedValidator, Direction)
from mantid.simpleapi import (SetBeam, SetSample, MonteCarloAbsorption, LoadInstrument, UpdateInstrumentFromFile,
                              ConvertToHistogram)


class SimpleShapeMonteCarloAbsorption(DataProcessorAlgorithm):
    # basic sample variables
    _input_ws_name = None
    _chemical_formula = None
    _density_type = None
    _density = None
    _shape = None
    _height = None

    # general variables
    _events = None
    _interpolation = None
    _output_ws = None

    # beam variables
    _beam_height = None
    _beam_width = None

    # flat plate variables
    _width = None
    _thickness = None
    _center = None
    _angle = None

    # cylinder variables
    _radius = None

    # annulus variables
    _inner_radius = None
    _outer_radius = None

    def category(self):
        return 'Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS'

    def summary(self):
        return 'Calculates absorption corrections for a given sample shape.'

    def PyInit(self):
        # basic sample options

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='Input workspace')

        self.declareProperty(name="Elastic", defaultValue=False,
                             doc="Select this option to perform elastic absorption corrections;"
                                 " uses elastic instrument definition file.")

        self.declareProperty(name='MaterialAlreadyDefined', defaultValue=False,
                             doc='Select this option if the material has already been defined')

        material_defined_prop = EnabledWhenProperty('MaterialAlreadyDefined', PropertyCriterion.IsDefault)

        self.declareProperty(name='ChemicalFormula', defaultValue='',
                             doc='Chemical formula of sample')
        self.setPropertySettings('ChemicalFormula', material_defined_prop)

        self.declareProperty(name='DensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density')
        self.setPropertySettings('DensityType', material_defined_prop)

        self.declareProperty(name='Density', defaultValue=0.1,
                             doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3)')
        self.setPropertySettings('Density', material_defined_prop)

        # -------------------------------------------------------------------------------------------

        # Monte Carlo options
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')

        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')

        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(['Linear', 'CSpline']),
                             doc='Type of interpolation')

        # -------------------------------------------------------------------------------------------

        # Beam size
        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')

        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        # -------------------------------------------------------------------------------------------

        # set up shape options

        self.declareProperty(name='Shape', defaultValue='FlatPlate',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Geometry of sample environment. Options are: FlatPlate, Cylinder, Annulus')

        flat_plate_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'FlatPlate')
        cylinder_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Cylinder')
        annulus_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Annulus')

        # height is common to all options

        self.declareProperty(name='Height', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the sample environment (cm)')

        # flat plate options

        self.declareProperty(name='Width', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the FlatPlate sample environment (cm)')
        self.setPropertySettings('Width', flat_plate_condition)

        self.declareProperty(name='Thickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(),
                             doc='Thickness of the FlatPlate sample environment (cm)')
        self.setPropertySettings('Thickness', flat_plate_condition)

        self.declareProperty(name='Center', defaultValue=0.0,
                             doc='Center of the FlatPlate sample environment')
        self.setPropertySettings('Center', flat_plate_condition)

        self.declareProperty(name='Angle', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Angle of the FlatPlate sample environment with respect to the beam (degrees)')
        self.setPropertySettings('Angle', flat_plate_condition)

        # cylinder options

        self.declareProperty(name='Radius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Radius of the Cylinder sample environment (cm)')
        self.setPropertySettings('Radius', cylinder_condition)

        # annulus options

        self.declareProperty(name='OuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the Annulus sample environment (cm)')
        self.setPropertySettings('OuterRadius', annulus_condition)

        self.declareProperty(name='InnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the Annulus sample environment (cm)')
        self.setPropertySettings('InnerRadius', annulus_condition)

        # -------------------------------------------------------------------------------------------

        # Output options
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrected workspace.')

    def PyExec(self):
        # setup progress reporting
        prog = Progress(self, 0.0, 1.0, 3)

        if self._elastic:
            self._check_qaxis()

        prog.report('Setting up sample environment')

        # set the beam shape
        SetBeam(self._input_ws_name,
                Geometry={'Shape': 'Slit',
                          'Width': self._beam_width,
                          'Height': self._beam_height})

        # set the sample geometry
        sample_geometry = dict()
        sample_geometry['Height'] = self._height

        if self._shape == 'FlatPlate':
            sample_geometry['Shape'] = 'FlatPlate'
            sample_geometry['Width'] = self._width
            sample_geometry['Thick'] = self._thickness
            sample_geometry['Center'] = [0.0, 0.0, self._center]
            sample_geometry['Angle'] = self._angle

        if self._shape == 'Cylinder':
            sample_geometry['Shape'] = 'Cylinder'
            sample_geometry['Radius'] = self._radius
            sample_geometry['Center'] = [0.0, 0.0, 0.0]

        if self._shape == 'Annulus':
            sample_geometry['Shape'] = 'HollowCylinder'
            sample_geometry['InnerRadius'] = self._inner_radius
            sample_geometry['OuterRadius'] = self._outer_radius
            sample_geometry['Center'] = [0.0, 0.0, 0.0]
            sample_geometry['Axis'] = 1

        # set sample
        if self._material_defined:
            # set sample without sample material
            SetSample(InputWorkspace=self._input_ws_name,
                      Geometry=sample_geometry)

        else:
            # set the sample material
            sample_material = dict()
            sample_material['ChemicalFormula'] = self._chemical_formula

            if self._density_type == 'Mass Density':
                sample_material['SampleMassDensity'] = self._density
            if self._density_type == 'Number Density':
                sample_material['SampleNumberDensity'] = self._density

            SetSample(InputWorkspace=self._input_ws_name,
                      Geometry=sample_geometry,
                      Material=sample_material)

        prog.report('Calculating sample corrections')

        MonteCarloAbsorption(InputWorkspace=self._input_ws_name,
                             OutputWorkspace=self._output_ws,
                             EventsPerPoint=self._events,
                             NumberOfWavelengthPoints=self._number_wavelengths,
                             Interpolation=self._interpolation)

        prog.report('Recording Sample Logs')

        log_names = ['beam_height', 'beam_width']
        log_values = [self._beam_height, self._beam_width]

        # add sample geometry to sample logs
        for key, value in sample_geometry.items():
            log_names.append('sample_' + key.lower())
            log_values.append(value)

        add_sample_log_alg = self.createChildAlgorithm('AddSampleLogMultiple', enableLogging=False)
        add_sample_log_alg.setProperty('Workspace', self._output_ws)
        add_sample_log_alg.setProperty('LogNames', log_names)
        add_sample_log_alg.setProperty('LogValues', log_values)
        add_sample_log_alg.execute()

        if self._elastic and self._q_axis == 'X':
            Transpose(InputWorkspace=self._output_ws,
                      OutputWorkspace=self._output_ws)
            mtd[self._output_ws].setX(0, self._q_values)
            mtd[self._output_ws].getAxis(0).setUnit("MomentumTransfer")

        self.setProperty('OutputWorkspace', self._output_ws)

    def _setup(self):

        # basic options
        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._elastic = self.getProperty("Elastic").value
        self._material_defined = self.getProperty('MaterialAlreadyDefined').value
        self._chemical_formula = self.getPropertyValue('ChemicalFormula')
        self._density_type = self.getPropertyValue('DensityType')
        self._density = self.getProperty('Density').value
        self._shape = self.getPropertyValue('Shape')

        self._number_wavelengths = self.getProperty('NumberOfWavelengthPoints').value
        self._events = self.getProperty('EventsPerPoint').value
        self._interpolation = self.getProperty('Interpolation').value

        # beam options
        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

        # shape options
        self._height = self.getProperty('Height').value

        # flat plate
        self._width = self.getProperty('Width').value
        self._thickness = self.getProperty('Thickness').value
        self._center = self.getProperty('Center').value
        self._angle = self.getProperty('Angle').value

        # cylinder
        self._radius = self.getProperty('Radius').value

        # annulus
        self._inner_radius = self.getProperty('InnerRadius').value
        self._outer_radius = self.getProperty('OuterRadius').value

        # output
        self._output_ws = self.getPropertyValue('OutputWorkspace')

    def validateInputs(self):

        self._setup()
        issues = dict()

        if (not self._material_defined) and (not self._chemical_formula):
            issues['ChemicalFormula'] = 'Please enter a chemical formula'

        if not self._height:
            issues['Height'] = 'Please enter a non-zero number for height'

        if self._shape == 'FlatPlate':
            if not self._width:
                issues['Width'] = 'Please enter a non-zero number for width'
            if not self._thickness:
                issues['Thickness'] = 'Please enter a non-zero number for thickness'

        if self._shape == 'Cylinder':
            if not self._radius:
                issues['Radius'] = 'Please enter a non-zero number for radius'

        if self._shape == 'Annulus':
            if not self._inner_radius:
                issues['InnerRadius'] = 'Please enter a non-zero number for inner radius'
            if not self._outer_radius:
                issues['OuterRadius'] = 'Please enter a non-zero number for outer radius'

            # Geometry validation: outer radius > inner radius
            if not self._outer_radius > self._inner_radius:
                issues['OuterRadius'] = 'Must be greater than InnerRadius'

        return issues

    def _check_qaxis(self):
        self._emode = mtd[self._input_ws_name].getEMode()
        logger.information('Emode is %s' % self._emode)

        # elastic indirect
        if self._emode == 'Indirect':
            self._efixed = self._get_efixed()
            logger.information('Efixed is %f' % self._efixed)
            x_unit = mtd[self._input_ws_name].getAxis(0).getUnit().unitID()
            y_unit = mtd[self._input_ws_name].getAxis(1).getUnit().unitID()

            if x_unit == 'MomentumTransfer':
                self._q_axis = 'X'
                logger.information('Input X-unit is Q')
                axis = mtd[self._input_ws_name].getAxis(0)
            elif y_unit == 'MomentumTransfer':
                self._q_axis = 'Y'
                logger.information('Input Y-unit is Q')
                axis = mtd[self._input_ws_name].getAxis(1)
            else:
                logger.warning('Q-Values not found in input workspace.')
                return

            self._q_values = axis.extractValues()

            if self._q_axis == 'X':
                Transpose(InputWorkspace=self._input_ws_name,
                          OutputWorkspace=self._input_ws_name)
            self._create_waves(self._input_ws_name)

    def _get_efixed(self):
        inst = mtd[self._input_ws_name].getInstrument()

        if inst.hasParameter('Efixed'):
            return inst.getNumberParameter('EFixed')[0]

        if inst.hasParameter('analyser'):
            analyser_name = inst.getStringParameter('analyser')[0]
            analyser_comp = inst.getComponentByName(analyser_name)

            if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
                return analyser_comp.getNumberParameter('EFixed')[0]

        raise ValueError('No Efixed parameter found')

    def _create_waves(self, input_ws):
        # ---------- Load Elastic Instrument Definition File ----------

        ws = mtd[input_ws]
        idf_name = ws.getInstrument().getName() + '_Elastic_Definition.xml'
        idf_path = os.path.join(config.getInstrumentDirectory(), idf_name)
        logger.information('IDF = %s' % idf_path)
        LoadInstrument(Workspace=input_ws,
                       Filename=idf_path,
                       RewriteSpectraMap=True)

        # Replace y-axis with spectra axis
        ws.replaceAxis(1, SpectraAxis.create(ws))
        e_fixed = float(self._efixed)
        logger.information('Efixed = %f' % e_fixed)

        # ---------- Set Instrument Parameters ----------

        sip_alg = self.createChildAlgorithm("SetInstrumentParameter", enableLogging=False)
        sip_alg.setProperty("Workspace", input_ws)
        sip_alg.setProperty("ParameterName", 'EFixed')
        sip_alg.setProperty("ParameterType", 'Number')
        sip_alg.setProperty("Value", str(e_fixed))
        sip_alg.execute()

        # ---------- Calculate Wavelength ----------

        self._wave = math.sqrt(81.787 / e_fixed)
        logger.information('Wavelength = %f' % self._wave)
        ws.getAxis(0).setUnit('Wavelength')

        # ---------- Format Input Workspace ---------

        ConvertToHistogram(InputWorkspace=input_ws,
                           OutputWorkspace=input_ws)
        self._crop_ws(input_ws, input_ws)

        # --------- Set wavelengths as X-values in Input Workspace ----------

        ws = mtd[input_ws]
        waves = (0.01 * np.arange(-1, ws.blocksize()-1)) + self._wave
        logger.information('waves : ' + str(waves))
        nhist = ws.getNumberHistograms()
        for idx in range(nhist):
            ws.setX(idx, waves)
        self._change_angles(input_ws)

    def _change_angles(self, input_ws):
        work_dir = config['defaultsave.directory']
        k0 = 4.0 * math.pi / self._wave
        theta = 2.0 * np.degrees(np.arcsin(self._q_values / k0))  # convert to angle

        filename = 'Elastic_angles.txt'
        path = os.path.join(work_dir, filename)
        logger.information('Creating angles file : ' + path)
        handle = open(path, 'w')
        head = 'spectrum,theta'
        handle.write(head + " \n")
        for n in range(0, len(theta)):
            handle.write(str(n + 1) + '   ' + str(theta[n]) + "\n")
        handle.close()
        UpdateInstrumentFromFile(Workspace=input_ws,
                                 Filename=path,
                                 MoveMonitors=False,
                                 IgnorePhi=True,
                                 AsciiHeader=head,
                                 SkipFirstNLines=1)

    def _crop_ws(self, input_ws, output_ws):
        x = mtd[input_ws].dataX(0)
        xmin = x[0]
        xmax = x[1]
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", input_ws)
        crop_alg.setProperty("OutputWorkspace", output_ws)
        crop_alg.setProperty("XMin", xmin)
        crop_alg.setProperty("XMax", xmax)
        crop_alg.execute()
        mtd.addOrReplace(output_ws, crop_alg.getProperty("OutputWorkspace").value)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SimpleShapeMonteCarloAbsorption)
