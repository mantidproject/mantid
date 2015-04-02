#pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty, \
                       WorkspaceGroupProperty, InstrumentValidator, WorkspaceUnitValidator
from mantid.kernel import StringListValidator, StringMandatoryValidator, IntBoundedValidator, \
                          FloatBoundedValidator, Direction, logger, CompositeValidator
import math, numpy as np


class FlatPlatePaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_number_density = None
    _sample_thickness = None
    _sample_angle = 0.0
    _use_can = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_number_density = None
    _can_front_thickness = None
    _can_back_thickness = None
    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _output_ws_name = None
    _angles = list()
    _waves = list()
    _elastic = 0.0


    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms;CorrectionFunctions\\AbsorptionCorrections"


    def summary(self):
        return "Calculates absorption corrections for a flat plate sample using Paalman & Pings format."


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
        self.declareProperty(name='CanNumberDensity', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Container number density in atoms/Angstrom3')

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
                             validator=StringListValidator(['Elastic', 'Indirect']),
                             doc='Emode: Elastic or Indirect')
        self.declareProperty(name='Efixed', defaultValue=1.0,
                             doc='Analyser energy')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='The output corrections workspace group')


    def validateInputs(self):
        issues = dict()

        can_ws_name = self.getPropertyValue('CanWorkspace')
        use_can = can_ws_name != ''

        # Ensure that a can chemical formula is given when using a can workspace
        if use_can:
            can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
            if can_chemical_formula == '':
                issues['CanChemicalFormula'] = 'Must provide a chemical foruma when providing a can workspace'

        return issues


    def PyExec(self):
        self._setup()
        self._wave_range()

        # Set sample material form chemical formula
        SetSampleMaterial(self._sample_ws_name , ChemicalFormula=self._sample_chemical_formula,
                          SampleNumberDensity=self._sample_number_density)

        # If using a can, set sample material using chemical formula
        if self._use_can:
            SetSampleMaterial(InputWorkspace=self._can_ws_name, ChemicalFormula=self._can_chemical_formula,
                              SampleNumberDensity=self._can_number_density)

        # Holders for the corrected data
        data_ass = []
        data_assc = []
        data_acsc = []
        data_acc = []

        self._get_angles()
        num_angles = len(self._angles)

        for angle_idx in range(num_angles):
            angle = self._angles[angle_idx]
            (ass, assc, acsc, acc) = self._flat_abs(angle)

            logger.information('Angle %d: %f successful' % (angle_idx+1, self._angles[angle_idx]))

            data_ass = np.append(data_ass, ass)
            data_assc = np.append(data_assc, assc)
            data_acsc = np.append(data_acsc, acsc)
            data_acc = np.append(data_acc, acc)

        sample_logs = {'sample_shape': 'flatplate', 'sample_filename': self._sample_ws_name,
                        'sample_thickness': self._sample_thickness, 'sample_angle': self._sample_angle}
        dataX = self._waves * num_angles

        # Create the output workspaces
        ass_ws = self._output_ws_name + '_ass'
        CreateWorkspace(OutputWorkspace=ass_ws, DataX=dataX, DataY=data_ass,
                        NSpec=num_angles, UnitX='Wavelength')
        self._add_sample_logs(ass_ws, sample_logs)

        workspaces = [ass_ws]

        if self._use_can:
            AddSampleLog(Workspace=ass_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            assc_ws = self._output_ws_name + '_assc'
            workspaces.append(assc_ws)
            CreateWorkspace(OutputWorkspace=assc_ws, DataX=dataX, DataY=data_assc,
                            NSpec=num_angles, UnitX='Wavelength')
            self._add_sample_logs(assc_ws, sample_logs)
            AddSampleLog(Workspace=assc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            acsc_ws = self._output_ws_name + '_acsc'
            workspaces.append(acsc_ws)
            CreateWorkspace(OutputWorkspace=acsc_ws, DataX=dataX, DataY=data_acsc,
                            NSpec=num_angles, UnitX='Wavelength')
            self._add_sample_logs(acsc_ws, sample_logs)
            AddSampleLog(Workspace=acsc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            acc_ws = self._output_ws_name + '_acc'
            workspaces.append(acc_ws)
            CreateWorkspace(OutputWorkspace=acc_ws, DataX=dataX, DataY=data_acc,
                            NSpec=num_angles, UnitX='Wavelength')
            self._add_sample_logs(acc_ws, sample_logs)
            AddSampleLog(Workspace=acc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

        if self._interpolate:
            self._interpolate_corrections(workspaces)

        try:
            self. _copy_detector_table(workspaces)
        except RuntimeError:
            logger.warning('Cannot copy spectra mapping. Check input workspace instrument.')

        GroupWorkspaces(InputWorkspaces=','.join(workspaces), OutputWorkspace=self._output_ws_name)
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)


    def _setup(self):
        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_number_density = self.getProperty('SampleNumberDensity').value
        self._sample_thickness = self.getProperty('SampleThickness').value
        self._sample_angle = self.getProperty('SampleAngle').value

        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''

        self._can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
        self._can_number_density = self.getProperty('CanNumberDensity').value
        self._can_front_thickness = self.getProperty('CanFrontThickness').value
        self._can_back_thickness = self.getProperty('CanBackThickness').value

        self._number_wavelengths = self.getProperty('NumberWavelengths').value
        self._interpolate = self.getProperty('Interpolate').value

        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')


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


    def _wave_range(self):
        wave_range = '__WaveRange'
        ExtractSingleSpectrum(InputWorkspace=self._sample_ws_name, OutputWorkspace=wave_range, WorkspaceIndex=0)
        Xin = mtd[wave_range].readX(0)
        wave_min = mtd[wave_range].readX(0)[0]
        wave_max = mtd[wave_range].readX(0)[len(Xin) - 1]
        number_waves = self._number_wavelengths
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


    def _copy_detector_table(self, workspaces):
        """
        Copy the detector table from the sample workspaces to the correction workspaces.

        @param workspaces List of correction workspaces
        """

        instrument = mtd[self._sample_ws_name].getInstrument().getName()

        for ws in workspaces:
            LoadInstrument(Workspace=ws,
                           InstrumentName=instrument)

            CopyDetectorMapping(WorkspaceToMatch=self._sample_ws_name,
                                WorkspaceToRemap=ws,
                                IndexBySpectrumNumber=True)


    def _add_sample_logs(self, ws, sample_logs):
        """
        Add a dictionary of logs to a workspace.

        The type of the log is inferred by the type of the value passed to the log.

        @param ws Workspace to add logs too.
        @param sample_logs Dictionary of logs to append to the workspace.
        """

        for key, value in sample_logs.iteritems():
            if isinstance(value, bool):
                log_type = 'String'
            elif isinstance(value, (int, long, float)):
                log_type = 'Number'
            else:
                log_type = 'String'

            AddSampleLog(Workspace=ws, LogName=key, LogType=log_type, LogText=str(value))


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
        if abs(abs(tsec) - 90.0) < 1.0:
            return ass, assc, acsc, acc

        sample = mtd[self._sample_ws_name].sample()
        sam_material = sample.getMaterial()

        tsec = tsec * PICONV

        sec1 = 1.0 / math.cos(canAngle)
        sec2 = 1.0 / math.cos(tsec)

        # List of wavelengths
        waves = np.array(self._waves)

        # Sample cross section
        sample_x_section = (sam_material.totalScatterXSection() + sam_material.absorbXSection() * waves / 1.8) * self._sample_number_density

        vecFact = np.vectorize(self._fact)
        fs = vecFact(sample_x_section, self._sample_thickness, sec1, sec2)

        sample_sect_1, sample_sect_2 = self._calc_thickness_at_x_sect(sample_x_section, self._sample_thickness, [sec1, sec2])

        if sec2 < 0.0:
            ass = fs / self._sample_thickness
        else:
            ass= np.exp(-sample_sect_2) * fs / self._sample_thickness

        if self._use_can:
            can_sample = mtd[self._can_ws_name].sample()
            can_material = can_sample.getMaterial()

            # Calculate can cross section
            can_x_section = (can_material.totalScatterXSection() + can_material.absorbXSection() * waves / 1.8) * self._can_number_density
            assc, acsc, acc = self._calculate_can(ass, can_x_section, sample_sect_1, sample_sect_2, [sec1, sec2])

        return ass, assc, acsc, acc


    def _fact(self, x_section, thickness, sec1, sec2):
        S = x_section * thickness * (sec1 - sec2)
        F = 1.0
        if S == 0.0:
            F = thickness
        else:
            S = (1 - math.exp(-S)) / S
            F = thickness*S
        return F


    def _calc_thickness_at_x_sect(self, x_section, thickness, sec):
        sec1, sec2 = sec

        thick_sec_1 = x_section * thickness * sec1
        thick_sec_2 = x_section * thickness * sec2

        return thick_sec_1, thick_sec_2


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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(FlatPlatePaalmanPingsCorrection)
