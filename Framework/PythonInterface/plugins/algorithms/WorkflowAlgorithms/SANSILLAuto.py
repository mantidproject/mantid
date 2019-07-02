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


def needs_loading(property_value, loading_type):
    """
        Checks whether a given unary input needs loading or is already loaded in ADS.
        @param property_value : the string value of the corresponding FileProperty
        @param loading_type : the type of input to load
    """
    loading = False
    ws_name = ''
    if property_value:
        ws_name = path.splitext(path.basename(property_value))[0]
        if mtd.doesExist(ws_name):
            logger.information('Reusing {0} workspace: {1}'.format(loading_type, ws_name))
        else:
            loading = True
    return [loading, ws_name]


def needs_processing(property_value, process_type):
    """
        Checks whether a given unary reduction needs processing or is already cached in ADS with expected name.
        @param property_value : the string value of the corresponding MultipleFile input property
        @param process_type : the type of process
    """
    process = False
    ws_name = ''
    if property_value:
        run_number = path.splitext(path.basename(property_value.split(',')[0].split('+')[0]))[0]
        ws_name = run_number + '_' + process_type
        if mtd.doesExist(ws_name):
            run = mtd[ws_name].getRun()
            if run.hasProperty('ProcessedAs'):
                process = run.getLogData('ProcessedAs').value
                if process == process_type:
                    logger.information('Reusing {0} workspace: {1}'.format(process_type, ws_name))
                else:
                    logger.warning(
                        '{0} workspace found, but processed differently: {1}'.format(process_type, ws_name))
                    process = True
            else:
                logger.warning(
                    '{0} workspace found, but missing the ProcessedAs flag: {1}'.format(process_type, ws_name))
                process = True
        else:
            process = True
    return [process, ws_name]


class SANSILLAuto(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data reduction.
    """

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Performs complete SANS data reduction at the ILL.'

    def seeAlso(self):
        return ['SANSILLReduction', 'SANSILLIntegration',]

    def name(self):
        return 'SANSILLAuto'

    def validateInputs(self):
        return dict()

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('SampleRuns', extensions=['nxs']),
                             doc='File path of sample run(s)/numors.')

        self.declareProperty(MultipleFileProperty('AbsorberRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of absorber (Cd/B4C) run(s)/numors.')

        self.declareProperty(MultipleFileProperty('BeamRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of empty beam run(s)/numors.')

        self.declareProperty(MultipleFileProperty('ContainerRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of empty container run(s)/numors.')

        self.declareProperty(MultipleFileProperty('SampleTransmissionRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of sample transmission run(s)/numors.')

        self.declareProperty(MultipleFileProperty('ContainerTransmissionRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of empty container transmission run(s)/numors.')

        self.declareProperty(MultipleFileProperty('TransmissionBeamRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of empty beam runs for transmission run(s)/numors.')

        self.declareProperty(MultipleFileProperty('TransmissionAbsorberRuns', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File path of absorber runs for transmission run(s)/numors.')

        self.declareProperty(FileProperty('SensitivityMap', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the map of relative detector efficiencies.')

        self.declareProperty(FileProperty('MaskFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the beam stop and other detector mask.')

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output sensitivity map workspace.')

        self.declareProperty('ReductionType', defaultValue='ReduceSample',
                             validator=StringListValidator(['ReduceSample', 'ReduceWater']),
                             doc='Choose whether to treat a sample or a water run to calculate sensitivity.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output workspace in Q space.')

        self.setPropertyGroup('SampleTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('ContainerTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionBeamRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionAbsorberRuns', 'Transmissions')

    def PyExec(self):

        sample = self.getPropertyValue('SampleRuns')
        absorber = self.getPropertyValue('AbsorberRuns')
        beam = self.getPropertyValue('BeamRuns')
        container = self.getPropertyValue('ContainerRuns')
        stransmission = self.getPropertyValue('SampleTransmissionRuns')
        ctransmission = self.getPropertyValue('ContainerTransmissionRuns')
        beam_transmission = self.getPropertyValue('TransmissionBeamRuns')
        absorber_transmission = self.getPropertyValue('TransmissionAbsorberRuns')
        sensitivity = self.getPropertyValue('SensitivityMap')
        mask = self.getPropertyValue('MaskFile')
        output = self.getPropertyValue('OutputWorkspace')
        output_sens = self.getPropertyValue('SensitivityOutputWorkspace')
        type = self.getPropertyValue('ReductionType')

        progress = Progress(self, start=0.0, end=1.0, nreports=10)

        [process_transmission_absorber, transmission_absorber_name] = needs_processing(absorber_transmission, 'Absorber')
        if process_transmission_absorber:
            progress.report('Processing transmission absorber')
            SANSILLReduction(Run=absorber_transmission, ProcessAs='Absorber', OutputWorkspace=transmission_absorber_name)
        else:
            progress.report('Using transmission absorber from ADS')

        [process_transmission_beam, transmission_beam_name] = needs_processing(beam_transmission, 'Beam')
        if process_transmission_beam:
            progress.report('Processing transmission beam')
            SANSILLReduction(Run=beam_transmission, ProcessAs='Beam', OutputWorkspace=transmission_beam_name,
                             AbsorberInputWorkspace=transmission_absorber_name)
        else:
            progress.report('Using transmission beam from ADS')

        [process_container_transmission, container_transmission_name] = needs_processing(ctransmission, 'Transmission')
        if process_container_transmission:
            progress.report('Processing container transmission')
            SANSILLReduction(Run=ctransmission, ProcessAs='Transmission', OutputWorkspace=container_transmission_name,
                             AbsorberInputWorkspace=transmission_absorber_name, BeamInputWorkspace=transmission_beam_name)
        else:
            progress.report('Using container transmission from ADS')

        [process_sample_transmission, sample_transmission_name] = needs_processing(stransmission, 'Transmission')
        if process_sample_transmission:
            progress.report('Processing sample transmission')
            SANSILLReduction(Run=stransmission, ProcessAs='Transmission', OutputWorkspace=sample_transmission_name,
                             AbsorberInputWorkspace=transmission_absorber_name, BeamInputWorkspace=transmission_beam_name)
        else:
            progress.report('Using sample transmission from ADS')

        [process_absorber, absorber_name] = needs_processing(absorber, 'Absorber')
        if process_absorber:
            progress.report('Processing absorber')
            SANSILLReduction(Run=absorber, ProcessAs='Absorber', OutputWorkspace=absorber_name)
        else:
            progress.report('Using absorber from ADS')

        [process_beam, beam_name] = needs_processing(beam, 'Beam')
        flux_name = beam_name + '_Flux'
        if process_beam:
            progress.report('Processing beam')
            SANSILLReduction(Run=beam, ProcessAs='Beam', OutputWorkspace=beam_name,
                             AbsorberInputWorkspace=absorber_name, FluxOutputWorkspace=flux_name)
        else:
            progress.report('Using beam from ADS')

        [process_container, container_name] = needs_processing(container, 'Container')
        if process_container:
            progress.report('Processing container')
            SANSILLReduction(Run=container, ProcessAs='Container', OutputWorkspace=container_name, AbsorberInputWorkspace=absorber_name,
                             BeamInputWorkspace=beam_name, TransmissionInputWorkspace=container_transmission_name)
        else:
            progress.report('Using container from ADS')

        [load_sensitivity, sensitivity_name] = needs_loading(sensitivity, 'Sensitivity')
        if load_sensitivity:
            progress.report('Loading sensitivity')
            LoadNexusProcessed(Filename=sensitivity, OutputWorkspace=sensitivity_name)
        progress.report('Using sensitivity from ADS')

        [load_mask, mask_name] = needs_loading(mask, 'Mask')
        if load_mask:
            progress.report('Loading mask')
            LoadNexusProcessed(Filename=mask, OutputWorkspace=mask_name)
        else:
            progress.report('Using mask from ADS')

        [_, sample_name] = needs_processing(sample, 'Sample')
        progress.report('Processing sample')
        if type == 'ReduceSample':
            SANSILLReduction(Run=sample, ProcessAs='Sample', OutputWorkspace=sample_name,
                             AbsorberInputWorkspace=absorber_name, BeamInputWorkspace=beam_name,
                             ContainerInputWorkspace=container_name, TransmissionInputWorkspace=sample_transmission_name,
                             MaskedInputWorkspace=mask_name, SensitivityInputWorkspace=sensitivity_name, FluxInputWorkspace=flux_name)

            SANSILLIntegration(InputWorkspace=sample_name, OutputWorkspace=output, CalculateResolution='None')
            self.setProperty('OutputWorkspace', mtd[output])
        elif type == 'ReduceWater':
            SANSILLReduction(Run=sample, ProcessAs='Reference', OutputWorkspace=sample_name,
                             AbsorberInputWorkspace=absorber_name,
                             BeamInputWorkspace=beam_name, ContainerInputWorkspace=container_name,
                             TransmissionInputWorkspace=sample_transmission_name, SensitivityOutputWorkspace=output_sens,
                             MaskedInputWorkspace=mask_name, FluxInputWorkspace=flux_name)
            self.setProperty('OutputWorkspace', mtd[sample_name])
            self.setProperty('SensitivityOutputWorkspace', mtd[output_sens])

AlgorithmFactory.subscribe(SANSILLAuto)
