# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, Progress, \
    WorkspaceGroupProperty, FileAction
from mantid.kernel import Direction, FloatBoundedValidator
from mantid.simpleapi import *
from os import path
import threading

EMPTY_TOKEN = '000000'


def get_run_number(value):
    """
    Extracts the run number from the first run out of the string value of a
    multiple file property of numors
    """
    return path.splitext(path.basename(value.split(',')[0].split('+')[0]))[0]


def needs_processing(property_value, process_reduction_type):
    """
    Checks whether a given unary reduction needs processing or is already cached
    in ADS with expected name.
    @param property_value: the string value of the corresponding MultipleFile
                           input property
    @param process_reduction_type: the reduction_type of process
    """
    do_process = False
    ws_name = ''
    if property_value:
        run_number = get_run_number(property_value)
        ws_name = run_number + '_' + process_reduction_type
        if mtd.doesExist(ws_name):
            run = mtd[ws_name].getRun()
            if run.hasProperty('ProcessedAs'):
                process = run.getLogData('ProcessedAs').value
                if process == process_reduction_type:
                    logger.notice('Reusing {0} workspace: {1}'
                                  .format(process_reduction_type, ws_name))
                else:
                    logger.warning('{0} workspace found, but processed '
                                   'differently: {1}'
                                   .format(process_reduction_type, ws_name))
                    do_process = True
            else:
                logger.warning('{0} workspace found, but missing the '
                               'ProcessedAs flag: {1}'
                               .format(process_reduction_type, ws_name))
                do_process = True
        else:
            do_process = True
    return [do_process, ws_name]


def needs_loading(property_value, loading_reduction_type):
    """
    Checks whether a given unary input needs loading or is already loaded in
    ADS.
    @param property_value: the string value of the corresponding FileProperty
    @param loading_reduction_type : the reduction_type of input to load
    """
    loading = False
    ws_name = ''
    if property_value:
        ws_name = path.splitext(path.basename(property_value))[0]
        if mtd.doesExist(ws_name):
            logger.notice('Reusing {0} workspace: {1}'
                          .format(loading_reduction_type, ws_name))
        else:
            loading = True
    return [loading, ws_name]


class AutoProcessContext:
    """
    Static lock for the access to the shared dictionnary of events.
    """
    AUTO_PROCESS_LOCK = threading.Lock()

    """
    Static dictionnary used to store the current processing events. If a
    concurent thread needs the result of a running processing, it will wait on
    the corresponding event.
    """
    AUTO_PROCESS_EVENTS = dict()

    def __init__(self, property_value, reduction_type, only_loading=False):
        self.value = property_value
        self.type = reduction_type
        self.loading = only_loading
        self.name = None

    def __enter__(self):
        """
        Enter the context. This method checks if the processing is needed and
        blocks if the processing is currently ran by an other thread.
        """
        self.__class__.AUTO_PROCESS_LOCK.acquire()
        if self.loading:
            [needed, self.name] = needs_loading(self.value, self.type)
        else:
            [needed, self.name] = needs_processing(self.value, self.type)
        if needed:
            if self.name in self.__class__.AUTO_PROCESS_EVENTS:
                event = self.__class__.AUTO_PROCESS_EVENTS[self.name]
                self.__class__.AUTO_PROCESS_LOCK.release()
                event.wait()
                return [False, self.name]
            else:
                self.__class__.AUTO_PROCESS_EVENTS[self.name] = \
                        threading.Event()
                self.__class__.AUTO_PROCESS_LOCK.release()
                return [True, self.name]
        else:
            self.__class__.AUTO_PROCESS_LOCK.release()
            return [False, self.name]

    def __exit__(self, type, value, traceback):
        """
        Exit the context. This method set the processing event to True to let
        the pending threads know. It also remove the event from the dictionnary.
        """
        with self.__class__.AUTO_PROCESS_LOCK:
            if self.name in self.__class__.AUTO_PROCESS_EVENTS:
                self.__class__.AUTO_PROCESS_EVENTS[self.name].set()
                del self.__class__.AUTO_PROCESS_EVENTS[self.name]


class SANSILLAutoProcess(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data; instruments D11, D16, D22, D33.
    """
    progress = None
    reduction_type = None
    sample = None
    absorber = None
    beam = None
    container = None
    stransmission = None
    ctransmission = None
    btransmission = None
    atransmission = None
    sensitivity = None
    mask = None
    flux = None
    default_mask = None
    output = None
    output_sens = None
    dimensionality = None
    reference = None
    normalise = None
    radius = None
    thickness = None
    theta_dependent = None

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Performs complete SANS data reduction at the ILL.'

    def seeAlso(self):
        return ['SANSILLReduction', 'SANSILLIntegration',]

    def name(self):
        return 'SANSILLAutoProcess'

    def validateInputs(self):
        result = dict()
        message = 'Wrong number of {0} runs: {1}. Provide one or as many as sample runs: {2}.'
        tr_message = 'Wrong number of {0} runs: {1}. Provide one or multiple runs summed with +.'
        sample_dim = self.getPropertyValue('SampleRuns').count(',')
        abs_dim = self.getPropertyValue('AbsorberRuns').count(',')
        beam_dim = self.getPropertyValue('BeamRuns').count(',')
        flux_dim = self.getPropertyValue('FluxRuns').count(',')
        can_dim = self.getPropertyValue('ContainerRuns').count(',')
        str_dim = self.getPropertyValue('SampleTransmissionRuns').count(',')
        ctr_dim = self.getPropertyValue('ContainerTransmissionRuns').count(',')
        btr_dim = self.getPropertyValue('TransmissionBeamRuns').count(',')
        atr_dim = self.getPropertyValue('TransmissionAbsorberRuns').count(',')
        mask_dim = self.getPropertyValue('MaskFiles').count(',')
        sens_dim = self.getPropertyValue('SensitivityMaps').count(',')
        ref_dim = self.getPropertyValue('ReferenceFiles').count(',')
        if self.getPropertyValue('SampleRuns') == '':
            result['SampleRuns'] = 'Please provide at least one sample run.'
        if abs_dim != sample_dim and abs_dim != 0:
            result['AbsorberRuns'] = message.format('Absorber', abs_dim, sample_dim)
        if beam_dim != sample_dim and beam_dim != 0:
            result['BeamRuns'] = message.format('Beam', beam_dim, sample_dim)
        if can_dim != sample_dim and can_dim != 0:
            result['ContainerRuns'] = message.format('Container', can_dim, sample_dim)
        if str_dim != 0:
            result['SampleTransmissionRuns'] = tr_message.format('SampleTransmission', str_dim)
        if ctr_dim != 0:
            result['ContainerTransmissionRuns'] = tr_message.format('ContainerTransmission', ctr_dim)
        if btr_dim != 0:
            result['TransmissionBeamRuns'] = tr_message.format('TransmissionBeam', btr_dim)
        if atr_dim != 0:
            result['TransmissionAbsorberRuns'] = tr_message.format('TransmissionAbsorber', atr_dim)
        if mask_dim != sample_dim and mask_dim != 0:
            result['MaskFiles'] = message.format('Mask', mask_dim, sample_dim)
        if ref_dim != sample_dim and ref_dim != 0:
            result['ReferenceFiles'] = message.format('Reference', ref_dim, sample_dim)
        if sens_dim != sample_dim and sens_dim != 0:
            result['SensitivityMaps'] = message.format('Sensitivity', sens_dim, sample_dim)
        if flux_dim != flux_dim and flux_dim != 0:
            result['FluxRuns'] = message.format('Flux')

        return result

    def setUp(self):
        self.sample = self.getPropertyValue('SampleRuns').split(',')
        self.absorber = self.getPropertyValue('AbsorberRuns').split(',')
        self.beam = self.getPropertyValue('BeamRuns').split(',')
        self.flux = self.getPropertyValue('FluxRuns').split(',')
        self.container = self.getPropertyValue('ContainerRuns').split(',')
        self.stransmission = self.getPropertyValue('SampleTransmissionRuns')
        self.ctransmission = self.getPropertyValue('ContainerTransmissionRuns')
        self.btransmission = self.getPropertyValue('TransmissionBeamRuns')
        self.atransmission = self.getPropertyValue('TransmissionAbsorberRuns')
        self.sensitivity = self.getPropertyValue('SensitivityMaps').split(',')
        self.default_mask = self.getPropertyValue('DefaultMaskFile')
        self.mask = self.getPropertyValue('MaskFiles').split(',')
        self.reference = self.getPropertyValue('ReferenceFiles').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.output_sens = self.getPropertyValue('SensitivityOutputWorkspace')
        self.normalise = self.getPropertyValue('NormaliseBy')
        self.theta_dependent = self.getProperty('ThetaDependent').value
        self.radius = self.getProperty('BeamRadius').value
        self.dimensionality = len(self.sample)
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10 * self.dimensionality)
        self.cleanup = self.getProperty('ClearCorrected2DWorkspace').value

    def PyInit(self):

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

        self.declareProperty(MultipleFileProperty('SampleRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs'],
                                                  allow_empty=True),
                             doc='Sample run(s).')

        self.declareProperty(MultipleFileProperty('AbsorberRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s).')

        self.declareProperty(MultipleFileProperty('BeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s).')

        self.declareProperty(MultipleFileProperty('FluxRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for flux calculation only; '
                                 'if left blank flux will be calculated from BeamRuns.')

        self.declareProperty(MultipleFileProperty('ContainerRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty container run(s).')

        self.setPropertyGroup('SampleRuns', 'Numors')
        self.setPropertyGroup('AbsorberRuns', 'Numors')
        self.setPropertyGroup('BeamRuns', 'Numors')
        self.setPropertyGroup('FluxRuns', 'Numors')
        self.setPropertyGroup('ContainerRuns', 'Numors')

        self.declareProperty(MultipleFileProperty('SampleTransmissionRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Sample transmission run(s).')

        self.declareProperty(MultipleFileProperty('ContainerTransmissionRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Container transmission run(s).')

        self.declareProperty(MultipleFileProperty('TransmissionBeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for transmission.')

        self.declareProperty(MultipleFileProperty('TransmissionAbsorberRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s) for transmission.')

        self.setPropertyGroup('SampleTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('ContainerTransmissionRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionBeamRuns', 'Transmissions')
        self.setPropertyGroup('TransmissionAbsorberRuns', 'Transmissions')
        self.copyProperties('SANSILLReduction',
                            ['ThetaDependent'])
        self.setPropertyGroup('ThetaDependent', 'Transmissions')

        self.declareProperty('SensitivityMaps', '',
                             doc='File(s) or workspaces containing the maps of relative detector efficiencies.')

        self.declareProperty('DefaultMaskFile', '',
                             doc='File or workspace containing the default mask (typically the detector edges and dead pixels/tubes)'
                                 ' to be applied to all the detector configurations.')

        self.declareProperty('MaskFiles','',
                             doc='File(s) or workspaces containing the detector mask (typically beam stop).')

        self.declareProperty('ReferenceFiles', '',
                             doc='File(s) or workspaces containing the corrected water data (in 2D) for absolute normalisation.')

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output sensitivity map workspace.')

        self.copyProperties('SANSILLReduction', ['NormaliseBy'])

        self.declareProperty('SampleThickness', 0.1, validator=FloatBoundedValidator(lower=0.),
                             doc='Sample thickness [cm]')

        self.declareProperty('BeamRadius', 0.05, validator=FloatBoundedValidator(lower=0.),
                             doc='Beam radius [m]; used for beam center finding, transmission and flux calculations.')

        self.declareProperty('WaterCrossSection', 1., doc='Provide water cross-section; '
                                                          'used only if the absolute scale is done by dividing to water.')

        self.setPropertyGroup('SensitivityMaps', 'Options')
        self.setPropertyGroup('DefaultMaskFile', 'Options')
        self.setPropertyGroup('MaskFiles', 'Options')
        self.setPropertyGroup('ReferenceFiles', 'Options')
        self.setPropertyGroup('SensitivityOutputWorkspace', 'Options')
        self.setPropertyGroup('NormaliseBy', 'Options')
        self.setPropertyGroup('SampleThickness', 'Options')
        self.setPropertyGroup('BeamRadius', 'Options')
        self.setPropertyGroup('WaterCrossSection', 'Options')

        self.copyProperties('SANSILLIntegration',
                            ['OutputType', 'CalculateResolution', 'DefaultQBinning', 'BinningFactor', 'OutputBinning',
                             'NPixelDivision', 'NumberOfWedges', 'WedgeWorkspace', 'WedgeAngle', 'WedgeOffset',
                             'AsymmetricWedges', 'MaxQxy', 'DeltaQ', 'IQxQyLogBinning', 'PanelOutputWorkspaces'])

        self.setPropertyGroup('OutputType', 'Integration Options')
        self.setPropertyGroup('CalculateResolution', 'Integration Options')
        self.declareProperty('ClearCorrected2DWorkspace', True,
                             'Whether to clear the fully corrected 2D workspace.')

    def PyExec(self):

        self.setUp()
        outputs = []
        panel_outputs = self.getPropertyValue('PanelOutputWorkspaces')
        panel_output_groups = []

        container_transmission, sample_transmission = \
            self.processTransmissions()

        for d in range(self.dimensionality):
            if self.sample[d] != EMPTY_TOKEN:
                absorber = self.processAbsorber(d)
                flux = self.processFlux(d, absorber)
                if flux:
                    beam, _ = self.processBeam(d, absorber)
                else:
                    beam, flux = self.processBeam(d, absorber)
                container = self.processContainer(d, beam, absorber,
                                                  container_transmission)
                self.processSample(d, flux, sample_transmission, beam, absorber,
                                   container)
                outputs.append(self.output+ '_' + str(d + 1))
                panel_ws_group = panel_outputs + '_' + str(d + 1)
                if mtd.doesExist(panel_ws_group) and panel_outputs:
                    panel_output_groups.append(panel_ws_group)
            else:
                self.log().information('Skipping empty token run.')

        GroupWorkspaces(InputWorkspaces=outputs, OutputWorkspace=self.output)
        self.setProperty('OutputWorkspace', mtd[self.output])
        if self.output_sens:
            self.setProperty('SensitivityOutputWorkspace', mtd[self.output_sens])

        if panel_outputs and len(panel_output_groups) != 0:
            GroupWorkspaces(InputWorkspaces=panel_output_groups, OutputWorkspace=panel_outputs)
            self.setProperty('PanelOutputWorkspaces', mtd[panel_outputs])

    def processTransmissions(self):
        with AutoProcessContext(self.atransmission, 'Absorber') as \
                [process_transmission_absorber, transmission_absorber_name]:
            if process_transmission_absorber:
                self.progress.report('Processing transmission absorber')
                SANSILLReduction(Run=self.atransmission,
                                 ProcessAs='Absorber',
                                 NormaliseBy=self.normalise,
                                 OutputWorkspace=transmission_absorber_name)

        with AutoProcessContext(self.btransmission, 'Beam') as \
                [process_transmission_beam, transmission_beam_name]:
            flux_name = transmission_beam_name + '_Flux'
            if process_transmission_beam:
                self.progress.report('Processing transmission beam')
                SANSILLReduction(Run=self.btransmission,
                                 ProcessAs='Beam',
                                 NormaliseBy=self.normalise,
                                 OutputWorkspace=transmission_beam_name,
                                 BeamRadius=self.radius,
                                 FluxOutputWorkspace=flux_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name)

        with AutoProcessContext(self.ctransmission, 'Transmission') as \
                [process_container_transmission, container_transmission_name]:
            if process_container_transmission:
                self.progress.report('Processing container transmission')
                SANSILLReduction(Run=self.ctransmission,
                                 ProcessAs='Transmission',
                                 OutputWorkspace=container_transmission_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name,
                                 BeamInputWorkspace=transmission_beam_name,
                                 NormaliseBy=self.normalise,
                                 BeamRadius=self.radius)

        with AutoProcessContext(self.stransmission, 'Transmission') as \
                [process_sample_transmission, sample_transmission_name]:
            if process_sample_transmission:
                self.progress.report('Processing sample transmission')
                SANSILLReduction(Run=self.stransmission,
                                 ProcessAs='Transmission',
                                 OutputWorkspace=sample_transmission_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name,
                                 BeamInputWorkspace=transmission_beam_name,
                                 NormaliseBy=self.normalise,
                                 BeamRadius=self.radius)
        return container_transmission_name, sample_transmission_name

    def processAbsorber(self, i):
        absorber = (self.absorber[i]
                    if len(self.absorber) == self.dimensionality
                    else self.absorber[0])
        with AutoProcessContext(absorber, 'Absorber') as \
                [process_absorber, absorber_name]:
            if process_absorber:
                self.progress.report('Processing absorber')
                SANSILLReduction(Run=absorber,
                                 ProcessAs='Absorber',
                                 NormaliseBy=self.normalise,
                                 OutputWorkspace=absorber_name)
        return absorber_name

    def processBeam(self, i, absorber_name):
        beam = (self.beam[i]
                if len(self.beam) == self.dimensionality
                else self.beam[0])
        with AutoProcessContext(beam, 'Beam') as [process_beam, beam_name]:
            flux_name = beam_name + '_Flux' if not self.flux[0] else ''
            if process_beam:
                self.progress.report('Processing beam')
                SANSILLReduction(Run=beam,
                                 ProcessAs='Beam',
                                 OutputWorkspace=beam_name,
                                 NormaliseBy=self.normalise,
                                 BeamRadius=self.radius,
                                 AbsorberInputWorkspace=absorber_name,
                                 FluxOutputWorkspace=flux_name)
        return beam_name, flux_name

    def processFlux(self, i, aborber_name):
        if self.flux[0]:
            flux = (self.flux[i]
                    if len(self.flux) == self.dimensionality
                    else self.flux[0])
            with AutoProcessContext(flux, 'Flux') as [process_flux, flux_name]:
                if process_flux:
                    self.progress.report('Processing flux')
                    SANSILLReduction(Run=flux,
                                     ProcessAs='Beam',
                                     OutputWorkspace=flux_name.replace('Flux',
                                                                       'Beam'),
                                     NormaliseBy=self.normalise,
                                     BeamRadius=self.radius,
                                     AbsorberInputWorkspace=absorber_name,
                                     FluxOutputWorkspace=flux_name)
            return flux_name
        else:
            return None

    def processContainer(self, i, beam_name, absorber_name,
                         container_transmission_name):
        container = (self.container[i]
                     if len(self.container) == self.dimensionality
                     else self.container[0])
        with AutoProcessContext(container, 'Container') as \
                [process_container, container_name]:
            if process_container:
                self.progress.report('Processing container')
                SANSILLReduction(Run=container,
                                 ProcessAs='Container',
                                 OutputWorkspace=container_name,
                                 AbsorberInputWorkspace=absorber_name,
                                 BeamInputWorkspace=beam_name,
                                 CacheSolidAngle=True,
                                 TransmissionInputWorkspace=
                                 container_transmission_name,
                                 ThetaDependent=self.theta_dependent,
                                 NormaliseBy=self.normalise)
        return container_name

    def processSample(self, i, flux_name, sample_transmission_name, beam_name,
                      absorber_name, container_name):
        # this is the default mask, the same for all the distance configurations
        with AutoProcessContext(self.default_mask, 'DefaultMask', True) as \
                [load_default_mask, default_mask_name]:
            if load_default_mask:
                self.progress.report('Loading default mask')
                LoadNexusProcessed(Filename=self.default_mask,
                                   OutputWorkspace=default_mask_name)

        # this is the beam stop mask, potentially different at each distance configuration
        mask = (self.mask[i]
                if len(self.mask) == self.dimensionality
                else self.mask[0])
        with AutoProcessContext(mask, 'Mask', True) as [load_mask, mask_name]:
            if load_mask:
                self.progress.report('Loading mask')
                LoadNexusProcessed(Filename=mask, OutputWorkspace=mask_name)

        # sensitivity
        sens_input = ''
        ref_input = ''
        if self.sensitivity:
            sens = (self.sensitivity[i]
                    if len(self.sensitivity) == self.dimensionality
                    else self.sensitivity[0])
            with AutoProcessContext(sens, 'Sensitivity', True) as \
                    [load_sensitivity, sensitivity_name]:
                sens_input = sensitivity_name
                if load_sensitivity:
                    self.progress.report('Loading sensitivity')
                    LoadNexusProcessed(Filename=sens,
                                       OutputWorkspace=sensitivity_name)

        # reference
        if self.reference:
            reference = (self.reference[i]
                         if len(self.reference) == self.dimensionality
                         else self.reference[0])
            with AutoProcessContext(reference, 'Reference', True) as \
                    [load_reference, reference_name]:
                ref_input = reference_name
                if load_reference:
                    self.progress.report('Loading reference')
                    LoadNexusProcessed(Filename=reference,
                                       OutputWorkspace=reference_name)

        # sample
        with AutoProcessContext(self.sample[i], 'Sample') as [_, sample_name]:
            output = self.output + '_' + str(i + 1)
            self.progress.report('Processing sample at detector configuration '
                                 + str(i + 1))
            SANSILLReduction(
                    Run=self.sample[i],
                    ProcessAs='Sample',
                    OutputWorkspace=sample_name,
                    ReferenceInputWorkspace=ref_input,
                    AbsorberInputWorkspace=absorber_name,
                    BeamInputWorkspace=beam_name,
                    CacheSolidAngle=True,
                    ContainerInputWorkspace=container_name,
                    TransmissionInputWorkspace=sample_transmission_name,
                    MaskedInputWorkspace=mask_name,
                    DefaultMaskedInputWorkspace=default_mask_name,
                    SensitivityInputWorkspace=sens_input,
                    SensitivityOutputWorkspace=self.output_sens,
                    FluxInputWorkspace=flux_name,
                    NormaliseBy=self.normalise,
                    ThetaDependent=self.theta_dependent,
                    SampleThickness=
                    self.getProperty('SampleThickness').value,
                    WaterCrossSection=
                    self.getProperty('WaterCrossSection').value
                    )

            panel_outputs = self.getPropertyValue('PanelOutputWorkspaces')
            panel_ws_group = (panel_outputs + '_' + str(i + 1)
                              if panel_outputs
                              else '')

            SANSILLIntegration(
                    InputWorkspace=sample_name,
                    OutputWorkspace=output,
                    OutputType=self.getPropertyValue('OutputType'),
                    CalculateResolution=
                    self.getPropertyValue('CalculateResolution'),
                    DefaultQBinning=self.getPropertyValue('DefaultQBinning'),
                    BinningFactor=self.getProperty('BinningFactor').value,
                    OutputBinning=self.getPropertyValue('OutputBinning'),
                    NPixelDivision=self.getProperty('NPixelDivision').value,
                    NumberOfWedges=self.getProperty('NumberOfWedges').value,
                    WedgeAngle=self.getProperty('WedgeAngle').value,
                    WedgeOffset=self.getProperty('WedgeOffset').value,
                    AsymmetricWedges=self.getProperty('AsymmetricWedges').value,
                    PanelOutputWorkspaces=panel_ws_group
                    )

            if self.cleanup:
                DeleteWorkspace(sample_name)


AlgorithmFactory.subscribe(SANSILLAutoProcess)
