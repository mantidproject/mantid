from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import StringListValidator, Direction, logger
import math, numpy as np


class FlatPaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _sample_chemical_formula = None
    _sample_number_density = None
    _sample_thickness = None
    _sample_angle = 0.0
    _usecan = False
    _can_ws_name = None
    _can_chemical_formula = None
    _can_number_density = None
    _can_thickness1 = None
    _can_thickness2 = None
    _can_scale = 1.0
    _number_wavelengths = 10
    _emode = None
    _efixed = 0.0
    _interpolate = 'Linear'
    _output_ws_name = None
    _angles = list()


    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"


    def summary(self):
        return "Calculates absorption corrections for a flat plate sample using Paalman & Pings format."


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                             direction=Direction.Input),
                             doc='Name for the input Sample workspace.')

        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             doc='Sample chemical formula')
        self.declareProperty(name='SampleNumberDensity', defaultValue=0.0,
                             doc='Sample number density')
        self.declareProperty(name='SampleThickness', defaultValue=0.0,
                             doc='Sample thickness')
        self.declareProperty(name='SampleAngle', defaultValue=0.0,
                             doc='Sample angle')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                             direction=Direction.Input),
                             doc="Name for the input Can workspace.")

        self.declareProperty(name='CanChemicalFormula', defaultValue='',
                             doc='Can chemical formula')
        self.declareProperty(name='CanNumberDensity', defaultValue=0.0,
                             doc='Can number density')

        self.declareProperty(name='CanFrontThickness', defaultValue=0.0,
                             doc='Can thickness1 front')
        self.declareProperty(name='CanBackThickness', defaultValue=0.0,
                             doc='Can thickness2 back')

        self.declareProperty(name='CanScaleFactor', defaultValue=1.0,
                             doc='Scale factor to multiply can data')

        self.declareProperty(name='NumberWavelengths', defaultValue=10,
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='Emode', defaultValue='Elastic',
                             validator=StringListValidator(['Elastic','Indirect']),
                             doc='Emode: Elastic or Indirect')
        self.declareProperty(name='Efixed', defaultValue=0.0,
                             doc='Efixed - analyser energy')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='The output corrections workspace group.')


    def PyExec(self):
        self._setup()
        self._wave_range()

        # Set sample material form chemical formula
        SetSampleMaterial(self._sample_ws_name , ChemicalFormula=self._sample_chemical_formula,
                          SampleNumberDensity=self._sample_number_density)

        # If using a can, set sample material using chemical formula
        if self._usecan:
            SetSampleMaterial(InputWorkspace=self._can_ws_name, ChemicalFormula=self._can_chemical_formula,
                              SampleNumberDensity=self._can_number_density)

        dataA1 = []
        dataA2 = []
        dataA3 = []
        dataA4 = []

        self._get_angles()
        num_angles = len(self._angles)

        for angle_idx in range(num_angles):
            angle = self._angles[angle_idx]
            (A1, A2, A3, A4) = self._flatAbs(angle)

            logger.information('Angle ' + str(angle_idx+1) + ' : ' + str(self._angles[angle_idx]) + ' successful')

            dataA1 = np.append(dataA1, A1)
            dataA2 = np.append(dataA2, A2)
            dataA3 = np.append(dataA3, A3)
            dataA4 = np.append(dataA4, A4)

        sample_logs = {'sample_shape': 'flatplate', 'sample_filename': self._sample_ws_name,
                        'sample_thickness': self._sample_thickness, 'sample_angle': self._sample_angle}
        dataX = self._waves * num_angles

        # Create the output workspaces
        ass_ws = self._output_ws_name + '_ass'
        CreateWorkspace(OutputWorkspace=ass_ws, DataX=dataX, DataY=dataA1,
                        NSpec=num_angles, UnitX='Wavelength')
        self._addSampleLogs(ass_ws, sample_logs)

        workspaces = [ass_ws]

        if self._usecan:
            AddSampleLog(Workspace=ass_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            assc_ws = self._output_ws_name + '_assc'
            workspaces.append(assc_ws)
            CreateWorkspace(OutputWorkspace=assc_ws, DataX=dataX, DataY=dataA2,
                            NSpec=num_angles, UnitX='Wavelength')
            self._addSampleLogs(assc_ws, sample_logs)
            AddSampleLog(Workspace=assc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            acsc_ws = self._output_ws_name + '_acsc'
            workspaces.append(acsc_ws)
            CreateWorkspace(OutputWorkspace=acsc_ws, DataX=dataX, DataY=dataA3,
                            NSpec=num_angles, UnitX='Wavelength')
            self._addSampleLogs(acsc_ws, sample_logs)
            AddSampleLog(Workspace=acsc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

            acc_ws = self._output_ws_name + '_acc'
            workspaces.append(acc_ws)
            CreateWorkspace(OutputWorkspace=acc_ws, DataX=dataX, DataY=dataA4,
                            NSpec=num_angles, UnitX='Wavelength')
            self._addSampleLogs(acc_ws, sample_logs)
            AddSampleLog(Workspace=acc_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws_name))

        GroupWorkspaces(InputWorkspaces=','.join(workspaces), OutputWorkspace=self._output_ws_name)
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)


    def _setup(self):
        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_number_density = self.getProperty('SampleNumberDensity').value
        self._sample_thickness = self.getProperty('SampleThickness').value
        self._sample_angle = self.getProperty('SampleAngle').value

        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._usecan = self._can_ws_name != ''

        self._can_chemical_formula = self.getPropertyValue('CanChemicalFormula')
        self._can_number_density = self.getProperty('CanNumberDensity').value
        self._can_thickness1 = self.getProperty('CanFrontThickness').value
        self._can_thickness2 = self.getProperty('CanBackThickness').value
        self._can_scale = self.getProperty('CanScaleFactor').value

        self._number_wavelengths = self.getProperty('NumberWavelengths').value
        self._emode = self.getPropertyValue('Emode')
        self._efixed = self.getProperty('Efixed').value
        self._output_ws_name = self.getPropertyValue('OutputWorkspace')


    def _get_angles(self):
        num_hist = mtd[self._sample_ws_name].getNumberHistograms()
        source_pos = mtd[self._sample_ws_name].getInstrument().getSource().getPos()
        sample_pos = mtd[self._sample_ws_name].getInstrument().getSample().getPos()
        beam_pos = sample_pos - source_pos
        self._angles = []
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
        wave_bin = (wave_max - wave_min)/(number_waves-1)
        waves = []
        for l in range(0,number_waves):
            waves.append(wave_min+l*wave_bin)
        self._waves = waves
        if self._emode == 'Elastic':
            self._elastic = waves[int(number_waves / 2)]
        if self._emode == 'Indirect':
            self._elastic = math.sqrt(81.787/self._efixed)  # elastic wavelength
        logger.information('Elastic lambda %f' % self._elastic)
        DeleteWorkspace(wave_range)


    def _addSampleLogs(self, ws, sample_logs):
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


    def _flatAbs(self, angle):
        """
        FlatAbs - calculate flat plate absorption factors

        For more information See:
          - MODES User Guide: http://www.isis.stfc.ac.uk/instruments/iris/data-analysis/modes-v3-user-guide-6962.pdf
          - C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)

        @param angle - angle
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

        #list of wavelengths
        waves = np.array(self._waves)

        #sample cross section
        sampleXSection = (sam_material.totalScatterXSection() + sam_material.absorbXSection() * waves / 1.8) * self._sample_number_density

        #vector version of fact
        vecFact = np.vectorize(self._fact)
        fs = vecFact(sampleXSection, self._sample_thickness, sec1, sec2)

        sampleSec1, sampleSec2 = self._calc_thickness_at_x_sect(sampleXSection, self._sample_thickness, [sec1, sec2])

        if sec2 < 0.0:
            ass = fs / self._sample_thickness
        else:
            ass= np.exp(-sampleSec2) * fs / self._sample_thickness

        if self._usecan:
            can_sample = mtd[self._can_ws_name].sample()
            can_material = can_sample.getMaterial()

            #calculate can cross section
            canXSection = (can_material.totalScatterXSection() + can_material.absorbXSection() * waves / 1.8) * self._can_number_density
            assc, acsc, acc = self._calculate_can(ass, canXSection, self._can_thickness1, self._can_thickness2, sampleSec1, sampleSec2, [sec1, sec2])

        return ass, assc, acsc, acc


    def _fact(self, xSection, thickness, sec1, sec2):
        S = xSection * thickness * (sec1 - sec2)
        F = 1.0
        if S == 0.0:
            F = thickness
        else:
            S = (1 - math.exp(-S)) / S
            F = thickness*S
        return F


    def _calc_thickness_at_x_sect(self, xSection, thickness, sec):
        sec1, sec2 = sec

        thickSec1 = xSection * thickness * sec1
        thickSec2 = xSection * thickness * sec2

        return thickSec1, thickSec2


    def _calculate_can(self, ass, canXSection, canThickness1, canThickness2, sampleSec1, sampleSec2, sec):
        assc = np.ones(ass.size)
        acsc = np.ones(ass.size)
        acc = np.ones(ass.size)

        sec1, sec2 = sec

        #vector version of fact
        vecFact = np.vectorize(self._fact)
        f1 = vecFact(canXSection, canThickness1, sec1, sec2)
        f2 = vecFact(canXSection, canThickness2, sec1, sec2)

        canThick1Sec1, canThick1Sec2 = self._calc_thickness_at_x_sect(canXSection, canThickness1, sec)
        _, canThick2Sec2 = self._calc_thickness_at_x_sect(canXSection, canThickness2, sec)

        if sec2 < 0.0:
            val = np.exp(-(canThick1Sec1-canThick1Sec2))
            assc = ass * val

            acc1 = f1
            acc2 = f2 * val

            acsc1 = acc1
            acsc2 = acc2 * np.exp(-(sampleSec1 - sampleSec2))

        else:
            val = np.exp(-(canThick1Sec1 + canThick2Sec2))
            assc = ass * val

            acc1 = f1 * np.exp(-(canThick1Sec2 + canThick2Sec2))
            acc2 = f2 * val

            acsc1 = acc1 * np.exp(-sampleSec2)
            acsc2 = acc2 * np.exp(-sampleSec1)

        canThickness = canThickness1 + canThickness2

        if canThickness > 0.0:
            acc = (acc1 + acc2) / canThickness
            acsc = (acsc1 + acsc2) / canThickness

        return assc, acsc, acc


# Register algorithm with Mantid
AlgorithmFactory.subscribe(FlatPaalmanPingsCorrection)
