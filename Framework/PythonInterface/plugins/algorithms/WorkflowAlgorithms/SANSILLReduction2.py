# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSILLCommon import *
from mantid.api import PythonAlgorithm, MatrixWorkspace, MatrixWorkspaceProperty, WorkspaceProperty, \
    MultipleFileProperty, PropertyMode, Progress, WorkspaceGroup, FileAction
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, LogicOperator, PropertyCriterion, \
    StringListValidator
from mantid.simpleapi import *


class SANSILLReduction2(PythonAlgorithm):
    """Performs unit data reduction of the given process type"""

    mode = None # the acquisition mode of the reduction
    instrument = None # the name of the instrument
    n_samples = None # how many samples
    n_frames = None # how many frames per sample in case of kinetic
    process = None # the process type
    is_point = None # whether or not the input is point data
    n_reports = None # how many progress report checkpoints
    progress = None # the global progress reporter

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS data reduction of a given process type.'

    def seeAlso(self):
        return ['SANSILLIntegration']

    def name(self):
        return 'SANSILLReduction'

    def version(self):
        return 2

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and not self.getPropertyValue('EmptyBeamWorkspace'):
            issues['EmptyBeamWorkspace'] = 'Empty beam input workspace is mandatory for transmission calculation.'
        return issues

    def PyInit(self):

        #================================MAIN PARAMETERS================================#

        options = ['DarkCurrent', 'EmptyBeam', 'Transmission', 'EmptyContainer', 'Water', 'Solvent', 'Sample']

        self.declareProperty(MultipleFileProperty(name='Runs',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs'],
                                                  allow_empty=True),
                             doc='File path of run(s).')

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(WorkspaceProperty(name='OutputWorkspace',
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

        not_dark = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'DarkCurrent')
        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'EmptyBeam')
        not_beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'EmptyBeam')
        not_dark_nor_beam = EnabledWhenProperty(not_dark, not_beam, LogicOperator.And)
        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')
        beam_or_transmission = EnabledWhenProperty(beam, transmission, LogicOperator.Or)
        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'EmptyContainer')
        water = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Water')
        solvent = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Solvent')
        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')
        solvent_sample = EnabledWhenProperty(solvent, sample, LogicOperator.Or)
        water_solvent_sample = EnabledWhenProperty(solvent_sample, water, LogicOperator.Or)
        can_water_solvent_sample = EnabledWhenProperty(water_solvent_sample, container, LogicOperator.Or)

        self.declareProperty(name='NormaliseBy',
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Time', 'Monitor']),
                             doc='Choose the normalisation type.')

        self.declareProperty(name='BeamRadius',
                             defaultValue=0.2,
                             validator=FloatBoundedValidator(lower=0.),
                             doc='Beam radius [m]; used for beam center finding, transmission and flux calculations.')

        self.setPropertySettings('BeamRadius', beam_or_transmission)

        self.declareProperty(name='SampleThickness',
                             defaultValue=0.1,
                             validator=FloatBoundedValidator(lower=-1),
                             doc='Sample thickness [cm] (if -1, the value is taken from the nexus file).')

        self.setPropertySettings('SampleThickness', water_solvent_sample)

        self.declareProperty(name='WaterCrossSection',
                             defaultValue=1.,
                             doc='Provide water cross-section [cm-1]; used only if the absolute scale is performed by dividing to water.')

        self.setPropertySettings('WaterCrossSection', solvent_sample)

        self.declareProperty('TransmissionThetaDependent', True,
                             doc='Whether or not to use 2theta dependent transmission correction')
        self.setPropertySettings('TransmissionThetaDependent', can_water_solvent_sample)

        #================================INPUT WORKSPACES================================#

        self.declareProperty(MatrixWorkspaceProperty(name='DarkCurrentWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the Cd/B4C input workspace.')

        self.setPropertySettings('DarkCurrentWorkspace', not_dark)

        self.declareProperty(MatrixWorkspaceProperty(name='EmptyBeamWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('EmptyBeamWorkspace', not_dark_nor_beam)

        self.declareProperty(MatrixWorkspaceProperty(name='FluxWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input direct beam flux workspace.')

        self.setPropertySettings('FluxWorkspace', water_solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='TransmissionWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the transmission input workspace.')

        self.setPropertySettings('TransmissionWorkspace', can_water_solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='EmptyContainerWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the container input workspace.')

        self.setPropertySettings('EmptyContainerWorkspace', water_solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='FlatFieldWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the water input workspace.')

        self.setPropertySettings('FlatFieldWorkspace', solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='SolventWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the solvent input workspace.')

        self.setPropertySettings('SolventWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty(name='SensitivityWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input sensitivity map workspace.')

        self.setPropertySettings('SensitivityWorkspace', solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='DefaultMaskWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace to copy the mask from; for example, the bad detector edges.')

        self.setPropertySettings('DefaultMaskWorkspace', water_solvent_sample)

        self.declareProperty(MatrixWorkspaceProperty(name='MaskWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace to copy the mask from; for example, the beam stop')

        self.setPropertySettings('MaskWorkspace', water_solvent_sample)

        #================================AUX OUTPUT WORKSPACES================================#

        self.declareProperty(MatrixWorkspaceProperty('OutputSensitivityWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output sensitivity workspace.')

        self.setPropertySettings('OutputSensitivityWorkspace', water)

    def reset(self):
        '''Resets the class member variables'''
        self.instrument = None
        self.mode = None
        self.n_frames = None
        self.n_reports = None
        self.n_samples = None
        self.is_point = None
        self.process = None
        self.progress = None

    def setup(self, ws):
        '''Performs a full setup, which can be done only after having loaded the sample data'''
        processes = ['DarkCurrent', 'EmptyBeam', 'Transmission', 'Container', 'Water', 'Solvent', 'Sample']
        self.process = self.getPropertyValue('ProcessAs')
        self.instrument = ws.getInstrument().getName()
        self.log().notice(f'Set the instrument name to {self.instrument}')
        unit = ws.getAxis(0).getUnit().unitID()
        self.n_frames = ws.blocksize()
        self.log().notice(f'Set the number of frames to {self.n_frames}')
        if unit == 'Wavelength':
            self.mode = AcqMode.TOF
        else:
            if self.n_frames > 1:
                self.mode = AcqMode.KINETIC
            else:
                self.mode = AcqMode.MONO
        self.log().notice(f'Set the acquisition mode to {self.mode}')
        self.is_point = not ws.isHistogramData()
        self.log().notice(f'Set the point data flag to {self.is_point}')
        if self.mode == AcqMode.KINETIC:
            if self.process != 'Sample':
                raise RuntimeError('Only the sample can be a kinetic measurement, the auxiliary calibration measurements cannot.')
        self.progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(self.process) + 1)

    def normalise_by_time(self, ws):
        '''Normalises the given workspace to time and applies the dead time correction'''
        run = ws.getRun()
        if run.hasProperty('timer'):
            duration = run['timer'].value
            if duration != 0.:
                Scale(InputWorkspace=ws, Factor=1./duration, OutputWorkspace=ws)
                self.apply_dead_time(ws)
            else:
                raise RuntimeError('Unable to normalise to time; duration found is 0.')
        else:
            raise RuntimeError('Normalise to time requested, but duration is not available in the workspace.')

    def apply_normalisation(self, ws):
        '''Normalizes the workspace by monitor (default) or acquisition time'''
        normalise_by = self.getPropertyValue('NormaliseBy')
        monitor_ids = monitor_id(self.instrument)
        if normalise_by == 'Monitor':
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=ws, DetectorList=monitor_ids[0], OutputWorkspace=mon)
            if mtd[mon].readY(0)[0] == 0:
                raise RuntimeError('Normalise to monitor requested, but monitor has 0 counts.')
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
                DeleteWorkspace(mon)
        elif normalise_by == 'Time':
            if self.mode == AcqMode.KINETIC:
                # for kinetic, the durations are stored in the second monitor
                mon = ws + '_duration'
                ExtractSpectra(InputWorkspace=ws, DetectorList=monitor_ids[1], OutputWorkspace=mon)
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
                DeleteWorkspace(mon)
            else:
                normalise_by_time(mtd[ws])
        # regardless on normalisation mask out the monitors not to skew the scale in the instrument viewer
        # but do not extract them, since extracting by ID is slow, so just leave them masked
        MaskDetectors(Workspace=ws, DetectorList=monitor_ids)

    def apply_dead_time(self, ws):
        '''Performs the dead time correction'''
        instrument = ws.getInstrument()
        if instrument.hasParameter('tau'):
            tau = instrument.getNumberParameter('tau')[0]
            if self.instrument == 'D33' or self.instrument == 'D11B':
                grouping_filename = self._instrument + '_Grouping.xml'
                grouping_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, MapFile=grouping_file, OutputWorkspace=ws)
            elif instrument.hasParameter('grouping'):
                pattern = instrument.getStringParameter('grouping')[0]
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, GroupingPattern=pattern, OutputWorkspace=ws)
            else:
                self.log().warning('No grouping available in IPF, dead time correction will be performed detector-wise.')
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, OutputWorkspace=ws)
        else:
            self.log().notice('No tau available in IPF, skipping dead time correction.')

    def apply_dark_current(self, ws):
        '''Applies Cd/B4C subtraction'''
        cadmium_ws = self.getPropertyValue('DarkCurrentWorkspace')
        if cadmium_ws:
            check_processed_flag(mtd[cadmium_ws], 'DarkCurrent')
            Minus(LHSWorkspace=ws, RHSWorkspace=cadmium_ws, OutputWorkspace=ws)

    def apply_direct_beam(self, ws):
        '''Applies the beam center correction'''
        beam_ws = self.getPropertyValue('EmptyBeamWorkspace')
        if beam_ws:
            check_processed_flag(mtd[beam_ws], 'EmptyBeam')
            check_distances_match(mtd[ws], mtd[beam_ws])
            if self.mode != AcqMode.TOF:
                run = mtd[beam_ws].getRun()
                beam_x = run['BeamCenterX'].value
                beam_y = run['BeamCenterY'].value
                AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number')
                AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number')
                self.apply_multipanel_beam_center_corr(ws, beam_x, beam_y)
                if 'BeamWidthX' in run:
                    beam_width_x = run['BeamWidthX']
                    AddSampleLog(Workspace=ws, LogName='BeamWidthX', LogText=str(beam_width_x), LogType='Number',
                                 LogUnit='rad')

    def apply_multipanel_beam_center_corr(self, ws, beam_x, beam_y):
        '''Applies the beam center correction on multipanel detectors'''
        instrument = mtd[ws].getInstrument()
        l2_main = mtd[ws].getRun()['L2'].value
        panel_names = instrument.getStringParameter('detector_panels')[0].split(',')
        for panel in panel_names:
            l2_panel = instrument.getComponentByName(panel).getPos()[2]
            MoveInstrumentComponent(Workspace=ws, X=-beam_x * l2_panel/l2_main, Y=-beam_y * l2_panel/l2_main, ComponentName=panel)

    def apply_transmission(self, ws):
        '''Applies transmission correction'''
        tr_ws = self.getPropertyValue('TransmissionWorkspace')
        theta_dependent = self.getProperty('TransmissionThetaDependent').value
        if tr_ws:
            check_processed_flag(mtd[tr_ws], 'Transmission')
            if self.mode == AcqMode.TOF:
                # wavelength dependent transmission, need to rebin
                tr_ws_rebin = tr_ws + '_tr_rebinned'
                RebinToWorkspace(WorkspaceToRebin=tr_ws, WorkspaceToMatch=ws,
                                 OutputWorkspace=tr_ws_rebin)
                ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionWorkspace=tr_ws_rebin,
                                            ThetaDependent=theta_dependent, OutputWorkspace=ws)
                DeleteWorkspace(tr_ws_rebin)
            else:
                check_wavelengths_match(mtd[tr_ws], mtd[ws])
                if theta_dependent and self.instrument == 'D16' and 75 < mtd[ws].getRun()['Gamma.value'].value < 105:
                    # D16 can do wide angles, which means it can cross 90 degrees, where theta dependent transmission is divergent
                    # gamma is the detector center's theta, if it is in a certain range, then some pixels are around 90 degrees
                    MaskAngle(Workspace=ws, MinAngle=89, MaxAngle=91, Angle='TwoTheta')
                ApplyTransmissionCorrection(InputWorkspace=ws,
                                            TransmissionWorkspace=tr_ws,
                                            ThetaDependent=theta_dependent,
                                            OutputWorkspace=ws)

    def apply_container(self, ws):
        '''Applies empty container subtraction'''
        can_ws = self.getPropertyValue('EmptyContainerWorkspace')
        if can_ws:
            check_processed_flag(mtd[can_ws], 'EmptyContainer')
            check_distances_match(mtd[can_ws], mtd[ws])
            if self.mode == AcqMode.TOF:
                # wavelength dependent subtraction, need to rebin
                can_ws_rebin = can_ws + '_tr_rebinned'
                RebinToWorkspace(WorkspaceToRebin=can_ws, WorkspaceToMatch=ws,
                                 OutputWorkspace=can_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(can_ws_rebin)
            else:
                check_wavelengths_match(mtd[can_ws], mtd[ws])
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws, OutputWorkspace=ws)

    def load(self, ws):
        '''
        Loads, merges and concatenates the input runs, if needed
        Concatenation is allowed only for sample and transmission runs, if the mode is not TOF
        For all the rest, only summing is permitted
        The input might be either a numor or a processed nexus as a result of rebinning of event data in mantid
        Hence, we cannot specify a name of a specifc loader
        Besides, if it is processed nexus, it will be histogram data, which needs to be converted to point data
        Currently, the loader too will load to a histogram data for monochromatic non-kinetic, so we need to run a conversion
        This is redundant, since the loader could load directly to point-data
        The latter is a breaking change for the loader, so will require a new version
        The final output of this must be a workspace and not a workspace group
        '''
        #TODO: note that this operation is quite generic, so perhaps concatenation can become an option directly in LoadAndMerge
        LoadAndMerge(Filename=self.getPropertyValue('Runs'), OutputWorkspace=ws)
        if isinstance(mtd[ws], WorkspaceGroup):
            self.setup(mtd[ws][0])
            if self.mode != AcqMode.TOF:
                if self.process == 'Sample' or self.process == 'Transmission':
                    if not self.is_point:
                        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
                    # TODO: inject blank frames for the empty token placeholder here at the right index
                    ConjoinXRuns(InputWorkspaces=ws, OutputWorkspace=ws + '__joined')
                    DeleteWorkspace(Workspace=ws)
                    RenameWorkspace(InputWorkspace=ws + '__joined', OutputWorkspace=ws)
                else:
                    raise RuntimeError('Listing of runs in MONO mode is allowed only for sample and transmission measurements.')
            else:
                raise RuntimeError('Listing of runs is not allowed for TOF mode as concatenation of multiple runs is not possible.')
        else:
            self.setup(mtd[ws])
        assert isinstance(mtd[ws], MatrixWorkspace)
        return ws

    def reduce(self, ws):
        '''Performs the corresponding reduction based on the process type'''
        self.apply_normalisation(ws)
        if self.process != 'DarkCurrent':
            self.apply_dark_current(ws)
            if self.process == 'EmptyBeam':
                pass # process beam
            else:
                self.apply_direct_beam(ws)
                if self.process == 'Transmission':
                    pass # calculate transmission
                else:
                    self.apply_transmission(ws)
                    if self.process == 'Container':
                        pass # process container
                    else:
                        self.apply_container(ws)
                        if self.process == 'Water':
                            pass # process water
                        else:
                            # apply water
                            if self.process == 'Solvent':
                                pass # process solvent
                            else:
                                pass # apply solvent

    def PyExec(self):
        self.reset()
        ws = self.getPropertyValue('OutputWorkspace')
        self.load(ws)
        self.reduce(ws)
        self.setProperty('OutputWorkspace', ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction2)
