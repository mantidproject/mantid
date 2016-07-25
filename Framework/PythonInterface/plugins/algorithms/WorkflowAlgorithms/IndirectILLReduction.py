#pylint: disable=no-init,invalid-name,too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.kernel import StringListValidator, IntBoundedValidator, Direction
from mantid.api import DataProcessorAlgorithm, PropertyMode,\
	AlgorithmFactory, FileProperty, FileAction, \
	MatrixWorkspaceProperty, MultipleFileProperty, \
	Progress, WorkspaceGroup
from mantid import config, logger, mtd
#from scipy.constants import codata, Planck # needed eventually for energy computation

import numpy as np
import os.path

"""
ws          : workspace
det_grouped : grouping of detectors and not a WorkspaceGroup (_workspace_group)
mnorm       : monitor normalised
vnorm       : vanadium normalised
red         : reduced

In the following, ws as function parameter refers to the input workspace whereas ws_out refers to the output workspace
"""

class IndirectILLReduction(DataProcessorAlgorithm):

    # Workspaces
    _raw_ws = None
    _det_grouped_ws = None
    _monitor_ws = None
    _mnorm_ws = None
    _vnorm_ws = None
    _red_ws = None
    _red_left_ws = None
    _red_right_ws = None
    _calibration_ws = None
    _vanadium_ws = None

    # Files
    _map_file = None
    _run_file = None
    _vanadium_run = None
    _parameter_file = None

    # Bool flags
    _control_mode = None
    _sum_runs = None
    _save = None
    _plot = None
    _mirror_sense = None

    # Integer
    _unmirror_option = 3

    # Other
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None
    _run_number = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs an energy transfer reduction for ILL indirect inelastic data, instrument IN16B.'

    def PyInit(self):
        # Input options
        # This has to be MultipleFileProperty.
        self.declareProperty(MultipleFileProperty('Run',
                extensions=["nxs"]),
                doc='File path of run (s).')

        self.declareProperty(name='Analyser',
                defaultValue='silicon',
                validator=StringListValidator(['silicon']),
                doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                defaultValue='111',
                validator=StringListValidator(['111']),
                doc='Analyser reflection.')

        self.declareProperty(FileProperty('MapFile',
                '',
                action=FileAction.OptionalLoad,
                extensions=["xml"]),
                doc='Filename of the map file to use. If left blank the default will be used.')

        # Output workspace properties
        self.declareProperty(MatrixWorkspaceProperty("ReducedWorkspace",
                "red",
                direction=Direction.Output),
                doc="Name for the output reduced workspace created. \n Depending on the unmirror option.")

        self.declareProperty(MatrixWorkspaceProperty("CalibrationWorkspace",
                "",
                optional=PropertyMode.Optional,
                direction=Direction.Input),
                doc="Workspace containing calibration intensities.")

        self.declareProperty(name='ControlMode',
                defaultValue=False,
                doc='Whether to output the workspaces in intermediate steps.')

        self.declareProperty(name='SumRuns',
                defaultValue=False,
                doc='Whether to sum all the input runs.')

        self.declareProperty(name='MirrorSense',
                defaultValue=False,
                doc='Whether the input data has two wings.')

        self.declareProperty(name='UnmirrorOption',
                defaultValue=3,
                validator=IntBoundedValidator(lower=0,upper=7),
                doc='Unmirroring options: \n 0 Normalisation of grouped workspace to monitor spectrum and no energy transfer\n 1 left workspace\n 2 right workspace\n 3 sum of left and right workspaces\n 4 shift right workspace according to maximum peak positions of left workspace \n 5 like option 4 and center all spectra \n 6 like 4, but use Vanadium workspace for estimating maximum peak positions \n 7 like 6 and center all spectra')

        self.declareProperty(MatrixWorkspaceProperty("RawWorkspace",
                "raw",
                optional=PropertyMode.Optional,
                direction=Direction.Output),
                doc="Name for the output raw workspace created.")

        # Optional workspaces created when running in debug mode
        self.declareProperty(MatrixWorkspaceProperty("MNormalisedWorkspace",
                "mnorm",
                optional=PropertyMode.Optional,
                direction=Direction.Output),
                doc="Name for the workspace normalised to monitor.")

        self.declareProperty(MatrixWorkspaceProperty("DetGroupedWorkspace",
                "detectors_grouped",
                optional=PropertyMode.Optional,
                direction=Direction.Output),
                doc="Name for the workspace with grouped detectors.")

        self.declareProperty(MatrixWorkspaceProperty("MonitorWorkspace",
                "monitor",
                optional=PropertyMode.Optional,
                direction=Direction.Output),
                doc="Name for the monitor spectrum.")

        self.declareProperty(MatrixWorkspaceProperty("VNormalisedWorkspace",
                "vnorm",
                optional=PropertyMode.Optional,
                direction=Direction.Output),
                doc="Name for the workspace normalised to vanadium.")

        # Output options
        self.declareProperty(name='Save',
                defaultValue=False,
                doc='Whether to save the output workpsaces to nxs file.')
        self.declareProperty(name='Plot',
                defaultValue=False,
                doc='Whether to plot the output workspace.')

        self.validateInputs()

    def validateInputs(self):

        issues = dict()
        # Unmirror options 6 and 7 require a Vanadium run as input workspace
        if self._unmirror_option==6 and self._vanadium_run!='':
            issues['UnmirrorOption'] = 'Requires calibration workspace to be set'
        if self._unmirror_option==7 and self._vanadium_run!='':
            issues['UnmirrorOption'] = 'Requires calibration workspace to be set'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')

        self._raw_ws = self.getPropertyValue('RawWorkspace')
        self._det_grouped_ws = self.getPropertyValue('DetGroupedWorkspace')
        self._red_ws= self.getPropertyValue('ReducedWorkspace')
        self._calibration_ws = self.getPropertyValue('CalibrationWorkspace')
        self._vnorm_ws = self.getPropertyValue('VNormalisedWorkspace')
        self._mnorm_ws = self.getPropertyValue('MNormalisedWorkspace')
        self._monitor_ws = self.getPropertyValue('MonitorWorkspace')

        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._reflection = self.getPropertyValue('Reflection')

        self._mirror_sense = self.getProperty('MirrorSense').value
        self._control_mode = self.getProperty('ControlMode').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value
        self._sum_runs = self.getProperty('SumRuns').value
        self._unmirror_option = self.getProperty('UnmirrorOption').value

    def PyExec(self):

        progress = Progress(self,start=0.0,end=1.0,nreports=3)

        self.setUp()

        if self._sum_runs:
            self._run_file = self._run_file.replace(',','+')

        progress.report('Loading file(s) : %s' % self._run_file)

        # This must be Load, to be able to treat multiple files.
        Load(Filename=self._run_file, OutputWorkspace=self._raw_ws)

        print('Loaded .nxs file(s) : %s' % self._run_file)

        progress.report('Running reduction')

        if isinstance(mtd[self._raw_ws],WorkspaceGroup):
            self._instrument = mtd[self._raw_ws].getItem(0).getInstrument()
            self._run_number = mtd[self._raw_ws].getItem(0).getRunNumber()
            self._load_config_files()
            print('Nxs file : %s' % self._run_file)

            output_workspace_group = self._multifile_reduction()

        else:
            self._instrument = mtd[self._raw_ws].getInstrument()
            self._run_number = mtd[self._raw_ws].getRunNumber()
            self._load_config_files()

            print('Nxs file : %s' % self._run_file)

            output_workspace = self._reduction()

        self._set_workspace_properties()

        progress.report("Done")

    def _load_config_files(self):
        """
        Loads parameter and detector grouping map file
        """

        print('Load instrument definition file and detectors grouping (map) file')
        self._instrument_name = self._instrument.getName()
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._run_file = self._instrument_name + '_' + str(self._run_number)

        idf_directory = config['instrumentDefinition.directory']
        ipf_name = self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'
        self._parameter_file = os.path.join(idf_directory, ipf_name)

        print('Parameter file : %s' % self._parameter_file)

        if self._map_file == '':
            # path name for default map file
            if self._instrument.hasParameter('Workflow.GroupingFile'):
               grouping_filename = self._instrument.getStringParameter('Workflow.GroupingFile')[0]
               self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
            else:
               raise ValueError("Failed to find default detector grouping file. Please specify manually.")

        print('Map file : %s' % self._map_file)
        print('Done')

    def _multifile_reduction(self):
        """
        Calls _reduction for items in GroupWorkspace

        @return    :: the reduced group workspace or a list of group
                      workspaces depending on the _control_mode
        """

        list_red = []
        list_monitor = []
        list_grouped = []
        list_mnorm = []
        list_vnorm = []

        for i in range(0,mtd[self._raw_ws].size()):

            output_workspaces = self._reduction(mtd[self._raw_ws].getItem(i))

            self._set_workspace_properties()

            list_red.append(output_workspaces[0])
            if self._control_mode:
                list_mnorm.append(output_workspaces[1])
                list_grouped.append(output_workspaces[2])
                list_monitor.append(output_workspaces[3])
                if self._unmirror_option > 0:
                    list_vnorm.append(output_workspaces[4])

        output_workspace_group = {list_red, list_mnorm, list_grouped, list_monitor, list_vnorm}

        return output_workspace_group

    def _reduction(self):
        """
        Run indirect reduction for IN16B

        @return         :: the reduced workspace or a list of workspaces
                           depending on the control mode
        """

        print('Reduction')
        print('Input workspace : %s' % self._raw_ws)


        # Raw workspace
        LoadParameterFile(Workspace=self._raw_ws,
                                                Filename=self._parameter_file)

        # Detectors grouped workspace
        GroupDetectors(InputWorkspace=self._raw_ws,
                                OutputWorkspace=self._det_grouped_ws,
                                MapFile=self._map_file,
                                Behaviour='Sum')

        # Monitor workspace
        ExtractSingleSpectrum(InputWorkspace=self._raw_ws,
                                OutputWorkspace=self._monitor_ws,
                                WorkspaceIndex=0)

        self._unmirror()

        print('Add unmirror_option and _sum_runs to Sample Log of reduced workspace (right click on workspace, click on Sample Logs...)')
        AddSampleLog(Workspace=self._red_ws, LogName="unmirror_option",
                                LogType="String", LogText=str(self._unmirror_option))
        AddSampleLog(Workspace=self._red_ws, LogName="_sum_runs",
                                LogType="String", LogText=str(self._sum_runs))
        print('Done')

        print('Return list of output workspaces')
        output_workspaces = []

        # The reduced and raw workspaces are the result workspaces of the IndirectILLReduction workflow algorithm
        output_workspaces.append(self._red_ws)
        """
        Control mode takes outputs temporary workspaces (not all) if set to True or deletes unused workspaces if set to False:
        True:
        Unmirror option 0 (no split of the grouped workspace): _raw_ws, _red_ws, _mnorm_ws, _det_grouped_ws, _monitor_ws
        Unmirror option 1, 2 (left or right workspace): _raw_ws, _red_ws, _mnorm_ws, _det_grouped_ws, _monitor_ws, (_vnorm_ws)
        Unmirror option 3, 4, 5, 6, 7  _raw_ws, _red_ws, _mnorm_ws, _det_grouped_ws, _monitor_ws, (_vnorm_ws)
        False: return always  _red_ws
        """
        if self._control_mode:
            output_workspaces.append(self._raw_ws)
            output_workspaces.append(self._mnorm_ws)
            output_workspaces.append(self._det_grouped_ws)
            output_workspaces.append(self._monitor_ws)
            if self._unmirror_option > 0 and self._calibration_ws!='':
                output_workspaces.append(self._vnorm_ws)

        if self._save:
            self._save_reduced_ws()

        if self._plot:
            self._plot_reduced_ws()

        print('Done')
        return output_workspaces

    def _unmirror(self):
        """
        Runs energy reduction for corresponding unmirror option.

        This function calls self._calculate_energy() for the energy transfer

        """
        print('Unmirror')

        if self._unmirror_option == 0:

            print('Normalisation of grouped workspace to monitor, bins will not be masked, X-axis will not be in energy transfer.')

            NormaliseToMonitor(InputWorkspace=self._det_grouped_ws,
                            MonitorWorkspace=self._monitor_ws,
                            OutputWorkspace=self._mnorm_ws)

            CloneWorkspace(Inputworkspace=self._mnorm_ws,
                                            OutputWorkspace=self._red_ws)

            # Energy transfer requires X-Axis to be in range -Emin Emax -Emin Emax ...

        else:
            # Get X-values of the grouped workspace
            x = mtd[self._det_grouped_ws].readX(0)
            x_end = len(x)
            mid_point = int((x_end - 1) / 2)

            if self._unmirror_option == 1:
                print('Left reduced, monitor, detectors grouped and normalised workspace')
                self._extract_workspace(0,
                                                                x[mid_point],
                                                                self._red_ws,
                                                                self._monitor_ws,
                                                                self._det_grouped_ws,
                                                                self._mnorm_ws)

            if self._unmirror_option == 2:
                print('Right reduced, monitor, detectors grouped and normalised workspace')
                self._extract_workspace(x[mid_point],
                                                                x_end,
                                                                self._red_ws,
                                                                self._monitor_ws,
                                                                self._det_grouped_ws,
                                                                self._mnorm_ws)

            if self._unmirror_option > 2:
                # Temporary workspace names needed for unmirror options 3 to 7
                __left_ws = '__left_ws'
                __left_monitor_ws = '__left_monitor_ws'
                __left_grouped_ws = '__left_grouped_ws'
                __left_mnorm_ws = '__left_mnorm_ws'
                __right_ws = '__right_ws'
                __right_monitor_ws = '__right_monitor_ws'
                __right_grouped_ws = '__right_grouped_ws'
                __right_mnorm_ws = '__right_mnorm_ws'

                __left = '__right'
                __right = '__left'

                # Left workspace
                self._extract_workspace(0,
                                                                x[mid_point],
                                                                __left_ws,
                                                                __left_monitor_ws,
                                                                __left_grouped_ws,
                                                                __left_mnorm_ws)

                # Right workspace
                self._extract_workspace(x[mid_point],
                                                                x_end,
                                                                __right_ws,
                                                                __right_monitor_ws,
                                                                __right_grouped_ws,
                                                                __right_mnorm_ws)

                if self._unmirror_option == 3:
                    print('Sum left and right workspace for unmirror option 3')
                    __left = __left_ws
                    __right = __right_ws
                elif self._unmirror_option == 4:
                    print('Shift each sepctrum of the right workspace according to the maximum peak positions of the corresponding spectrum of the left workspace')
                    self._shift_spectra(__right_ws, __left_ws, __right)
                    __left = __left_ws
                    # Shifted workspace in control mode?
                elif self._unmirror_option == 5:
                    # _shift_spectra needs extension
                    pass
                elif self._unmirror_option == 6:
                    # Vanadium file must be loaded, left and right workspaces extracted
                    # Update PyExec and _set_workspace_properties accordingly, _vanadium_ws = None
                    pass
                elif self._unmirror_option == 7:
                    # Vanadium file must be loaded, left and right workspaces extracted
                    # Update PyExec and _set_workspace_properties accordingly, _vanadium_ws = None
                    pass

                # Sum left, right and corresponding workspaces
                self._perform_mirror(__left, __right,
                                                        __left_monitor_ws,
                                                        __right_monitor_ws,
                                                        __left_grouped_ws,
                                                        __right_grouped_ws,
                                                        __left_mnorm_ws,
                                                        __right_mnorm_ws)

        print('Done')

    def _calculate_energy(self, ws, ws_out):
        """
        Convert the input run to energy transfer
        @param ws       :: energy transfer for workspace ws
                @param ws_out   :: output workspace
        """

        print('Energy calculation')

        # Apply the detector intensity calibration
        if self._calibration_ws != '':
            Divide(LHSWorkspace=ws,
                            RHSWorkspace=self._calibration_ws,
                            OutputWorkspace=self._vnorm_ws)
        else:
            CloneWorkspace(InputWorkspace=ws,
                            OutputWorkspace=self._vnorm_ws)

        formula = self._energy_range(self._vnorm_ws)

        ConvertAxisByFormula(InputWorkspace=self._vnorm_ws,
                                OutputWorkspace=ws_out,
                                Axis='X',
                                Formula=formula)

        # Set unit of the X-Axis
        mtd[ws_out].getAxis(0).setUnit('Label').setLabel('Energy Transfer','micro eV')

        xnew = mtd[ws_out].readX(0)  # energy array
        print('Energy range : %f to %f' % (xnew[0], xnew[-1]))
        print('Energy range : %f to %f' % (xnew[0], xnew[-1]))

        print('Done')

    def _energy_range(self, ws):
        """
        Calculate the energy range for the workspace

        @param ws :: name of the input workspace
        @return   :: formula to transform from time channel to energy transfer
        """
        print('Calculate energy range')

        x = mtd[ws].readX(0)
        npt = len(x)
        imid = float( npt / 2 + 1 )
        gRun = mtd[ws].getRun()
        energy = 0

        if gRun.hasProperty('Doppler.maximum_delta_energy'):
            # Check whether Doppler drive has incident_wavelength
            velocity_profile = gRun.getLogData('Doppler.velocity_profile').value
            if velocity_profile == 0:
                energy = gRun.getLogData('Doppler.maximum_delta_energy').value  # max energy in meV
                print('Doppler max energy : %s' % energy)
            else:
                print('Check operation mode of Doppler drive: velocity profile 0 (sinudoidal) required')
        elif gRun.hasProperty('Doppler.delta_energy'):
            energy = gRun.getLogData('Doppler.delta_energy').value  # delta energy in meV
            print('Warning: Doppler delta energy used : %s' % energy)
        else:
            print('Error: Energy is 0 micro electron Volt')

        dele = 2.0 * energy / (npt - 1)
        formula = '(x-%f)*%f' % (imid, dele)

        print('Done')
        return formula

    def _monitor_range(self, monitor_ws):
        """
        Get sensible values for the min and max cropping range
        @param monitor_ws :: name of the monitor workspace
        @return tuple containing the min and max x values in the range
        """
        print('Determine monitor range')

        x = mtd[monitor_ws].readX(0)  # energy array
        y = mtd[monitor_ws].readY(0)  # energy array

        # mid x value in order to seach for first and right monitor range delimiter
        mid = int(len(x) / 2)

        imin = np.argmax(np.array(y[0 : mid])) - 1
        nch = len(y)
        im = np.argmax(np.array(y[nch - mid : nch]))
        imax = nch - mid + 1 + im + 1

        print('Cropping range %f to %f' % (x[imin], x[imax]))
        print('Cropping range %f to %f' % (x[imin], x[imax]))

        print('Done')
        return x[imin], x[imax]

    def _extract_workspace(self, x_start, x_end, ws_out, monitor, det_grouped, mnorm):
        """
        Extract left or right workspace from detector grouped workspace and perform energy transfer
        @params      :: x_start defines start bin of workspace to be extracted
        @params      :: x_end defines end bin of workspace to be extracted
        @params      :: x_shift determines shift of x-values
        @return      :: ws_out reduced workspace, normalised, bins masked, energy transfer
        @return      :: corresponding monitor spectrum
        @return      :: corresponding detector grouped workspace
        @return      :: corresponding normalised workspace
        """
        print('Extract left or right workspace from detector grouped workspace and perform energy transfer')

        CropWorkspace(InputWorkspace=self._det_grouped_ws,
                                        OutputWorkspace=det_grouped,
                                        XMin=x_start,
                                        XMax=x_end)

        CropWorkspace(InputWorkspace=self._monitor_ws,
                                        OutputWorkspace=monitor,
                                        XMin=x_start,
                                        XMax=x_end)

        # Shift X-values of extracted workspace and its corresponding monitor in order to let X-values start with 0
        __x = mtd[det_grouped].readX(0)
        x_shift =  - __x[0]

        ScaleX(InputWorkspace=det_grouped,
                                        OutputWorkspace=det_grouped,
                                        Factor=x_shift,
                                        Operation='Add')

        ScaleX(InputWorkspace=monitor,
                                        OutputWorkspace=monitor,
                                        Factor=x_shift,
                                        Operation='Add')

        # Normalise left workspace to left monitor spectrum
        NormaliseToMonitor(InputWorkspace=det_grouped,
                                OutputWorkspace=mnorm,
                                MonitorWorkspace=monitor)

        # Division by zero for bins that will be masked since MaskBins does not take care of them
        ReplaceSpecialValues(InputWorkspace=mnorm,
                                                OutputWorkspace=mnorm,
                                                NaNValue=0)

        # Mask bins according to monitor range
        xmin, xmax = self._monitor_range(monitor)

        # MaskBins cannot mask nan values!
        # Mask bins (first bins) outside monitor range (nan's after normalisation)
        MaskBins(InputWorkspace=mnorm,
                                                OutputWorkspace=mnorm,
                                                XMin=0,
                                                XMax=xmin)

        # Mask bins (last bins)  outside monitor range (nan's after normalisation)
        MaskBins(InputWorkspace=mnorm,
                                                OutputWorkspace=mnorm,
                                                XMin=xmax,
                                                XMax=x_end)

        self._calculate_energy(mnorm, ws_out)

        print('Done')

    def _perform_mirror(self, ws1, ws2, monitor1=None, monitor2=None, detgrouped1=None, detgrouped2=None, mnorm1=None, mnorm2=None):
        """
        @params    :: ws1 reduced left workspace
        @params    :: ws2 reduced right workspace
        @params    :: monitor1 optional left monitor workspace
        @params    :: monitor2 optional right monitor workspace
        @params    :: detgrouped1 optional left detectors grouped workspace
        @params    :: detgrouped2 optional right detectors grouped workspace
        @params    :: mnorm1 optional left normalised workspace
        @params    :: mnorm2 optional right normalised workspace
        """

        # Reduced workspaces
        Plus(LHSWorkspace=ws1,
                RHSWorkspace=ws2,
                OutputWorkspace=self._red_ws)
        Scale(InputWorkspace=self._red_ws,
                OutputWorkspace=self._red_ws,
                Factor=0.5, Operation='Multiply')

        if (monitor1 is not None and monitor2 is not None
           and detgrouped1 is not None and detgrouped2 is not None
           and mnorm1 is not None and mnorm2 is not None):
            # Monitors
            Plus(LHSWorkspace=monitor1,
                    RHSWorkspace=monitor2,
                    OutputWorkspace=self._monitor_ws)
            Scale(InputWorkspace=self._monitor_ws,
                    OutputWorkspace=self._monitor_ws,
                    Factor=0.5, Operation='Multiply')

            # detectors grouped workspaces
            Plus(LHSWorkspace=detgrouped1,
                    RHSWorkspace=detgrouped2,
                    OutputWorkspace=self._det_grouped_ws)
            Scale(InputWorkspace=self._det_grouped_ws,
                    OutputWorkspace=self._det_grouped_ws,
                    Factor=0.5, Operation='Multiply')

            # Normalised workspaces
            Plus(LHSWorkspace=mnorm1,
                    RHSWorkspace=mnorm2,
                    OutputWorkspace=self._mnorm_ws)
            Scale(InputWorkspace=self._mnorm_ws,
                    OutputWorkspace=self._mnorm_ws,
                    Factor=0.5, Operation='Multiply')

    def _shift_spectra(self, ws, ws_shift_origin, ws_out):
        """
        @params   :: ws workspace to be shifted
        @params   :: ws_shift_origin that provides the spectra according to which the shift will be performed
        @params   :: ws_out shifted output workspace

        """
        print('Shift spectra')

        # Get a table maximum peak positions via optimization with Gaussian
        # FindEPP needs modification: validator for TOF axis disable and category generalisation
        table_shift = FindEPP(InputWorkspace=ws_shift_origin)

        number_spectra = mtd[ws].getNumberHistograms()

        # Shift each single spectrum
        for i in range(number_spectra):
            print('Process spectrum ' + str(i))

            __temp = ExtractSingleSpectrum(InputWorkspace=ws, WorkspaceIndex=i)
            peak_position = int(table_shift.row(i)["PeakCentre"])

            x_values = np.array(__temp.readX(0)) # will not change
            y_values = np.array(__temp.readY(0)) # will be shifted
            e_values = np.array(__temp.readE(0)) # will be shifted

            # Perform shift only if FindEPP returns success. Possibility to use self._peak_maximum_position() instead
            if peak_position:

                # A proposition was to use ConvertAxisByFormula. I my opinion the code would be much longer
                # This is the implementation of a circular shift
                if peak_position < 0:
                    # Shift to the right
                    # y-values, e-values : insert last values at the beginning
                    for k in range(peak_position):
                        y_values = np.insert(y_values, 0, y_values[-1])
                        y_values = np.delete(y_values, -1)
                        e_values = np.insert(e_values, 0, e_values[-1])
                        e_values = np.delete(e_values, -1)
                else:
                    # Shift to the left
                    # y-values, e-values : insert last values at the beginning
                    for k in range(peak_position):
                        y_values = np.append(y_values, y_values[0])
                        y_values = np.delete(y_values, 0)
                        e_values = np.append(e_values, e_values[0])
                        e_values = np.delete(e_values, 0)

            if not i:
                # Initial spectrum 0
                CreateWorkspace(OutputWorkspace=ws_out, DataX=x_values, DataY=y_values, DataE=e_values, NSpec=1)#UnitX='TOF'
                # Mask shifted bins missing here
            else:
                # Override temporary workspace __temp by ith shifted spectrum and append to output workspace
                __temp_single_spectrum = CreateWorkspace(DataX=x_values, DataY=y_values, DataE=e_values, NSpec=1)
                # Mask shifted bins missing here
                AppendSpectra(InputWorkspace1=ws_out,
                                                InputWorkspace2=__temp_single_spectrum,
                                                OutputWorkspace=ws_out)

        print('Done')

    def _peak_maximum_position(self):
        """
        @return    :: position where peak of single spectrum has its maximum
        """
        print('Get (reasonable) position at maximum peak value if FindEPP cannot determine it (peak too narrow)')

        # Consider the normalised workspace
        x_values = np.array(mtd[self._mnorm_ws].readX(0))
        y_values = np.array(mtd[self._mnorm_ws].readY(0))

        y_imax = np.argmax(y_values)

        maximum_position = x_values[y_imax]

        bin_range = int(len(x_values) / 4)
        mid = int(len(x_values) / 2)

        if maximum_position in range(mid - bin_range, mid + bin_range):
                print('Maybe no peak present, take mid position instead (no shift operation of this spectrum)')
                maximum_position = mid

        print('Done')

        return maximum_position

    def _set_workspace_properties(self):
        print('Set workspace properties and delete unused or temporary workspaces. Taking care of control mode.')
        unique_ws_name=str(self._run_number) + '_'

        RenameWorkspace(InputWorkspace=self._red_ws,
                                        OutputWorkspace=unique_ws_name + self._red_ws)
        self.setPropertyValue('ReducedWorkspace', unique_ws_name + self._red_ws)

        if self._control_mode:
            # Rename
            RenameWorkspace(InputWorkspace=self._raw_ws,
                                                            OutputWorkspace=unique_ws_name + self._raw_ws)
            RenameWorkspace(InputWorkspace=self._mnorm_ws,
                                                            OutputWorkspace=unique_ws_name + self._mnorm_ws)
            RenameWorkspace(InputWorkspace=self._det_grouped_ws,
                                                            OutputWorkspace=unique_ws_name + self._det_grouped_ws)
            RenameWorkspace(InputWorkspace=self._monitor_ws,
                                                            OutputWorkspace=unique_ws_name + self._monitor_ws)
            if self._unmirror_option > 0 and self._calibration_ws != '':
                    RenameWorkspace(InputWorkspace=self._vnorm_ws,
                                                    OutputWorkspace=unique_ws_name + self._vnorm_ws)
            # Set properties
            self.setPropertyValue('RawWorkspace', unique_ws_name + self._raw_ws)
            self.setPropertyValue('MNormalisedWorkspace', unique_ws_name + self._mnorm_ws)
            self.setPropertyValue('DetGroupedWorkspace', unique_ws_name + self._det_grouped_ws)
            self.setPropertyValue('MonitorWorkspace', unique_ws_name + self._monitor_ws)
            if self._unmirror_option > 0 and self._calibration_ws != '':
                self.setPropertyValue('VNormalisedWorkspace', unique_ws_name + self._vnorm_ws)
        else:
            # Cleanup unused workspaces
            DeleteWorkspace(self._raw_ws)
            DeleteWorkspace(self._mnorm_ws)
            DeleteWorkspace(self._det_grouped_ws)
            DeleteWorkspace(self._monitor_ws)

        if self._unmirror_option > 0 and self._calibration_ws == '':
                DeleteWorkspace(self._vnorm_ws)

        print('Done')

    def _save_reduced_ws(self):
        """
        Saves the reduced workspace
        """
        filename = str(self._run_number) + '_' + self._red_ws + '.nxs'

        print('Save ' + filename + ' in current directory')

        #workdir = config['defaultsave.directory']
        #file_path = os.path.join(workdir, filename)
        SaveNexusProcessed(InputWorkspace=self._red_ws, Filename=filename)
        print('Saved file : ' + filename)

    def _plot_reduced_ws(self):
        """
        Plots the reduced workspace
        """
        _summed_spectra = str(self._run_number) + '_' + self._red_ws + '_summed_spectra'

        # Delete summed spectra workspace if exists (needs to be implemented)

        print('Plot (all spectra summed)' + _summed_spectra)

        SumSpectra(InputWorkspace=self._red_ws,
                                OutputWorkspace=_summed_spectra)

        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()
        graph = mtd_plot.newGraph()

        mtd_plot.plotSpectrum(_summed_spectra, 0, window=graph)

        layer = graph.activeLayer()
        layer.setAxisTitle(mtd_plot.Layer.Bottom, 'Energy Transfer (micro eV)')
        layer.setAxisTitle(mtd_plot.Layer.Left, '')
        layer.setTitle('')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
