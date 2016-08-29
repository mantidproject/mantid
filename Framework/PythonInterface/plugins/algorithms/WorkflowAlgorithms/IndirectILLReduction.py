#pylint: disable=no-init,too-many-instance-attributes,invalid-name,too-many-branches
from __future__ import (absolute_import, division, print_function)

import os.path
import numpy as np
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, mtd, logger
from IndirectImport import import_mantidplot

# first the helpers

_ws_or_none = lambda s: mtd[s] if s != '' else None

# plot and save to move to GUI
def save_ws(ws):
    """
    Saves given workspace in default save directory
    @param ws : input workspace or workspace group name
    """
    filename = mtd[ws].getName() + '.nxs'
    workdir = config['defaultsave.directory']
    file_path = os.path.join(workdir, filename)
    SaveNexusProcessed(InputWorkspace=ws, Filename=file_path)

def plot_ws(ws):
    """
    plots the given workspace
    @param ws : input workspace name
    """
    # think about getting the unit and label from ws
    # x_unit = mtd[ws].getAxis(0).getUnit()
    x_label = 'Energy Transfer [mev]'
    mtd_plot = import_mantidplot()
    graph = mtd_plot.newGraph()
    mtd_plot.plotSpectrum(ws, 0, window=graph)
    layer = graph.activeLayer()
    layer.setAxisTitle(mtd_plot.Layer.Bottom, x_label)
    layer.setAxisTitle(mtd_plot.Layer.Left, 'Intensity [a.u.]')
    layer.setTitle('')

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
    imin = np.argmax(np.array(y[0:mid])) - 1
    # Maximum position right
    imax = np.argmax(np.array(y[mid:size])) + 1 + mid + 1
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

# get_peak_positions with shift_spectra to move to separate algorithm
def get_peak_position(ws, i):
    """
    Gives bin of the peak of i-th spectrum in the ws
    @param ws        :: input workspace
    @param i         :: spectrum index of input workspace
    @return          :: bin number of the peak position
    """
    __temp = ExtractSingleSpectrum(InputWorkspace=ws, WorkspaceIndex=i)

    __fit_table = FindEPP(InputWorkspace=__temp)

    # Mid bin number
    mid_bin = int(__temp.blocksize() / 2)

    # Bin number, where Y has its maximum
    y_values = __temp.readY(0)

    # Delete unused single spectrum
    DeleteWorkspace(__temp)

    # Bin range: difference between mid bin and peak bin should be in this range
    tolerance = int(mid_bin / 2)

    # Peak bin (not in energy)
    peak_bin = __fit_table.row(0)["PeakCentre"]

    # Reliable check for peak bin
    fit_status = __fit_table.row(0)["FitStatus"]

    if peak_bin < 0 or peak_bin > len(y_values) or \
            (fit_status != 'success') or (abs(peak_bin - mid_bin) > tolerance):
        # Fit failed (too narrow peak) or outside bin range
        if abs(np.argmax(y_values) - mid_bin) < tolerance:
            # Take bin of maximum peak
            peak_bin = np.argmax(y_values)
        else:
            # Take the center (i.e. do no shift the spectrum)
            peak_bin = mid_bin

    try:
        if 'EPPfit_NormalisedCovarianceMatrix':
            DeleteWorkspace('EPPfit_NormalisedCovarianceMatrix')
        if 'EPPfit_Parameters':
            DeleteWorkspace('EPPfit_Parameters')
    except ValueError:
        logger.debug('No Fit table available for deletion')

    DeleteWorkspace(__fit_table)

    return peak_bin

def shift_spectra(ws1, ws2=None, shift_option=False, masking=False):
    """
    If only ws1 is given, each single spectrum will be centered around 0 meV
    If in addition ws2 is given and shift_option is False, ws1 will be shifted to match the peak positions of ws2
    If in addition ws2 is given and shift_option is True, ws1 will be shifted by the
    number of bins that is required for ws2 to be centered
    @param ws1                         ::   input workspace that will be shifted
    @param ws2                         ::   optional workspace according to which ws1 will be shifted
    @param shift_option                ::   option to shift ws1 by number of bins (ws2 to center)
    @return                            ::   bins before and after masking are proposed to take place
    """
    number_spectra = mtd[ws1].getNumberHistograms()
    size = mtd[ws1].blocksize()

    if ws2 is not None and \
            (size != mtd[ws2].blocksize() or number_spectra != mtd[ws2].getNumberHistograms()):
        logger.warning('Input workspaces should have the same number of bins and spectra')

    mid_bin = int(size / 2)

    # Initial values for bin range of output workspace. Bins outside this range will be masked
    start_bin = 0
    end_bin = size

    # Shift each single spectrum of the input workspace ws1
    for i in range(number_spectra):

        # Find peak positions in ws1
        logger.debug('Get peak position of spectrum %d' % i)
        peak_bin1 = get_peak_position(ws1, i)

        # If only one workspace is given as an input, this workspace will be shifted
        if ws2 is None:
            to_shift = peak_bin1 - mid_bin
        else:
            # Find peak positions in ws2
            peak_bin2 = get_peak_position(ws2, i)

            if not shift_option:
                # ws1 will be shifted according to peak position of ws2
                to_shift = peak_bin1 - peak_bin2
            else:
                # ws1 will be shifted according to centered peak of ws2
                to_shift = peak_bin2 - mid_bin

        # Shift Y and E values of spectrum i by a number of to_shift bins
        # Note the - sign, since np.roll shifts right if the argument is positive
        # while here if to_shift is positive, it means we need to shift to the left
        mtd[ws1].setY(i, np.roll(mtd[ws1].dataY(i), int(-to_shift)))
        mtd[ws1].setE(i, np.roll(mtd[ws1].dataE(i), int(-to_shift)))

        if (size - to_shift) < end_bin:
            end_bin = size - to_shift
            logger.debug('New right boundary for masking due to left shift by %d bins' % to_shift)
        elif abs(to_shift) > start_bin:
            start_bin = abs(to_shift)
            logger.debug('New left boundary for masking due to right shift by %d bins' % abs(to_shift))
        else:
            logger.debug('Shifting does not result in a new range for masking')

    # Mask bins to the left of the final bin range
    if masking is True:
        # Mask corrupted bins according to shifted workspaces
        logger.debug('Mask bin numbers smaller than %d and larger than %d' % (start_bin, end_bin - 1))
        logger.notice('Bins out of energy range [%f %f] meV will be masked' % (x[start_bin], x[end_bin - 1]))
        # Mask bins to the left and right of the final bin range
        MaskBins(InputWorkspace=ws1, OutputWorkspace=ws1, XMin=x[0], XMax=x[start_bin])
        MaskBins(InputWorkspace=ws1, OutputWorkspace=ws1, XMin=x[end_bin], XMax=x[end])

    return start_bin, end_bin

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
    logger.information('Energy range : %f to %f' % (xnew[0], xnew[-1]))

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
    energy = 0
    scale = 1.e-3  # from micro ev to milli ev

    if gRun.hasProperty('Doppler.maximum_delta_energy'):
        energy = gRun.getLogData('Doppler.maximum_delta_energy').value  # max energy in micro eV
        logger.information('Doppler max delta energy in micro eV : %s' % energy)
    elif gRun.hasProperty('Doppler.delta_energy'):
        energy = gRun.getLogData('Doppler.delta_energy').value  # delta energy in micro eV
        logger.information('Doppler delta energy in micro eV : %s' % energy)
    else:
        logger.warning('Input run has no property Doppler.mirror_sense. Check your input file.')
        logger.warning('Doppler maximum delta energy is 0 micro eV')

    formula = '(x-%f)*%f' % (mid, 2.0 * energy / (size - 1) * scale)

    logger.information('Energy transform formula: ' + formula)

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
        logger.information('Unmirror 1: Return the left wing')
        CloneWorkspace(InputWorkspace=left, OutputWorkspace=red)

    elif option == 2:
        logger.information('Unmirror 2: Return the right wing')
        CloneWorkspace(InputWorkspace=right, OutputWorkspace=red)

    elif option == 3:
        logger.information('Unmirror 3: Sum the left and right wings')

    elif option == 4:
        logger.information('Unmirror 4: Shift the right according to left')
        start_bin, end_bin = shift_spectra(right, left)

    elif option == 5:
        logger.information('Unmirror 5: Shift the right according to right of the vanadium and sum to left')
        start_bin, end_bin = shift_spectra(right, 'right_van', True)

    elif option == 6:
        logger.information('Unmirror 6: Center both the right and the left')
        start_bin_left, endbin_left = shift_spectra(left)
        start_bin_right, endbin_right = shift_spectra(right)
        start_bin = np.maximum(start_bin_left, start_bin_right)
        end_bin = np.minimum(endbin_left, endbin_right)

    elif option == 7:
        logger.information('Unmirror 7: Shift both the right and the left according to vanadium and sum')
        start_bin_left, endbin_left = shift_spectra(left, 'left_van', True)
        start_bin_right, endbin_right = shift_spectra(right, 'right_van', True)
        start_bin = np.maximum(start_bin_left, start_bin_right)
        end_bin = np.minimum(endbin_left, endbin_right)

    if option > 2:
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
    _sum_runs = None
    _save = None
    _plot = None

    # Integer
    _unmirror_option = None

    # Other
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None

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
                             doc='File path of vanadium run. Used for UnmirrorOption=[5,7]')

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

        self.declareProperty(name='SumRuns',
                             defaultValue=False,
                             doc='Whether to sum all the input runs.')

        self.declareProperty(name='UnmirrorOption',defaultValue=6,
                             validator=IntBoundedValidator(lower=0, upper=7),
                             doc='Unmirroring options: \n'
                                 '0 no unmirroring\n'
                                 '1 left\n'
                                 '2 right\n'
                                 '3 sum of left and right\n'
                                 '4 shift right according to left and sum\n'
                                 '5 like 4, but use Vanadium run for peak positions\n'
                                 '6 center both left and right at zero and sum\n'
                                 '7 like 6, but use Vanadium run for peak positions')

        # Output options
        self.declareProperty(name='Save',defaultValue=False,
                             doc='Whether to save the reduced workpsace to nxs file.')

        self.declareProperty(name='Plot',defaultValue=False,
                             doc='Whether to plot the reduced workspace.')

        self.declareProperty(name='DebugMode',
                             defaultValue=False,
                             doc='Whether to output the workspaces in intermediate steps.')

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

        # this is run before setUp, so need to get properties also here!
        issues = dict()
        # Unmirror options 5 and 7 require a Vanadium run as input workspace
        if (self.getProperty('UnmirrorOption').value == 5 or self.getProperty('UnmirrorOption').value == 7) \
                and self.getPropertyValue('VanadiumRun') == "":
            issues['VanadiumRun'] = 'Given unmirror option requires vanadium run to be set'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')
        self._vanadium_file = self.getPropertyValue('VanadiumRun')
        self._background_file = self.getPropertyValue('BackgroundRun')
        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._calib_ws = _ws_or_none(self.getPropertyValue('CalibrationWorkspace'))
        self._reflection = self.getPropertyValue('Reflection')
        self._debug_mode = self.getProperty('DebugMode').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value
        self._sum_runs = self.getProperty('SumRuns').value
        self._unmirror_option = self.getProperty('UnmirrorOption').value

        self._red_ws = self.getPropertyValue('OutputWorkspace')
        self._raw_ws = self._red_ws + '_' + self.getPropertyValue('RawWorkspace')
        self._monitor_ws = self._red_ws + '_' + self.getPropertyValue('MonitorWorkspace')
        self._det_ws = self._red_ws + '_' + self.getPropertyValue('DetWorkspace')
        self._mnorm_ws = self._red_ws + '_' + self.getPropertyValue('MnormWorkspace')
        self._bsub_ws = self._red_ws + '_' + self.getPropertyValue('BsubWorkspace')
        if self._calib_ws is not None:
            self._vnorm_ws = self._red_ws + '_' + self.getPropertyValue('VnormWorkspace')
        self._left_ws = self._red_ws + '_' + self.getPropertyValue('LeftWorkspace')
        self._right_ws = self._red_ws + '_' + self.getPropertyValue('RightWorkspace')

        if self._sum_runs is True:
            self.log().notice('All the runs will be summed')
            self._run_file = self._run_file.replace(',', '+')

    def PyExec(self):

        self.setUp()
        # this must be Load, to be able to treat multiple files
        Load(Filename=self._run_file, OutputWorkspace=self._red_ws)

        self.log().information('Loaded .nxs file(s) : %s' % self._run_file)
        runlist = []
        nonQENSrunlist = []

        # check if it is a workspace or workspace group and perform reduction correspondingly
        if isinstance(mtd[self._red_ws],WorkspaceGroup):

            # get instrument from the first ws in a group and load config files
            self._instrument = mtd[self._red_ws].getItem(0).getInstrument()

            self._load_auxiliary_files()

            # figure out number of progress reports, i.e. one for each input workspace/file
            progress = Progress(self, start=0.0, end=1.0, nreports = mtd[self._red_ws].size())

            # traverse over items in workspace group and reduce individually
            for i in range(0, mtd[self._red_ws].size()):

                run = '{0:06d}'.format(mtd[self._red_ws].getItem(i).getRunNumber())
                ws = run + '_' + self._red_ws
                # prepend run number
                RenameWorkspace(InputWorkspace = run, OutputWorkspace = ws)

                progress.report("Reducing run #" + run)
                # check if the run is QENS type and call reduction for each run
                if check_QENS(ws):
                    runlist.append(run)
                    self._reduce_run(run)
                else:
                    nonQENSrunlist.append(run)
        else:
            # get instrument name and load config files
            self._instrument = mtd[self._red_ws].getInstrument()

            self._load_auxiliary_files()

            run = '{0:06d}'.format(mtd[self._red_ws].getRunNumber())
            ws = run + '_' + self._red_ws
            # prepend run number
            RenameWorkspace(InputWorkspace = self._red_ws, OutputWorkspace = ws)

            # check if the run is QENS type and call reduction
            if check_QENS(ws):
                runlist.append(run)
                self._reduce_run(run)
            else:
                nonQENSrunlist.append(run)

        # remove any loaded non-QENS type data if was given:
        for nonQENS in nonQENSrunlist:
            DeleteWorkspace(nonQENS + '_' + self._red_ws)

        if not runlist:
            self.log().error('None of the given input files where of QENS type.')
        else:
            self._finalize(runlist)

    def _load_auxiliary_files(self):
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

        self.log().information('Set parameter file : %s' % self._parameter_file)

        if self._map_file == '':
            # path name for default map file
            if self._instrument.hasParameter('Workflow.GroupingFile'):
                grouping_filename = self._instrument.getStringParameter('Workflow.GroupingFile')[0]
                self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
            else:
                raise ValueError("Failed to find default detector grouping file. Please specify manually.")

        self.log().information('Set detector map file : %s' % self._map_file)

        # load background run if needed
        if self._background_file:
            self._load_background_run()
            self.log().information('Loaded background run: %s' % self._background_file)

        # load vanadium run if needed
        if self._unmirror_option == 5 or self._unmirror_option == 7:
            self._load_vanadium_run()
            self.log().information('Loaded vanadium run: %s' % self._vanadium_file)

    def _load_vanadium_run(self):
        """
        Loads vanadium run into workspace and extracts left and right wings to use in shift spectra
        Used only in unmirror =5,7 for alignment. This file is the same for all the files to be reduced.
        Note the recursion!
        """
        # call IndirectILLReduction for vanadium run with unmirror 1 and 2 to get left and right

        left_van = IndirectILLReduction(Run=self._vanadium_file, MapFile=self._map_file, Analyser=self._analyser,
                                        Reflection=self._reflection, SumRuns=True, UnmirrorOption=1)

        right_van = IndirectILLReduction(Run=self._vanadium_file, MapFile=self._map_file, Analyser=self._analyser,
                                         Reflection=self._reflection, SumRuns=True, UnmirrorOption=2)

        # if vanadium run is not of QENS type, output will be empty, exit with error
        if not left_van or not right_van:
            self.log().error('Failed to load vanadium run #%s. Not a QENS type? Aborting.' % self._vanadium_file)
        else:
            # note, that run number will be prepended, so need to rename
            RenameWorkspace(left_van.getItem(0).getName(),'left_van')
            RenameWorkspace(right_van.getItem(0).getName(), 'right_van')

    def _load_background_run(self):
        """
        Loads background run. This file is the same for all the files to be reduced.
        """
        background = Load(Filename=self._background_file)
        LoadParameterFile(Workspace=background, Filename=self._parameter_file)
        NormaliseToMonitor(InputWorkspace=background, OutputWorkspace=background, MonitorSpectrum=1)
        GroupDetectors(InputWorkspace=background, OutputWorkspace=background, MapFile=self._map_file, Behaviour='Sum')

    def _reduce_run(self, run):
        """
        Performs the reduction for a given single run
        All the main reduction workflow logic goes here
        @param run :: string of run number to reduce
        """
        self.log().information('Reducing run #' + run)

        red = run + '_' + self._red_ws
        raw = run + '_' + self._raw_ws
        det = run + '_' + self._det_ws
        mon = run + '_' + self._monitor_ws
        mnorm = run + '_' + self._mnorm_ws
        left = run + '_' + self._left_ws
        right = run + '_' + self._right_ws

        self._debug(red, raw)

        # Main reduction workflow
        LoadParameterFile(Workspace=red, Filename=self._parameter_file)

        ExtractSingleSpectrum(InputWorkspace=red, OutputWorkspace=mon, WorkspaceIndex=0)

        GroupDetectors(InputWorkspace=red, OutputWorkspace=red, MapFile=self._map_file, Behaviour='Sum')

        self._debug(red, det)

        NormaliseToMonitor(InputWorkspace=red, OutputWorkspace=red, MonitorWorkspace=mon)

        self._debug(red, mnorm)

        # subtract the background if specified
        if self._background_file:
            Minus(LHSWorkspace=red, RHSWorkspace='background', OutputWorkspace=red)
            bsub = run + '_' + self._bsub_ws
            self._debug(red, bsub)

        # Calibrate to vanadium calibration workspace if specified
        # note, this is a one-column calibration workspace
        if self._calib_ws is not None:
            Divide(LHSWorkspace=red, RHSWorkspace=self._calib_ws, OutputWorkspace=red)
            vnorm = run + '_' + self._vnorm_ws
            self._debug(red, vnorm)

        # Number of bins
        size = mtd[red].blocksize()

        # Get the left and right wings
        extract_workspace(red, left, 0, int(size / 2))
        extract_workspace(red, right, int(size / 2), size)
        # Get the left and right monitors, needed to identify the masked bins
        extract_workspace(mon, '__left_mon', 0, int(size / 2))
        extract_workspace(mon, '__right_mon', int(size / 2), size)

        # Mask bins out of monitor range (zero bins) for left and right wings
        xmin_left, xmax_left = monitor_range('__left_mon')
        xmin_right, xmax_right = monitor_range('__right_mon')
        # Delete the left and right monitors
        DeleteWorkspace('__left_mon')
        DeleteWorkspace('__right_mon')
        # Determine global bins for masking
        xmin = np.maximum(xmin_left, xmin_right)
        xmax = np.minimum(xmax_left, xmax_right)

        # Masking bins according to their bin numbers
        if xmin > 0:
            self.log().debug('Mask bins (left ws and right ws) smaller than %d' % xmin)
            MaskBins(InputWorkspace=left, OutputWorkspace=left, XMin=0, XMax=xmin)
            MaskBins(InputWorkspace=right, OutputWorkspace=right, XMin=0, XMax=xmin)
        if xmax < size:
            self.log().debug('Mask bins (left ws and right ws) larger than %d' % (xmax - 1))
            MaskBins(InputWorkspace=left, OutputWorkspace=left, XMin=xmax, XMax=size)
            MaskBins(InputWorkspace=right, OutputWorkspace=right, XMin=xmax, XMax=size)

        # Get new reduced workspaces
        start_bin, end_bin = perform_unmirror(red, left, right, self._unmirror_option)

        convert_to_energy(left)
        convert_to_energy(right)

        # Energy transfer
        mirror_sense = 0
        if self._unmirror_option == 0:
            # Get mirror_sense from run
            if mtd[red].getRun().hasProperty('Doppler.mirror_sense'):
                # mirror_sense 14 : two wings
                # mirror_sense 16 : one wing
                mirror_sense = mtd[red].getRun().getLogData('Doppler.mirror_sense').value
                if mirror_sense == 14:
                    self.log().warning('Input run #%s has two wings, no energy transfer can be performed' % run)
                elif mirror_sense == 16:
                    self.log().information('Input run #%s has one wing, perform energy transfer' % run)
                    convert_to_energy(red)
            else:
                self.log().warning('Input run #%s has no property Doppler.mirror_sense. ' % run)
        else:
            convert_to_energy(red)

        ConvertSpectrumAxis(InputWorkspace=left, OutputWorkspace=left, Target='Theta', EMode='Indirect')
        ConvertSpectrumAxis(InputWorkspace=right, OutputWorkspace=right, Target='Theta', EMode='Indirect')
        ConvertSpectrumAxis(InputWorkspace=red, OutputWorkspace=red, Target='Theta', EMode='Indirect')

        # Mask corrupted bins according to shifted workspaces
        # Reload X-values (now in meV)
        x = mtd[red].readX(0)

        # Mask bins of reduced workspace
        if self._unmirror_option == 0 and mirror_sense == 14:
            if xmin_left > 0:
                self.log().debug('Mask red ws bins smaller than %d' % xmin_left)
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[0], XMax=x[xmin_left])
            if xmin_right < size and xmax_left < size:
                self.log().debug('Mask red ws bins between %d, %d' % (xmax_left, int(size / 2) + xmin_right - 1))
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[xmax_left], XMax=x[int(size / 2) + xmin_right])
            if xmax_right < size:
                self.log().debug('Mask red ws bins larger than %d' % (xmax_right + int(size / 2) - 1))
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[xmax_right + int(size / 2)], XMax=x[-1])
        else:
            # Mask bins out of final energy range
            if start_bin > 0:
                self.log().debug('Mask bins smaller than %d and larger than %d' % (start_bin, end_bin - 1))
                self.log().notice(
                    'Bins out of energy range [%f %f] meV will be masked' % (x[start_bin], x[end_bin - 1]))
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[0], XMax=x[start_bin])
            if end_bin < len(x) - 1:
                self.log().debug('Mask bins smaller than %d and larger than %d' % (start_bin, end_bin - 1))
                self.log().notice(
                    'Bins out of energy range [%f %f] meV will be masked' % (x[start_bin], x[end_bin - 1]))
                MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[end_bin], XMax=x[-1])

        # cleanup by-products if not needed
        if not self._debug_mode:
            DeleteWorkspace(mon)
            DeleteWorkspace(left)
            DeleteWorkspace(right)

    def _debug(self, ws, name):
        """
        in DebugMode, clones ws with a new name
        @param ws   : input workspace name
        @param name : name of the clone workspace
        """
        if self._debug_mode:
            CloneWorkspace(InputWorkspace=ws, OutputWorkspace=name)

    def _finalize(self, runlist):

        # remove cached left and right of vanadium run
        if self._unmirror_option == 5 or self._unmirror_option == 7:
            DeleteWorkspace('left_van')
            DeleteWorkspace('right_van')

        if self._background_file:
            DeleteWorkspace('background')

        list_red = []
        # first group and set reduced ws
        for run in runlist:
            list_red.append(run + '_' + self._red_ws)

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=list_red, OutputWorkspace=self._red_ws)

        self.setProperty('OutputWorkspace', self._red_ws)

        # group optional ws in debug mode
        if self._debug_mode:

            list_raw = []
            list_monitor = []
            list_det = []
            list_mnorm = []
            if self._background_file:
                list_bsub = []
            if self._calib_ws is not None:
                list_vnorm = []
            list_right = []
            list_left = []

            for run in runlist:
                list_raw.append(run + '_' + self._raw_ws)
                list_monitor.append(run + '_' + self._monitor_ws)
                list_det.append(run + '_' + self._det_ws)
                list_mnorm.append(run + '_' + self._mnorm_ws)
                if self._background_file:
                    list_bsub.append(run + '_' + self._bsub_ws)
                if self._calib_ws is not None:
                    list_vnorm.append(run + '_' + self._vnorm_ws)
                list_right.append(run + '_' + self._right_ws)
                list_left.append(run + '_' + self._left_ws)

            GroupWorkspaces(InputWorkspaces=list_raw, OutputWorkspace=self._raw_ws)
            GroupWorkspaces(InputWorkspaces=list_monitor, OutputWorkspace=self._monitor_ws)
            GroupWorkspaces(InputWorkspaces=list_det, OutputWorkspace=self._det_ws)
            GroupWorkspaces(InputWorkspaces=list_mnorm, OutputWorkspace=self._mnorm_ws)
            if self._background_file:
                GroupWorkspaces(InputWorkspaces=list_bsub, OutputWorkspace=self._bsub_ws)
            if self._calib_ws is not None:
                GroupWorkspaces(InputWorkspaces=list_vnorm, OutputWorkspace=self._vnorm_ws)
            GroupWorkspaces(InputWorkspaces=list_right, OutputWorkspace=self._right_ws)
            GroupWorkspaces(InputWorkspaces=list_left, OutputWorkspace=self._left_ws)

            self.setProperty('RawWorkspace', self._raw_ws)
            self.setProperty('MonitorWorkspace', self._monitor_ws)
            self.setProperty('DetWorkspace', self._det_ws)
            self.setProperty('MnormWorkspace', self._mnorm_ws)
            if self._background_file:
                self.setProperty('BsubWorkspace', self._bsub_ws)
            if self._calib_ws is not None:
                self.setProperty('VnormWorkspace', self._vnorm_ws)
            self.setProperty('RightWorkspace', self._right_ws)
            self.setProperty('LeftWorkspace', self._left_ws)

        # Save if needed
        if self._save:
            save_ws(self._red_ws)

        if self._plot:
            if len(runlist) > 1:
                self.log().warning('Automatic plotting for multiple files is disabled.')
            else:
                SumSpectra(InputWorkspace=runlist[0] + '_' + self._red_ws, OutputWorkspace='sum_to_plot')
                plot_ws('sum_to_plot')
                # do not delete summed spectra while the plot is open

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
