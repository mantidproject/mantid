# pylint: disable=no-init,invalid-name,too-many-instance-attributes

from __future__ import (absolute_import, division, print_function)
import math
from six import iteritems
from six import integer_types

import numpy as np
from mantid.simpleapi import *
from mantid.api import (PythonAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, InstrumentValidator, Progress)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, MaterialBuilder)


class FlatPlatePaalmanPingsCorrection(PythonAlgorithm):
    # Sample variables
    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_thickness = None
    _sample_angle = 0.0

    # Container Variables
    _use_can = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_density_type = None
    _can_density = None
    _can_front_thickness = None
    _can_back_thickness = None

    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _output_ws_name = None
    _angles = list()
    _waves = list()
    _interpolate = None

    # ------------------------------------------------------------------------------

    def category(self):
        return "Workflow\\MIDAS;CorrectionFunctions\\AbsorptionCorrections"

    def summary(self):
        return "Calculates absorption corrections for a flat plate sample using Paalman & Pings format."

    # ------------------------------------------------------------------------------

    def PyInit(self):
        ws_validator = InstrumentValidator()

        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     direction=Direction.Input,
                                                     validator=ws_validator),
                             doc='Name for the input sample workspace')

        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')

        self.declareProperty(name='SampleDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density')

        self.declareProperty(name='SampleDensity', defaultValue=0.1,
                             doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3)')

        self.declareProperty(name='SampleThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample thickness in cm')

        self.declareProperty(name='SampleAngle', defaultValue=0.0,
                             doc='Sample angle in degrees')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional,
                                                     validator=ws_validator),
                             doc="Name for the input container workspace")

        self.declareProperty(name='CanChemicalFormula', defaultValue='',
                             doc='Container chemical formula')

        self.declareProperty(name='CanDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density')

        self.declareProperty(name='CanDensity', defaultValue=0.1,
                             doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3)')

        self.declareProperty(name='CanFrontThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container front thickness in cm')

        self.declareProperty(name='CanBackThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container back thickness in cm')

        self.declareProperty(name='NumberWavelengths', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')

        self.declareProperty(name='Interpolate', defaultValue=True,
                             doc='Interpolate the correction workspaces to match the sample workspace')

        self.declareProperty(name='Emode', defaultValue='Elastic',
                             validator=StringListValidator(['Elastic', 'Indirect', 'Direct', 'Efixed']),
                             doc='Energy transfer mode. Only Efixed behaves differently. '
                                 'Others are equivalent for this algorithm, and are left only for legacy access.')

        self.declareProperty(name='Efixed', defaultValue=0.,
                             doc='Analyser energy (mev). By default will be read from the instrument parameters. '
                                 'Specify manually to override. This is used only in Efixed energy transfer mode.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output corrections workspace group')

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        issues = dict()

        sample_ws_name = self.getPropertyValue('SampleWorkspace')
        can_ws_name = self.getPropertyValue('CanWorkspace')
        use_can = can_ws_name != ''

        # Ensure that a can chemical formula is given when using a can workspace
        if use_can:
            can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
            if can_chemical_formula == '':
                issues['CanChemicalFormula'] = 'Must provide a chemical formula when providing a can workspace'

        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value

        if self._emode != 'Efixed':
            # require both sample and can ws have wavelenght as x-axis
            if mtd[sample_ws_name].getAxis(0).getUnit().unitID() != 'Wavelength':
                issues['SampleWorkspace'] = 'Workspace must have units of wavelenght.'

            if use_can and mtd[can_ws_name].getAxis(0).getUnit().unitID() != 'Wavelength':
                issues['CanWorkspace'] = 'Workspace must have units of wavelength.'

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):
        self._setup()
        self._wave_range()

        setup_prog = Progress(self, start=0.0, end=0.2, nreports=2)
        # Set sample material form chemical formula
        setup_prog.report('Set sample material')
        self._sample_density = self._set_material(self._sample_ws_name,
                                                  self._sample_chemical_formula,
                                                  self._sample_density_type,
                                                  self._sample_density)

        # If using a can, set sample material using chemical formula
        if self._use_can:
            setup_prog.report('Set container sample material')
            self._can_density = self._set_material(self._can_ws_name,
                                                   self._can_chemical_formula,
                                                   self._can_density_type,
                                                   self._can_density)

        # Holders for the corrected data
        data_ass = []
        data_assc = []
        data_acsc = []
        data_acc = []

        self._get_angles()
        num_angles = len(self._angles)
        workflow_prog = Progress(self, start=0.2, end=0.8, nreports=num_angles * 2)
        for angle_idx in range(num_angles):
            workflow_prog.report('Running flat correction for angle %s' % angle_idx)
            angle = self._angles[angle_idx]
            (ass, assc, acsc, acc) = self._flat_abs(angle)

            logger.information('Angle %d: %f successful' % (angle_idx + 1, self._angles[angle_idx]))
            workflow_prog.report('Appending data for angle %s' % angle_idx)
            data_ass = np.append(data_ass, ass)
            data_assc = np.append(data_assc, assc)
            data_acsc = np.append(data_acsc, acsc)
            data_acc = np.append(data_acc, acc)

        log_prog = Progress(self, start=0.8, end=1.0, nreports=8)

        sample_logs = {'sample_shape': 'flatplate', 'sample_filename': self._sample_ws_name,
                       'sample_thickness': self._sample_thickness, 'sample_angle': self._sample_angle,
                       'emode': self._emode, 'efixed': self._efixed}
        dataX = self._waves * num_angles

        # Create the output workspaces
        ass_ws = self._output_ws_name + '_ass'
        log_prog.report('Creating ass output Workspace')
        CreateWorkspace(OutputWorkspace=ass_ws,
                        DataX=dataX,
                        DataY=data_ass,
                        NSpec=num_angles,
                        UnitX='Wavelength',
                        VerticalAxisUnit='SpectraNumber',
                        ParentWorkspace=self._sample_ws_name,
                        EnableLogging=False)
        log_prog.report('Adding sample logs')
        self._add_sample_logs(ass_ws, sample_logs)

        workspaces = [ass_ws]

        if self._use_can:
            log_prog.report('Adding can sample logs')
            AddSampleLog(Workspace=ass_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name), EnableLogging=False)

            assc_ws = self._output_ws_name + '_assc'
            workspaces.append(assc_ws)
            log_prog.report('Creating assc output workspace')
            CreateWorkspace(OutputWorkspace=assc_ws,
                            DataX=dataX,
                            DataY=data_assc,
                            NSpec=num_angles,
                            UnitX='Wavelength',
                            VerticalAxisUnit='SpectraNumber',
                            ParentWorkspace=self._sample_ws_name,
                            EnableLogging=False)
            log_prog.report('Adding assc sample logs')
            self._add_sample_logs(assc_ws, sample_logs)
            AddSampleLog(Workspace=assc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name), EnableLogging=False)

            acsc_ws = self._output_ws_name + '_acsc'
            workspaces.append(acsc_ws)
            log_prog.report('Creating acsc outputworkspace')
            CreateWorkspace(OutputWorkspace=acsc_ws,
                            DataX=dataX,
                            DataY=data_acsc,
                            NSpec=num_angles,
                            UnitX='Wavelength',
                            VerticalAxisUnit='SpectraNumber',
                            ParentWorkspace=self._sample_ws_name,
                            EnableLogging=False)
            log_prog.report('Adding acsc sample logs')
            self._add_sample_logs(acsc_ws, sample_logs)
            AddSampleLog(Workspace=acsc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name), EnableLogging=False)

            acc_ws = self._output_ws_name + '_acc'
            workspaces.append(acc_ws)
            log_prog.report('Creating acc workspace')
            CreateWorkspace(OutputWorkspace=acc_ws,
                            DataX=dataX,
                            DataY=data_acc,
                            NSpec=num_angles,
                            UnitX='Wavelength',
                            VerticalAxisUnit='SpectraNumber',
                            ParentWorkspace=self._sample_ws_name,
                            EnableLogging=False)
            log_prog.report('Adding acc sample logs')
            self._add_sample_logs(acc_ws, sample_logs)
            AddSampleLog(Workspace=acc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name), EnableLogging=False)

        if self._interpolate:
            self._interpolate_corrections(workspaces)
        log_prog.report('Grouping Output Workspaces')
        GroupWorkspaces(InputWorkspaces=','.join(workspaces), OutputWorkspace=self._output_ws_name, EnableLogging=False)
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)

    # ------------------------------------------------------------------------------

    def _setup(self):
        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_density_type = self.getPropertyValue('SampleDensityType')
        self._sample_density = self.getProperty('SampleDensity').value
        self._sample_thickness = self.getProperty('SampleThickness').value
        self._sample_angle = self.getProperty('SampleAngle').value

        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''

        self._can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
        self._can_density_type = self.getPropertyValue('CanDensityType')
        self._can_density = self.getProperty('CanDensity').value
        self._can_front_thickness = self.getProperty('CanFrontThickness').value
        self._can_back_thickness = self.getProperty('CanBackThickness').value

        self._number_wavelengths = self.getProperty('NumberWavelengths').value
        self._interpolate = self.getProperty('Interpolate').value

        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value

        if self._emode == 'Efixed' and self._efixed == 0.:
            # Efixed mode requested with default efixed, try to read from Instrument Parameters
            try:
                self._efixed = self._getEfixed()
                logger.information('Found Efixed = {0}'.format(self._efixed))
            except ValueError:
                raise RuntimeError('Efixed mode requested with the default value,'
                                   'but could not find the Efixed parameter in the instrument.')

        if self._emode == 'Efixed':
            logger.information('No interpolation is possible in Efixed mode.')
            self._interpolate = False

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

        # purge the lists
        self._angles = list()
        self._waves = list()

    # ------------------------------------------------------------------------------

    def _set_material(self, ws_name, chemical_formula, density_type, density):
        """
        Sets the sample material for a given workspace
        @param ws_name              :: name of the workspace to set sample material for
        @param chemical_formula     :: Chemical formula of sample
        @param density_type         :: 'Mass Density' or 'Number Density'
        @param density              :: Density of sample
        @return pointer to the workspace with sample material set
                AND
                number density of the sample material
        """
        set_material_alg = self.createChildAlgorithm('SetSampleMaterial')
        if density_type == 'Mass Density':
            set_material_alg.setProperty('SampleMassDensity', density)
            builder = MaterialBuilder()
            mat = builder.setFormula(chemical_formula).setMassDensity(density).build()
            number_density = mat.numberDensity
        else:
            number_density = density
        set_material_alg.setProperty('InputWorkspace', ws_name)
        set_material_alg.setProperty('ChemicalFormula', chemical_formula)
        set_material_alg.setProperty('SampleNumberDensity', number_density)
        set_material_alg.execute()
        return number_density

    # ------------------------------------------------------------------------------

    def _get_angles(self):
        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = list()
        for index in range(0, num_hist):
            detector = mtd[self._sample_ws_name].getDetector(index)
            two_theta = detector.getTwoTheta(sample_pos, beam_pos) * 180.0 / math.pi  # calc angle
            self._angles.append(two_theta)

    # ------------------------------------------------------------------------------

    def _wave_range(self):
        if self._emode == 'Efixed':
            lambda_fixed = math.sqrt(81.787 / self._efixed)
            self._waves.append(lambda_fixed)
            logger.information('Efixed mode, setting lambda_fixed to {0}'.format(lambda_fixed))
        else:

            wave_range = '__WaveRange'
            ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)
            Xin = mtd[wave_range].readX(0)
            wave_min = mtd[wave_range].readX(0)[0]
            wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
            number_waves = self._number_wavelengths
            wave_bin = (wave_max - wave_min) / (number_waves - 1)

            self._waves = list()
            for idx in range(0, number_waves):
                self._waves.append(wave_min + idx * wave_bin)

            DeleteWorkspace(wave_range, EnableLogging=False)

    # ------------------------------------------------------------------------------

    def _getEfixed(self):
        inst = mtd[self._sample_ws_name].getInstrument()

        if inst.hasParameter('Efixed'):
            return inst.getNumberParameter('EFixed')[0]

        if inst.hasParameter('analyser'):
            analyser_name = inst.getStringParameter('analyser')[0]
            analyser_comp = inst.getComponentByName(analyser_name)

            if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
                return analyser_comp.getNumberParameter('EFixed')[0]

        raise ValueError('No Efixed parameter found')

    # ------------------------------------------------------------------------------

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

    # ------------------------------------------------------------------------------

    def _add_sample_logs(self, ws, sample_logs):
        """
        Add a dictionary of logs to a workspace.

        The type of the log is inferred by the type of the value passed to the log.

        @param ws Workspace to add logs too.
        @param sample_logs Dictionary of logs to append to the workspace.
        """

        for key, value in iteritems(sample_logs):
            if isinstance(value, bool):
                log_type = 'String'
            elif isinstance(value, (integer_types, float)):
                log_type = 'Number'
            else:
                log_type = 'String'

            AddSampleLog(Workspace=ws, LogName=key, LogType=log_type, LogText=str(value), EnableLogging=False)

    # ------------------------------------------------------------------------------

    def _flat_abs(self, angle):
        """
        FlatAbs - calculate flat plate absorption factors

        For more information See:
          - MODES User Guide: http://www.isis.stfc.ac.uk/instruments/iris/data-analysis/modes-v3-user-guide-6962.pdf
          - C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)
        """

        PICONV = math.pi / 180.0

        canAngle = self._sample_angle * PICONV

        # tsec is the angle the scattered beam makes with the normal to the sample surface.
        tsec = angle - self._sample_angle

        nlam = len(self._waves)

        ass = np.ones(nlam)
        assc = np.ones(nlam)
        acsc = np.ones(nlam)
        acc = np.ones(nlam)

        # Case where tsec is close to 90 degrees.
        # CALCULATION IS UNRELIABLE
        # Default to 1 for everything
        if abs(abs(tsec) - 90.0) < 0.1:
            return ass, assc, acsc, acc

        sample = mtd[self._sample_ws_name].sample()
        sam_material = sample.getMaterial()

        tsec *= PICONV

        sec1 = 1.0 / math.cos(canAngle)
        sec2 = 1.0 / math.cos(tsec)

        # List of wavelengths
        waves = np.array(self._waves)

        # Sample cross section
        sample_x_section = (sam_material.totalScatterXSection() + sam_material.absorbXSection() * waves / 1.8) * self._sample_density

        vecFact = np.vectorize(self._fact)
        fs = vecFact(sample_x_section, self._sample_thickness, sec1, sec2)

        sample_sect_1, sample_sect_2 = self._calc_thickness_at_x_sect(sample_x_section, self._sample_thickness, [sec1, sec2])

        if sec2 < 0.0:
            ass = fs / self._sample_thickness
        else:
            ass = np.exp(-sample_sect_2) * fs / self._sample_thickness

        if self._use_can:
            can_sample = mtd[self._can_ws_name].sample()
            can_material = can_sample.getMaterial()

            # Calculate can cross section
            can_x_section = (can_material.totalScatterXSection() + can_material.absorbXSection() * waves / 1.8) * self._can_density
            assc, acsc, acc = self._calculate_can(ass, can_x_section, sample_sect_1, sample_sect_2, [sec1, sec2])

        return ass, assc, acsc, acc

    # ------------------------------------------------------------------------------

    def _fact(self, x_section, thickness, sec1, sec2):
        S = x_section * thickness * (sec1 - sec2)
        F = 1.0
        if S == 0.0:
            F = thickness
        else:
            S = (1 - math.exp(-S)) / S
            F = thickness * S
        return F

    # ------------------------------------------------------------------------------

    def _calc_thickness_at_x_sect(self, x_section, thickness, sec):
        sec1, sec2 = sec

        thick_sec_1 = x_section * thickness * sec1
        thick_sec_2 = x_section * thickness * sec2

        return thick_sec_1, thick_sec_2

    # ------------------------------------------------------------------------------

    # pylint: disable=too-many-arguments
    def _calculate_can(self, ass, can_x_section, sample_sect_1, sample_sect_2, sec):
        """
        Calculates the A_s,sc, A_c,sc and A_c,c data.
        """

        assc = np.ones(ass.size)
        acsc = np.ones(ass.size)
        acc = np.ones(ass.size)

        sec1, sec2 = sec

        vecFact = np.vectorize(self._fact)
        f1 = vecFact(can_x_section, self._can_front_thickness, sec1, sec2)
        f2 = vecFact(can_x_section, self._can_back_thickness, sec1, sec2)

        can_thick_1_sect_1, can_thick_1_sect_2 = self._calc_thickness_at_x_sect(can_x_section, self._can_front_thickness, sec)
        _, can_thick_2_sect_2 = self._calc_thickness_at_x_sect(can_x_section, self._can_back_thickness, sec)

        if sec2 < 0.0:
            val = np.exp(-(can_thick_1_sect_1 - can_thick_1_sect_2))
            assc = ass * val

            acc1 = f1
            acc2 = f2 * val

            acsc1 = acc1
            acsc2 = acc2 * np.exp(-(sample_sect_1 - sample_sect_2))

        else:
            val = np.exp(-(can_thick_1_sect_1 + can_thick_2_sect_2))
            assc = ass * val

            acc1 = f1 * np.exp(-(can_thick_1_sect_2 + can_thick_2_sect_2))
            acc2 = f2 * val

            acsc1 = acc1 * np.exp(-sample_sect_2)
            acsc2 = acc2 * np.exp(-sample_sect_1)

        can_thickness = self._can_front_thickness + self._can_back_thickness

        if can_thickness > 0.0:
            acc = (acc1 + acc2) / can_thickness
            acsc = (acsc1 + acsc2) / can_thickness

        return assc, acsc, acc


# ------------------------------------------------------------------------------

# Register algorithm with Mantid
AlgorithmFactory.subscribe(FlatPlatePaalmanPingsCorrection)
