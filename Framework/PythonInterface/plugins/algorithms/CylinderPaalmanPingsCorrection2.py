#pylint: disable=no-init,too-many-locals,too-many-instance-attributes,too-many-arguments,invalid-name
from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
from mantid.simpleapi import *
from mantid.api import (PythonAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator, Progress)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator)


class CylinderPaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_number_density = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _use_can = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_number_density = None
    _can_outer_radius = None
    _number_can = 1
    _ms = 1
    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _step_size = None
    _output_ws_name = None
    _beam = list()
    _angles = list()
    _waves = list()
    _elastic = 0.0
    _sig_s = None
    _sig_a = None
    _density = None
    _radii = None
    _interpolate = False

#------------------------------------------------------------------------------

    def version(self):
        return 2

    def category(self):
        return "Workflow\\MIDAS;CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Calculates absorption corrections for a cylindrical or annular sample using Paalman & Pings format."

#------------------------------------------------------------------------------

    def PyInit(self):
        ws_validator = CompositeValidator([WorkspaceUnitValidator('Wavelength'), InstrumentValidator()])

        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     validator=ws_validator,
                                                     direction=Direction.Input),
                             doc="Name for the input Sample workspace.")

        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')
        self.declareProperty(name='SampleNumberDensity', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample number density')
        self.declareProperty(name='SampleInnerRadius', defaultValue=0.05,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample inner radius')
        self.declareProperty(name='SampleOuterRadius', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample outer radius')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                                                     optional=PropertyMode.Optional,
                                                     validator=ws_validator,
                                                     direction=Direction.Input),
                             doc="Name for the input Can workspace.")

        self.declareProperty(name='CanChemicalFormula', defaultValue='',
                             doc='Can chemical formula')
        self.declareProperty(name='CanNumberDensity', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Can number density')
        self.declareProperty(name='CanOuterRadius', defaultValue=0.15,
                             validator=FloatBoundedValidator(0.0),
                             doc='Can outer radius')

        self.declareProperty(name='BeamHeight', defaultValue=3.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Beam height')
        self.declareProperty(name='BeamWidth', defaultValue=2.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Beam width')

        self.declareProperty(name='StepSize', defaultValue=0.002,
                             validator=FloatBoundedValidator(0.0),
                             doc='Step size')
        self.declareProperty(name='Interpolate', defaultValue=True,
                             doc='Interpolate the correction workspaces to match the sample workspace')
        self.declareProperty(name='NumberWavelengths', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')

        self.declareProperty(name='Emode', defaultValue='Elastic',
                             validator=StringListValidator(['Elastic', 'Indirect', 'Direct']),
                             doc='Energy transfer mode')
        self.declareProperty(name='Efixed', defaultValue=1.0,
                             doc='Analyser energy')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output corrections workspace group')
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

    def PyExec(self):
        self._setup()
        self._sample()
        self._wave_range()
        self._get_angles()
        self._transmission()

        dataA1 = []
        dataA2 = []
        dataA3 = []
        dataA4 = []

        data_prog = Progress(self, start=0.1, end=0.85, nreports=len(self._angles))
        for angle in self._angles:
            (A1, A2, A3, A4) = self._cyl_abs(angle)
            logger.information('Angle : %f * successful' % (angle))
            data_prog.report('Appending data for angle %f' % (angle))
            dataA1 = np.append(dataA1, A1)
            dataA2 = np.append(dataA2, A2)
            dataA3 = np.append(dataA3, A3)
            dataA4 = np.append(dataA4, A4)

        dataX = self._waves * len(self._angles)

        wrk_reports = 5
        if self._use_can:
            wrk_reports = 8
        workflow_prog = Progress(self, start=0.85, end=1.0, nreports=wrk_reports)
        # Create the output workspaces
        ass_ws = self._output_ws_name + '_ass'
        workflow_prog.report('Creating Workspace')
        CreateWorkspace(OutputWorkspace=ass_ws,
                        DataX=dataX,
                        DataY=dataA1,
                        NSpec=len(self._angles),
                        UnitX='Wavelength',
                        ParentWorkspace=self._sample_ws_name)
        workspaces = [ass_ws]

        if self._use_can:
            workflow_prog.report('Creating assc Workspace')
            assc_ws = self._output_ws_name + '_assc'
            workspaces.append(assc_ws)
            CreateWorkspace(OutputWorkspace=assc_ws,
                            DataX=dataX,
                            DataY=dataA2,
                            NSpec=len(self._angles),
                            UnitX='Wavelength',
                            ParentWorkspace=self._sample_ws_name)

            workflow_prog.report('Creating acsc Workspace')
            acsc_ws = self._output_ws_name + '_acsc'
            workspaces.append(acsc_ws)
            CreateWorkspace(OutputWorkspace=acsc_ws,
                            DataX=dataX,
                            DataY=dataA3,
                            NSpec=len(self._angles),
                            UnitX='Wavelength',
                            ParentWorkspace=self._sample_ws_name)

            workflow_prog.report('Creating acc Workspace')
            acc_ws = self._output_ws_name + '_acc'
            workspaces.append(acc_ws)
            CreateWorkspace(OutputWorkspace=acc_ws,
                            DataX=dataX,
                            DataY=dataA4,
                            NSpec=len(self._angles),
                            UnitX='Wavelength',
                            ParentWorkspace=self._sample_ws_name)

        if self._interpolate:
            self._interpolate_corrections(workspaces)

        workflow_prog.report('Constructing Sample Logs')
        sample_log_workspaces = workspaces
        sample_logs = [('sample_shape', 'cylinder'),
                       ('sample_filename', self._sample_ws_name),
                       ('sample_inner', self._sample_inner_radius),
                       ('sample_outer', self._sample_outer_radius)]

        if self._use_can:
            sample_logs.append(('can_filename', self._can_ws_name))
            sample_logs.append(('can_outer', self._can_outer_radius))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]

        for ws_name in sample_log_workspaces:
            workflow_prog.report('Adding sample logs to %s' % ws_name)
            AddSampleLogMultiple(Workspace=ws_name, LogNames=log_names, LogValues=log_values)

        workflow_prog.report('Create GroupWorkpsace Output')
        GroupWorkspaces(InputWorkspaces=','.join(workspaces), OutputWorkspace=self._output_ws_name)
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)
        workflow_prog.report('Algorithm complete')

#------------------------------------------------------------------------------

    def _setup(self):
        setup_prog = Progress(self, start=0.00, end=0.01, nreports=2)
        setup_prog.report('Obtaining input properties')
        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_number_density = self.getProperty('SampleNumberDensity').value
        self._sample_inner_radius = self.getProperty('SampleInnerRadius').value
        self._sample_outer_radius = self.getProperty('SampleOuterRadius').value
        self._number_can = 1

        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''
        self._can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
        self._can_number_density = self.getProperty('CanNumberDensity').value
        self._can_outer_radius = self.getProperty('CanOuterRadius').value
        if self._use_can:
            self._number_can = 2

        self._step_size = self.getProperty('StepSize').value
        self._radii = np.zeros(self._number_can +1)
        self._radii[0] = self._sample_inner_radius
        self._radii[1] = self._sample_outer_radius
        if (self._radii[1] - self._radii[0]) < 1e-4:
            raise ValueError('Sample outer radius not > inner radius')
        else:
            logger.information('Sample : inner radius = %f ; outer radius = %f' % (self._radii[0], self._radii[1]))
            self._ms = int((self._radii[1] - self._radii[0] + 0.0001)/self._step_size)
            if self._ms < 20:
                raise ValueError('Number of steps ( %i ) should be >= 20' % (self._ms))
            else:
                if self._ms < 1:
                    self._ms=1
                logger.information('Sample : ms = %i ' % (self._ms))
        if self._use_can:
            self._radii[2] = self._can_outer_radius
            if (self._radii[2] - self._radii[1]) < 1e-4:
                raise ValueError('Can outer radius not > sample outer radius')
            else:
                logger.information('Can : inner radius = %f ; outer radius = %f' % (self._radii[1], self._radii[2]))
        setup_prog.report('Obtaining beam values')
        beam_width = self.getProperty('BeamWidth').value
        beam_height = self.getProperty('BeamHeight').value
        self._beam = [beam_height,
                      0.5 * beam_width,
                      -0.5 * beam_width,
                      (beam_width / 2),
                      -(beam_width / 2),
                      0.0,
                      beam_height,
                      0.0,
                      beam_height]

        self._interpolate = self.getProperty('Interpolate').value
        self._number_wavelengths = self.getProperty('NumberWavelengths').value

        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

#------------------------------------------------------------------------------

    def _sample(self):
        sample_prog = Progress(self, start=0.01, end=0.03, nreports=2)
        sample_prog.report('Setting Sample Material for Sample')
        SetSampleMaterial(self._sample_ws_name , ChemicalFormula=self._sample_chemical_formula,
                          SampleNumberDensity=self._sample_number_density)
        sample = mtd[self._sample_ws_name].sample()
        sam_material = sample.getMaterial()
        # total scattering x-section
        self._sig_s = np.zeros(self._number_can)
        self._sig_s[0] = sam_material.totalScatterXSection()
        # absorption x-section
        self._sig_a = np.zeros(self._number_can)
        self._sig_a[0] = sam_material.absorbXSection()
        # density
        self._density = np.zeros(self._number_can)
        self._density[0] = self._sample_number_density

        if self._use_can:
            sample_prog.report('Setting Sample Material for Container')
            SetSampleMaterial(InputWorkspace=self._can_ws_name, ChemicalFormula=self._can_chemical_formula,
                              SampleNumberDensity=self._can_number_density)
            can_sample = mtd[self._can_ws_name].sample()
            can_material = can_sample.getMaterial()
            self._sig_s[1] = can_material.totalScatterXSection()
            self._sig_a[1] = can_material.absorbXSection()
            self._density[1] = self._can_number_density

#------------------------------------------------------------------------------

    def _get_angles(self):
        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        angle_prog = Progress(self, start=0.03, end=0.07, nreports=num_hist)
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = list()
        for index in range(0, num_hist):
            angle_prog.report('Obtaining data for detector angle %i' % index)
            detector = mtd[self._sample_ws_name].getDetector(index)
            two_theta = detector.getTwoTheta(sample_pos, beam_pos) * 180.0 / math.pi
            self._angles.append(two_theta)
        logger.information('Detector angles : %i from %f to %f ' % (len(self._angles), self._angles[0], self._angles[-1]))

#------------------------------------------------------------------------------

    def _wave_range(self):
        wave_range = '__wave_range'
        ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)

        Xin = mtd[wave_range].readX(0)
        wave_min = mtd[wave_range].readX(0)[0]
        wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
        number_waves = self._number_wavelengths
        wave_bin = (wave_max - wave_min) / (number_waves-1)

        self._waves = list()
        wave_prog = Progress(self, start=0.07, end = 0.10, nreports=number_waves)
        for idx in range(0, number_waves):
            wave_prog.report('Appending wave data: %i' % idx)
            self._waves.append(wave_min + idx * wave_bin)
        DeleteWorkspace(wave_range)

        if self._emode == 'Elastic':
            self._elastic = self._waves[int(len(self._waves) / 2)]
        else:
            self._elastic = math.sqrt(81.787/self._efixed) # elastic wavelength

        logger.information('Elastic lambda : %f' % (self._elastic))
        logger.information('Lambda : %i values from %f to %f' % (len(self._waves), self._waves[0], self._waves[-1]))

#------------------------------------------------------------------------------

    def _transmission(self):
        distance = self._radii[1] - self._radii[0]
        trans= math.exp(-distance*self._density[0]*(self._sig_s[0] + self._sig_a[0]))
        logger.information('Sample transmission : %f' % (trans))
        if self._use_can:
            distance = self._radii[2] - self._radii[1]
            trans= math.exp(-distance*self._density[1]*(self._sig_s[1] + self._sig_a[1]))
            logger.information('Can transmission : %f' % (trans))

#------------------------------------------------------------------------------

    def _interpolate_corrections(self, workspaces):
        """
        Performs interpolation on the correction workspaces such that the number of bins
        matches that of the input sample workspace.

        @param workspaces List of correction workspaces to interpolate
        """

        for ws in workspaces:
            SplineInterpolation(WorkspaceToMatch=self._sample_ws_name,
                                WorkspaceToInterpolate=ws,
                                OutputWorkspace=ws,
                                OutputWorkspaceDeriv='')

#------------------------------------------------------------------------------

    def _cyl_abs(self, angle):
        #  Parameters :
        #  self._step_size - step size
        #  self._beam - beam parameters
        #  nan - number of annuli
        #  radii - list of radii (for each annulus)
        #  density - list of densities (for each annulus)
        #  sigs - list of scattering cross-sections (for each annulus)
        #  siga - list of absorption cross-sections (for each annulus)
        #  angle - list of angles
        #  wavelas - elastic wavelength
        #  waves - list of wavelengths
        #  Output parameters :  A1 - Ass ; A2 - Assc ; A3 - Acsc ; A4 - Acc

        amu_scat = np.zeros(self._number_can)
        amu_scat = self._density*self._sig_s
        sig_abs = np.zeros(self._number_can)
        sig_abs = self._density*self._sig_a
        amu_tot_i = np.zeros(self._number_can)
        amu_tot_s = np.zeros(self._number_can)

        theta = angle*math.pi/180.
        A1 = []
        A2 = []
        A3 = []
        A4 = []
        #loop over wavelengths
        for wave in self._waves:
            #loop over annuli
            if self._emode == 'Elastic':
                amu_tot_i = amu_scat + sig_abs*self._elastic/1.7979
                amu_tot_s = amu_scat + sig_abs*self._elastic/1.7979
            if self._emode == 'Direct':
                amu_tot_i = amu_scat + sig_abs*self._elastic/1.7979
                amu_tot_s = amu_scat + sig_abs*wave/1.7979
            if self._emode == 'Indirect':
                amu_tot_i = amu_scat + sig_abs*wave/1.7979
                amu_tot_s = amu_scat + sig_abs*self._elastic/1.7979
            (Ass, Assc, Acsc, Acc) = self._acyl(theta, amu_scat, amu_tot_i, amu_tot_s)
            A1.append(Ass)
            A2.append(Assc)
            A3.append(Acsc)
            A4.append(Acc)
        return A1, A2, A3, A4

#------------------------------------------------------------------------------

    def _acyl(self, theta, amu_scat, amu_tot_i, amu_tot_s):
        A = self._beam[1]
        Area_s = 0.0
        Ass = 0.0
        Acc = 0.0
        Acsc = 0.0
        Assc = 0.0
        nan = self._number_can
        if self._number_can < 2:
#
#  No. STEPS ARE CHOSEN SO THAT STEP WIDTH IS THE SAME FOR ALL ANNULI
#
            AAAA, BBBA, Area_A = self._sum_rom(0, 0, A, self._radii[0], self._radii[1], self._ms,
                                               theta, amu_scat, amu_tot_i, amu_tot_s)
            AAAB, BBBB, Area_B = self._sum_rom(0, 0, -A, self._radii[0], self._radii[1], self._ms,
                                               theta, amu_scat, amu_tot_i, amu_tot_s)
            Area_s += Area_A + Area_B
            Ass += AAAA + AAAB
            Ass = Ass/Area_s
        else:
            for i in range(0, self._number_can -1):
                radius_1 = self._radii[i]
                radius_2 = self._radii[i+1]
#
#  No. STEPS ARE CHOSEN SO THAT STEP WIDTH IS THE SAME FOR ALL ANNULI
#
                ms = int(self._ms*(radius_2 - radius_1)/(self._radii[1] - self._radii[0]))
                if ms < 1:
                    ms = 1
                AAAA, BBBA, Area_A = self._sum_rom(i, 0, A, radius_1, radius_2,
                                                   ms, theta, amu_scat, amu_tot_i, amu_tot_s)
                AAAB, BBBB, Area_B = self._sum_rom(i, 0, -A, radius_1, radius_2,
                                                   ms, theta, amu_scat, amu_tot_i, amu_tot_s)
                Area_s += Area_A + Area_B
                Ass += AAAA + AAAB
                Assc += BBBA + BBBB
            Ass = Ass/Area_s
            Assc = Assc/Area_s
            radius_1 = self._radii[nan -1]
            radius_2 = self._radii[nan]
            ms = int(self._ms*(radius_2 - radius_1)/(self._radii[1] - self._radii[0]))
            if ms < 1:
                ms = 1
            AAAA, BBBA, Area_A = self._sum_rom(nan-1, 1, A, radius_1, radius_2,
                                               ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            AAAB, BBBB, Area_B = self._sum_rom(nan-1, 1, -A, radius_1, radius_2,
                                               ms, theta, amu_scat, amu_tot_i, amu_tot_s)
            Area_C = Area_A + Area_B
            Acsc = (AAAA + AAAB)/Area_C
            Acc = (BBBA + BBBB)/Area_C
        return Ass, Assc, Acsc, Acc

#------------------------------------------------------------------------------

    def _sum_rom(self, n_scat, n_abs, a, r1, r2, ms, theta, amu_scat, amu_tot_i, amu_tot_s):
        #n_scat is region for scattering
        #n_abs is region for absorption
        nan = self._number_can
        omega_add = 0.
        if a < 0.:
            omega_add = math.pi
        AAA = 0.
        BBB = 0.
        Area = 0.
        theta_deg = math.pi - theta
        num = ms
        r_step = (r2 - r1)/ms
        r_add = -0.5*r_step + r1

# start loop over M
        for M in range(1, num+1):
            r = M*r_step + r_add
            number_omega = int(math.pi*r/r_step)
            omega_ster = math.pi/number_omega
            omega_deg = -0.5*omega_ster + omega_add
            Area_y = r*r_step*omega_ster*amu_scat[n_scat]
            sum_1 = 0.
            sum_2 = 0.
            I = 1
            Area_sum = 0.
            for _ in range(1, number_omega +1):
                omega = I*omega_ster + omega_deg
                distance = r*math.sin(omega)

                skip = True
                if abs(distance) <= a:
#
# CALCULATE DISTANCE INCIDENT NEUTRON PASSES THROUGH EACH ANNULUS
                    LIS = []
                    for j in range(0, nan):
                        LIST = self._distance(r, self._radii[j], omega)
                        LISN = self._distance(r, self._radii[j+1], omega)
                        LIS.append(LISN - LIST)
#
# CALCULATE DISTANCE SCATTERED NEUTRON PASSES THROUGH EACH ANNULUS
                    O = omega + theta_deg
                    LSS = []
                    for j in range(0, nan):
                        LSST = self._distance(r, self._radii[j], O)
                        LSSN = self._distance(r, self._radii[j+1], O)
                        LSS.append(LSSN - LSST)
#
# CALCULATE ABSORBTION FOR PATH THROUGH ALL ANNULI,AND THROUGH INNER ANNULI
                    path = np.zeros(3)
#	split into input (I) and scattered (S) paths
                    path[0] += amu_tot_i[0]*LIS[0] + amu_tot_s[0]*LSS[0]
                    if nan == 2:
                        path[2] += amu_tot_i[1]*LIS[1] + amu_tot_s[1]*LSS[1]
                        path[1] = path[0] + path[2]
                    sum_1 += math.exp(-path[n_abs])
                    sum_2 += math.exp(-path[n_abs +1])
                    Area_sum += 1.0
                    skip = False

                if skip:
                    I = number_omega -I +2
                else:
                    I += 1
        AAA += sum_1*Area_y
        BBB += sum_2*Area_y
        Area += Area_sum*Area_y
        return AAA, BBB, Area

#------------------------------------------------------------------------------

    def _distance(self, r1, radius, omega):
        r = r1
        distance = 0.
        b = r*math.sin(omega)
        if abs(b) < radius:
            t = r*math.cos(omega)
            c = radius*radius -b*b
            d = math.sqrt(c)
            if r <= radius:
                distance = t + d
            else:
                distance = d*(1.0 + math.copysign(1.0,t))
        return distance

#------------------------------------------------------------------------------

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CylinderPaalmanPingsCorrection)
