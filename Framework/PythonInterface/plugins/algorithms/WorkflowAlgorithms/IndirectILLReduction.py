#pylint: disable=no-init,invalid-name,too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)

import os.path
import numpy as np
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, logger, mtd
from IndirectImport import import_mantidplot

class IndirectILLReduction(DataProcessorAlgorithm):
    """
    Authors:
    G.Vardanyan :  vardanyan@ill.fr
    V.Reimund   :  reimund@ill.fr
    This version is created on 01/08/2016 partly based on previous work by S.Howells et. Al.
    This is an algorithm for data reduction from IN16B instrument at ILL.
    It reads raw .nxs files and produces the reduced workspace with series of
    manipulations performed dependent on the many input options.
    This file is part of Mantid.
    For the full documentation, see
    http://docs.mantidproject.org/nightly/algorithms/IndirectILLReduction-v1.html?highlight=indirectillreduction
    """

    # Output Workspaces
    _red_ws = 'red'
    _left_ws = 'left'
    _right_ws = 'right'

    _raw_ws = 'raw'
    _det_ws = 'detgrouped'
    _monitor_ws = 'monitor'
    _mnorm_ws = 'mnorm'
    _vnorm_ws = 'vnorm'

    # Optional input calibration workspace
    _calib_ws = None

    # Files
    _map_file = None
    _run_file = None
    _vanadium_file = None
    _parameter_file = None

    # Bool flags
    _control_mode = None
    _sum_runs = None
    _save = None
    _plot = None
    _mirror_sense = None

    # Integer
    _unmirror_option = None

    # Other
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None
    _formula = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs an energy transfer reduction for ILL indirect geometry data, instrument IN16B.'

    def PyInit(self):
        # File properties
        # This has to be MultipleFileProperty.
        self.declareProperty(MultipleFileProperty('Run',extensions=['nxs']),
                             doc='File path of run (s).')

        self.declareProperty(FileProperty('VanadiumRun','',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']),
                             doc='File path of vanadium run (s).')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Filename of the map file to use. If left blank the default will be used.')

        # Other inputs
        self.declareProperty(MatrixWorkspaceProperty("CalibrationWorkspace", "",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc="Workspace containing calibration intensities.")

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111']),
                             doc='Analyser reflection.')

        self.declareProperty(name='SumRuns',
                             defaultValue=False,
                             doc='Whether to sum all the input runs.')

        self.declareProperty(name='ControlMode',
                             defaultValue=False,
                             doc='Whether to output the workspaces in intermediate steps.')

        self.declareProperty(name='MirrorSense',
                             defaultValue=True,
                             doc='Whether the input data has two wings.')

        self.declareProperty(name='UnmirrorOption',defaultValue=3,
                             validator=IntBoundedValidator(lower=0,upper=7),
                             doc='Unmirroring options: \n'
                                 '0 no unmirroring\n'
                                 '1 left\n '
                                 '2 right\n '
                                 '3 sum of left and right\n'
                                 '4 shift right according to left and sum\n'
                                 '5 center both left and right at zero\n'
                                 '6 like 4, but use Vanadium run for peak positions \n'
                                 '7 like 5, but use Vanadium run for peak positions')

        # Output options
        self.declareProperty(name='Save',defaultValue=False,
                             doc='Whether to save the reduced workpsace to nxs file.')

        self.declareProperty(name='Plot',defaultValue=False,
                             doc='Whether to plot the reduced workspace.')

        # Output workspace properties
        self.declareProperty(MatrixWorkspaceProperty("ReducedWorkspace", "red",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the output reduced workspace created.")

        self.declareProperty(MatrixWorkspaceProperty("ReducedLeftWorkspace", "left",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the output reduced left workspace created.")

        self.declareProperty(MatrixWorkspaceProperty("ReducedRightWorkspace", "right",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the output reduced left workspace created.")

    def validateInputs(self):

        issues = dict()
        # Unmirror options 6 and 7 require a Vanadium run as input workspace
        if self._mirror_sense and self._unmirror_option > 5 and self._vanadium_file!='':
            issues['UnmirrorOption'] = 'Given unmirror option requires vanadium run to be set'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')
        self._vanadium_file = self.getPropertyValue('VanadiumRun')

        self._red_ws = self.getPropertyValue('ReducedWorkspace')
        self._left_ws = self.getPropertyValue('ReducedLeftWorkspace')
        self._right_ws= self.getPropertyValue('ReducedRightWorkspace')

        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._reflection = self.getPropertyValue('Reflection')

        self._mirror_sense = self.getProperty('MirrorSense').value
        self._control_mode = self.getProperty('ControlMode').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value
        self._sum_runs = self.getProperty('SumRuns').value
        self._unmirror_option = self.getProperty('UnmirrorOption').value

        if not self._mirror_sense:
            self.log().warning('MirrorSense is OFF, UnmirrorOption will fall back to 0 (i.e. no unmirroring)')
            self._unmirror_option = 0

        if self._sum_runs:
            self.log().information('All the runs will be summed')
            self._run_file = self._run_file.replace(',', '+')

    def PyExec(self):

        self.setUp()
        self.validateInputs()

        Load(Filename=self._run_file, OutputWorkspace=self._raw_ws)
        # this must be Load, to be able to treat multiple files
        # when multiple files are loaded, _raw_ws will be a group workspace
        # containing workspaces having run numbers as names
        # when single file (or sum) is loaded, the workspace name will be _raw_ws

        self.log().information('Loaded .nxs file(s) : %s' % self._run_file)

        # check if it is a workspace or workspace group and perform reduction correspondingly
        if isinstance(mtd[self._raw_ws],WorkspaceGroup):

            # get instrument from the first ws in a group and load config files
            self._instrument = mtd[self._raw_ws].getItem(0).getInstrument()
            self._load_config_files()

            # figure out number of progress reports, i.e. one for each input workspace/file
            progress = Progress(self, start=0.0, end=1.0, nreports = mtd[self._raw_ws].size())

            # to collect the list of runs
            runlist = []

            # traverse over items in workspace group and reduce individually
            for i in range(0, mtd[self._raw_ws].size()):

                run = str(mtd[self._raw_ws].getItem(i).getRunNumber())
                runlist.append(run)
                ws = run + '_' + self._raw_ws
                RenameWorkspace(InputWorkspace = run, OutputWorkspace = ws)

                # call reduction for each run
                self._reduce_run(run)

                # report progress
                progress.report("Reduced run #"+run)

            # after all the runs are reduced, set output ws
            self._set_output_workspace_properties(runlist)

        else:
            # get instrument name and laod config files
            self._instrument = mtd[self._raw_ws].getInstrument()
            self._load_config_files()

            run = str(mtd[self._raw_ws].getRunNumber())
            ws = run + '_' + self._raw_ws
            RenameWorkspace(InputWorkspace = self._raw_ws, OutputWorkspace = ws)

            # reduce
            self._reduce_run(run)

            # after reduction, set output ws
            self._set_output_workspace_properties([run])

    def _load_config_files(self):
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

    def _reduce_run(self, run):
        """
        Performs the reduction for a given single run
        All the main reduction workflow logic goes here
        @param run :: string of run number to reduce
        """
        self.log().information('Reducing run #' + run)
        # Must use named temporaries here
        raw = run + '_' + self._raw_ws
        det = run + '_' + self._det_ws
        mon = run + '_' + self._monitor_ws
        red = run + '_' + self._red_ws
        mnorm = run + '_' + self._mnorm_ws
        vnorm = run + '_' + self._vnorm_ws
        left = run + '_' + self._left_ws
        right = run + '_' + self._right_ws

        # Main reduction workflow
        LoadParameterFile(Workspace=raw,Filename=self._parameter_file)

        GroupDetectors(InputWorkspace=raw,OutputWorkspace=det,MapFile=self._map_file,Behaviour='Sum')

        ExtractSingleSpectrum(InputWorkspace=raw,OutputWorkspace=mon,WorkspaceIndex=0)

        NormaliseToMonitor(InputWorkspace=det, OutputWorkspace=mnorm, MonitorWorkspace=mon)

        # calibrate to vanadium calibration workspace if specified
        # note, this is a one-column calibration workspace, it is not extracted from VanadiumRun (maybe it should?)
        if self._calib_ws != None:
            Divide(LHSWorkspace=mnorm, RHSWorkspace=self._calib_ws, OutputWorkspace=vnorm)
        else:
            CloneWorkspace(InputWorkspace=mnorm, OutputWorkspace=vnorm)

        # get the start, end, mid points
        x = mtd[vnorm].readX(0)
        start = 0
        end = len(x) - 1
        mid = int(end / 2)

        # get the left and right wings
        self._extract_workspace(vnorm, left, x[start], x[mid])
        self._extract_workspace(vnorm, right, x[mid], x[end])
        # get the left and right monitors, needed to identify the masked bins
        self._extract_workspace(mon, 'left_mon', x[start], x[mid])
        self._extract_workspace(mon, 'right_mon', x[mid], x[end])

        # Get sensible range from left monitor and mask correspondingly
        xmin, xmax = self._monitor_range('left_mon')
        # Mask first bins, where monitor count was 0
        MaskBins(InputWorkspace=left, OutputWorkspace=left, XMin=0, XMax=xmin)
        # Mask last bins, where monitor count was 0
        MaskBins(InputWorkspace=left, OutputWorkspace=left, XMin=xmax, XMax=end)
        # the same for the right
        xmin, xmax = self._monitor_range('right_mon')
        # Mask first bins, where monitor count was 0
        MaskBins(InputWorkspace=right, OutputWorkspace=right, XMin=0, XMax=xmin)
        # Mask last bins, where monitor count was 0
        MaskBins(InputWorkspace=right, OutputWorkspace=right, XMin=xmax, XMax=end)

        # delete the left and right monitors
        DeleteWorkspace('left_mon')
        DeleteWorkspace('right_mon')

        # first convert both to energy
        # it is crucial to do this first, since this sets the axis unit
        # which is needed for shift operations for unmirror > 3
        self._convert_to_energy(left)
        self._convert_to_energy(right)

        # finally perform unmirror
        o = self._unmirror_option
        if o == 0:
            self.log().information('Unmirror 0: X-axis will not be converted to energy transfer.')
            CloneWorkspace(Inputworkspace=mnorm, OutputWorkspace=red)

        elif o == 1:
            self.log().information('Unmirror 1: return the left wing')
            RenameWorkspace(InputWorkspace=left,OutputWorkspace=red)

        elif o == 2:
            self.log().information('Unmirror 2: return the light wing')
            RenameWorkspace(InputWorkspace=right,OutputWorkspace=red)

        elif o == 3:
            self.log().information('Unmirror 3: sum the left and right wings')
            self._perform_mirror(left, right, red)

        elif o == 4:
            self.log().information('Unmirror 4: shift the right according to left and sum to left')
            self._shift_spectra(right, left, 'right_shifted')
            self._perform_mirror(left, 'right_shifted', red)
            DeleteWorkspace('right_shifted')

        elif o == 5:
            # Vanadium file must be loaded, left and right workspaces extracted
            pass

        elif o == 6:
            self.log().information('Unmirror 5: center both the right and the left and sum')
            self._shift_spectra(right, right, 'right_shifted', True)
            self._shift_spectra(left, left, 'left_shifted', True)
            self._perform_mirror('left_shifted', 'right_shifted', red)
            DeleteWorkspace('right_shifted')
            DeleteWorkspace('left_shifted')

        elif o == 7:
            # Vanadium file must be loaded, left and right workspaces extracted
            pass

    def _convert_to_energy(self, ws):
        """
        Convert the input ws x-axis from channel # to energy transfer
        @param ws     :: input workspace name
        """
        # get energy formula from cache or compute if it is not yet set
        formula = (self._energy_formula(ws) if self._formula is None else self._formula)
        ConvertAxisByFormula(InputWorkspace=ws,OutputWorkspace=ws,Axis='X',Formula=formula)
        mtd[ws].getAxis(0).setUnit('DeltaE') # in mev
        xnew = mtd[ws].readX(0)  # energy array
        self.log().information('Energy range : %f to %f' % (xnew[0], xnew[-1]))

    def _energy_formula(self, ws):
        """
        Calculate the formula for channel number to energy transfer transformation
        @param ws :: name of the input workspace
        @return   :: formula to transform from time channel to energy transfer
        """
        x = mtd[ws].readX(0)
        npt = len(x)
        imid = float( npt / 2 + 1 )
        gRun = mtd[ws].getRun()
        energy = 0
        scale = 1000. # from mev to micro ev

        if gRun.hasProperty('Doppler.maximum_delta_energy'):
            energy = gRun.getLogData('Doppler.maximum_delta_energy').value / scale  # max energy in meV
            self.log().information('Doppler max energy : %s' % energy)
        elif gRun.hasProperty('Doppler.delta_energy'):
            energy = gRun.getLogData('Doppler.delta_energy').value / scale # delta energy in meV
            self.log().information('Doppler delta energy : %s' % energy)

        dele = 2.0 * energy / (npt - 1)
        formula = '(x-%f)*%f' % (imid, dele)
        self.log().information('Energy transform formula: '+formula)
        # set in the cache and return
        self._formula = formula
        return formula

    def _monitor_range(self, ws):
        """
        Get sensible x-range where monitor has meaningful content
        Used to mask out the first and last few channels, where monitor count is 0
        @param ws :: name of workspace
        @return   :: tuple of xmin and xmax
        """
        x = mtd[ws].readX(0)
        y = mtd[ws].readY(0)
        # mid x value in order to search for left and right monitor range delimiter
        mid = int(len(x) / 2)
        imin = np.argmax(np.array(y[0 : mid])) - 1
        nch = len(y)
        im = np.argmax(np.array(y[nch - mid : nch]))
        imax = nch - mid + 1 + im + 1
        self.log().information('Masking range %f to %f' % (x[imin], x[imax]))
        return x[imin], x[imax]

    def _extract_workspace(self, ws, ws_out, x_start, x_end):
        """
        Extracts part of the workspace
        @param  ws      :: input workspace name
        @param  ws_out  :: output workspace name
        @param  x_start :: start bin of workspace to be extracted
        @param  x_end   :: end bin of workspace to be extracted
        """
        CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws_out, XMin=x_start, XMax=x_end)
        ScaleX(InputWorkspace=ws_out, OutputWorkspace=ws_out, Factor=-x_start, Operation='Add')

    def _perform_mirror(self, ws1, ws2, ws_out):
        """
        Sum and average two ws
        @param ws1          ::  left workspace
        @param ws2          ::  right workspace
        @param ws_out       ::  output ws name
        """
        Plus(LHSWorkspace=ws1,RHSWorkspace=ws2,OutputWorkspace=ws_out)
        Scale(InputWorkspace=ws_out,OutputWorkspace=ws_out,Factor=0.5, Operation='Multiply')

    def _shift_spectra(self, ws1, ws2, ws_out, center=False):
        """
        Shifts workspace corresponding to peak positions in second workspace or just centers it at 0
        @param ws1              ::   workspace to be shifted
        @param ws2              ::   workspace according to which to shift
        @param center           ::   whether or not to center at zero, if True, ws2 is not needed
        @param ws_out           ::   shifted output workspace
        """
        number_spectra = mtd[ws1].getNumberHistograms()
        size = mtd[ws1].blocksize()
        mid_bin = int(size / 2)
        fit_table1 = FindEPP(InputWorkspace=ws1)

        if not center:
            fit_table2 = FindEPP(InputWorkspace=ws2)

        # shift each single spectrum in a workspace
        for i in range(number_spectra):

            temp1 = ExtractSingleSpectrum(InputWorkspace=ws1,WorkspaceIndex=i)

            x_values = np.array(temp1.readX(0))
            y_values = np.array(temp1.readY(0))
            e_values = np.array(temp1.readE(0))

            peak_position1 = fit_table1.row(i)["PeakCentre"]
            fit_status1 = fit_table1.row(i)["FitStatus"]

            if fit_status1 == 'success':
                peak_bin1 = temp1.binIndexOf(peak_position1)
            else:
                y_imax1 = np.argmax(y_values)
                if abs(y_imax1 - mid_bin) < mid_bin / 4:
                    peak_bin1 = y_imax1
                else:
                    self.log().warning('Maybe no peak present, taking mid position instead')
                    peak_bin1 = mid_bin

            DeleteWorkspace(temp1)

            if center:
                to_shift = peak_bin1 - mid_bin
            else:
                temp2 = ExtractSingleSpectrum(InputWorkspace=ws2, WorkspaceIndex=i)
                y_values2 = np.array(temp2.readY(0))
                peak_position2 = fit_table2.row(i)["PeakCentre"]
                fit_status2 = fit_table2.row(i)["FitStatus"]

                if fit_status2 == 'success':
                    peak_bin2 = temp2.binIndexOf(peak_position2)
                else:
                    y_imax2 = np.argmax(y_values2)
                    if abs(y_imax2 - mid_bin) < mid_bin / 4:
                        peak_bin2 = y_imax2
                    else:
                        self.log().warning('Maybe no peak present, taking mid position instead')
                        peak_bin2 = mid_bin

                to_shift = peak_bin1 - peak_bin2

                DeleteWorkspace(temp2)

            if to_shift > 0:
                # shift to the left
                for k in range(size):
                    if k < size - to_shift:
                        y_values[k] = y_values[k + to_shift]
                        e_values[k] = e_values[k + to_shift]
                    else:
                        y_values[k] = 0
                        e_values[k] = 0
                mask_min = size - to_shift
                mask_max = size
            elif to_shift < 0:
                # shift to the right
                for k in range(size-1,0,-1):
                    if k > -to_shift:
                        y_values[k] = y_values[k + to_shift]
                        e_values[k] = e_values[k + to_shift]
                    else:
                        y_values[k] = 0
                        e_values[k] = 0
                mask_min = 0
                mask_max = -to_shift

            if i==0:
                CreateWorkspace(OutputWorkspace=ws_out, DataX=x_values, DataY=y_values, DataE=e_values, NSpec=1, UnitX='DeltaE')
                # there is problem with masking...
                #if to_shift != 0:
                    #MaskBins(InputWorkspace=ws_out,OutputWorkspace=ws_out,XMin=mask_min,XMax=mask_max)
            else:
                CreateWorkspace(OutputWorkspace='temp',DataX=x_values, DataY=y_values, DataE=e_values, NSpec=1, UnitX='DeltaE')
                # there is problem with masking...
                #if to_shift != 0:
                #    MaskBins(InputWorkspace='temp', OutputWorkspace='temp',XMin=mask_min, XMax=mask_max)
                AppendSpectra(InputWorkspace1=ws_out,InputWorkspace2='temp',OutputWorkspace=ws_out)
                DeleteWorkspace('temp')

        DeleteWorkspace(fit_table1)
        if not center:
            DeleteWorkspace(fit_table2)

    def _set_output_workspace_properties(self, runlist):

        if len(runlist) > 1:
            # multiple runs
            list_red = []
            list_right = []
            list_left = []

            # first group and set reduced ws
            for run in runlist:
                list_red.append(run + '_' + self._red_ws)
                list_right.append(run + '_' + self._right_ws)
                list_left.append(run + '_' + self._left_ws)

            GroupWorkspaces(list_red, OutputWorkspace=self._red_ws)
            GroupWorkspaces(list_left, OutputWorkspace=self._red_ws + '_' + self._left_ws)
            GroupWorkspaces(list_right, OutputWorkspace=self._red_ws + '_' + self._right_ws)

            self.setPropertyValue('ReducedWorkspace', self._red_ws)
            self.setPropertyValue('ReducedLeftWorkspace', self._red_ws + '_' + self._left_ws)
            self.setPropertyValue('ReducedRightWorkspace', self._red_ws + '_' + self._right_ws)

            if not self._control_mode:
                # delete everything else
                for run in runlist:
                    DeleteWorkspace(run + '_' + self._raw_ws)
                    DeleteWorkspace(run + '_' + self._mnorm_ws)
                    DeleteWorkspace(run + '_' + self._det_ws)
                    DeleteWorkspace(run + '_' + self._monitor_ws)
                    DeleteWorkspace(run + '_' + self._vnorm_ws)
            else:
                # group optional workspace properties
                list_raw = []
                list_monitor = []
                list_det = []
                list_mnorm = []
                list_vnorm = []
                for run in runlist:
                    list_raw.append(run + '_' + self._raw_ws)
                    list_monitor.append(run + '_' + self._monitor_ws)
                    list_det.append(run + '_' + self._det_ws)
                    list_mnorm.append(run + '_' + self._mnorm_ws)
                    list_vnorm.append(run + '_' + self._vnorm_ws)

                GroupWorkspaces(list_raw, OutputWorkspace=self._red_ws + '_' + self._raw_ws)
                GroupWorkspaces(list_monitor, OutputWorkspace=self._red_ws + '_' + self._monitor_ws)
                GroupWorkspaces(list_det, OutputWorkspace=self._red_ws + '_' + self._det_ws)
                GroupWorkspaces(list_mnorm, OutputWorkspace=self._red_ws + '_' +  self._mnorm_ws)
                GroupWorkspaces(list_vnorm, OutputWorkspace=self._red_ws + '_' +  self._vnorm_ws)

            # save if needed, before deleting
            if self._save:
                self._save_ws(self._red_ws)

            if self._plot:
                self.log().warning('Automatic plotting for multiple files is disabled.')

        else:
            # single run

            # named temporaries
            red = runlist[0] + '_' + self._red_ws
            left = runlist[0] + '_' + self._left_ws
            right = runlist[0] + '_' + self._right_ws

            raw = runlist[0] + '_' + self._raw_ws
            mnorm = runlist[0] + '_' + self._mnorm_ws
            vnorm = runlist[0] + '_' + self._vnorm_ws
            det = runlist[0] + '_' + self._det_ws
            mon = runlist[0] + '_' + self._monitor_ws

            self.setPropertyValue('ReducedWorkspace', red)
            self.setPropertyValue('ReducedWorkspace', left)
            self.setPropertyValue('ReducedWorkspace', right)

            if not self._control_mode:
                # Cleanup unused workspaces
                DeleteWorkspace(raw)
                DeleteWorkspace(mnorm)
                DeleteWorkspace(det)
                DeleteWorkspace(mon)
                DeleteWorkspace(vnorm)

            # save and plot
            if self._save:
                self._save_ws(red)

            if self._plot:
                SumSpectra(InputWorkspace=red, OutputWorkspace=red + '_sum_to_plot')
                self._plot_ws(red + '_sum_to_plot')
                # do not delete summed spectra while the plot is open

    def _save_ws(self, ws):
        """
        Saves given workspace in default save directory
        @param ws : input workspace or workspace group name
        """
        filename = mtd[ws].getName() + '.nxs'
        workdir = config['defaultsave.directory']
        file_path = os.path.join(workdir, filename)
        SaveNexusProcessed(InputWorkspace=ws, Filename=file_path)
        self.log().information('Saved file: ' + filename)

    def _plot_ws(self, ws):
        """
        plots the given workspace
        @param ws : input workspace name
        """
        # think about getting the unit and label from ws
        #x_unit = mtd[ws].getAxis(0).getUnit()
        x_label = 'Energy Transfer [mev]'
        mtd_plot = import_mantidplot()
        graph = mtd_plot.newGraph()
        mtd_plot.plotSpectrum(ws, 0, window=graph)
        layer = graph.activeLayer()
        layer.setAxisTitle(mtd_plot.Layer.Bottom, x_label)
        layer.setAxisTitle(mtd_plot.Layer.Left, 'Intensity [a.u.]')
        layer.setTitle('')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
