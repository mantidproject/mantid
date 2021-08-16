# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSILLCommon import *
from mantid.api import DataProcessorAlgorithm, WorkspaceGroupProperty, MultipleFileProperty, FileAction
from mantid.kernel import Direction, FloatBoundedValidator, FloatArrayProperty, IntBoundedValidator, StringListValidator
from mantid.simpleapi import *
from os import path

N_DISTANCES = 5 # maximum number of distinct distance configurations
N_LAMBDAS = 2 # maximum number of distinct wavelengths used in the experiment


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
            if isinstance(mtd[ws_name], WorkspaceGroup):
                run = mtd[ws_name][0].getRun()
            else:
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


class SANSILLMultiProcess(DataProcessorAlgorithm):
    '''
    Performs an entire experiment processing from ILL SANS beamlines.
    Supports standard monochromatic and kinetic SANS. Support for TOF to be added.
    Provides azimuthal integration only (I(Q)), optionally with sectors.
    Reduces all the samples at all the distances together in the most optimal way.
    '''
    instrument = None # the name of the instrument [D11, D11B, D16, D22, D22B, D33]
    rank = None # the rank of the reduction, i.e. the number of (detector distance, wavelength) configurations
    lambda_rank = None # how many wavelengths are we dealing with, i.e. how many transmissions need to be calculated
    n_samples = None # how many samples are being reduced
    sample_transmissions = dict() # map of sample transmissions keyed on wavelength values
    container_transmissions = dict() # map of container transmissions keyed on wavelength values

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Performs SANS data reduction of the entire experiment.'

    def seeAlso(self):
        return ['SANSILLReduction', 'SANSILLIntegration']

    def name(self):
        return 'SANSILLMultiProcess'

    def PyInit(self):

        #================================INPUT RUNS================================#

        for d in range(N_DISTANCES):
            p_name = f'SampleRunsD{d+1}'
            self.declareProperty(MultipleFileProperty(name=p_name,
                                                      action=FileAction.OptionalLoad,
                                                      extensions=['nxs'],
                                                      allow_empty=True),
                                 doc=f'Sample run(s) at the distance #{d+1}.')
            self.setPropertyGroup(p_name, 'Numors')

        self.declareProperty(MultipleFileProperty(name='DarkCurrentRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s).')
        self.setPropertyGroup('DarkCurrentRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='EmptyBeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s).')
        self.setPropertyGroup('EmptyBeamRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='FluxRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for flux calculation only; '
                                 'if left blank the flux will be calculated from EmptyBeamRuns.')
        self.setPropertyGroup('FluxRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='EmptyContainerRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty container run(s).')
        self.setPropertyGroup('EmptyContainerRuns', 'Numors')

        #================================TR INPUT RUNS================================#

        for l in range(N_LAMBDAS):
            p_name = f'SampleTrRunsW{l+1}'
            self.declareProperty(MultipleFileProperty(name=p_name,
                                                      action=FileAction.OptionalLoad,
                                                      extensions=['nxs'],
                                                      allow_empty=True),
                                 doc=f'Sample run(s) at the wavelength #{l+1}.')
            self.setPropertyGroup(p_name, 'Transmission Numors')

        self.declareProperty(MultipleFileProperty(name='TrDarkCurrentRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s) for transmission calculation.')

        self.declareProperty(MultipleFileProperty(name='ContainerTrRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Container transmission run(s).')
        self.setPropertyGroup('ContainerTrRuns', 'Transmission Numors')

        self.declareProperty(MultipleFileProperty(name='TrEmptyBeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for transmission calculation.')

        self.setPropertyGroup('TrDarkCurrentRuns', 'Transmission Numors')
        self.setPropertyGroup('ContainerTrRuns', 'Transmission Numors')
        self.setPropertyGroup('TrEmptyBeamRuns', 'Transmission Numors')

        #=============================INPUT FILES/WORKSPACES==============================#

        self.declareProperty(name='SensitivityMap', defaultValue='',
                             doc='File or workspace containing the map of the relative detector efficiencies.')
        self.setPropertyGroup('SensitivityMap', 'Input Files/Workspaces')

        self.declareProperty(name='DefaultMask', defaultValue='',
                             doc='File or workspace containing the default mask (detector edges and dead pixels/tubes)'
                                 ' to be applied to all the detector configurations.')
        self.setPropertyGroup('DefaultMask', 'Input Files/Workspaces')

        self.declareProperty(name='BeamStopMasks', defaultValue='',
                             doc='File(s) or workspace(s) containing the detector mask per distance configuration (typically beam stop).')
        self.setPropertyGroup('BeamStopMasks', 'Input Files/Workspaces')

        self.declareProperty(name='FlatFields', defaultValue='',
                             doc='File(s) or workspaces containing the reduced water data (in 2D) for absolute normalisation.')
        self.setPropertyGroup('FlatFields', 'Input Files/Workspaces')

        self.declareProperty(name='Solvents', defaultValue='',
                             doc='File(s) or workspace(s) containing the reduced solvent/buffer data (in 2D) for solvent subtraction.')
        self.setPropertyGroup('Solvents', 'Input Files/Workspaces')

        #==============================REDUCTION PARAMETERS===============================#

        self.declareProperty(name='TransmissionThetaDependent', defaultValue=True,
                             doc='Whether or not to apply the transmission correction in 2theta-dependent way.')
        self.setPropertyGroup('TransmissionThetaDependent', 'Parameters')

        self.declareProperty(name='NormaliseBy', defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Time', 'Monitor']))
        self.setPropertyGroup('NormaliseBy', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='TrBeamRadius', values=[1.]),
                             doc='Beam radius [m] used for transmission and flux calculations.')
        self.setPropertyGroup('TrBeamRadius', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='BeamRadius', values=[1.]),
                             doc='Beam radius [m] used for beam center finding.')
        self.setPropertyGroup('BeamRadius', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='SampleThickness', values=[0.1]),
                             doc='Sample thickness [cm] used in final normalisation.')
        self.setPropertyGroup('SampleThickness', 'Parameters')

        self.declareProperty(name='WaterCrossSection', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0.),
                             doc='Provide the water cross-section; used only if the absolute scale is done by dividing to water.')
        self.setPropertyGroup('WaterCrossSection', 'Parameters')

        self.declareProperty(name='SensitivityWithOffsets', defaultValue=False,
                             doc='Whether the sensitivity data has been measured with different horizontal offsets (D22 only).')
        self.setPropertyGroup('SensitivityWithOffsets', 'Parameters')

        self.declareProperty(name='OutputType', defaultValue='I(Q)',
                             validator=StringListValidator(['None', 'I(Q)']),
                             doc='The type of the integration to perform.')
        self.setPropertyGroup('OutputType', 'Parameters')

        self.declareProperty(name='SubAlgorithmOffset', defaultValue=True,
                             doc='Whether to offset logging from child algorithms one level down.')
        self.setPropertyGroup('SubAlgorithmOffset', 'Parameters')

        #===================================I(Q) OPTIONS==================================#

        self.declareProperty(name='OutputBinning', defaultValue='',
                             doc='Output binning for each distance( : separated list of binning params).')
        self.setPropertyGroup('OutputBinning', 'I(Q) Options')

        iq_options = ['CalculateResolution', 'DefaultQBinning', 'BinningFactor', 'NumberOfWedges', 'WedgeAngle',
                      'WedgeOffset', 'AsymmetricWedges', 'WavelengthRange', 'ShapeTable']
        self.copyProperties('SANSILLIntegration', iq_options)
        for opt in iq_options:
            self.setPropertyGroup(opt, 'I(Q) Options')

        #==================================STITCH OPTIONS=================================#

        stitch_options = ['ManualScaleFactors', 'TieScaleFactors', 'ScaleFactorCalculation']
        self.copyProperties('Stitch', stitch_options)
        for opt in stitch_options:
            self.setPropertyGroup(opt, 'Stitch Options')

        self.declareProperty(name='StitchReferenceIndex', defaultValue=1,
                             validator=IntBoundedValidator(lower=0),
                             doc='The index of the reference workspace during stitching, '
                                 'by default the middle distance will be chosen as reference if there are 3.')
        self.setPropertyGroup('StitchReferenceIndex', 'Stitch Options')

        #================================OUTPUT WORKSPACES================================#

        # This will be a group containing all the main and diagnostic outputs
        self.declareProperty(WorkspaceGroupProperty(name='OutputWorkspace', defaultValue='',
                                                    direction=Direction.Output),
                             doc='The output workspace group containing the reduced data.')
        self.setPropertyGroup('OutputWorkspace', 'Output Workspaces')

    def _reset(self):
        '''Resets the class member variables'''
        self.instrument = None
        self.rank = None
        self.lambda_rank = None
        self.n_samples = None
        self.properties = dict()
        self.sample_transmissions = dict()
        self.container_transmissions = dict()

    def _set_rank(self):
        '''Sets the actual rank of the reduction'''
        self.rank = 0
        for d in range(N_DISTANCES):
            if self.getPropertyValue(f'SampleRunsD{d+1}'):
                self.rank += 1
        if self.rank == 0:
            raise RuntimeError('No sample runs are provided, at least one distance must not be empty.')
        else:
            self.log().notice(f'Set the rank of reduction to {self.rank}')

    def _set_lambda_rank(self):
        '''Sets the actual lambda rank'''
        self.lambda_rank = 0
        for l in range(N_LAMBDAS):
            if self.getPropertyValue(f'SampleTrRunsW{l+1}'):
                self.lambda_rank += 1
        self.log().notice(f'Set the lambda rank of reduction to {self.lambda_rank}')

    def _set_n_samples(self):
        '''Sets the number of samples based on the inputs of the sample runs in the first non-empty distance'''
        for d in range(self.rank):
            if self.getPropertyValue(f'SampleRunsD{d+1}'):
                self.n_samples = self.getPropertyValue(f'SampleRunsD{d+1}').count(',') + 1
                self.log().notice(f'Set number of samples to {self.n_samples}')
                break

    def _setup_light(self):
        '''Performs a light setup, which can be done before loading any data'''
        self._reset()
        self._set_rank()
        self._set_lambda_rank()
        self._set_n_samples() # must be after set_rank()

    def _setup(self, ws):
        '''Performs a full setup, which can be done only after having loaded the sample data'''
        self._setup_light()
        self.instrument = ws.getInstrument().getName()
        unit = ws.getAxis(0).getUnit().unitID()
        if unit == 'Wavelength':
            raise RuntimeError('TOF is not yet supported in the new workflow, please use the old one.')

    def _check_sample_runs_dimensions(self):
        '''Makes sure all the sample inputs have matching extents'''
        issues = dict()
        for d in range(self.rank):
            prop = f'SampleRunsD{d+1}'
            n_items = self.getPropertyValue(prop).count(',') + 1
            if n_items != self.n_samples:
                issues[prop] = f'{prop} has {n_items} elements instead of {self.n_samples}'
        return issues

    def _check_tr_runs_dimensions(self):
        '''Makes sure all the sample transmission inputs have matching extents'''
        issues = dict()
        for l in range(self.lambda_rank):
            prop = f'SampleRunsD{l+1}'
            n_items = self.getPropertyValue(prop).count(',') + 1
            if n_items > 1 and n_items != self.n_samples:
                issues[prop] = f'{prop} has {n_items} elements instead of {self.n_samples}'
        return issues

    def _check_aux_sample_input_dimensions(self):
        '''Checks if the provided correction inputs have the right rank'''
        issues = dict()
        props_to_match_rank = ['DarkCurrentRuns', 'EmptyBeamRuns', 'FluxRuns', 'EmptyContainerRuns',
                               'BeamStopMasks', 'FlatFields', 'Solvents']
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(',') + 1
                if prop_rank > 1 and prop_rank != self.rank:
                    issues[prop] = f'{prop} has {prop_rank} elements which does not match the number of distances {self.rank}'
        return issues

    def _check_aux_tr_input_dimensions(self):
        '''Checks if the provided correction inputs have the right lambda rank'''
        issues = dict()
        props_to_match_lambda_rank = ['TrDarkCurrentRuns', 'ContainerTrRuns', 'TrEmptyBeamRuns']
        for prop in props_to_match_lambda_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(',') + 1
                if prop_rank > 1 and prop_rank != self.lambda_rank:
                    issues[prop] = f'{prop} has {prop_rank} elements which does not match the number of wavelengths {self.lambda_rank}'
        return issues

    def _check_aux_sample_params_dimensions(self):
        '''Checks if provided primitive parameters match the rank of the samples'''
        issues = dict()
        props_to_match_rank = ['SampleThickness', 'BeamRadius']
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(',') + 1
                if prop_rank > 1 and prop_rank != self.rank:
                    issues[prop] = f'{prop} has {prop_rank} elements which does not match the number of distances {self.rank}'
        return issues

    def _check_aux_tr_params_dimensions(self):
        '''Checks if provided primitive parameters for transmissions match the rank of the wavelengths'''
        issues = dict()
        props_to_match_rank = ['TrBeamRadius']
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(',') + 1
                if prop_rank > 1 and prop_rank != self.lambda_rank:
                    issues[prop] = f'{prop} has {prop_rank} elements which does not match the number of distances {self.lambda_rank}'
        return issues

    def _check_q_ranges(self):
        '''Makes sure the OutputBinning is composed of : delimited q rebin params'''
        issues = dict()
        qbinning = self.getPropertyValue('OutputBinning')
        if qbinning:
            if qbinning.count(':') + 1 != self.rank:
                issues['OutputBinning'] = 'Number of Q binning parameter sets must be equal to the number of distances.'
            else:
                for qbin_params in qbinning.split(':'):
                    if qbin_params:
                        for qbin_param in qbin_params.split(','):
                            try:
                                float(qbin_param)
                            except ValueError:
                                issues['OutputBinning'] = 'Q binning params must be float numbers.'
                                break
        return issues

    def _check_output_name(self):
        '''Makes sure the name of the output workspace starts with a letter'''
        issues = dict()
        if self.getPropertyValue('OutputWorkspace')[0].isdigit():
            issues['OutputWorkspace'] = "Output workspace name must be alphanumeric, it should start with a letter."
        return issues

    def validateInputs(self):
        '''Validates all the inputs'''
        issues = dict()
        self._setup_light()
        issues = self._check_sample_runs_dimensions()
        if not issues:
            issues = self._check_tr_runs_dimensions()
        if not issues:
            issues = self._check_aux_sample_input_dimensions()
        if not issues:
            issues = self._check_aux_tr_input_dimensions()
        if not issues:
            issues = self._check_aux_sample_params_dimensions()
        if not issues:
            issues = self._check_aux_tr_params_dimensions()
        if not issues:
            issues = self._check_q_ranges()
        if not issues:
            issues = self._check_output_name()
        return issues

    def PyExec(self):
        '''Executes the algorithm'''
        self._setup_light()
        self.process_transmissions()
        self.process_samples()
        self.finalize()

    def process_transmissions(self):
        '''Calculates all the transmissions'''
        for l in range(self.lambda_rank):
            self.process_all_transmissions_at_lambda(l)

    def process_samples(self):
        '''Reduces all the samples'''
        for d in range(self.rank):
            self.process_all_samples_at_distance(d)

    def process_all_transmissions_at_lambda(self, l):
        '''
        Calculates transmissions at the given lambda index
        '''
        tr_dark_current_ws = self.process_tr_dark_current(l)
        [tr_empty_beam_ws, tr_empty_beam_flux] = self.process_tr_empty_beam(l, tr_dark_current_ws)
        tr_empty_can_ws = self.process_empty_can_tr(l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux)
        tr_sample_ws = self.process_sample_tr(l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux)
        if tr_empty_can_ws:
            w = mtd[tr_empty_can_ws].getRun()['wavelength'].value
            self.container_transmissions[str(round(w, 2))] = tr_empty_can_ws
        if tr_sample_ws:
            w = mtd[tr_sample_ws].getRun()['wavelength'].value
            self.sample_transmissions[str(round(w,2))] = tr_sample_ws

    def process_tr_dark_current(self, l):
        runs = self.getPropertyValue('TrDarkCurrentRuns')
        if runs:
            tr_dark_current = runs.split(',')[l]
            [process_tr_dark_current, tr_dark_current_ws] = needs_processing(tr_dark_current, 'DarkCurrent')
            if process_tr_dark_current:
                SANSILLReduction(Runs=tr_dark_current,
                                 ProcessAs='DarkCurrent',
                                 NormaliseBy=self.getProperty('NormaliseBy').value,
                                 OutputWorkspace=tr_dark_current_ws)
            return tr_dark_current_ws
        else:
            return ''

    def process_tr_empty_beam(self, l, tr_dark_current_ws):
        runs = self.getPropertyValue('TrEmptyBeamRuns')
        if runs:
            tr_empty_beam = runs.split(',')[l]
            [process_tr_empty_beam, tr_empty_beam_ws] = needs_processing(tr_empty_beam, 'EmptyBeam')
            tr_empty_beam_flux = tr_empty_beam_ws + 'Flux'
            if process_tr_empty_beam:
                SANSILLReduction(Runs=tr_empty_beam,
                                 ProcessAs='EmptyBeam',
                                 DarkCurrentWorkspace=tr_dark_current_ws,
                                 NormaliseBy=self.getProperty('NormaliseBy').value,
                                 TrBeamRadius=self.getProperty('TrBeamRadius').value[l],
                                 OutputWorkspace=tr_empty_beam_ws,
                                 OutputFluxWorkspace=tr_empty_beam_flux)
            return [tr_empty_beam_ws, tr_empty_beam_flux]
        else:
            return ['','']

    def process_empty_can_tr(self, l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux):
        runs = self.getPropertyValue('ContainerTrRuns')
        if runs:
            tr_empty_can = runs.split(',')[l]
            [process_tr_empty_can, tr_empty_can_ws] = needs_processing(tr_empty_can, 'Transmission')
            if process_tr_empty_can:
                SANSILLReduction(Runs=tr_empty_can,
                                 ProcessAs='Transmission',
                                 DarkCurrentWorkspace=tr_dark_current_ws,
                                 EmptyBeamWorkspace=tr_empty_beam_ws,
                                 FluxWorkspace=tr_empty_beam_flux,
                                 NormaliseBy=self.getProperty('NormaliseBy').value,
                                 TrBeamRadius=self.getProperty('TrBeamRadius').value[l],
                                 OutputWorkspace=tr_empty_can_ws)
            return tr_empty_can_ws
        else:
            return ''

    def process_sample_tr(self, l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux):
        tr_sample = self.getPropertyValue(f'SampleTrRunsW{l+1}')
        if tr_sample:
            [_, tr_sample_ws] = needs_processing(tr_sample, 'Transmission')
            SANSILLReduction(Runs=tr_sample,
                             ProcessAs='Transmission',
                             DarkCurrentWorkspace=tr_dark_current_ws,
                             EmptyBeamWorkspace=tr_empty_beam_ws,
                             FluxWorkspace=tr_empty_beam_flux,
                             NormaliseBy=self.getProperty('NormaliseBy').value,
                             TrBeamRadius=self.getProperty('TrBeamRadius').value[l],
                             OutputWorkspace=tr_sample_ws)
            return tr_sample_ws
        else:
            return ''

    def process_all_samples_at_distance(self, d):
        '''
        Reduces all the samples at a given distance
        Note, there must be no loop beyond this point in stack
        '''
        pass

    def finalize(self):
        pass


AlgorithmFactory.subscribe(SANSILLMultiProcess)
