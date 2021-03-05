# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, Progress, \
    WorkspaceGroupProperty, FileAction
from mantid.kernel import Direction, FloatBoundedValidator, FloatArrayProperty, IntBoundedValidator
from mantid.simpleapi import *
import numpy as np
from os import path

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


class SANSILLAutoProcess(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data; instruments D11, D11B, D16, D22, D22B, D33.
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
        self.setUp()

        result = dict()
        message = 'Wrong number of {0} runs: {1}. Provide one or as many as '\
                  'sample runs: {2}.'
        message_value = 'Wrong number of {0} values: {1}. Provide one or as ' \
                        'many as sample runs: {2}.'

        # array parameters checks
        sample_dim = len(self.sample)
        abs_dim = len(self.absorber)
        beam_dim = len(self.beam)
        flux_dim = len(self.flux)
        can_dim = len(self.container)
        mask_dim = len(self.mask)
        sens_dim = len(self.sensitivity)
        ref_dim = len(self.reference)
        maxqxy_dim = len(self.maxqxy)
        deltaq_dim = len(self.deltaq)
        radius_dim = len(self.radius)
        if self.getPropertyValue('SampleRuns') == '':
            result['SampleRuns'] = 'Please provide at least one sample run.'
        if abs_dim != sample_dim and abs_dim > 1:
            result['AbsorberRuns'] = \
                message.format('Absorber', abs_dim, sample_dim)
        if beam_dim != sample_dim and beam_dim > 1:
            result['BeamRuns'] = message.format('Beam', beam_dim, sample_dim)
        if can_dim != sample_dim and can_dim > 1:
            result['ContainerRuns'] = \
                message.format('Container', can_dim, sample_dim)
        if mask_dim != sample_dim and mask_dim > 1:
            result['MaskFiles'] = message.format('Mask', mask_dim, sample_dim)
        if ref_dim != sample_dim and ref_dim > 1:
            result['ReferenceFiles'] = \
                    message.format('Reference', ref_dim, sample_dim)
        if sens_dim != sample_dim and sens_dim > 1:
            result['SensitivityMaps'] = \
                    message.format('Sensitivity', sens_dim, sample_dim)
        if flux_dim != sample_dim and flux_dim > 1:
            result['FluxRuns'] = message.format('Flux')
        if maxqxy_dim != sample_dim and maxqxy_dim > 1:
            result['MaxQxy'] = \
                message_value.format('MaxQxy', maxqxy_dim, sample_dim)
        if deltaq_dim != sample_dim and deltaq_dim > 1:
            result['DeltaQ'] = \
                message_value.format('DeltaQ', deltaq_dim, sample_dim)
        if radius_dim != sample_dim and radius_dim > 1:
            result['BeamRadius'] = \
                    message_value.format('BeamRadius', radius_dim, sample_dim)

        # transmission runs checks
        str_dim = len(self.stransmission.split(','))
        ctr_dim = len(self.ctransmission.split(','))
        btr_dim = len(self.btransmission.split(','))
        atr_dim = len(self.atransmission.split(','))
        if str_dim != sample_dim and str_dim != 1:
            result['SampleTransmissionRuns'] = message.format('SampleTransmission', str_dim, sample_dim)
        if ctr_dim != can_dim and ctr_dim != 1:
            result['ContainerTransmissionRuns'] = message.format('ContainerTransmission', ctr_dim, can_dim)
        if (btr_dim != sample_dim or btr_dim != can_dim) and btr_dim != 1:
            result['TransmissionBeamRuns'] = message.format('TransmissionBeam', btr_dim, sample_dim)
        if (atr_dim != sample_dim or atr_dim != can_dim) and atr_dim != 1:
            result['TransmissionAbsorberRuns'] = message.format('TransmissionAbsorber', atr_dim, sample_dim)

        # other checks
        if self.output_type == 'I(Phi,Q)' and self.n_wedges == 0:
            result['NumberOfWedges'] = "For I(Phi,Q) processing, the number " \
                                       "of wedges must be different from 0."

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
        self.sensitivity = self.getPropertyValue('SensitivityMaps') \
            .replace(' ', '').split(',')
        self.default_mask = self.getPropertyValue('DefaultMaskFile')
        self.mask = self.getPropertyValue('MaskFiles') \
            .replace(' ', '').split(',')
        self.reference = self.getPropertyValue('ReferenceFiles') \
            .replace(' ', '').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.output_panels = self.output + "_panels"
        self.output_sens = self.getPropertyValue('SensitivityOutputWorkspace')
        self.normalise = self.getPropertyValue('NormaliseBy')
        self.theta_dependent = self.getProperty('ThetaDependent').value
        self.tr_radius = self.getProperty('TransmissionBeamRadius').value
        self.radius = self.getPropertyValue('BeamRadius').split(',')
        self.dimensionality = len(self.sample)
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10 * self.dimensionality)
        self.cleanup = self.getProperty('ClearCorrected2DWorkspace').value
        self.n_wedges = self.getProperty('NumberOfWedges').value
        self.maxqxy = self.getPropertyValue('MaxQxy').split(',')
        self.deltaq = self.getPropertyValue('DeltaQ').split(',')
        self.output_type = self.getPropertyValue('OutputType')
        self.stitch_reference_index = self.getProperty('StitchReferenceIndex').value

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

        self.declareProperty('SampleThickness', 0.1,
                             validator=FloatBoundedValidator(lower=-1),
                             doc='Sample thickness [cm]')

        self.declareProperty('TransmissionBeamRadius', 0.1,
                             validator=FloatBoundedValidator(lower=0.),
                             doc='Beam radius [m]; used for transmission '
                             'calculations.')

        self.declareProperty(FloatArrayProperty('BeamRadius', values=[0.1]),
                             doc='Beam radius [m]; used for beam center '
                             'finding and flux calculations.')

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
        self.setPropertyGroup('TransmissionBeamRadius', 'Options')
        self.setPropertyGroup('WaterCrossSection', 'Options')

        self.declareProperty(FloatArrayProperty('MaxQxy', values=[-1]),
                             doc='Maximum of absolute Qx and Qy.')
        self.declareProperty(FloatArrayProperty('DeltaQ', values=[-1]),
                             doc='The dimension of a Qx-Qy cell.')

        self.declareProperty('OutputPanels', False,
                             doc='Whether or not process the individual '
                             'detector panels.')

        self.copyProperties('SANSILLIntegration',
                            ['OutputType', 'CalculateResolution',
                             'DefaultQBinning', 'BinningFactor',
                             'OutputBinning', 'NPixelDivision',
                             'NumberOfWedges', 'WedgeAngle', 'WedgeOffset',
                             'AsymmetricWedges', 'IQxQyLogBinning', 'WavelengthRange'])

        self.setPropertyGroup('OutputType', 'Integration Options')
        self.setPropertyGroup('CalculateResolution', 'Integration Options')
        self.declareProperty('ClearCorrected2DWorkspace', True,
                             'Whether to clear the fully corrected 2D workspace.')

        self.declareProperty('SensitivityWithOffsets', False,
                             'Whether the sensitivity data has been measured with different horizontal offsets.')

        self.declareProperty('StitchReferenceIndex', defaultValue=1,
                             validator=IntBoundedValidator(lower=0),
                             doc='Index of reference workspace during stitching.')

    def PyExec(self):

        self.setUp()
        outputs = []
        panel_output_groups = []
        sensitivity_outputs = []

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
                sample, panels, sensitivity = self.processSample(d, flux,
                                                                 sample_transmission, beam,
                                                                 absorber, container)
                outputs.append(sample)
                if sensitivity:
                    sensitivity_outputs.append(sensitivity)
                if panels:
                    panel_output_groups.append(panels)
            else:
                self.log().information('Skipping empty token run.')

        for output in outputs:
            ConvertToPointData(InputWorkspace=output,
                               OutputWorkspace=output)
        if len(outputs) > 1 and self.getPropertyValue('OutputType') == 'I(Q)':
            try:
                stitched = self.output + "_stitched"
                Stitch1DMany(InputWorkspaces=outputs,
                             OutputWorkspace=stitched,
                             ScaleRHSWorkspace=True,
                             indexOfReference=self.stitch_reference_index)
                outputs.append(stitched)
            except RuntimeError as re:
                self.log().warning("Unable to stitch automatically, consider "
                                   "stitching manually: " + str(re))

        GroupWorkspaces(InputWorkspaces=outputs, OutputWorkspace=self.output)

        # group wedge workspaces
        if self.output_type == "I(Q)":
            for w in range(self.n_wedges):
                wedge_ws = [self.output + "_wedge_" + str(w + 1) + "_" + str(d + 1)
                            for d in range(self.dimensionality)]
                # convert to point data and remove nan and 0 from edges
                for ws in wedge_ws:
                    ConvertToPointData(InputWorkspace=ws,
                                       OutputWorkspace=ws)
                    ReplaceSpecialValues(InputWorkspace=ws,
                                         OutputWorkspace=ws,
                                         NaNValue=0)
                    y = mtd[ws].readY(0)
                    x = mtd[ws].readX(0)
                    nonzero = np.nonzero(y)

                    CropWorkspace(InputWorkspace=ws,
                                  XMin=x[nonzero][0] - 1,
                                  XMax=x[nonzero][-1],
                                  OutputWorkspace=ws)

                # and stitch if possible
                if len(wedge_ws) > 1:
                    try:
                        stitched = self.output + "_wedge_" + str(w + 1) \
                                   + "_stitched"
                        Stitch1DMany(InputWorkspaces=wedge_ws,
                                     OutputWorkspace=stitched)
                        wedge_ws.append(stitched)
                    except RuntimeError as re:
                        self.log().warning("Unable to stitch automatically, "
                                           "consider stitching manually: "
                                           + str(re))
                GroupWorkspaces(InputWorkspaces=wedge_ws,
                                OutputWorkspace=self.output + "_wedge_" + str(w + 1))

        self.setProperty('OutputWorkspace', mtd[self.output])
        if self.output_sens:
            if len(sensitivity_outputs) > 1:
                GroupWorkspaces(InputWorkspaces=sensitivity_outputs, OutputWorkspace=self.output_sens)
            if self.getProperty('SensitivityWithOffsets').value:
                tmp_group_name = self.output_sens + '_group'
                RenameWorkspace(InputWorkspace=self.output_sens, OutputWorkspace=tmp_group_name)
                CalculateEfficiency(InputWorkspace=tmp_group_name, MergeGroup=True, OutputWorkspace=self.output_sens)
                DeleteWorkspace(Workspace=tmp_group_name)
            self.setProperty('SensitivityOutputWorkspace', mtd[self.output_sens])

        # group panels
        if panel_output_groups:
            GroupWorkspaces(InputWorkspaces=panel_output_groups,
                            OutputWorkspace=self.output_panels)

    def processTransmissions(self):
        absorber_transmission_names = []
        beam_transmission_names = []
        container_transmission_names = []
        sample_transmission_names = []
        for absorber in self.atransmission.split(','):
            [process_transmission_absorber, transmission_absorber_name] = \
                needs_processing(absorber, 'Absorber')
            absorber_transmission_names.append(transmission_absorber_name)
            self.progress.report('Processing transmission absorber')
            if process_transmission_absorber:
                SANSILLReduction(Run=absorber,
                                 ProcessAs='Absorber',
                                 NormaliseBy=self.normalise,
                                 OutputWorkspace=transmission_absorber_name)
        for beam_no, beam in enumerate(self.btransmission.split(',')):
            [process_transmission_beam, transmission_beam_name] = \
                needs_processing(beam, 'Beam')
            beam_transmission_names.append(transmission_beam_name)
            flux_name = transmission_beam_name + '_Flux'
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[beam_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            self.progress.report('Processing transmission beam')
            if process_transmission_beam:
                SANSILLReduction(Run=beam,
                                 ProcessAs='Beam',
                                 NormaliseBy=self.normalise,
                                 OutputWorkspace=transmission_beam_name,
                                 BeamRadius=self.tr_radius,
                                 FluxOutputWorkspace=flux_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name)
        for transmission_no, transmission in enumerate(self.ctransmission.split(',')):
            [process_container_transmission, container_transmission_name] = \
                needs_processing(transmission, 'Transmission')
            self.progress.report('Processing container transmission')
            container_transmission_names.append(container_transmission_name)
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[transmission_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            if len(beam_transmission_names) > 1:
                transmission_beam_name = beam_transmission_names[transmission_no]
            else:
                transmission_beam_name = beam_transmission_names[0]
            if process_container_transmission:
                SANSILLReduction(Run=transmission,
                                 ProcessAs='Transmission',
                                 OutputWorkspace=container_transmission_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name,
                                 BeamInputWorkspace=transmission_beam_name,
                                 NormaliseBy=self.normalise,
                                 BeamRadius=self.tr_radius)
        for transmission_no, transmission in enumerate(self.stransmission.split(',')):
            [process_sample_transmission, sample_transmission_name] = \
                needs_processing(transmission, 'Transmission')
            self.progress.report('Processing sample transmission')
            sample_transmission_names.append(sample_transmission_name)
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[transmission_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            if len(beam_transmission_names) > 1:
                transmission_beam_name = beam_transmission_names[transmission_no]
            else:
                transmission_beam_name = beam_transmission_names[0]
            if process_sample_transmission:
                SANSILLReduction(Run=transmission,
                                 ProcessAs='Transmission',
                                 OutputWorkspace=sample_transmission_name,
                                 AbsorberInputWorkspace=
                                 transmission_absorber_name,
                                 BeamInputWorkspace=transmission_beam_name,
                                 NormaliseBy=self.normalise,
                                 BeamRadius=self.tr_radius)
        return container_transmission_names, sample_transmission_names

    def processAbsorber(self, i):
        absorber = (self.absorber[i]
                    if len(self.absorber) == self.dimensionality
                    else self.absorber[0])
        [process_absorber, absorber_name] = \
            needs_processing(absorber, 'Absorber')
        self.progress.report('Processing absorber')
        if process_absorber:
            SANSILLReduction(Run=absorber,
                             ProcessAs='Absorber',
                             NormaliseBy=self.normalise,
                             OutputWorkspace=absorber_name)
        return absorber_name

    def processBeam(self, i, absorber_name):
        beam = (self.beam[i]
                if len(self.beam) == self.dimensionality
                else self.beam[0])
        radius = (self.radius[i]
                  if len(self.radius) == self.dimensionality
                  else self.radius[0])
        [process_beam, beam_name] = needs_processing(beam, 'Beam')
        flux_name = beam_name + '_Flux' if not self.flux[0] else ''
        self.progress.report('Processing beam')
        if process_beam:
            SANSILLReduction(Run=beam,
                             ProcessAs='Beam',
                             OutputWorkspace=beam_name,
                             NormaliseBy=self.normalise,
                             BeamRadius=radius,
                             AbsorberInputWorkspace=absorber_name,
                             FluxOutputWorkspace=flux_name)
        return beam_name, flux_name

    def processFlux(self, i, aborber_name):
        if self.flux[0]:
            flux = (self.flux[i]
                    if len(self.flux) == self.dimensionality
                    else self.flux[0])
            radius = (self.radius[i]
                      if len(self.radius) == self.dimensionality
                      else self.radius[0])
            [process_flux, flux_name] = needs_processing(flux, 'Flux')
            self.progress.report('Processing flux')
            if process_flux:
                SANSILLReduction(Run=flux,
                                 ProcessAs='Beam',
                                 OutputWorkspace=flux_name.replace('Flux',
                                                                   'Beam'),
                                 NormaliseBy=self.normalise,
                                 BeamRadius=radius,
                                 AbsorberInputWorkspace=absorber_name,
                                 FluxOutputWorkspace=flux_name)
            return flux_name
        else:
            return None

    def processContainer(self, i, beam_name, absorber_name,
                         container_transmission_names):
        container = (self.container[i]
                     if len(self.container) == self.dimensionality
                     else self.container[0])
        [process_container, container_name] = \
            needs_processing(container, 'Container')
        if len(container_transmission_names) > 1:
            container_transmission_name = container_transmission_names[i]
        else:
            container_transmission_name = container_transmission_names[0]
        self.progress.report('Processing container')
        if process_container:
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

    def processSample(self, i, flux_name, sample_transmission_names, beam_name,
                      absorber_name, container_name):
        # this is the default mask, the same for all the distance configurations
        [load_default_mask, default_mask_name] = \
                needs_loading(self.default_mask, 'DefaultMask')
        self.progress.report('Loading default mask')
        if load_default_mask:
            LoadNexusProcessed(Filename=self.default_mask,
                               OutputWorkspace=default_mask_name)

        # this is the beam stop mask, potentially different at each distance configuration
        mask = (self.mask[i]
                if len(self.mask) == self.dimensionality
                else self.mask[0])
        [load_mask, mask_name] = needs_loading(mask, 'Mask')
        self.progress.report('Loading mask')
        if load_mask:
            LoadNexusProcessed(Filename=mask, OutputWorkspace=mask_name)
        # sensitivity
        sens_input = ''
        ref_input = ''
        if self.sensitivity:
            sens = (self.sensitivity[i]
                    if len(self.sensitivity) == self.dimensionality
                    else self.sensitivity[0])
            [load_sensitivity, sensitivity_name] = \
                needs_loading(sens, 'Sensitivity')
            sens_input = sensitivity_name
            self.progress.report('Loading sensitivity')
            if load_sensitivity:
                LoadNexusProcessed(Filename=sens,
                                   OutputWorkspace=sensitivity_name)

        # reference
        if self.reference:
            reference = (self.reference[i]
                         if len(self.reference) == self.dimensionality
                         else self.reference[0])
            [load_reference, reference_name] = \
                needs_loading(reference, 'Reference')
            ref_input = reference_name
            self.progress.report('Loading reference')
            if load_reference:
                LoadNexusProcessed(Filename=reference,
                                   OutputWorkspace=reference_name)

        # get correct transmission
        if len(sample_transmission_names) > 1:
            sample_transmission_name = sample_transmission_names[i]
        else:
            sample_transmission_name = sample_transmission_names[0]

        # sample
        [_, sample_name] = needs_processing(self.sample[i], 'Sample')
        output = self.output + '_' + str(i + 1)
        self.progress.report('Processing sample at detector configuration '
                             + str(i + 1))

        if (self.getPropertyValue('SensitivityOutputWorkspace') != ''
                and self.dimensionality > 1):
            output_sens = self.output_sens + '_' + str(i + 1)
        else:
            output_sens = self.output_sens
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
                SensitivityOutputWorkspace=output_sens,
                FluxInputWorkspace=flux_name,
                NormaliseBy=self.normalise,
                ThetaDependent=self.theta_dependent,
                SampleThickness=
                self.getProperty('SampleThickness').value,
                WaterCrossSection=
                self.getProperty('WaterCrossSection').value,
                )

        if self.getProperty('OutputPanels').value:
            panel_ws_group = self.output_panels + '_' + str(i + 1)
        else:
            panel_ws_group = ""

        if self.n_wedges and self.output_type == "I(Q)":
            output_wedges = self.output + "_wedge_d" + str(i + 1)
        else:
            output_wedges = ""

        if self.getProperty('SensitivityWithOffsets').value:
            CloneWorkspace(InputWorkspace=sample_name, OutputWorkspace=output_sens)

        SANSILLIntegration(
                InputWorkspace=sample_name,
                OutputWorkspace=output,
                OutputType=self.output_type,
                CalculateResolution=
                self.getPropertyValue('CalculateResolution'),
                DefaultQBinning=self.getPropertyValue('DefaultQBinning'),
                BinningFactor=self.getProperty('BinningFactor').value,
                OutputBinning=self.getPropertyValue('OutputBinning'),
                NPixelDivision=self.getProperty('NPixelDivision').value,
                NumberOfWedges=self.n_wedges,
                WedgeAngle=self.getProperty('WedgeAngle').value,
                WedgeOffset=self.getProperty('WedgeOffset').value,
                WedgeWorkspace=output_wedges,
                AsymmetricWedges=self.getProperty('AsymmetricWedges').value,
                PanelOutputWorkspaces=panel_ws_group,
                MaxQxy=(self.maxqxy[i]
                        if len(self.maxqxy) == self.dimensionality
                        else self.maxqxy[0]),
                DeltaQ=(self.deltaq[i]
                        if len(self.deltaq) == self.dimensionality
                        else self.deltaq[0]),
                IQxQyLogBinning=self.getProperty('IQxQyLogBinning').value,
                WavelengthRange=self.getProperty('WavelengthRange').value
                )

        # wedges ungrouping and renaming
        if self.n_wedges and self.output_type == "I(Q)":
            wedges_old_names = [output_wedges + "_" + str(w + 1)
                                for w in range(self.n_wedges)]
            wedges_new_names = [self.output + "_wedge_" + str(w + 1)
                                + "_" + str(i + 1)
                                for w in range(self.n_wedges)]
            UnGroupWorkspace(InputWorkspace=output_wedges)
            RenameWorkspaces(InputWorkspaces=wedges_old_names,
                             WorkspaceNames=wedges_new_names)

        if self.cleanup:
            DeleteWorkspace(sample_name)

        if not mtd.doesExist(panel_ws_group):
            panel_ws_group = ""

        return output, panel_ws_group, output_sens


AlgorithmFactory.subscribe(SANSILLAutoProcess)
