#pylint: disable=no-init,too-many-instance-attributes,invalid-name
from __future__ import (absolute_import, division, print_function)

import os.path
import numpy as np
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, mtd, logger


ws_or_none = lambda s: mtd[s] if s != '' else None


def extract_workspace(ws, ws_out, x_start, x_end):
    """
    Extracts a part of the workspace
    @param  ws      :: input workspace name
    @param  ws_out  :: output workspace name
    @param  x_start :: start bin of workspace to be extracted
    @param  x_end   :: end bin of workspace to be extracted
    """
    CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws_out, XMin=x_start, XMax=x_end)
    ScaleX(InputWorkspace=ws_out, OutputWorkspace=ws_out, Factor=-x_start, Operation='Add')


def monitor_range(ws):
    """
    Get sensible x-range where monitor count is not zero
    Used to mask out the first and last few channels
    @param ws :: name of workspace
    @return   :: tuple of xmin and xmax
    """
    x = mtd[ws].readX(0)
    y = mtd[ws].readY(0)
    # mid x value in order to search for left and right monitor range delimiter
    size = len(x)
    # Maximum search in left and right half of the workspace
    mid = int(size / 2)
    # Maximum position left
    imin = np.nanargmax(np.array(y[0:mid])) - 1
    # Maximum position right
    imax = np.nanargmax(np.array(y[mid:size])) + 1 + mid
    return x[imin], x[imax]


# possibility to replace by the use of SelectNexusFilesByMetadata
def check_QENS(ws):
    """
    Checks if the given ws is of QENS type
    @param ws :: input ws name
    @return   :: True if it is, False otherwise
    """
    runobject = mtd[ws].getRun()
    runnumber = mtd[ws].getRunNumber()
    result = True

    if not runobject.hasProperty('Doppler.maximum_delta_energy'):
        if not runobject.hasProperty('Doppler.velocity_profile'):
            logger.warning('Run #%s has no Doppler.velocity_profile neither '
                           'Doppler.maximum_delta_energy. Assuming QENS type.' % runnumber)
        else:
            profile = runobject.getLogData('Doppler.velocity_profile').value
            if profile == 0:
                logger.warning('Run #%s has no Doppler.maximum_delta_energy but '
                               'Doppler.velocity_profile is 0. Assuming QENS type.' % runnumber)
            else:
                logger.warning('Run #%s has no Doppler.maximum_delta_energy but '
                               'Doppler.velocity_profile is not 0. Not a QENS data. Skipping.' % runnumber)
                result = False
    else:
        energy = runobject.getLogData('Doppler.maximum_delta_energy').value
        if energy == 0:
            logger.warning('Run #%s has Doppler.maximum_delta_energy 0. Not a QENS data. Skipping.' % runnumber)
            result = False
        else:
            if not runobject.hasProperty('Doppler.velocity_profile'):
                logger.warning('Run #%s has no Doppler.velocity_profile but '
                               'Doppler.maximum_delta_energy is not 0. Assuming QENS data.' % runnumber)
            else:
                profile = runobject.getLogData('Doppler.velocity_profile').value
                if profile != 0:
                    logger.warning('Run #%s has Doppler.velocity_profile not 0. Not a QENS data. Skipping.'
                                   % runnumber)
                    result = False

    return result


# possibility to replace by the use of SelectNexusFilesByMetadata
def select_fws(workspace):
    """
    Select fixed-window scan
    @param workspace :: input ws name
    @return   :: bool
    """
    run_object = mtd[workspace].getRun()
    run_number = mtd[workspace].getRunNumber()

    # Only older Nexus files seem to have a Doppler.frequency defined

    velocity_profile = 1
    energy = -1

    if run_object.hasProperty('Doppler.velocity_profile'):
        velocity_profile = run_object.getLogData('Doppler.velocity_profile').value
    else:
        logger.debug('Run {0} has no Doppler.velocity_profile'.format(run_number))

    if run_object.hasProperty('Doppler.maximum_delta_energy'):
        energy = run_object.getLogData('Doppler.maximum_delta_energy').value
    elif run_object.hasProperty('Doppler.delta_energy'):
        energy = run_object.getLogData('Doppler.delta_energy').value
    else:
        logger.error('Run {0} has neither Doppler.maximum_delta_energy nor Doppler.delta_energy'.
                     format(run_number))

    if velocity_profile == 1:
        fws = True
        logger.information('Run {0} FWS data'.format(run_number))
    elif energy == 0:
        fws = True
        logger.information('Run {0} FWS data'.format(run_number))
    else:
        fws = False
        logger.information('Run {0} not FWS data'.format(run_number))

    return fws


def mask_reduced_ws(ws_to_mask, xstart, xend):
    """
    Calls MaskBins twice, for masking the first and last bins of a workspace
    Args:
        red:      reduced workspace
        xstart:   MaskBins between x[0] and x[xstart]
        xend:     MaskBins between x[xend] and x[-1]

    """
    x_values = mtd[ws_to_mask].readX(0)

    if xstart > 0:
        logger.debug('Mask bins smaller than {0}'.format(xstart))
        MaskBins(InputWorkspace=mtd[ws_to_mask], OutputWorkspace=mtd[ws_to_mask], XMin=x_values[0],
                 XMax=x_values[xstart])
    else:
        logger.debug('No masking due to x bin <= 0!: {0}'.format(xstart))
    if xend < len(x_values) - 1:
        logger.debug('Mask bins larger than {0}'.format(xend))
        MaskBins(InputWorkspace=mtd[ws_to_mask], OutputWorkspace=ws_to_mask, XMin=x_values[xend + 1], XMax=x_values[-1])
    else:
        logger.debug('No masking due to x bin >= len(x_values) - 1!: {0}'.format(xend))

    if xstart > 0 and xend < len(x_values) - 1:
        logger.notice('Bins out of range {0} {1} [Unit of X-axis] are masked'.format(x_values[xstart],
                                                                                     x_values[xend + 1]))


def convert_to_energy(ws):
    """
    Convert the input ws x-axis from channel to energy transfer
    @param ws     :: input workspace name
    """
    # get energy formula
    formula = energy_formula(ws)
    ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='X', Formula=formula)
    mtd[ws].getAxis(0).setUnit('DeltaE')  # in mev
    xnew = mtd[ws].readX(0)  # energy array
    logger.information('Energy range : {0} to {1}'.format(xnew[0], xnew[-1]))


def energy_formula(ws):
    """
    Calculate the formula for channel number to energy transfer transformation
    @param ws :: name of the input workspace
    @return   :: formula to transform from time channel to energy transfer
    """
    x = mtd[ws].readX(0)
    size = len(x)
    mid = float((size - 1) / 2)
    gRun = mtd[ws].getRun()
    delta_energy = 0.
    scale = 1000  # from micro ev to milli ev

    if gRun.hasProperty('Doppler.maximum_delta_energy'):
        delta_energy = gRun.getLogData('Doppler.maximum_delta_energy').value  # max energy in micro eV
        logger.information('Doppler max delta energy in micro eV : {0}'.format(delta_energy))
    elif gRun.hasProperty('Doppler.delta_energy'):
        delta_energy = gRun.getLogData('Doppler.delta_energy').value  # delta energy in micro eV
        logger.information('Doppler delta energy in micro eV : {0}'.format(delta_energy))
    else:
        logger.warning('Input run has no property Doppler.mirror_sense. Check your input file.')
        logger.warning('Doppler maximum delta energy is 0 micro eV')

    if delta_energy != 0:
        formula = '(x/{0} - 1)*{1}'.format(mid, delta_energy / scale)
        logger.information('Energy transform formula: ' + formula)
    else:
        # Center the data for elastic fixed window scan, for integration over the elastic peak
        formula = 'x-{0}'.format(mid)

    return formula


def perform_unmirror(red, left, right, option):
    """
    Handling unmirror options > 0 and sum left and right wing if needed
    @param red::          reduced workspace, will be updated
    @param left::         left workspace
    @param right::        right workspace
    @param option::       the unmirror option
    @return:: start_bin   bins with smaller bin number will be masked
    @return:: end_bin     bins with higher bin number will be masked
    """
    # Initial bins out of which range masking will be performed
    start_bin = 0
    end_bin = mtd[red].blocksize()

    if option == 0:
        logger.information('Unmirror 0: Nothing to be done')

    elif option == 1:
        logger.information('Unmirror 1: Sum the left and right wings')

    elif option == 2:
        logger.information('Unmirror 2: Return the left wing')
        CloneWorkspace(InputWorkspace=left, OutputWorkspace=red)

    elif option == 3:
        logger.information('Unmirror 3: Return the right wing')
        CloneWorkspace(InputWorkspace=right, OutputWorkspace=red)

    elif option == 4:
        logger.information('Unmirror 4: Shift the right according to left')
        _bin_range = '__bin_range'
        MatchPeaks(InputWorkspace=right, OutputWorkspace=right, InputWorkspace2=left, BinRangeTable=_bin_range)
        bin_table = mtd[_bin_range].row(0)
        start_bin = bin_table['MinBin']
        end_bin = bin_table['MaxBin']

    elif option == 5:
        logger.information('Unmirror 5: Shift the right according to right of the vanadium and sum to left')
        _bin_range = '__bin_range'
        MatchPeaks(InputWorkspace=right, InputWorkspace2='right_van', OutputWorkspace=left, BinRangeTable=_bin_range)
        bin_table = mtd[_bin_range].row(0)
        start_bin = bin_table['MinBin']
        end_bin = bin_table['MaxBin']

    elif option == 6:
        logger.information('Unmirror 6: Center both the right and the left')
        _bin_range_left = '__bin_range_left'
        _bin_range_right = '__bin_range_right'
        MatchPeaks(InputWorkspace=left, OutputWorkspace=left, BinRangeTable=_bin_range_left)
        MatchPeaks(InputWorkspace=right, OutputWorkspace=right, BinRangeTable=_bin_range_right)
        left_table = mtd[_bin_range_left].row(0)
        right_table = mtd[_bin_range_right].row(0)
        start_bin = np.max([left_table['MinBin'], right_table['MinBin']])
        end_bin = np.min([left_table['MaxBin'], right_table['MaxBin']])

    elif option == 7:
        logger.information('Unmirror 7: Shift both the right and the left according to vanadium and sum')
        _bin_range_left = 'bin_range_left'
        _bin_range_right = 'bin_range_right'
        MatchPeaks(InputWorkspace=left, InputWorkspace2='left_van', OutputWorkspace=left, MatchInput2ToCenter=True,
                   BinRangeTable=_bin_range_left)
        MatchPeaks(InputWorkspace=right, InputWorkspace2='right_van', OutputWorkspace=right, MatchInput2ToCenter=True,
                   BinRangeTable=_bin_range_right)
        left_table = mtd[_bin_range_left].row(0)
        right_table = mtd[_bin_range_right].row(0)
        start_bin = np.max([left_table['MinBin'], right_table['MinBin']])
        end_bin = np.min([left_table['MaxBin'], right_table['MaxBin']])

    if option > 3 or option == 1:
        # Perform unmirror option by summing left and right workspaces
        Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=red)
        Scale(InputWorkspace=red, OutputWorkspace=red, Factor=0.5, Operation='Multiply')

    return start_bin, end_bin


class IndirectILLReduction(DataProcessorAlgorithm):

    # Optional input calibration workspace
    _calib_ws = None

    # Files
    _map_file = None
    _run_file = None
    _vanadium_file = None
    _background_file = None
    _parameter_file = None

    # Bool flags
    _debug_mode = None
    _scan_type = None
    _sum_runs = None

    # Integer
    _unmirror_option = None
    _mirror_sense = None

    # Other
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None

    # Output Workspace names
    _out_suffixes = [] # 1D list of output ws suffixes w/o run numbers

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs QENS energy transfer reduction for ILL indirect geometry data, instrument IN16B.'

    def PyInit(self):
        # File properties
        self.declareProperty(MultipleFileProperty('Run',extensions=['nxs']),
                             doc='File path of run (s).')

        self.declareProperty(FileProperty('VanadiumRun', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='File path of vanadium run. Used for UnmirrorOption=[5, 7]')

        self.declareProperty(FileProperty('BackgroundRun', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='File path of background run.')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Filename of the detector grouping map file to use. \n'
                                 'If left blank the default will be used.')
        # Other inputs
        self.declareProperty(MatrixWorkspaceProperty('CalibrationWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace containing calibration intensities for each detector')

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111','311']),
                             doc='Analyser reflection.')

        self.declareProperty(name='ScanType',
                             defaultValue='QENS',
                             validator=StringListValidator(['QENS', 'FWS']),
                             doc='Select quasi-elastic scan QENS or fixed-window scan (FWS) data.')

        self.declareProperty(name='SumRuns',
                             defaultValue=False,
                             doc='Whether to sum all the input runs.')

        self.declareProperty(name='DebugMode',
                             defaultValue=False,
                             doc='Whether to output the workspaces in intermediate steps.')

        self.declareProperty(name='UnmirrorOption',defaultValue=1,
                             validator=IntBoundedValidator(lower=0, upper=7),
                             doc='Unmirroring options: \n'
                                 '0 no unmirroring\n'
                                 '1 sum of left and right\n'
                                 '2 left\n'
                                 '3 right\n'
                                 '4 shift right according to left and sum\n'
                                 '5 like 4, but use Vanadium run for peak positions\n'
                                 '6 center both left and right at zero and sum\n'
                                 '7 like 6, but use Vanadium run for peak positions')

        # Output workspace properties
        self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "red",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the reduced workspace(s).")

        # Debug mode
        self.declareProperty(WorkspaceGroupProperty("RawWorkspace", "raw",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the raw workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("MonitorWorkspace", "monitor",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the monitor workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("DetWorkspace", "detgrouped",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the det workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("MnormWorkspace", "mnorm",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the mnorm workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("BsubWorkspace", "bsub",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the bsub workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("VnormWorkspace", "vnorm",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the vnorm workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("RightWorkspace", "right",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the right workspace(s).")

        self.declareProperty(WorkspaceGroupProperty("LeftWorkspace", "left",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the left workspace(s).")

    def validateInputs(self):
        # This is run before setUp, so need to get properties also here!
        issues = dict()
        # Unmirror options 5 and 7 require a Vanadium run as input workspace
        if (self.getProperty('UnmirrorOption').value == 5 or self.getProperty('UnmirrorOption').value == 7) \
                and not self.getPropertyValue('VanadiumRun'):
            issues['VanadiumRun'] = 'Given unmirror option requires vanadium run to be set'

        # Check if calibration workspace seems of correct shape, if specified
        if self.getPropertyValue('CalibrationWorkspace'):
            __calib = mtd[self.getPropertyValue('CalibrationWorkspace')]
            if __calib is None:
                issues['CalibrationWorkspace'] = 'Calibration workspace does not exist.'
            else:
                if isinstance(__calib,WorkspaceGroup):
                    issues['CalibrationWorkspace'] = 'Calibration workspace should not be a workspace group.'
                else:
                    if __calib.blocksize() != 1:
                        issues['CalibrationWorkspace'] = 'Calibration workspace should contain only ' \
                                                         'one column of calibration constants.'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')
        self._vanadium_file = self.getPropertyValue('VanadiumRun')
        self._background_file = self.getPropertyValue('BackgroundRun')
        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._calib_ws = ws_or_none(self.getPropertyValue('CalibrationWorkspace'))
        self._reflection = self.getPropertyValue('Reflection')
        self._debug_mode = self.getProperty('DebugMode').value
        self._scan_type = self.getProperty('ScanType').value
        self._sum_runs = self.getProperty('SumRuns').value
        self._unmirror_option = self.getProperty('UnmirrorOption').value

        self._red_ws = self.getPropertyValue('OutputWorkspace')
        self._raw_ws = self._red_ws + '_' + self.getPropertyValue('RawWorkspace')
        self._monitor_ws = self._red_ws + '_' + self.getPropertyValue('MonitorWorkspace')
        self._det_ws = self._red_ws + '_' + self.getPropertyValue('DetWorkspace')
        self._mnorm_ws = self._red_ws + '_' + self.getPropertyValue('MnormWorkspace')
        self._bsub_ws = self._red_ws + '_' + self.getPropertyValue('BsubWorkspace')
        self._vnorm_ws = self._red_ws + '_' + self.getPropertyValue('VnormWorkspace')
        self._left_ws = self._red_ws + '_' + self.getPropertyValue('LeftWorkspace')
        self._right_ws = self._red_ws + '_' + self.getPropertyValue('RightWorkspace')

        if self._sum_runs is True:
            self.log().notice('All the runs will be summed')
            self._run_file = self._run_file.replace(',', '+')

        self._out_suffixes = [self._red_ws, self._raw_ws, self._det_ws, self._monitor_ws, self._mnorm_ws,
                              self._left_ws, self._right_ws, self._bsub_ws, self._vnorm_ws]

    def PyExec(self):

        self.setUp()
        # This must be Load, to be able to treat multiple files
        Load(Filename=self._run_file, OutputWorkspace=self._red_ws)

        self.log().information('Loaded .nxs file(s) : {0}'.format(self._run_file))

        not_selected_runs = []
        out_ws_names = []

        # check if it is a workspace or workspace group and perform reduction correspondingly
        if isinstance(mtd[self._red_ws], WorkspaceGroup):

            # get instrument from the first ws in a group and load config files
            self._instrument = mtd[self._red_ws].getItem(0).getInstrument()

            self.load_auxiliary_files()

            # figure out number of progress reports, i.e. one for each input workspace/file
            progress = Progress(self, start=0.0, end=1.0, nreports=mtd[self._red_ws].size())

            # traverse over items in workspace group and reduce individually
            for i in range(mtd[self._red_ws].size()):

                # get the run number (if it is summed the run number of the first ws will be for the sum)
                run = '{0:06d}'.format(mtd[self._red_ws].getItem(i).getRunNumber())

                # get the name of the ws (it won't be the same as run, if there is a sum,
                # since the loader will concatenate run numbers in the name, e.g. RUN1_RUN2_...
                name = mtd[self._red_ws].getItem(i).getName()

                ws = run + '_' + self._red_ws
                # prepend run number
                RenameWorkspace(InputWorkspace=name, OutputWorkspace=ws)

                progress.report("Reducing run #" + run)
                # check if the run is QENS type and call reduction for each run
                if (check_QENS(ws) and self._scan_type == 'QENS') or (select_fws(ws) and self._scan_type == 'FWS'):
                    self._reduce_run(run, out_ws_names)
                else:
                    not_selected_runs.append(run)
        else:
            # get instrument name and load config files
            self._instrument = mtd[self._red_ws].getInstrument()

            self.load_auxiliary_files()

            run = '{0:06d}'.format(mtd[self._red_ws].getRunNumber())
            ws = run + '_' + self._red_ws
            # prepend run number
            RenameWorkspace(InputWorkspace=self._red_ws, OutputWorkspace=ws)

            # check if the run is QENS type and call reduction
            if (check_QENS(ws) and self._scan_type == 'QENS') or (select_fws(ws) and self._scan_type == 'FWS'):
                self._reduce_run(run, out_ws_names)
            else:
                not_selected_runs.append(run)

        # remove any loaded non-QENS type data if was given:
        for not_selected_ws in not_selected_runs:
            DeleteWorkspace(not_selected_ws + '_' + self._red_ws)

        # wrap up the output
        self._finalize(out_ws_names)

    def load_auxiliary_files(self):
        """
        Loads parameter and detector grouping map file
        self._instrument must be already set before calling this
        """
        self._instrument_name = self._instrument.getName()
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        idf_directory = config['instrumentDefinition.directory']
        ipf_name = self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'
        self._parameter_file = os.path.join(idf_directory, ipf_name)

        self.log().information('Set parameter file : {0}'.format(self._parameter_file))

        if self._map_file == '':
            # path name for default map file
            if self._instrument.hasParameter('Workflow.GroupingFile'):
                grouping_filename = self._instrument.getStringParameter('Workflow.GroupingFile')[0]
                self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
            else:
                raise ValueError("Failed to find default detector grouping file. Please specify manually.")

        self.log().information('Set detector map file : {0}'.format(self._map_file))

        # load background run if needed
        if self._background_file:
            self.load_background_run()
            self.log().information('Loaded background run: {0}'.format(self._background_file))

        # load vanadium run if needed
        if self._unmirror_option == 5 or self._unmirror_option == 7:
            self._load_vanadium_run()
            self.log().information('Loaded vanadium run: {0}'.format(self._vanadium_file))

    def _load_vanadium_run(self):
        """
        Loads vanadium run into workspace and extracts left and right wings to use in shift spectra
        Used only in unmirror =5,7 for alignment. This file is the same for all the files to be reduced.
        Note the recursion!
        """
        self.log().notice('Loading vanadium run #{0}'.format(self._vanadium_file))
        # call IndirectILLReduction for vanadium run with unmirror 2 and 3 to get left and right

        left_vanadium = IndirectILLReduction(Run=self._vanadium_file, MapFile=self._map_file, Analyser=self._analyser,
                                             Reflection=self._reflection, SumRuns=True, UnmirrorOption=2)

        right_vanadium = IndirectILLReduction(Run=self._vanadium_file, MapFile=self._map_file, Analyser=self._analyser,
                                              Reflection=self._reflection, SumRuns=True, UnmirrorOption=3)

        # if vanadium run is not of QENS type, output will be empty, exit with error
        if not left_vanadium or not right_vanadium:
            self.log().error('Failed to load vanadium run #{0}. Not a QENS type? Aborting.'.format(self._vanadium_file))
        else:
            # note, that run number will be prepended, so need to rename
            RenameWorkspace(left_vanadium.getItem(0).getName(),'left_van')
            RenameWorkspace(right_vanadium.getItem(0).getName(), 'right_van')

    def load_background_run(self):
        """
        Loads background run. This file is the same for all the files to be reduced.
        """
        background = Load(Filename=self._background_file)
        LoadParameterFile(Workspace=background, Filename=self._parameter_file)
        NormaliseToMonitor(InputWorkspace=background, OutputWorkspace=background, MonitorSpectrum=1)
        GroupDetectors(InputWorkspace=background, OutputWorkspace=background, MapFile=self._map_file, Behaviour='Sum')

    def _reduce_run(self, run, ws_names):
        """
        Performs the reduction for a given single run
        All the main reduction workflow logic goes here
        @param run :: string of run number to reduce
        @param ws_names :: a list to keep track of created ws names
        """

        self.log().information('Reducing run #' + run)

        # temporary list of ws names for the given run
        temp_run_ws_list = []

        # prepend run number for each of the output ws
        for item in self._out_suffixes:
            temp_run_ws_list.append(run + '_' + item)

        # subscribe the list to the general list
        ws_names.append(temp_run_ws_list)

        # just shortcuts
        red = temp_run_ws_list[0]
        raw = temp_run_ws_list[1]
        det = temp_run_ws_list[2]
        mon = temp_run_ws_list[3]
        mnorm = temp_run_ws_list[4]
        left = temp_run_ws_list[5]
        right = temp_run_ws_list[6]
        bsub = temp_run_ws_list[7]
        vnorm = temp_run_ws_list[8]

        # Get mirror_sense
        if mtd[red].getRun().hasProperty('Doppler.mirror_sense'):
            # Get mirror_sense from run
            # mirror_sense 14 : two wings
            # mirror_sense 16 : one wing
            self._mirror_sense = mtd[red].getRun().getLogData('Doppler.mirror_sense').value

        self._debug(red, raw)

        LoadParameterFile(Workspace=red, Filename=self._parameter_file)

        ExtractSingleSpectrum(InputWorkspace=red, OutputWorkspace=mon, WorkspaceIndex=0)

        GroupDetectors(InputWorkspace=red, OutputWorkspace=red, MapFile=self._map_file, Behaviour='Sum')

        self._debug(red, det)

        self._normalisation(red, mon)

        self._debug(red, mnorm)

        self._background_subtraction(red)

        self._debug(red, bsub)

        self._vanadium_calibration(red)

        self._debug(red, vnorm)

        if self._mirror_sense == 14:
            # Number of bins
            size = mtd[red].blocksize()

            # Get the left and right wings
            extract_workspace(red, left, 0, int(size / 2))
            extract_workspace(red, right, int(size / 2), size)
            # Get the left and right monitors, needed to identify the masked bins
            extract_workspace(mon, '__left_mon', 0, int(size / 2))
            extract_workspace(mon, '__right_mon', int(size / 2), size)

            # Initialisation monitor range
            xmin_left = 0
            xmin_right = 0
            xmax_left = mtd[left].blocksize()
            xmax_right = mtd[left].blocksize()

            if self._scan_type == 'QENS':
                # Mask bins out of monitor range (zero bins) for left and right wings
                xmin_left, xmax_left = monitor_range('__left_mon')
                xmin_right, xmax_right = monitor_range('__right_mon')

                # left and right must be masked here, such that perform_unmirror will work ok
                mask_reduced_ws(left, xmin_left, xmax_left)
                mask_reduced_ws(right, xmin_right, xmax_right)

                # Convert left and right wing axis conversions, vanadium runs are already converted
                convert_to_energy(left)
                convert_to_energy(right)
                ConvertSpectrumAxis(InputWorkspace=left, OutputWorkspace=left, Target='Theta', EMode='Indirect')
                ConvertSpectrumAxis(InputWorkspace=right, OutputWorkspace=right, Target='Theta', EMode='Indirect')

        # Energy transfer according to mirror_sense and unmirror_option
        start_bin = 0
        end_bin = 0

        if self._mirror_sense == 14 and self._unmirror_option == 0:
            self.log().warning('Input run #{0} has two wings, no energy transfer can be performed'.format(run))
        elif self._mirror_sense == 14 and self._unmirror_option > 0:
            # QENS: Reduced workspace will be in energy transfer since both, left and right workspaces are in energy transfer
            # FWS: Reduced workspace will not be in energy transfer since Vanadium will not be used -- good
            start_bin, end_bin = perform_unmirror(red, left, right, self._unmirror_option)
        elif self._mirror_sense == 16:
            self.log().information(
                'Input run #{0} has one wing, perform energy transfer, no unmirroring will happen'.format(run))
            convert_to_energy(red)
            ConvertSpectrumAxis(InputWorkspace=red, OutputWorkspace=red, Target='Theta', EMode='Indirect')
        else:
            self.log().warning('Input run #{0}: no Doppler.mirror_sense defined, assuming one wing.'.format(run))
            convert_to_energy(red)
            ConvertSpectrumAxis(InputWorkspace=red, OutputWorkspace=red, Target='Theta', EMode='Indirect')

        # Mask corrupted bins according to shifted workspaces or monitor range
        # Initialisation
        xmin = 0
        xmax = mtd[red].blocksize()

        # Mask bins out of final energy or monitor range
        if start_bin != 0 or end_bin != 0:
            xmin = np.maximum(xmin_left, xmin_right)
            xmax = np.minimum(xmax_left, xmax_right)
            # Shifted workspaces
            xmin = np.maximum(xmin, start_bin)
            xmax = np.minimum(xmax, end_bin)
        elif self._mirror_sense == 14 and self._unmirror_option == 0:
            # Reload X-values (now in meV)
            x = mtd[red].readX(0)
            xmin = xmin_left
            xmax = xmax_right + int(x[-1] / 2)
            if xmin_right < size and xmax_left < size:
                # Mask mid bins
                self.log().debug('Mask red ws bins between {0}, {1}'.format(xmax_left, int(size / 2) + xmin_right - 1))
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[xmax_left], XMax=x[int(size / 2) + xmin_right])
        elif self._mirror_sense == 14 and self._unmirror_option > 0:
            xmin = np.maximum(xmin_left, xmin_right)
            xmax = np.minimum(xmax_left, xmax_right)
        elif self._mirror_sense == 16:
            # One wing, no right workspace
            xmin, xmax = monitor_range(mon)

        if self._scan_type == 'QENS':
            mask_reduced_ws(red, xmin, xmax)

        # cleanup by-products if not needed
        if not self._debug_mode:
            DeleteWorkspace(mon)
            if self._mirror_sense == 14:
                DeleteWorkspace(left)
                DeleteWorkspace(right)

    def _normalisation(self, red, mon):
        '''
        Args:
            red: input workspace to be normalised to monitor
            mon: monitor

        Returns:
            normalised to monitor workspace

        '''

        # Get Doppler energy
        if mtd[red].getRun().hasProperty('Doppler.maximum_delta_energy'):
            energy = mtd[red].getRun().getLogData('Doppler.maximum_delta_energy').value
        elif mtd[red].getRun().hasProperty('Doppler.delta_energy'):
            energy = mtd[red].getRun().getLogData('Doppler.delta_energy').value
        else:
            energy = -1

        if self._scan_type == 'QENS':
            # Use default bin by bin normalisation to monitor
            NormaliseToMonitor(InputWorkspace=red, OutputWorkspace=red, MonitorWorkspace=mon)
        elif self._scan_type == 'FWS' and energy == 0.0:
            # Integrate over the elastic peak range
            x = mtd[red].readX(0)
            xmin = x[0]
            xmax = x[-1]
            NormaliseToMonitor(InputWorkspace=red, OutputWorkspace=red, IntegrationRangeMin=xmin,
                                IntegrationRangeMax=xmax, MonitorWorkspace=mon)
        elif self._scan_type == 'FWS' and energy > 0.0:
            # Normalise over the left and right peak of inelastic scans and add the normalised values
            size = mtd[red].blocksize()
            x_start, x_stop = monitor_range(red)
            x_start -= 1
            x = mtd[red].readX(0)
            # Take into account varying blocksize and define integration range for normalisation
            int_interval = int(len(x) / 50)
            xmin = x_start + int_interval
            xmax = x_stop - int_interval
            # Split the reduced workspace in left and right
            extract_workspace(red, '__red_left', 0, int(size / 2))
            extract_workspace(red, '__red_right', int(size / 2), size)
            # Normalise over left and right ranges
            self.log().information('IFWS scan ranges (bin numbers) [{0} {1}] and [{2} {3}]'.format
                                    (x[0], xmin, xmax, x[len(x) - 1]))
            NormaliseToMonitor(InputWorkspace='__red_left', OutputWorkspace='__mon_left', IntegrationRangeMin=x[0],
                                IntegrationRangeMax=xmin, MonitorWorkspace=mon)
            NormaliseToMonitor(InputWorkspace='__red_right', OutputWorkspace='__mon_right',
                                IntegrationRangeMin=xmax, IntegrationRangeMax=x[len(x) - 1], MonitorWorkspace=mon)
            Plus(LHSWorkspace='__mon_left', RHSWorkspace='__mon_right', OutputWorkspace=red)

        if self._scan_type == 'FWS':
            ReplaceSpecialValues(InputWorkspace=red, OutputWorkspace=red, NaNValue=0, NaNError=0)

        return red

    def _background_subtraction(self, red):
        # subtract the background if specified
        if self._background_file:
            Minus(LHSWorkspace=red, RHSWorkspace='background', OutputWorkspace=red)
            # check the integral after subtraction
            __temp = ReplaceSpecialValues(InputWorkspace=red, NaNValue='0')
            __temp = Integration(InputWorkspace=__temp)
            for i in range(__temp.getNumberHistograms()):
                if __temp.dataY(i)[0] < 0:
                    self.log().warning('Integral of spectrum #{0} is negative after background subtraction.'
                                       'Check the background run {1}'.format(i, self._background_file))
            DeleteWorkspace(__temp)

        return red

    def _vanadium_calibration(self, red):
        # Calibrate to vanadium calibration workspace if specified
        # note, this is a one-column calibration workspace
        if self._calib_ws is not None:
            if self._calib_ws.getNumberHistograms() == mtd[red].getNumberHistograms():
                Divide(LHSWorkspace=red, RHSWorkspace=self._calib_ws, OutputWorkspace=red)
            else:
                self.log().error("Calibration workspace has wrong dimensions.")

    def _masking(self, ws):
        pass

    def _debug(self, ws, name):
        """
        in DebugMode, clones ws with a new name
        @param ws   : input workspace name
        @param name : name of the clone workspace
        """
        if self._debug_mode:
            CloneWorkspace(InputWorkspace=ws, OutputWorkspace=name)

    def _finalize(self, out_ws_names):
        """
        This method does the main bookkeeping.
        Cleans up unneeded workspaces, groups and sets output properties.
        @param  out_ws_names: input 2D list of workspace names
        """

        # remove cached left and right of vanadium run
        if self._unmirror_option == 5 or self._unmirror_option == 7:
            DeleteWorkspace('left_van')
            DeleteWorkspace('right_van')

        output = np.array(out_ws_names)
        self.log().debug(str(output))

        if output.size == 0:
            self.log().error('No runs found for input option {0}'.format(self._scan_type))
            raise RuntimeError('No runs found for input option {0}'.format(self._scan_type))

        # group and set the main output
        GroupWorkspaces(InputWorkspaces=output[:, 0], OutputWorkspace=self._out_suffixes[0])
        self.setProperty('OutputWorkspace', self._out_suffixes[0])

        # group optional ws in debug mode
        if self._debug_mode:

            for i in range(1, 5):
                GroupWorkspaces(InputWorkspaces=output[:, i], OutputWorkspace=self._out_suffixes[i])
            self.setProperty('RawWorkspace', self._out_suffixes[1])
            self.setProperty('DetWorkspace', self._out_suffixes[2])
            self.setProperty('MonitorWorkspace', self._out_suffixes[3])
            self.setProperty('MnormWorkspace', self._out_suffixes[4])

            if self._mirror_sense == 14:
                self.setProperty('LeftWorkspace', self._out_suffixes[5])
                GroupWorkspaces(InputWorkspaces=output[:, 5], OutputWorkspace=self._out_suffixes[5])
                self.setProperty('RightWorkspace', self._out_suffixes[6])
                GroupWorkspaces(InputWorkspaces=output[:, 6], OutputWorkspace=self._out_suffixes[6])

            if self._background_file:
                GroupWorkspaces(InputWorkspaces=output[:, 7], OutputWorkspace=self._out_suffixes[7])
                self.setProperty('BsubWorkspace', self._out_suffixes[7])
                GroupWorkspaces(InputWorkspaces=output[:, 7], OutputWorkspace=self._out_suffixes[7])

            if self._calib_ws is not None:
                GroupWorkspaces(InputWorkspaces=output[:, 8], OutputWorkspace=self._out_suffixes[8])
                self.setProperty('VnormWorkspace', self._out_suffixes[8])
                GroupWorkspaces(InputWorkspaces=output[:, 8], OutputWorkspace=self._out_suffixes[8])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
