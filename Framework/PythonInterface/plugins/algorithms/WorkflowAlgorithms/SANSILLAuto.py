# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
from os import path


def needs_loading(property_value, loading_reduction_type):
    """
        Checks whether a given unary input needs loading or is already loaded in ADS.
        @param property_value : the string value of the corresponding FileProperty
        @param loading_reduction_type : the reduction_type of input to load
    """
    loading = False
    ws_name = ''
    if property_value:
        ws_name = path.splitext(path.basename(property_value))[0]
        if mtd.doesExist(ws_name):
            logger.information('Reusing {0} workspace: {1}'.format(loading_reduction_type, ws_name))
        else:
            loading = True
    return [loading, ws_name]


def needs_processing(property_value, process_reduction_type):
    """
        Checks whether a given unary reduction needs processing or is already cached in ADS with expected name.
        @param property_value : the string value of the corresponding MultipleFile input property
        @param process_reduction_type : the reduction_type of process
    """
    process = False
    ws_name = ''
    if property_value:
        run_number = path.splitext(path.basename(property_value.split(',')[0].split('+')[0]))[0]
        ws_name = run_number + '_' + process_reduction_type
        if mtd.doesExist(ws_name):
            run = mtd[ws_name].getRun()
            if run.hasProperty('ProcessedAs'):
                process = run.getLogData('ProcessedAs').value
                if process == process_reduction_type:
                    logger.information('Reusing {0} workspace: {1}'.format(process_reduction_type, ws_name))
                else:
                    logger.warning(
                        '{0} workspace found, but processed differently: {1}'.format(process_reduction_type, ws_name))
                    process = True
            else:
                logger.warning(
                    '{0} workspace found, but missing the ProcessedAs flag: {1}'.format(process_reduction_type, ws_name))
                process = True
        else:
            process = True
    return [process, ws_name]


class SANSILLAuto(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data; instruments D11, D22, D33.
    """
    progress = None
    reduction_type = None
    sample = None
    beam = None
    container = None
    stransmission = None
    ctransmission = None
    btransmission = None
    atransmission = None
    sensitivity = None
    mask = None
    output = None
    output_sens = None
    dimensionality = None
    references = None

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Performs complete SANS data reduction at the ILL.'

    def seeAlso(self):
        return ['SANSILLReduction', 'SANSILLIntegration',]

    def name(self):
        return 'SANSILLAuto'

    def validateInputs(self):
        result = dict()
        message = 'Wrong number of {0} runs. Provide one or as many as sample runs.'
        tr_message = 'Wrong number of {0} runs. Provide one or multiple runs summed with +.'
        sample_dim = self.getPropertyValue('SampleRuns').count(',')
        abs_dim = self.getPropertyValue('AbsorberRuns').count(',')
        beam_dim = self.getPropertyValue('BeamRuns').count(',')
        can_dim = self.getPropertyValue('ContainerRuns').count(',')
        str_dim = self.getPropertyValue('SampleTransmissionRuns').count(',')
        ctr_dim = self.getPropertyValue('ContainerTransmissionRuns').count(',')
        btr_dim = self.getPropertyValue('TransmissionBeamRuns').count(',')
        atr_dim = self.getPropertyValue('TransmissionAbsorberRuns').count(',')
        mask_dim = self.getPropertyValue('MaskFiles').count(',')
        ref_dim = self.getPropertyValue('ReferenceFiles').count(',')
        if abs_dim != sample_dim and abs_dim != 0:
            result['AbsorberRuns'] = message.format('Absorber')
        if beam_dim != sample_dim and beam_dim != 0:
            result['BeamRuns'] = message.format('Beam')
        if can_dim != sample_dim and can_dim != 0:
            result['ContainerRuns'] = message.format('Container')
        if str_dim != 0:
            issues['SampleTransmissionRuns'] = tr_message.format('SampleTransmission')
        if ctr_dim != 0:
            issues['ContainerTransmissionRuns'] = tr_message.format('ContainerTransmission')
        if btr_dim != 0:
            issues['TransmissionBeamRuns'] = tr_message.format('TransmissionBeam')
        if atr_dim != 0:
            issues['TransmissionAbsorberRuns'] = tr_message.format('TransmissionAbsorber')
        if mask_dim != sample_dim and mask_dim != 0:
            result['MaskFiles'] = message.format('Mask')
        if ref_dim != sample_dim and ref_dim != 0:
            result['ReferenceFiles'] = message.format('Reference')
        return result

    def setUp(self):
        self.sample = self.getPropertyValue('SampleRuns').split(',')
        self.absorber = self.getPropertyValue('AbsorberRuns').split(',')
        self.beam = self.getPropertyValue('BeamRuns').split(',')
        self.container = self.getPropertyValue('ContainerRuns').split(',')
        self.stransmission = self.getPropertyValue('SampleTransmissionRuns')
        self.ctransmission = self.getPropertyValue('ContainerTransmissionRuns')
        self.btransmission = self.getPropertyValue('TransmissionBeamRuns')
        self.atransmission = self.getPropertyValue('TransmissionAbsorberRuns')
        self.sensitivity = self.getPropertyValue('SensitivityMap')
        self.mask = self.getPropertyValue('MaskFiles').split(',')
        self.reference = self.getPropertyValue('ReferenceFiles').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.output_sens = self.getPropertyValue('SensitivityOutputWorkspace')
        self.reduction_type = self.getPropertyValue('ReductionType')
        self.dimensionality = len(self.sample)
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10 * self.dimensionality)

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('SampleRuns', extensions=['nxs']),
                             doc='Sample run(s).')

        self.declareProperty(MultipleFileProperty('AbsorberRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s).')

        self.declareProperty(MultipleFileProperty('BeamRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Empty beam run(s).')

        self.declareProperty(MultipleFileProperty('ContainerRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Empty container run(s).')

        self.declareProperty(MultipleFileProperty('SampleTransmissionRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Sample transmission run(s).')

        self.declareProperty(MultipleFileProperty('ContainerTransmissionRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Container transmission run(s).')

        self.declareProperty(MultipleFileProperty('TransmissionBeamRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Empty beam run(s) for transmission.')

        self.declareProperty(MultipleFileProperty('TransmissionAbsorberRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s) for transmission.')

        self.declareProperty(FileProperty('SensitivityMap', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the map of relative detector efficiencies.')

        self.declareProperty(MultipleFileProperty('MaskFiles', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Files containing the beam stop and other detector mask.')

        self.declareProperty(MultipleFileProperty('ReferenceFiles', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Files containing the corrected water data for absolute normalisation.')

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output sensitivity map workspace.')

        self.declareProperty('ReductionType', defaultValue='ReduceSample',
                             validator=StringListValidator(['ReduceSample', 'ReduceWater']),
                             doc='Choose whether to treat a sample or a water run to calculate sensitivity.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

        self.setPropertyGroup('SampleTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('ContainerTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionBeamRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionAbsorberRuns', 'Transmissions')

    def PyExec(self):

        self.setUp()
        outputs = []
        for i in range(self.dimensionality):
            self.reduce(i)
            outputs.append(self.output+'_'+str(i+1))

        GroupWorkspaces(InputWorkspaces=outputs, OutputWorkspace=self.output)
        self.setProperty('OutputWorkspace', mtd[self.output])
        if self.reduction_type == 'ReduceWater':
            self.setProperty('SensitivityOutputWorkspace', mtd[self.output_sens])

    def reduce(self, i):

        [process_transmission_absorber, transmission_absorber_name] = needs_processing(self.atransmission, 'Absorber')
        self.progress.report('Processing transmission absorber')
        if process_transmission_absorber:
            SANSILLReduction(Run=self.atransmission, ProcessAs='Absorber', OutputWorkspace=transmission_absorber_name)

        [process_transmission_beam, transmission_beam_name] = needs_processing(self.btransmission, 'Beam')
        self.progress.report('Processing transmission beam')
        if process_transmission_beam:
            SANSILLReduction(Run=self.btransmission, ProcessAs='Beam', OutputWorkspace=transmission_beam_name,
                             AbsorberInputWorkspace=transmission_absorber_name)

        [process_container_transmission, container_transmission_name] = needs_processing(self.ctransmission, 'Transmission')
        self.progress.report('Processing container transmission')
        if process_container_transmission:
            SANSILLReduction(Run=self.ctransmission, ProcessAs='Transmission', OutputWorkspace=container_transmission_name,
                             AbsorberInputWorkspace=transmission_absorber_name, BeamInputWorkspace=transmission_beam_name)

        [process_sample_transmission, sample_transmission_name] = needs_processing(self.stransmission, 'Transmission')
        self.progress.report('Processing sample transmission')
        if process_sample_transmission:
            SANSILLReduction(Run=self.stransmission, ProcessAs='Transmission', OutputWorkspace=sample_transmission_name,
                             AbsorberInputWorkspace=transmission_absorber_name, BeamInputWorkspace=transmission_beam_name)

        absorber = self.absorber[i] if len(self.absorber) == self.dimensionality else self.absorber[0]
        [process_absorber, absorber_name] = needs_processing(absorber, 'Absorber')
        self.progress.report('Processing absorber')
        if process_absorber:
            SANSILLReduction(Run=absorber, ProcessAs='Absorber', OutputWorkspace=absorber_name)

        beam = self.beam[i] if len(self.beam) == self.dimensionality else self.beam[0]
        [process_beam, beam_name] = needs_processing(beam, 'Beam')
        flux_name = beam_name + '_Flux'
        self.progress.report('Processing beam')
        if process_beam:
            SANSILLReduction(Run=beam, ProcessAs='Beam', OutputWorkspace=beam_name,
                             AbsorberInputWorkspace=absorber_name, FluxOutputWorkspace=flux_name)

        container = self.container[i] if len(self.container) == self.dimensionality else self.container[0]
        [process_container, container_name] = needs_processing(container, 'Container')
        self.progress.report('Processing container')
        if process_container:
            SANSILLReduction(Run=container, ProcessAs='Container', OutputWorkspace=container_name, AbsorberInputWorkspace=absorber_name,
                             BeamInputWorkspace=beam_name, TransmissionInputWorkspace=container_transmission_name)

        mask = self.mask[i] if len(self.mask) == self.dimensionality else self.mask[0]
        [load_mask, mask_name] = needs_loading(mask, 'Mask')
        self.progress.report('Loading mask')
        if load_mask:
            LoadNexusProcessed(Filename=mask, OutputWorkspace=mask_name)

        if self.sensitivity:
            [load_sensitivity, sensitivity_name] = needs_loading(self.sensitivity, 'Sensitivity')
            self.progress.report('Loading sensitivity')
            if load_sensitivity:
                LoadNexusProcessed(Filename=self.sensitivity, OutputWorkspace=sensitivity_name)
        else:
            reference = self.reference[i] if len(self.reference) == self.dimensionality else self.reference[0]
            [load_reference, reference_name] = needs_loading(reference, 'Reference')
            self.progress.report('Loading reference')
            if load_reference:
                LoadNexusProcessed(Filename=reference, OutputWorkspace=reference_name)

        output = self.output + '_' + str(i + 1)
        [_, sample_name] = needs_processing(self.sample[i], 'Sample')
        self.progress.report('Processing sample at detector configuration '+str(i+1))
        if self.reduction_type == 'ReduceSample':
            sens_input = sensitivity_name if self.sensitivity else ''
            ref_input = reference_name if not self.sensitivity else ''
            flux_input = flux_name if self.sensitivity else ''
            SANSILLReduction(Run=self.sample[i], ProcessAs='Sample', OutputWorkspace=sample_name, ReferenceInputWorkspace=ref_input,
                             AbsorberInputWorkspace=absorber_name, BeamInputWorkspace=beam_name, CacheSolidAngle=True,
                             ContainerInputWorkspace=container_name, TransmissionInputWorkspace=sample_transmission_name,
                             MaskedInputWorkspace=mask_name, SensitivityInputWorkspace=sens_input, FluxInputWorkspace=flux_input)
            SANSILLIntegration(InputWorkspace=sample_name, OutputWorkspace=output, CalculateResolution='None')
        elif self.reduction_type == 'ReduceWater':
            SANSILLReduction(Run=self.sample[i], ProcessAs='Reference', OutputWorkspace=output, AbsorberInputWorkspace=absorber_name,
                             BeamInputWorkspace=beam_name, ContainerInputWorkspace=container_name,
                             TransmissionInputWorkspace=sample_transmission_name, SensitivityOutputWorkspace=self.output_sens,
                             MaskedInputWorkspace=mask_name, FluxInputWorkspace=flux_name)

AlgorithmFactory.subscribe(SANSILLAuto)
