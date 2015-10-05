#pylint: disable=no-init,too-many-locals,too-many-instance-attributes

from mantid.simpleapi import *
from mantid.api import (PythonAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator)
from mantid.kernel import (StringListValidator, StringMandatoryValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator)
from mantid import config

import math
import numpy as np


class CylinderPaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_number_density = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _usecan = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_number_density = None
    _can_outer_radius = None
    _step_size =  None
    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _output_ws_name = None
    _use_can = None
    _beam_height = None
    _beam_width = None
    _interpolate = None
    _angles = None
    _waves = None
    _elastic = None

#------------------------------------------------------------------------------

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms;CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Calculates absorption corrections for a cylindrical or annular sample using Paalman & Pings format."

#------------------------------------------------------------------------------

    def PyInit(self):
        ws_validator = CompositeValidator([WorkspaceUnitValidator('Wavelength'), InstrumentValidator()])

        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     direction=Direction.Input,
                                                     validator=ws_validator),
                             doc='Name for the input sample workspace')

        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')
        self.declareProperty(name='SampleNumberDensity', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample number density in atoms/Angstrom3')
        self.declareProperty(name='SampleInnerRadius', defaultValue=0.05,
                             doc='Sample inner radius')
        self.declareProperty(name='SampleOuterRadius', defaultValue=0.1,
                             doc='Sample outer radius')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional,
                                                     validator=ws_validator),
                             doc="Name for the input container workspace")

        self.declareProperty(name='CanChemicalFormula', defaultValue='',
                             doc='Container chemical formula')
        self.declareProperty(name='CanNumberDensity', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container number density in atoms/Angstrom3')
        self.declareProperty(name='CanOuterRadius', defaultValue=0.15,
                             doc='Can outer radius')

        self.declareProperty(name='BeamHeight', defaultValue=3.0,
                             doc='Height of the beam at the sample.')
        self.declareProperty(name='BeamWidth', defaultValue=2.0,
                             doc='Width of the beam at the sample.')

        self.declareProperty(name='StepSize', defaultValue=0.002,
                             doc='Step size for calculation')
        self.declareProperty(name='Interpolate', defaultValue=True,
                             doc='Interpolate the correction workspaces to match the sample workspace')

        self.declareProperty(name='Emode', defaultValue='Elastic',
                             validator=StringListValidator(['Elastic', 'Indirect']),
                             doc='Emode: Elastic or Indirect')
        self.declareProperty(name='Efixed', defaultValue=1.0,
                             doc='Analyser energy')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output corrections workspace group')

#------------------------------------------------------------------------------

    def PyExec(self):

        from IndirectImport import is_supported_f2py_platform, import_f2py

        if is_supported_f2py_platform():
            cylabs = import_f2py("cylabs")
        else:
            raise RuntimeError('This algorithm is only available on Windows')

        workdir = config['defaultsave.directory']
        self._setup()
        self._wave_range()

        # Set sample material from chemical formula
        SetSampleMaterial(self._sample_ws_name, ChemicalFormula=self._sample_chemical_formula,
                          SampleNumberDensity=self._sample_number_density)
        sample = mtd[self._sample_ws_name].sample()
        sam_material = sample.getMaterial()
        # total scattering x-section
        sigs = [sam_material.totalScatterXSection()]
        # absorption x-section
        siga = [sam_material.absorbXSection()]
        density = [self._sample_number_density, self._can_number_density, self._can_number_density]
        half_width = 0.5*float(self._beam_width)
        beam = [self._beam_height, half_width, -half_width, half_width, -half_width, 0.0, self._beam_height, 0.0, self._beam_height]
        radii = [self._sample_inner_radius, self._sample_outer_radius, self._can_outer_radius, self._can_outer_radius]
        ncan = 0

        # If using a can, set sample material using chemical formula
        if self._use_can:
            ncan = 2
            SetSampleMaterial(InputWorkspace=self._can_ws_name, ChemicalFormula=self._can_chemical_formula,
                              SampleNumberDensity=self._can_number_density)
            can_sample = mtd[self._can_ws_name].sample()
            can_material = can_sample.getMaterial()

            # total scattering x-section for can
            sigs.append(can_material.totalScatterXSection())
            sigs.append(can_material.totalScatterXSection())
            # absorption x-section for can
            siga.append(can_material.absorbXSection())
            siga.append(can_material.absorbXSection())

        else:
            # total scattering x-section for can
            sigs.append(0.0)
            sigs.append(0.0)
            # absorption x-section for can
            siga.append(0.0)
            siga.append(0.0)

        # Holders for the corrected data
        data_ass = []
        data_assc = []
        data_acsc = []
        data_acc = []

        # initially set errors to zero
        wrk = workdir + self._can_ws_name
        self._get_angles()
        number_angles = len(self._angles)

        for angle_idx in range(number_angles):
            kill, ass, assc, acsc, acc = cylabs.cylabs(self._step_size, beam, ncan, radii,
                                                       density, sigs, siga, self._angles[angle_idx],
                                                       self._elastic, self._waves, angle_idx, wrk, 0)
            if kill == 0:
                logger.information('Angle %d: %f successful' % (angle_idx+1, self._angles[angle_idx]))

                data_ass = np.append(data_ass, ass)
                data_assc = np.append(data_assc, assc)
                data_acsc = np.append(data_acsc, acsc)
                data_acc = np.append(data_acc, acc)

            else:
                raise ValueError('Angle ' + str(angle_idx) + ' : ' + str(self._angles[angle_idx]) + ' *** failed : Error code ' + str(kill))

        sample_logs = {'sample_shape': 'cylinder', 'sample_filename': self._sample_ws_name,
                       'sample_inner_radius': self._sample_inner_radius, 'sample_outer_radius': self._sample_outer_radius}
        dataX = self._waves * number_angles

        # Create the output workspaces
        ass_ws = self._output_ws_name + '_ass'

        CreateWorkspace(OutputWorkspace=ass_ws, DataX=dataX, DataY=data_ass,
                        NSpec=number_angles, UnitX='Wavelength')
        self._add_sample_logs(ass_ws, sample_logs)
        workspaces = [ass_ws]

        if self._use_can:
            sample_logs['can_filename'] = self._can_ws_name
            sample_logs['can_outer_radius'] = self._can_outer_radius

            assc_ws = self._output_ws_name + '_assc'
            workspaces.append(assc_ws)
            CreateWorkspace(OutputWorkspace=assc_ws, DataX=dataX, DataY=data_assc,
                            NSpec=number_angles, UnitX='Wavelength')
            self._add_sample_logs(assc_ws, sample_logs)

            acsc_ws = self._output_ws_name + '_acsc'
            workspaces.append(acsc_ws)
            CreateWorkspace(OutputWorkspace=acsc_ws, DataX=dataX, DataY=data_acsc,
                            NSpec=number_angles, UnitX='Wavelength')
            self._add_sample_logs(acsc_ws, sample_logs)

            acc_ws = self._output_ws_name + '_acc'
            workspaces.append(acc_ws)
            CreateWorkspace(OutputWorkspace=acc_ws, DataX=dataX, DataY=data_acc,
                            NSpec=number_angles, UnitX='Wavelength')
            self._add_sample_logs(acc_ws, sample_logs)

        if self._interpolate:
            self._interpolate_corrections(workspaces)

        try:
            self. _copy_detector_table(workspaces)
        except RuntimeError:
            logger.warning('Cannot copy spectra mapping. Check input workspace instrument.')

        GroupWorkspaces(InputWorkspaces=','.join(workspaces), OutputWorkspace=self._output_ws_name)
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)

#------------------------------------------------------------------------------

    def validateInputs(self):
        self._setup()
        issues = dict()

        # Ensure that a can chemical formula is given when using a can workspace
        if self._use_can:
            can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
            if can_chemical_formula == '':
                issues['CanChemicalFormula'] = 'Must provide a chemical foruma when providing a can workspace'

        # Ensure there are enough steps
        number_steps = int((self._sample_outer_radius - self._sample_inner_radius) / self._step_size)
        if number_steps < 20:
            issues['StepSize'] = 'Number of steps (%d) should be >= 20' % number_steps

        return issues

#------------------------------------------------------------------------------

    def _setup(self):
        """
        Get algorithm properties.
        """

        # This is fixed by the Fortran code
        self._number_wavelengths = 10

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')

        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_number_density = self.getProperty('SampleNumberDensity').value
        self._sample_inner_radius = self.getProperty('SampleInnerRadius').value
        self._sample_outer_radius = self.getProperty('SampleOuterRadius').value

        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''

        self._can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
        self._can_number_density = self.getProperty('CanNumberDensity').value
        self._can_outer_radius = self.getProperty('CanOuterRadius').value

        self._step_size = self.getProperty('StepSize').value

        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

        self._interpolate = self.getProperty('Interpolate').value

        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

#------------------------------------------------------------------------------

    def _get_angles(self):
        """
        Populates the list of workspace angles.
        """

        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = list()
        for index in range(0, num_hist):
            detector = mtd[self._sample_ws_name].getDetector(index)
            two_theta = detector.getTwoTheta(sample_pos, beam_pos) * 180.0 / math.pi  # calc angle
            self._angles.append(two_theta)

#------------------------------------------------------------------------------

    def _wave_range(self):
        wave_range = '__WaveRange'
        ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)
        Xin = mtd[wave_range].readX(0)
        wave_min = mtd[wave_range].readX(0)[0]
        wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
        number_waves = int(self._number_wavelengths)
        wave_bin = (wave_max - wave_min) / (number_waves-1)

        self._waves = list()
        for idx in range(0, number_waves):
            self._waves.append(wave_min + idx * wave_bin)

        if self._emode == 'Elastic':
            self._elastic = self._waves[int(number_waves / 2)]
        elif self._emode == 'Indirect':
            self._elastic = math.sqrt(81.787 / self._efixed)  # elastic wavelength

        logger.information('Elastic lambda %f' % self._elastic)
        DeleteWorkspace(wave_range)

#------------------------------------------------------------------------------

    def _interpolate_corrections(self, workspaces):
        """
        Performs interpolation on the correction workspaces such that the number of bins
        matches that of the input sample workspace.

        @param workspaces List of correction workspaces to interpolate
        """

        for wrksp in workspaces:
            SplineInterpolation(WorkspaceToMatch=self._sample_ws_name,
                                WorkspaceToInterpolate=wrksp,
                                OutputWorkspace=wrksp,
                                OutputWorkspaceDeriv='')

#------------------------------------------------------------------------------

    def _copy_detector_table(self, workspaces):
        """
        Copy the detector table from the sample workspaces to the correction workspaces.

        @param workspaces List of correction workspaces
        """

        instrument = mtd[self._sample_ws_name].getInstrument().getName()

        for wrksp in workspaces:
            LoadInstrument(Workspace=wrksp,
                           InstrumentName=instrument)

            CopyDetectorMapping(WorkspaceToMatch=self._sample_ws_name,
                                WorkspaceToRemap=wrksp,
                                IndexBySpectrumNumber=True)

#------------------------------------------------------------------------------

    def _add_sample_logs(self, wrksp, sample_logs):
        """
        Add a dictionary of logs to a workspace.

        The type of the log is inferred by the type of the value passed to the log.

        @param wrksp - workspace to add logs too.
        @param sample_logs - dictionary of logs to append to the workspace.
        """

        for key, value in sample_logs.iteritems():
            if isinstance(value, bool):
                log_type = 'String'
            elif isinstance(value, (int, long, float)):
                log_type = 'Number'
            else:
                log_type = 'String'

            AddSampleLog(Workspace=wrksp, LogName=key, LogType=log_type, LogText=str(value))

#------------------------------------------------------------------------------

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CylinderPaalmanPingsCorrection)
