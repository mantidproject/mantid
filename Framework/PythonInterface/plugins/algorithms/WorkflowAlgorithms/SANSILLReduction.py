# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, Progress, WorkspaceGroup
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, LogicOperator, PropertyCriterion, StringListValidator
from mantid.simpleapi import *
from math import fabs
import numpy as np


class SANSILLReduction(PythonAlgorithm):

    _mode = 'Monochromatic'
    _instrument = None

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS data reduction at the ILL.'

    def seeAlso(self):
        return ['SANSILLIntegration']

    def name(self):
        return 'SANSILLReduction'

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'
        return issues

    @staticmethod
    def _make_solid_angle_name(ws):
        return mtd[ws].getInstrument().getName()+'_'+str(int(mtd[ws].getRun().getLogData('L2').value))+'m_SolidAngle'

    @staticmethod
    def _check_distances_match(ws1, ws2):
        """
            Checks if the detector distance between two workspaces are close enough
            @param ws1 : workspace 1
            @param ws2 : workspace 2
        """
        tolerance = 0.01 #m
        l2_1 = ws1.getRun().getLogData('L2').value
        l2_2 = ws2.getRun().getLogData('L2').value
        r1 = ws1.getRunNumber()
        r2 = ws2.getRunNumber()
        if fabs(l2_1 - l2_2) > tolerance:
            logger.warning('Different distances detected! {0}: {1}, {2}: {3}'.format(r1, l2_1, r2, l2_2))

    @staticmethod
    def _check_processed_flag(ws, value):
        return ws.getRun().getLogData('ProcessedAs').value == value

    @staticmethod
    def _cylinder(radius):
        """
            Returns XML for an infinite cylinder with axis of z (beam) and given radius [m]
            @param radius : the radius of the cylinder [m]
            @return : XML string for the geometry shape
        """
        return '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>' \
               '<radius val="{0}"/></infinite-cylinder>'.format(radius)

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Reference', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

        not_absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Absorber')

        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')

        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Beam')

        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')

        not_beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Beam')

        reference = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Reference')

        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Container')

        self.declareProperty(name='NormaliseBy',
                             defaultValue='Timer',
                             validator=StringListValidator(['None', 'Timer', 'Monitor']),
                             doc='Choose the normalisation type.')

        self.declareProperty('BeamRadius', 0.05, validator=FloatBoundedValidator(lower=0.),
                             doc='Beam raduis [m]; used for beam center finding, transmission and flux calculations.')

        self.setPropertySettings('BeamRadius',
                                 EnabledWhenProperty(beam, transmission, LogicOperator.Or))

        self.declareProperty('BeamFinderMethod', 'DirectBeam', StringListValidator(['DirectBeam', 'ScatteredBeam']),
                             doc='Choose between direct beam or scattered beam method for beam center finding.')

        self.setPropertySettings('BeamFinderMethod', beam)

        self.declareProperty('SampleThickness', 0.1, validator=FloatBoundedValidator(lower=0.), doc='Sample thickness [cm]')

        self.setPropertySettings('SampleThickness', EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('AbsorberInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace.')

        self.setPropertySettings('AbsorberInputWorkspace', not_absorber)

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('BeamInputWorkspace',
                                 EnabledWhenProperty(not_absorber, not_beam, LogicOperator.And))

        self.declareProperty(MatrixWorkspaceProperty('TransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the transmission input workspace.')

        self.setPropertySettings('TransmissionInputWorkspace',
                                 EnabledWhenProperty(container,
                                                     EnabledWhenProperty(reference, sample, LogicOperator.Or),
                                                     LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('ContainerInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the container workspace.')

        self.setPropertySettings('ContainerInputWorkspace',
                                 EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('ReferenceInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the reference workspace.')

        self.setPropertySettings('ReferenceInputWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty('SensitivityInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input sensitivity workspace.')

        self.setPropertySettings('SensitivityInputWorkspace',
                                 EnabledWhenProperty(sample,
                                                     EnabledWhenProperty('ReferenceInputWorkspace',
                                                                         PropertyCriterion.IsEqualTo, ''),
                                                     LogicOperator.And))

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output sensitivity workspace.')

        self.setPropertySettings('SensitivityOutputWorkspace', reference)

        self.declareProperty(MatrixWorkspaceProperty('MaskedInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace to copy the mask from; for example, the beam stop')

        self.setPropertySettings('MaskedInputWorkspace', EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('FluxInputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input direct beam flux workspace.')

        self.setPropertySettings('FluxInputWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty('FluxOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output direct beam flux workspace.')

        self.setPropertySettings('FluxOutputWorkspace', beam)

        self.declareProperty('CacheSolidAngle', False, doc='Whether or not to cache the solid angle workspace.')

        self.declareProperty('WaterCrossSection', 1., doc='Provide water cross-section; '
                                                          'used only if the absolute scale is done by dividing to water.')

    def _normalise(self, ws):
        """
            Normalizes the workspace by time (SampleLog Timer) or Monitor (ID=100000)
            @param ws : the input workspace
        """
        normalise_by = self.getPropertyValue('NormaliseBy')
        if normalise_by == 'Monitor':
            mon = ws + '_mon'
            monID = 100000
            if mtd[ws].getInstrument().getName() == 'D33':
                monID = 500000
            ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=mon)
            if mtd[mon].readY(0)[0] == 0:
                raise RuntimeError('Normalise to monitor requested, but monitor has 0 counts.')
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
                DeleteWorkspace(mon)
        elif normalise_by == 'Timer':
            if mtd[ws].getRun().hasProperty('timer'):
                duration = mtd[ws].getRun().getLogData('timer').value
                if duration != 0.:
                    Scale(InputWorkspace=ws, Factor=1./duration, OutputWorkspace=ws)
                    self._apply_dead_time(ws)
                else:
                    raise RuntimeError('Unable to normalise to time; duration found is 0 seconds.')
            else:
                raise RuntimeError('Normalise to timer requested, but timer information is not available.')

    def _process_beam(self, ws):
        """
            Calculates the beam center's x,y coordinates, and the beam flux
            @param ws : the input [empty beam] workspace
        """
        centers = ws + '_centers'
        method = self.getPropertyValue('BeamFinderMethod')
        radius = self.getProperty('BeamRadius').value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=(method == 'DirectBeam'), BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0,1)
        beam_y = mtd[centers].cell(1,1)
        AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number')
        AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number')
        DeleteWorkspace(centers)
        if self._mode != 'TOF':
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
        run = mtd[ws].getRun()
        if run.hasProperty('attenuator.attenuation_coefficient'):
            att_coeff = run.getLogData('attenuator.attenuation_coefficient').value
        elif run.hasProperty('attenuator.attenuation_value'):
            att_value = run.getLogData('attenuator.attenuation_value').value
            if float(att_value) < 10. and self._instrument == 'D33':
                instrument = mtd[ws].getInstrument()
                param = 'att'+str(int(att_value))
                if instrument.hasParameter(param):
                    att_coeff = instrument.getNumberParameter(param)[0]
                else:
                    raise RuntimeError('Unable to find the attenuation coefficient for D33 attenuator #'+str(int(att_value)))
            else:
                att_coeff = att_value
        else:
            raise RuntimeError('Unable to process as beam: could not find attenuation coefficient nor value.')
        self.log().information('Found attenuator coefficient/value: {0}'.format(att_coeff))
        flux_out = self.getPropertyValue('FluxOutputWorkspace')
        if flux_out:
            flux = ws + '_flux'
            CalculateFlux(InputWorkspace=ws, OutputWorkspace=flux, BeamRadius=radius)
            Scale(InputWorkspace=flux, Factor=att_coeff, OutputWorkspace=flux)
            nspec = mtd[ws].getNumberHistograms()
            x = mtd[flux].readX(0)
            y = mtd[flux].readY(0)
            e = mtd[flux].readE(0)
            CreateWorkspace(DataX=x, DataY=np.tile(y, nspec), DataE=np.tile(e, nspec), NSpec=nspec,
                            ParentWorkspace=flux, UnitX='Wavelength', OutputWorkspace=flux)
            mtd[flux].getRun().addProperty('ProcessedAs', 'Beam', True)
            RenameWorkspace(InputWorkspace=flux, OutputWorkspace=flux_out)
            self.setProperty('FluxOutputWorkspace', mtd[flux_out])

    def _process_transmission(self, ws, beam_ws):
        """
            Calculates the transmission
            @param ws: input workspace name
            @param beam_ws: empty beam workspace
        """
        self._check_distances_match(mtd[ws], beam_ws)
        RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=beam_ws, OutputWorkspace=ws)
        radius = self.getProperty('BeamRadius').value
        shapeXML = self._cylinder(radius)
        det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
        lambdas = mtd[ws].extractX()
        min_lambda = np.min(lambdas)
        max_lambda = np.max(lambdas)
        width_lambda = lambdas[0][1] - lambdas[0][0]
        lambda_binning = [min_lambda, width_lambda, max_lambda]
        self.log().information('Rebinning for transmission calculation to: ' + str(lambda_binning))
        Rebin(InputWorkspace=ws, Params=lambda_binning, OutputWorkspace=ws)
        beam_rebinned = Rebin(InputWorkspace=beam_ws, Params=lambda_binning, StoreInADS=False)
        CalculateTransmission(SampleRunWorkspace=ws, DirectRunWorkspace=beam_rebinned,
                              TransmissionROI=det_list, OutputWorkspace=ws, RebinParams=lambda_binning)

    def _process_sensitivity(self, ws, sensitivity_out):
        """
            Generates the detector sensitivity map
            @param ws: input workspace
            @param sensitivity_out: sensitivity output map
        """
        CalculateEfficiency(InputWorkspace=ws, OutputWorkspace=sensitivity_out)
        mtd[sensitivity_out].getRun().addProperty('ProcessedAs', 'Sensitivity', True)
        self.setProperty('SensitivityOutputWorkspace', mtd[sensitivity_out])

    def _process_sample(self, ws):
        """
            Processes the sample
            @param ws: input workspace
        """
        reference_ws = self.getProperty('ReferenceInputWorkspace').value
        coll_ws = None
        if reference_ws:
            if not self._check_processed_flag(reference_ws, 'Reference'):
                self.log().warning('Reference input workspace is not processed as reference.')
            Divide(LHSWorkspace=ws, RHSWorkspace=reference_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
            Scale(InputWorkspace=ws, Factor=self.getProperty('WaterCrossSection').value, OutputWorkspace=ws)
            # propagate the mask of the reference on top of the existing mask
            MaskDetectors(Workspace=ws, MaskedWorkspace=reference_ws)
            coll_ws = reference_ws
        else:
            sensitivity_in = self.getProperty('SensitivityInputWorkspace').value
            if sensitivity_in:
                if not self._check_processed_flag(sensitivity_in, 'Sensitivity'):
                    self.log().warning('Sensitivity input workspace is not processed as sensitivity.')
                Divide(LHSWorkspace=ws, RHSWorkspace=sensitivity_in, OutputWorkspace=ws, WarnOnZeroDivide=False)
                # propagate the mask of the sensitivity also to the sample
                MaskDetectors(Workspace=ws, MaskedWorkspace=sensitivity_in)
            flux_in = self.getProperty('FluxInputWorkspace').value
            if flux_in:
                coll_ws = flux_in
                flux_ws = ws + '_flux'
                if self._mode == 'TOF':
                    RebinToWorkspace(WorkspaceToRebin=flux_in, WorkspaceToMatch=ws, OutputWorkspace=flux_ws)
                    Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
                    DeleteWorkspace(flux_ws)
                else:
                    Divide(LHSWorkspace=ws, RHSWorkspace=flux_in, OutputWorkspace=ws, WarnOnZeroDivide=False)
        if coll_ws:
            self._check_distances_match(mtd[ws], coll_ws)
            sample_coll = mtd[ws].getRun().getLogData('collimation.actual_position').value
            ref_coll = coll_ws.getRun().getLogData('collimation.actual_position').value
            flux_factor = (sample_coll ** 2) / (ref_coll ** 2)
            self.log().notice('Flux factor is: ' + str(flux_factor))
            Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)
            ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws,
                                 NaNValue=0., NaNError=0., InfinityValue=0., InfinityError=0.)

    def _apply_absorber(self, ws, absorber_ws):
        """
            Subtracts the dark current
            @param ws: input workspace
            @parma absorber_ws: dark current workspace
        """
        if not self._check_processed_flag(absorber_ws, 'Absorber'):
            self.log().warning('Absorber input workspace is not processed as absorber.')
        Minus(LHSWorkspace=ws, RHSWorkspace=absorber_ws, OutputWorkspace=ws)

    def _apply_beam(self, ws, beam_ws):
        """
            Applies the beam center correction
            @param ws: input workspace
            @parma beam_ws: empty beam workspace
        """
        if not self._check_processed_flag(beam_ws, 'Beam'):
            self.log().warning('Beam input workspace is not processed as beam.')
        if self._mode != 'TOF':
            beam_x = beam_ws.getRun().getLogData('BeamCenterX').value
            beam_y = beam_ws.getRun().getLogData('BeamCenterY').value
            AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number')
            AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number')
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
        self._check_distances_match(mtd[ws], beam_ws)

    def _apply_transmission(self, ws, transmission_ws):
        """
            Applies transmission correction
            @param ws: input workspace
            @param transmission_ws: transmission workspace
        """
        if not self._check_processed_flag(transmission_ws, 'Transmission'):
            self.log().warning('Transmission input workspace is not processed as transmission.')
        if transmission_ws.blocksize() == 1:
            # monochromatic mode, scalar transmission
            transmission = transmission_ws.readY(0)[0]
            transmission_err = transmission_ws.readE(0)[0]
            ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionValue=transmission,
                                        TransmissionError=transmission_err, OutputWorkspace=ws)
        else:
            # wavelenght dependent transmission, need to rebin
            transmission_rebinned = ws + '_tr_rebinned'
            RebinToWorkspace(WorkspaceToRebin=transmission_ws, WorkspaceToMatch=ws,
                             OutputWorkspace=transmission_rebinned)
            ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionWorkspace=transmission_rebinned,
                                        OutputWorkspace=ws)
            DeleteWorkspace(transmission_rebinned)

    def _apply_container(self, ws, container_ws):
        """
            Applies empty container subtraction
            @param ws: input workspace
            @param container_ws: empty container workspace
        """
        if not self._check_processed_flag(container_ws, 'Container'):
            self.log().warning('Container input workspace is not processed as container.')
        self._check_distances_match(mtd[ws], container_ws)
        Minus(LHSWorkspace=ws, RHSWorkspace=container_ws, OutputWorkspace=ws)

    def _apply_parallax(self, ws):
        """
            Applies the parallax correction
            @param ws : the input workspace
        """
        self.log().information('Performing parallax correction')
        if self._instrument == 'D33':
            components = ['back_detector', 'front_detector_top', 'front_detector_bottom',
                          'front_detector_left', 'front_detector_right']
        else:
            components = ['detector']
        ParallaxCorrection(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames=components)

    def _apply_dead_time(self, ws):
        """
            Performs the dead time correction
            @param ws : the input workspace
        """

        instrument = mtd[ws].getInstrument()
        if instrument.hasParameter('tau'):
            tau = instrument.getNumberParameter('tau')[0]
            if instrument.hasParameter('grouping'):
                pattern = instrument.getStringParameter('grouping')[0]
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, GroupingPattern=pattern, OutputWorkspace=ws)
            else:
                self.log().warning('No grouping available in IPF, dead time correction will be performed pixel-wise.')
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, OutputWorkspace=ws)
        else:
            self.log().information('No tau available in IPF, skipping dead time correction.')

    def _finalize(self, ws, process):
        if process != 'Transmission':
            if self._instrument == 'D33':
                CalculateDynamicRange(Workspace=ws, ComponentNames=['back_detector', 'front_detector'])
            else:
                CalculateDynamicRange(Workspace=ws)
        mtd[ws].getRun().addProperty('ProcessedAs', process, True)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

    def PyExec(self):
        process = self.getPropertyValue('ProcessAs')
        processes = ['Absorber', 'Beam', 'Transmission', 'Container', 'Reference', 'Sample']
        progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(process) + 1)
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        # we do not want the summing done by LoadAndMerge since it will be pair-wise and slow
        # instead we load and list, and merge once with merge runs
        LoadAndMerge(Filename=self.getPropertyValue('Run').replace('+',','), LoaderName='LoadILLSANS', OutputWorkspace=ws)
        if isinstance(mtd[ws], WorkspaceGroup):
            tmp = '__tmp'+ws
            MergeRuns(InputWorkspaces=ws, OutputWorkspace=tmp)
            DeleteWorkspaces(ws)
            RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=ws)
        self._normalise(ws)
        #ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws)
        self._instrument = mtd[ws].getInstrument().getName()
        run = mtd[ws].getRun()
        if run.hasProperty('tof_mode'):
            if run.getLogData('tof_mode').value == 'TOF':
                self._mode = 'TOF'
        progress.report()
        if process in ['Beam', 'Transmission', 'Container', 'Reference', 'Sample']:
            absorber_ws = self.getProperty('AbsorberInputWorkspace').value
            if absorber_ws:
                self._apply_absorber(ws, absorber_ws)
            if process == 'Beam':
                self._process_beam(ws)
                progress.report()
            else:
                beam_ws = self.getProperty('BeamInputWorkspace').value
                if beam_ws:
                    self._apply_beam(ws, beam_ws)
                if process == 'Transmission':
                    self._process_transmission(ws, beam_ws)
                    progress.report()
                else:
                    transmission_ws = self.getProperty('TransmissionInputWorkspace').value
                    if transmission_ws:
                        self._apply_transmission(ws, transmission_ws)
                    solid_angle = self._make_solid_angle_name(ws)
                    cache = self.getProperty('CacheSolidAngle').value
                    if cache:
                        if not mtd.doesExist(solid_angle):
                            SolidAngle(InputWorkspace=ws, OutputWorkspace=solid_angle)
                    else:
                        SolidAngle(InputWorkspace=ws, OutputWorkspace=solid_angle)
                    Divide(LHSWorkspace=ws, RHSWorkspace=solid_angle, OutputWorkspace=ws, WarnOnZeroDivide=False)
                    if not cache:
                        DeleteWorkspace(solid_angle)
                    progress.report()
                    if process in ['Reference', 'Sample']:
                        container_ws = self.getProperty('ContainerInputWorkspace').value
                        if container_ws:
                            self._apply_container(ws, container_ws)
                        mask_ws = self.getProperty('MaskedInputWorkspace').value
                        if mask_ws:
                            # for the first time, check if the workspace has the monitors, if so remove them
                            if mask_ws.detectorInfo().isMonitor(mask_ws.getNumberHistograms()-1):
                                #ExtractMonitors(InputWorkspace=mask_ws, DetectorWorkspace=mask_ws)
                                pass
                            MaskDetectors(Workspace=ws, MaskedWorkspace=mask_ws)
                        thickness = self.getProperty('SampleThickness').value
                        NormaliseByThickness(InputWorkspace=ws, OutputWorkspace=ws, SampleThickness=thickness)
                        # parallax (gondola) effect
                        if self._instrument in ['D22', 'D22lr', 'D33']:
                            self._apply_parallax(ws)
                        progress.report()
                        if process == 'Reference':
                            sensitivity_out = self.getPropertyValue('SensitivityOutputWorkspace')
                            if sensitivity_out:
                                self._process_sensitivity(ws, sensitivity_out)
                        elif process == 'Sample':
                            self._process_sample(ws)
                            progress.report()
        self._finalize(ws, process)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction)
