# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSILLCommon import *
from mantid.api import PythonAlgorithm, MatrixWorkspace, MatrixWorkspaceProperty, WorkspaceProperty, \
    MultipleFileProperty, PropertyMode, Progress, WorkspaceGroup, FileAction
from mantid.dataobjects import SpecialWorkspace2D
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, LogicOperator, PropertyCriterion, \
    StringListValidator
from mantid.simpleapi import *
import numpy as np

EMPTY_TOKEN = '000000' # empty run rumber to act as a placeholder where a sample measurement is missing


class SANSILLReduction(PythonAlgorithm):
    """
        Performs unit data reduction of the given process type
        Supports stadard monochromatic, kinetic and TOF measurements
        Supports D11, D16, D22, and D33 instruments at the ILL
    """

    mode = None # the acquisition mode of the reduction
    instrument = None # the name of the instrument
    n_samples = None # how many samples
    n_frames = None # how many frames per sample in case of kinetic
    process = None # the process type
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
        runs = self.getPropertyValue('Runs').split(',')
        non_blank_runs = list(filter(lambda x: x != EMPTY_TOKEN, runs))
        if not non_blank_runs:
            issues['Runs'] = 'Only blanks runs have been provided, there must be at least one non-blank run.'
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and not self.getPropertyValue('FluxWorkspace'):
            issues['FluxWorkspace'] = 'Empty beam flux input workspace is mandatory for transmission calculation.'
        samples_thickness = len(self.getProperty('SampleThickness').value)
        if samples_thickness != 1 and samples_thickness != self.getPropertyValue('Runs').count(',') + 1:
            issues['SampleThickness'] = 'Sample thickness must have either a single value or as many as there are samples.'
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
                             defaultValue=[0.1],
                             doc='Sample thickness [cm] (if -1, the value is taken from the nexus file).')

        self.setPropertySettings('SampleThickness', water_solvent_sample)

        self.declareProperty(name='WaterCrossSection',
                             defaultValue=1.,
                             doc='Provide water cross-section [cm-1]; used only if the absolute scale is performed by dividing to water.')

        self.setPropertySettings('WaterCrossSection', solvent_sample)

        self.declareProperty(name='TransmissionThetaDependent',
                             defaultValue=True,
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
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input empty beam flux workspace.')

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

        self.declareProperty(MatrixWorkspaceProperty('OutputFluxWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output empty beam flux workspace.')

        self.setPropertySettings('OutputFluxWorkspace', beam)

    def reset(self):
        '''Resets the class member variables'''
        self.instrument = None
        self.mode = None
        self.n_frames = None
        self.n_reports = None
        self.n_samples = None
        self.process = None
        self.progress = None

    def setup(self, ws):
        '''Performs a full setup, which can be done only after having loaded the sample data'''
        processes = ['DarkCurrent', 'EmptyBeam', 'Transmission', 'EmptyContainer', 'Water', 'Solvent', 'Sample']
        self.process = self.getPropertyValue('ProcessAs')
        self.instrument = ws.getInstrument().getName()
        self.log().notice(f'Set the instrument name to {self.instrument}')
        unit = ws.getAxis(0).getUnit().unitID()
        self.n_frames = ws.blocksize()
        self.log().notice(f'Set the number of frames to {self.n_frames}')
        if self.n_frames > 1:
            if unit == 'Wavelength':
                self.mode = AcqMode.TOF
            elif unit == 'TOF':
                self.mode = AcqMode.REVENT
            else:
                self.mode = AcqMode.KINETIC
        else:
            self.mode = AcqMode.MONO
        self.log().notice(f'Set the acquisition mode to {self.mode}')
        if self.mode == AcqMode.KINETIC:
            if self.process != 'Sample':
                raise RuntimeError('Only the sample can be a kinetic measurement, the auxiliary calibration measurements cannot.')
        self.progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(self.process) + 1)

    #==============================METHODS TO APPLY CORRECTIONS==============================#

    def apply_normalisation(self, ws):
        '''Normalizes the workspace by monitor (default) or acquisition time'''
        normalise_by = self.getPropertyValue('NormaliseBy')
        monitor_ids = monitor_id(self.instrument)
        if normalise_by == 'Monitor':
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=ws, DetectorList=monitor_ids[0], OutputWorkspace=mon)
            Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws, WarnOnZeroDivide=False)
            DeleteWorkspace(mon)
        elif normalise_by == 'Time':
            # the durations are stored in the second monitor
            mon = ws + '_duration'
            ExtractSpectra(InputWorkspace=ws, DetectorList=monitor_ids[1], OutputWorkspace=mon)
            Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws, WarnOnZeroDivide=False)
            self.apply_dead_time(ws)
            DeleteWorkspace(mon)
        # regardless on normalisation mask out the monitors not to skew the scale in the instrument viewer
        # but do not extract them, since extracting by ID is slow, so just leave them masked
        MaskDetectors(Workspace=ws, DetectorList=monitor_ids)

    def apply_dead_time(self, ws):
        '''Performs the dead time correction'''
        instrument = mtd[ws].getInstrument()
        if instrument.hasParameter('tau'):
            tau = instrument.getNumberParameter('tau')[0]
            if self.instrument == 'D33' or self.instrument == 'D11B':
                grouping_filename = self.instrument + '_Grouping.xml'
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
            if self.mode == AcqMode.TOF:
                cadmium_ws_rebin = cadmium_ws + '_rebinned'
                RebinToWorkspace(WorkspaceToRebin=cadmium_ws, WorkspaceToMatch=ws,
                                 OutputWorkspace=cadmium_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=cadmium_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(cadmium_ws_rebin)
            else:
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
                    AddSampleLog(Workspace=ws, LogName='BeamWidthX', LogText=str(run['BeamWidthX'].value),
                                 LogType='Number', LogUnit='rad')

    def apply_multipanel_beam_center_corr(self, ws, beam_x, beam_y):
        '''Applies the beam center correction on multipanel detectors'''
        instrument = mtd[ws].getInstrument()
        l2_main = mtd[ws].getRun()['L2'].value
        if instrument.hasParameter('detector_panels'):
            panel_names = instrument.getStringParameter('detector_panels')[0].split(',')
            for panel in panel_names:
                l2_panel = instrument.getComponentByName(panel).getPos()[2]
                MoveInstrumentComponent(Workspace=ws, X=-beam_x * l2_panel/l2_main, Y=-beam_y * l2_panel/l2_main, ComponentName=panel)
        else:
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')

    def apply_flux(self, ws):
        '''Applies empty beam flux absolute scale normalisation'''
        flux_ws = self.getPropertyValue('FluxWorkspace')
        if flux_ws:
            if self.mode == AcqMode.TOF:
                tmp = flux_ws + '_rebinned'
                RebinToWorkspace(WorkspaceToRebin=flux_ws, WorkspaceToMatch=ws, OutputWorkspace=tmp)
                Divide(LHSWorkspace=ws, RHSWorkspace=tmp, OutputWorkspace=ws, WarnOnZeroDivide=False)
                DeleteWorkspace(tmp)
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)
            AddSampleLog(Workspace=ws, LogText='True', LogType='String', LogName='NormalisedByFlux')

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
                tr_to_apply = tr_ws
                needs_broadcasting = self.mode == AcqMode.KINETIC and mtd[tr_ws].blocksize() < mtd[ws].blocksize()
                if needs_broadcasting:
                    # if the sample is kinetic, but the transmission is monochromatic, need to broadcast
                    # sometimes, the transmission itself can be kinetic, in which case there is nothing to do
                    # furthermore, in some configurations the same scattering run can be used for transmission calculation
                    tr_to_apply = self.broadcast_kinetic(tr_ws)
                ApplyTransmissionCorrection(InputWorkspace=ws,
                                            TransmissionWorkspace=tr_to_apply,
                                            ThetaDependent=theta_dependent,
                                            OutputWorkspace=ws)
                if needs_broadcasting:
                    DeleteWorkspace(tr_to_apply)
                if theta_dependent and self.instrument == 'D16' and 75 < mtd[ws].getRun()['Gamma.value'].value < 105:
                    # D16 can do wide angles, which means it can cross 90 degrees, where theta dependent transmission is divergent
                    # gamma is the detector center's theta, if it is in a certain range, then some pixels are around 90 degrees
                    MaskAngle(Workspace=ws, MinAngle=89, MaxAngle=91, Angle='TwoTheta')

    def broadcast_kinetic(self, ws):
        '''
        Broadcasts the given workspace to the dimensions of the sample workspace
        Repeats the values by the number of frames in order to allow vectorized application of the correction
        '''
        x = mtd[ws].readX(0)
        y = mtd[ws].readY(0)
        e = mtd[ws].readE(0)
        out = ws + '_broadcast'
        CreateWorkspace(ParentWorkspace=ws, OutputWorkspace=out, NSpec=1,
                        DataX=np.repeat(x, self.n_frames),
                        DataY=np.repeat(y, self.n_frames),
                        DataE=np.repeat(e, self.n_frames))
        return out

    def apply_container(self, ws):
        '''Applies empty container subtraction'''
        can_ws = self.getPropertyValue('EmptyContainerWorkspace')
        if can_ws:
            check_processed_flag(mtd[can_ws], 'EmptyContainer')
            check_distances_match(mtd[can_ws], mtd[ws])
            if self.mode == AcqMode.TOF:
                # wavelength dependent subtraction, need to rebin
                can_ws_rebin = can_ws + '_rebinned'
                RebinToWorkspace(WorkspaceToRebin=can_ws, WorkspaceToMatch=ws,
                                 OutputWorkspace=can_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(can_ws_rebin)
            else:
                check_wavelengths_match(mtd[can_ws], mtd[ws])
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws, OutputWorkspace=ws)

    def apply_water(self, ws):
        '''Applies flat-field (water) normalisation for detector efficiency and absolute scale'''
        flat_ws = self.getPropertyValue('FlatFieldWorkspace')
        if flat_ws:
            check_processed_flag(mtd[flat_ws], 'Water')
            # do not check for distance, since flat field is typically not available at the longest distances
            if self.mode != AcqMode.TOF:
                check_wavelengths_match(mtd[flat_ws], mtd[ws])
            # flat field is time-independent, so even for tof and kinetic it must be just one frame
            Divide(LHSWorkspace=ws, RHSWorkspace=flat_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
            Scale(InputWorkspace=ws, Factor=self.getProperty('WaterCrossSection').value, OutputWorkspace=ws)
            # copy the mask of water to the ws as it might be larger than the beam stop used for ws
            MaskDetectors(Workspace=ws, MaskedWorkspace=flat_ws)
            # rescale the absolute scale if the ws and flat field are at different distances
            self.rescale_flux(ws, flat_ws)

    def rescale_flux(self, ws, flat_ws):
        '''
            Rescales the absolute scale if ws and flat_ws are at different distances
            If both sample and flat runs are normalised by flux, there is nothing to do, they are both in abs scale
            If one is normalised, the other is not, we raise an error
            If neither is normalised by flux, only then we have to rescale by the factor
        '''
        message = 'Sample and flat field runs are not consistent in terms of flux normalisation; ' \
                  'unable to perform flat field normalisation. ' \
                  'Make sure either they are both normalised or both not normalised by direct beam flux.'
        run = mtd[ws].getRun()
        run_ref = mtd[flat_ws].getRun()
        has_log = run.hasProperty('NormalisedByFlux')
        has_log_ref = run_ref.hasProperty('NormalisedByFlux')
        if has_log != has_log_ref:
            raise RuntimeError(message)
        if has_log and has_log_ref:
            log_val = run['NormalisedByFlux'].value
            log_val_ref = run_ref['NormalisedByFlux'].value
            if log_val != log_val_ref:
                raise RuntimeError(message)
            elif not log_val:
                self.do_rescale_flux(ws, flat_ws)
        else:
            raise RuntimeError(message)

    def do_rescale_flux(self, ws, flat_ws):
        '''
        Scales ws by the flux factor wrt the flat field
        Formula 14, Grillo I. (2008) Small-Angle Neutron Scattering and Applications in Soft Condensed Matter
        '''
        if self.mode != AcqMode.TOF:
            check_wavelengths_match(mtd[ws], mtd[flat_ws])
        sample_l2 = mtd[ws].getRun()['L2'].value
        ref_l2 = mtd[flat_ws].getRun()['L2'].value
        flux_factor = (sample_l2 ** 2) / (ref_l2 ** 2)
        self.log().notice(f'Flux factor is: {flux_factor}')
        Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)

    def apply_solvent(self, ws):
        '''Applies pixel-by-pixel solvent/buffer subtraction'''
        solvent_ws = self.getPropertyValue('SolventWorkspace')
        if solvent_ws:
            check_processed_flag(mtd[solvent_ws], 'Solvent')
            check_distances_match(mtd[solvent_ws], mtd[ws])
            if self.mode == AcqMode.TOF:
                # wavelength dependent subtraction, need to rebin
                solvent_ws_rebin = solvent_ws + '_tr_rebinned'
                RebinToWorkspace(WorkspaceToRebin=solvent_ws, WorkspaceToMatch=ws,
                                 OutputWorkspace=solvent_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=solvent_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(solvent_ws_rebin)
            else:
                check_wavelengths_match(mtd[solvent_ws], mtd[ws])
                Minus(LHSWorkspace=ws, RHSWorkspace=solvent_ws, OutputWorkspace=ws)

    def apply_solid_angle(self, ws):
        '''Calculates solid angle and divides by it'''
        sa_ws = ws + '_solidangle'
        # D22B has the front panel tilted, hence the Rectangle approximation is wrong
        # D16 can be rotated around the sample, where again rectangle is wrong unless we rotate back
        method = 'GenericShape' if self.instrument == 'D22B' or seld.instrument == 'D16' else 'Rectangle'
        SolidAngle(InputWorkspace=ws, OutputWorkspace=sa_ws, Method=method)
        Divide(LHSWorkspace=ws, RHSWorkspace=sa_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
        DeleteWorkspace(Workspace=sa_ws)

    def apply_masks(self, ws):
        '''
        Applies default (edges and permanently bad pixels) and beam stop masks
        Masks can be created in many ways: manually, via MaskBTP, etc.
        When creating a mask with instrument viewer, there are 2 possibilities:
        o) hitting the button Apply and Save as > Detector Mask to Workspace
            This is the legacy way of propagating the masks.
            In this case the workspace is a SpecialWorkspace2D which just contains 1s as intensity for the detector that is masked
        o) hitting the button Apply to Data
            In this case the workspace has a proper mask.
        '''
        edge_mask = self.getProperty('DefaultMaskWorkspace').value
        if edge_mask and (isinstance(edge_mask, SpecialWorkspace2D) or edge_mask.detectorInfo().hasMaskedDetectors()):
            MaskDetectors(Workspace=ws, MaskedWorkspace=edge_mask)
        beam_stop_mask = self.getProperty('MaskWorkspace').value
        if beam_stop_mask and (isinstance(beam_stop_mask, SpecialWorkspace2D) or beam_stop_mask.detectorInfo().hasMaskedDetectors()):
            MaskDetectors(Workspace=ws, MaskedWorkspace=beam_stop_mask)

    def apply_thickness(self, ws):
        '''Normalises by sample thickness'''
        thicknesses = self.getProperty('SampleThickness').value
        length = len(thicknesses)
        if length == 1:
            Scale(InputWorkspace=ws, Factor=1/thicknesses[0], OutputWorkspace=ws)
        elif length > 1:
            thick_ws = ws + '_thickness'
            CreateWorkspace(NSpec=1, OutputWorkspace=thick_ws,
                            DataY=np.array(length), DataE=np.zeros(length),
                            DataX=np.arange(length))
            thick_to_apply = thick_ws
            if self.mode == AcqMode.KINETIC:
                thick_to_apply = self.broadcast_kinetic(thick_ws)
            Divide(LHSWorkspace=ws, RHSWorkspace=thick_to_apply, OutputWorkspace=ws)
            DeleteWorkspace(thick_to_apply)
            if mtd[thick_ws]:
                DeleteWorkspace(thick_ws)

    def apply_parallax(self, ws):
        '''Applies the parallax correction'''
        components = ['detector']
        offsets = [0.]
        if self.instrument == 'D22B':
            # The front detector of D22B is often tilted around its own axis
            # The tilt angle must be subtracted from 2thetas before putting them into the parallax correction formula
            # TODO: note that in cycle 211, the front detector was Detector 1, so we should rather get it from IPF
            offsets.append(mtd[ws].getRun()['Detector 2.dan2_actual'].value)
        if mtd[ws].getInstrument().hasParameter('detector_panels'):
            components = mtd[ws].getInstrument().getStringParameter('detector_panels')[0].split(',')
            ParallaxCorrection(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames=components, AngleOffsets=offsets)

    #===============================METHODS TO PROCESS BY TYPE===============================#

    def treat_empty_beam(self, ws):
        '''Processes as empty beam, i.e. calculates beam center, beam width and incident flux'''
        centers = ws + '_centers'
        radius = self.getProperty('BeamRadius').value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=True, BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0,1)
        beam_y = mtd[centers].cell(1,1)
        AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number', LogUnit='meters')
        AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number', LogUnit='meters')
        DeleteWorkspace(centers)
        if self.mode != AcqMode.TOF:
            # correct for beam center before calculating the beam width for resolution
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
            self.fit_beam_width(ws)
        self.calculate_flux(ws)

    def calculate_flux(self, ws):
        '''Calculates the incident flux'''
        if self.process == 'EmptyBeam':
            flux = self.getPropertyValue('OutputFluxWorkspace')
            if not flux:
                return
        elif self.process == 'Transmission':
            flux = ws
        run = mtd[ws].getRun()
        att_coeff = 1.
        if run.hasProperty('attenuator.attenuation_coefficient'):
            att_coeff = run['attenuator.attenuation_coefficient'].value
        elif run.hasProperty('attenuator.attenuation_value'):
            att_value = run['attenuator.attenuation_value'].value
            if float(att_value) < 10. and self.instrument == 'D33':
                # for D33, it's not always the attenuation value, it could be the index of the attenuator
                # if it is <10, we consider it's the index and take the corresponding value from the IPF
                instrument = mtd[ws].getInstrument()
                param = 'att'+str(int(att_value))
                if instrument.hasParameter(param):
                    att_coeff = instrument.getNumberParameter(param)[0]
                else:
                    raise RuntimeError(f'Unable to find the attenuation coefficient for D33 attenuator #{att_value}')
            else:
                att_coeff = float(att_value)
        self.log().information('Attenuator 1 coefficient/value: {0}'.format(att_coeff))
        if run.hasProperty('attenuator2.attenuation_value'):
            # D22 can have the second, chopper attenuator
            # In principle, either of the 2 attenuators can be in or out
            # In practice, only one (standard or chopper) is in at a time
            # If one is out, its attenuation_value is set to 1, so it's safe to take the product
            att2_value = run['attenuator2.attenuation_value'].value
            self.log().information(f'Attenuator 2 coefficient/value: {att2_value}')
            att_coeff *= float(att2_value)
        self.log().information(f'Attenuation coefficient used is: {att_coeff}')
        radius = self.getProperty('BeamRadius').value
        CalculateFlux(InputWorkspace=ws, OutputWorkspace=flux, BeamRadius=radius)
        Scale(InputWorkspace=flux, Factor=att_coeff, OutputWorkspace=flux)
        if self.process == 'EmptyBeam':
            if self.mode == AcqMode.TOF:
                self.prepare_tof_flux(ws)
            self.setProperty('OutputFluxWorkspace', flux)

    def prepare_tof_flux(self, ws):
        '''Broadcasts the direct beam flux that can be easily rebinned at application time'''
        # for TOF, the flux is wavelength dependent, and the sample workspace is ragged
        # hence the flux must be rebinned to the sample before it can be divided by flux
        # that's why we broadcast the empty beam workspace to the same size as the sample
        # this allows rebinning spectrum-wise when processing the sample w/o tiling the empty beam data for each sample
        # for mono however, this must be a single count workspace
        nspec = mtd[ws].getNumberHistograms()
        x = mtd[flux].readX(0)
        y = mtd[flux].readY(0)
        e = mtd[flux].readE(0)
        CreateWorkspace(DataX=x, DataY=np.tile(y, nspec), DataE=np.tile(e, nspec), NSpec=nspec,
                        ParentWorkspace=ws, OutputWorkspace=flux)

    def fit_beam_width(self, ws):
        ''''Groups detectors vertically and fits the horizontal beam width with a Gaussian profile'''
        tmp_ws = ws + '_beam_width'
        CloneWorkspace(InputWorkspace=ws, OutputWorkspace=tmp_ws)
        grouping_pattern = get_vertical_grouping_pattern(tmp_ws)
        if not grouping_pattern: # unsupported instrument
            return
        GroupDetectors(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, GroupingPattern=grouping_pattern)
        ConvertSpectrumAxis(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, Target='SignedInPlaneTwoTheta')
        Transpose(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws)
        background = 'name=FlatBackground, A0=1e-4'
        distribution_width = np.max(mtd[tmp_ws].getAxis(0).extractValues())
        function = "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2}".format(
            0, np.max(mtd[tmp_ws].getAxis(1).extractValues()), 0.1*distribution_width)
        constraints = "{0} < f1.PeakCentre < {1}".format(-0.1*distribution_width, 0.1*distribution_width)
        fit_function = [background, function]
        fit_output = Fit(Function=';'.join(fit_function),
                         InputWorkspace=tmp_ws,
                         Constraints=constraints,
                         CreateOutput=False,
                         IgnoreInvalidData=True,
                         Output=tmp_ws+"_fit_output")
        param_table = fit_output.OutputParameters
        beam_width = param_table.column(1)[3] * np.pi / 180.0
        AddSampleLog(Workspace=ws, LogName='BeamWidthX', LogText=str(beam_width), LogType='Number', LogUnit='rad')
        DeleteWorkspaces(WorkspaceList=[tmp_ws, tmp_ws+'_fit_output_Parameters', tmp_ws+'_fit_output_Workspace',
                                        tmp_ws+'_fit_output_NormalisedCovarianceMatrix'])

    def calculate_transmission(self, ws):
        '''Calculates the transmission'''
        flux_ws = self.getPropertyValue('FluxWorkspace')
        check_distances_match(mtd[ws], mtd[flux_ws])
        self.calculate_flux(ws)
        if self.mode != AcqMode.TOF:
            check_wavelengths_match(mtd[ws], mtd[flux_ws])
        else:
            RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=flux_ws, OutputWorkspace=ws)
        Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)

    def generate_sensitivity(self, ws):
        '''Creates relative inter-pixel efficiency map'''
        sens = self.getPropertyValue('OutputSensitivityWorkspace')
        if sens:
            CalculateEfficiency(InputWorkspace=ws, OutputWorkspace=sens)
            self.setProperty('OutputSensitivityWorkspace', mtd[sens])

    def inject_blank_samples(self, ws):
        '''Creates blank workspaces with the right dimension to include in the input list of workspaces'''
        reference = mtd[ws][0]
        nbins = self.n_frames
        nspec = reference.getNumberHistograms()
        xaxis = reference.getAxis(0).extractValues()
        runs = self.getPropertyValue('Runs').split(',')
        result = [w.getName() for w in mtd[ws]]
        blank_indices = [i for i,r in enumerate(runs) if r == EMPTY_TOKEN]
        for index in blank_indices:
            blank = f'__blank_{index}'
            CreateWorkspace(ParentWorkspace=reference, NSpec=nspec, DataX=np.tile(xaxis, nspec),
                            DataY=np.zeros(nspec * nbins), DataE=np.zeros(nspec * nbins),
                            OutputWorkspace=blank, UnitX='Empty')
            result.insert(index,blank)
        return result

    def set_process_as(self, ws):
        '''Sets the process as flag as sample log for future sanity checks'''
        AddSampleLog(Workspace=ws, LogName='ProcessedAs', LogText=self.process)

    #===============================ENTRY POINT AND CORE LOGIC===============================#

    def PyExec(self):
        self.reset()
        self.load()
        self.reduce()

    def load(self):
        '''
        Loads, merges and concatenates the input runs, if needed
        TODO: once v2 of the loader is in, move this out to a separate algorithm
        '''
        ws = self.getPropertyValue('OutputWorkspace')
        tmp = f'__{ws}'
        runs = self.getPropertyValue('Runs').split(',')
        non_blank_runs = list(filter(lambda x: x != EMPTY_TOKEN, runs))
        blank_runs = list(filter(lambda x: x == EMPTY_TOKEN, runs))
        LoadAndMerge(Filename=','.join(non_blank_runs), OutputWorkspace=tmp)
        if isinstance(mtd[tmp], MatrixWorkspace) and blank_runs:
            # if we loaded a single workspace but there are blanks, need to make a group so that the blanks can be insterted
            RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=tmp+'_0')
            GroupWorkspaces(InputWorkspaces=[tmp+'_0'], OutputWorkspace=tmp)
        if isinstance(mtd[tmp], WorkspaceGroup):
            self.setup(mtd[tmp][0])
            self.preprocess_group(tmp)
            if self.mode != AcqMode.TOF:
                if self.process == 'Sample' or self.process == 'Transmission':
                    ws_list = self.inject_blank_samples(tmp)
                    ConjoinXRuns(InputWorkspaces=ws_list, OutputWorkspace=ws, LinearizeAxis=True)
                    DeleteWorkspaces(WorkspaceList=ws_list)
                else:
                    raise RuntimeError('Listing of runs in MONO mode is allowed only for sample and transmission measurements.')
            else:
                raise RuntimeError('Listing of runs is not allowed for TOF mode as concatenation of multiple runs is not possible.')
        else:
            self.setup(mtd[tmp])
            self.preprocess(tmp)
            if self.mode == AcqMode.REVENT or self.mode == AcqMode.MONO:
                self.linearize_axis(tmp)
            RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=ws)
        self.set_process_as(ws)
        assert isinstance(mtd[ws], MatrixWorkspace)
        return ws

    def linearize_axis(self, ws):
        '''Linearizes x-axis for a single rebinned event workspace to transform it to kinetic'''
        x = np.arange(mtd[ws].blocksize())
        for s in range(mtd[ws].getNumberHistograms()):
            mtd[ws].setX(s, x)

    def preprocess_group(self, wsg):
        '''Preprocesses a loaded workspace group'''
        for ws in mtd[wsg]:
            self.preprocess(ws.name())

    def preprocess(self, ws):
        '''Prepares the loaded workspace based on the acq mode'''
        # TODO: all these must be done in v2 of the loader directly
        # this should be removed once v2 of the loader is plugged in
        if self.mode != AcqMode.TOF:
            mtd[ws].getAxis(0).setUnit('Empty')
        if self.mode == AcqMode.MONO or self.mode == AcqMode.REVENT:
            ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
            blank_mon = mtd[ws].getNumberHistograms() + blank_monitor_ws_neg_index(self.instrument)
            run = mtd[ws].getRun()
            if 'duration' in run:
                time = run['duration'].value
            elif 'time' in run:
                time = run['time'].value
            elif 'timer' in run:
                time = run['timer'].value
            if self.mode == AcqMode.MONO:
                mtd[ws].setY(blank_mon, np.array([time]))
                mtd[ws].setE(blank_mon, np.array([0.]))
            else:
                bsize = mtd[ws].blocksize()
                mtd[ws].setY(blank_mon, np.full(bsize, time))
                mtd[ws].setE(blank_mon, np.full(bsize, 0.))

    def reduce(self):
        '''
        Performs the corresponding reduction based on the process type
        This is the core logic of the reduction realised as a hard-wired dependency graph, for example:
        If we are processing the empty beam we apply the dark current correction and process as empty beam
        If we are processing transmission, we apply both dark current and empty beam corrections and process as transmission
        '''
        ws = self.getPropertyValue('OutputWorkspace')
        self.apply_normalisation(ws)
        if self.process != 'DarkCurrent':
            self.apply_dark_current(ws)
            if self.process == 'EmptyBeam':
                self.treat_empty_beam(ws)
            else:
                self.apply_direct_beam(ws)
                if self.process == 'Transmission':
                    self.calculate_transmission(ws)
                else:
                    self.apply_transmission(ws)
                    self.apply_solid_angle(ws)
                    if self.process != 'EmptyContainer':
                        self.apply_container(ws)
                        self.apply_masks(ws)
                        self.apply_parallax(ws)
                        self.apply_thickness(ws)
                        self.apply_flux(ws)
                        if self.process == 'Water':
                            self.generate_sensitivity(ws)
                        else:
                            self.apply_water(ws)
                            if self.process != 'Solvent':
                                self.apply_solvent(ws)
        self.setProperty('OutputWorkspace', ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction)
