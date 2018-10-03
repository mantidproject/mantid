# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as ms

from LoadEmptyVesuvio import LoadEmptyVesuvio

import copy
import numpy as np
import os
import re
import six

RUN_PROP = "Filename"
WKSP_PROP = "OutputWorkspace"
MODE_PROP = "Mode"
MODES=["SingleDifference", "DoubleDifference", "ThickDifference", "FoilOut", "FoilIn", "FoilInOut"]
SPECTRA_PROP = "SpectrumList"
INST_PAR_PROP = "InstrumentParFile"
SUM_PROP = "SumSpectra"
LOAD_MON = "LoadMonitors"
LOAD_LOG_FILES = "LoadLogFiles"
WKSP_PROP_LOAD_MON= "OutputMonitorWorkspace"

FILENAME_RE = re.compile(r'^([0-9]+)(\.[a-zA-z]+)?$')

# Raw workspace names which are necessary at the moment
SUMMED_WS = "__loadraw_evs"
# Enumerate the indexes for the different foil state sums
IOUT = 0
ITHIN = 1
ITHICK = 2
# Enumerate the detector types
BACKWARD = 0
FORWARD = 1

# Child Algorithm logging
_LOGGING_ = False


class LoadVesuvio(LoadEmptyVesuvio):

    _ws_index = None
    _spectrum_no = None
    foil_map = None
    _back_scattering = None
    _load_common_called = False
    _load_monitors = False
    _load_monitors_workspace = None
    _crop_required = False
    _mon_spectra = None
    _mon_index = None
    _backward_spectra_list = None
    _forward_spectra_list = None
    _mon_scale = None
    _beta = None
    _tof_max = None
    _mon_tof_max = None
    _back_mon_norm = None
    _back_period_sum1 = None
    _back_period_sum2 = None
    _back_foil_out_norm = None
    _forw_mon_norm = None
    _forw_period_sum1 = None
    _forw_period_sum2 = None
    _forw_foil_out_norm = None
    _diff_opt = None
    _spectra = None
    _sumspectra = None
    _raw_grp = None
    _raw_monitors = None
    _nperiods = None
    pt_times = None
    delta_t = None
    mon_pt_times = None
    delta_tmon = None
    summed_ws = None
    summed_mon = None
    _spectra_type = None
    _mon_norm_start = None
    _mon_norm_end = None
    _period_sum1_start = None
    _period_sum1_end = None
    _period_sum2_start = None
    _period_sum2_end = None
    _foil_out_norm_start = None
    _foil_out_norm_end = None
    sum1 = None
    sum2 = None
    sum3 = None
    foil_thin = None
    mon_out = None
    mon_thin = None
    foil_thick = None
    mon_thick = None
    foil_out = None


#----------------------------------------------------------------------------------------

    def summary(self):
        return "Loads raw data produced by the Vesuvio instrument at ISIS."

#----------------------------------------------------------------------------------------

    def category(self):
        """ Defines the category the algorithm will be put in the algorithm browser
        """
        return 'DataHandling\\Raw'

#----------------------------------------------------------------------------------------

    def seeAlso(self):
        return [ "LoadEmptyVesuvio" ,"LoadRaw" ]

#----------------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(RUN_PROP, "", StringMandatoryValidator(),
                             doc="The run numbers that should be loaded. E.g."
                                 "14188  - for single run"
                                 "14188-14195 - for summed consecutive runs"
                                 "14188,14195 - for summed non-consecutive runs")

        self.declareProperty(SPECTRA_PROP, "", StringMandatoryValidator(),
                             doc="The spectrum numbers to load. "
                                 "A dash will load a range and a semicolon delimits spectra to sum")

        self.declareProperty(MODE_PROP, "DoubleDifference", StringListValidator(MODES),
                             doc="The difference option. Valid values: %s" % str(MODES))

        self.declareProperty(FileProperty(INST_PAR_PROP, "", action=FileAction.OptionalLoad,
                                          extensions=["dat", "par"]),
                             doc="An optional IP file. If provided the values are used to correct "
                                 "the default instrument values and attach the t0 values to each "
                                 "detector")

        self.declareProperty(SUM_PROP, False,
                             doc="If true then the final output is a single spectrum containing "
                                 "the sum of all of the requested spectra. All detector angles/"
                                 "parameters are averaged over the individual inputs")

        self.declareProperty(LOAD_MON, False,
                             doc="If true then the monitor data is loaded and will be output by the "
                                 "algorithm into a separate workspace.")

        self.declareProperty(LOAD_LOG_FILES, True,
                             doc="If true, then the log files for the specified runs will be loaded.")

        self.declareProperty(WorkspaceProperty(WKSP_PROP, "", Direction.Output),
                             doc="The name of the output workspace.")

#----------------------------------------------------------------------------------------

    def PyExec(self):
        self._load_common_inst_parameters()
        self._retrieve_input()

        if "Difference" in self._diff_opt:
            self._exec_difference_mode()
        else:
            self._exec_single_foil_state_mode()

#----------------------------------------------------------------------------------------

    def validateInputs(self):
        self._load_common_inst_parameters()
        issues = {}

        # Validate run number ranges
        user_input = self.getProperty(RUN_PROP).value
        run_str = os.path.basename(user_input)
        # String could be a full file path
        if "-" in run_str:
            lower, upper = run_str.split("-")
            issues = self._validate_range_formatting(lower, upper, RUN_PROP, issues)

        # Validate SpectrumList
        grp_spectra_list = self.getProperty(SPECTRA_PROP).value
        if "," in grp_spectra_list:
            # Split on ',' if in form of 2-3,6-7
            grp_spectra_list = grp_spectra_list.split(",")
        elif ";" in grp_spectra_list:
            # Split on ';' if in form of 2-3;6-7
            grp_spectra_list = grp_spectra_list.split(";")
        else:
            # Treat input as a list (for use in loop)
            grp_spectra_list = [grp_spectra_list]

        for spectra_grp in grp_spectra_list:
            spectra_list = None
            if "-" in spectra_grp:
                # Split ranges
                spectra_list = spectra_grp.split("-")
                # Validate format
                issues = self._validate_range_formatting(spectra_list[0], spectra_list[1], SPECTRA_PROP, issues)
            else:
                # Single spectra (put into list for use in loop)
                spectra_list = [spectra_grp]

            # Validate boundaries
            for spec in spectra_list:
                spec = int(spec)
                issues = self._validate_spec_min_max(spec, issues)

        return issues

#----------------------------------------------------------------------------------------

    def _validate_range_formatting(self, lower, upper, property_name, issues):
        """
        Validates is a range style input is in the correct form of lower-upper
        """
        upper = int(upper)
        lower = int(lower)
        if upper < lower:
            issues[property_name] = "Range must be in format lower-upper"
        return issues

#----------------------------------------------------------------------------------------

    def _validate_spec_min_max(self, spectra, issues):
        """
        Validates if the spectra is with the minimum and maximum boundaries
        """
        # Only validate boundaries if in difference Mode
        if "Difference" in self.getProperty(MODE_PROP).value:
            specMin = self._backward_spectra_list[0]
            specMax = self._forward_spectra_list[-1]
            if spectra < specMin:
                issues[SPECTRA_PROP] = ("Lower limit for spectra is %d in difference mode" % specMin)
            if spectra > specMax:
                issues[SPECTRA_PROP] = ("Upper limit for spectra is %d in difference mode" % specMax)

        return issues

#----------------------------------------------------------------------------------------

    def _exec_difference_mode(self):
        """
           Execution path when a difference mode is selected
        """
        try:
            all_spectra = [item for sublist in self._spectra for item in sublist]
            self._raise_error_if_mix_fwd_back(all_spectra)
            self._raise_error_mode_scatter(self._diff_opt, self._back_scattering)
            self._set_spectra_type(all_spectra[0])
            self._setup_raw(all_spectra)
            self._create_foil_workspaces()

            for ws_index, spectrum_no in enumerate(all_spectra):
                self._ws_index, self._spectrum_no = ws_index, spectrum_no
                self.foil_map = SpectraToFoilPeriodMap(self._nperiods)

                self._integrate_periods()
                self._sum_foil_periods()
                self._normalise_by_monitor()
                self._normalise_to_foil_out()
                self._calculate_diffs()
            # end of differencing loop

            ip_file = self.getPropertyValue(INST_PAR_PROP)
            if len(ip_file) > 0:
                self.foil_out = self._load_ip_file(self.foil_out, ip_file)

            if self._sumspectra:
                self._sum_all_spectra()

            self._store_results()
        finally: # Ensures it happens whether there was an error or not
            self._cleanup_raw()

#----------------------------------------------------------------------------------------

    def _raise_error_if_mix_fwd_back(self, spectra):
        """
        Checks that in input spectra are all in the forward or all in the backward
        scattering range
        Assumes that the spectra is sorted sorted
        """
        if len(spectra) == 1:
            self._back_scattering = self._is_back_scattering(spectra[0])
            return
        all_back = self._is_back_scattering(spectra[0])
        for spec_no in spectra[1:]:
            if all_back and self._is_fwd_scattering(spec_no):
                raise RuntimeError("Mixing backward and forward spectra is not permitted. "
                                   "Please correct the SpectrumList property.")
        self._back_scattering = all_back

#----------------------------------------------------------------------------------------

    def _raise_error_period_scatter(self, run_str, back_scattering):
        """
        Checks that the input is valid for the number of periods in the data with the current scattering
        2 Period - Only Forward Scattering
        3 Period - Only Back Scattering
        6 Period - Both Forward and Back
        """
        rfi_alg = self.createChildAlgorithm(name='RawFileInfo', enableLogging=False)
        rfi_alg.setProperty('Filename', run_str)
        rfi_alg.execute()
        nperiods = rfi_alg.getProperty('PeriodCount').value

        if nperiods == 2:
            if back_scattering:
                raise RuntimeError("2 period data can only be used for forward scattering spectra")

        if nperiods == 3:
            if not back_scattering:
                raise RuntimeError("3 period data can only be used for back scattering spectra")


#----------------------------------------------------------------------------------------

    def _raise_error_mode_scatter(self, mode, back_scattering):
        """
        Checks that the input is valid for the Mode of operation selected with the current scattering
        SingleDifference - Forward Scattering
        DoubleDifference - Back Scattering
        """

        if mode == "DoubleDifference" or mode == "ThickDifference":
            if not back_scattering:
                raise RuntimeError("%s can only be used for back scattering spectra" % mode)

#----------------------------------------------------------------------------------------

    def _exec_single_foil_state_mode(self):
        """
        Execution path when a single foil state is requested
        """

        runs = self._get_runs()
        all_spectra = [item for sublist in self._spectra for item in sublist]

        if len(runs) > 1:
            self._set_spectra_type(all_spectra[0])
            self._setup_raw(all_spectra)
        else:
            self._load_single_run_spec_and_mon(all_spectra, self._get_filename(runs[0]))

        raw_group = mtd[SUMMED_WS]
        self._nperiods = raw_group.size()
        first_ws = raw_group[0]
        foil_out = WorkspaceFactory.create(first_ws)
        x_values = first_ws.readX(0)
        self.foil_out = foil_out

        foil_map = SpectraToFoilPeriodMap(self._nperiods)
        for ws_index, spectrum_no in enumerate(all_spectra):
            self._set_spectra_type(spectrum_no)
            foil_out_periods, foil_thin_periods, _ = self._get_foil_periods()

            if self._diff_opt == "FoilOut":
                raw_grp_indices = foil_map.get_indices(spectrum_no, foil_out_periods)
            elif self._diff_opt == "FoilIn":
                indices_thin = foil_map.get_indices(spectrum_no, foil_thin_periods)
                indices_thick = foil_map.get_indices(spectrum_no, foil_thin_periods)
                raw_grp_indices = indices_thin + indices_thick
            elif self._diff_opt == "FoilInOut":
                raw_grp_indices = list(range(0, self._nperiods))
            else:
                raise RuntimeError("Unknown single foil mode: %s." % self._diff_opt)

            dataY = foil_out.dataY(ws_index)
            dataE = foil_out.dataE(ws_index)
            for group_index in raw_grp_indices:
                dataY += raw_group[group_index].readY(ws_index)
                dataE += np.square(raw_group[group_index].readE(ws_index))
            np.sqrt(dataE, dataE)
            foil_out.setX(ws_index, x_values)

            if len(runs) > 1:
                # Create monitor workspace for normalisation
                first_mon_ws = self._raw_monitors[0]
                nmonitor_bins = first_mon_ws.blocksize()
                nhists = first_ws.getNumberHistograms()
                data_kwargs = {'NVectors': nhists, 'XLength': nmonitor_bins, 'YLength': nmonitor_bins}
                mon_out = WorkspaceFactory.create(first_mon_ws, **data_kwargs)

                mon_raw_t = self._raw_monitors[0].readX(0)
                delay = mon_raw_t[2] - mon_raw_t[1]
                # The original EVS loader, raw.for/rawb.for, does this. Done here to match results
                mon_raw_t = mon_raw_t - delay
                self.mon_pt_times = mon_raw_t[1:]

                if self._nperiods == 6 and self._spectra_type == FORWARD:
                    mon_periods = (5, 6)
                    raw_grp_indices = foil_map.get_indices(spectrum_no, mon_periods)

                outY = mon_out.dataY(ws_index)
                for grp_index in raw_grp_indices:
                    raw_ws = self._raw_monitors[grp_index]
                    outY += raw_ws.readY(self._mon_index)

                # Normalise by monitor
                indices_in_range = np.where((self.mon_pt_times >= self._mon_norm_start) & (self.mon_pt_times < self._mon_norm_end))
                mon_values = mon_out.readY(ws_index)
                mon_values_sum = np.sum(mon_values[indices_in_range])
                foil_state = foil_out.dataY(ws_index)
                foil_state *= (self._mon_scale/mon_values_sum)
                err = foil_out.dataE(ws_index)
                err *= (self._mon_scale/mon_values_sum)

        ip_file = self.getPropertyValue(INST_PAR_PROP)
        if len(ip_file) > 0:
            self.foil_out = self._load_ip_file(self.foil_out, ip_file)

        if self._sumspectra:
            self._sum_all_spectra()

        ms.DeleteWorkspace(Workspace=SUMMED_WS, EnableLogging=_LOGGING_)
        self._store_results()
        self._cleanup_raw()

#----------------------------------------------------------------------------------------

    def _get_filename(self, run_or_filename):
        """Given a string containing either a filename/partial filename or run number find the correct
        file prefix"""
        vesuvio = config.getInstrument("VESUVIO")
        if isinstance(run_or_filename, six.integer_types):
            run_no = run_or_filename
            return vesuvio.filePrefix(int(run_no)) + str(run_or_filename)
        else:
            match = FILENAME_RE.match(run_or_filename)
            if match:
                run_no = match.group(1)
                return vesuvio.filePrefix(int(run_no)) + str(run_or_filename)
            else:
                # Assume file is okay and give it a go with Load
                return run_or_filename

    #----------------------------------------------------------------------------------------

    def _load_single_run_spec_and_mon(self, all_spectra, run_str):
        self._raise_error_period_scatter(run_str, self._back_scattering)
        # check if the monitor spectra are already in the spectra list
        filtered_spectra = sorted([i for i in all_spectra if i <= self._mon_spectra[-1]])
        mons_in_ws = False
        if filtered_spectra == self._mon_spectra and self._load_monitors:
            # Load monitors in workspace if defined by user
            self._load_monitors = False
            mons_in_ws = True
            logger.warning("LoadMonitors is true while monitor spectra are defined in the spectra list.")
            logger.warning("Monitors have been loaded into the data workspace not separately.")
        if mons_in_ws:
            ms.Load(Filename=run_str, OutputWorkspace=SUMMED_WS, SpectrumList=all_spectra,
                    LoadLogFiles=self._load_log_files, EnableLogging=_LOGGING_)
        else:
            all_spec_inc_mon = self._mon_spectra
            all_spec_inc_mon.extend(all_spectra)
            ms.Load(Filename=run_str, OutputWorkspace=SUMMED_WS, SpectrumList=all_spec_inc_mon,
                    LoadLogFiles=self._load_log_files, LoadMonitors='Separate', EnableLogging=_LOGGING_)
            if self._load_monitors:
                monitor_group = mtd[SUMMED_WS +'_monitors']
                mon_out_name = self.getPropertyValue(WKSP_PROP) + "_monitors"
                clone = self.createChildAlgorithm("CloneWorkspace", False)
                clone.setProperty("InputWorkspace", monitor_group.getItem(0))
                clone.setProperty("OutputWorkspace", mon_out_name)
                clone.execute()
                self._load_monitors_workspace = clone.getProperty("OutputWorkspace").value
                self._load_monitors_workspace = self._sum_monitors_in_group(monitor_group,
                                                                            self._load_monitors_workspace)
            self._raw_monitors = mtd[SUMMED_WS +'_monitors']

#----------------------------------------------------------------------------------------

    def _load_common_inst_parameters(self):
        """
            Loads an empty VESUVIO instrument and attaches the necessary
            parameters as attributes
        """
        if self._load_common_called:
            return

        empty_vesuvio_ws = self._load_empty_evs()
        empty_vesuvio = empty_vesuvio_ws.getInstrument()

        def to_int_list(str_param):
            """Return the list of numbers described by the string range"""
            elements = str_param.split("-")
            return list(range(int(elements[0]),int(elements[1]) + 1)) # range goes x_l,x_h-1

        # Attach parameters as attributes
        parnames = empty_vesuvio.getParameterNames(False)
        for name in parnames:
            # Irritating parameter access doesn't let you query the type
            # so resort to trying
            try:
                parvalue = empty_vesuvio.getNumberParameter(name)
            except RuntimeError:
                parvalue = empty_vesuvio.getStringParameter(name)
            setattr(self, name, parvalue[0]) # Adds attributes to self from Parameter file

        int_mon_spectra = self.monitor_spectra.split(',')
        int_mon_spectra = [int(i) for i in int_mon_spectra]
        self._mon_spectra = int_mon_spectra
        self._mon_index = int_mon_spectra[0] - 1

        self._backward_spectra_list = to_int_list(self.backward_scatter_spectra)
        self._forward_spectra_list = to_int_list(self.forward_scatter_spectra)
        self._mon_scale = self.monitor_scale
        self._beta =  self.double_diff_mixing
        self._tof_max = self.tof_max
        self._mon_tof_max = self.monitor_tof_max

        # Normalisation ranges
        def to_range_tuple(str_range):
            """Return a list of 2 floats giving the lower,upper range"""
            elements = str_range.split("-")
            return float(elements[0]), float(elements[1])

        self._back_mon_norm = to_range_tuple(self.backward_monitor_norm)
        self._back_period_sum1 = to_range_tuple(self.backward_period_sum1)
        self._back_period_sum2 = to_range_tuple(self.backward_period_sum2)
        self._back_foil_out_norm = to_range_tuple(self.backward_foil_out_norm)

        self._forw_mon_norm = to_range_tuple(self.forward_monitor_norm)
        self._forw_period_sum1 = to_range_tuple(self.forward_period_sum1)
        self._forw_period_sum2 = to_range_tuple(self.forward_period_sum2)
        self._forw_foil_out_norm = to_range_tuple(self.forward_foil_out_norm)
        self._load_common_called = True

#----------------------------------------------------------------------------------------

    def _load_diff_mode_parameters(self, workspace):
        """
        Loads the relevant parameter file for the current difference mode
        into the given workspace
        """
        load_parameter_file = self.createChildAlgorithm("LoadParameterFile")
        load_parameter_file.setLogging(_LOGGING_)
        load_parameter_file.setPropertyValue("Workspace", workspace.name())
        load_parameter_file.setProperty("Filename",
                                        self._get_parameter_filename(self._diff_opt))
        load_parameter_file.execute()

#----------------------------------------------------------------------------------------

    def _get_parameter_filename(self, diff_opt):
        """
        Returns the filename for the diff-mode specific parameters
        """
        if "Difference" not in diff_opt:
            raise RuntimeError("Trying to load parameters for difference mode when not doing differencing! "
                               "This is most likely a bug in the code. Please report this to the developers")

        template = "VESUVIO_{0}_diff_Parameters.xml"
        if diff_opt == "SingleDifference":
            return template.format("single")
        else:
            return template.format("double")

#----------------------------------------------------------------------------------------

    def _retrieve_input(self):
        self._diff_opt = self.getProperty(MODE_PROP).value
        self._load_monitors = self.getProperty(LOAD_MON).value
        # Check for sets of spectra to sum. Semi colon delimits sets
        # that should be summed
        spectra_str = self.getPropertyValue(SPECTRA_PROP)
        summed_blocks = spectra_str.split(";")
        self._spectra = []
        for block in summed_blocks:
            prop = IntArrayProperty("unnamed", block)
            numbers = prop.value.tolist()
            if self._mon_spectra in numbers:
                numbers.remove(self._spectra)
            numbers.sort()
            self._spectra.append(numbers)
        #endfor

        self._sumspectra = self.getProperty(SUM_PROP).value
        self._load_log_files = self.getProperty(LOAD_LOG_FILES).value

#----------------------------------------------------------------------------------------

    def _setup_raw(self, spectra):
        self._raw_grp, self._raw_monitors = self._load_and_sum_runs(spectra)
        nperiods = self._raw_grp.size()

        first_ws = self._raw_grp[0]
        self._nperiods = nperiods

        # Cache delta_t values
        raw_t = first_ws.readX(0)
        delay = raw_t[2] - raw_t[1]
        # The original EVS loader, raw.for/rawb.for, does this. Done here to match results
        raw_t = raw_t - delay
        self.pt_times = raw_t[1:]
        self.delta_t = (raw_t[1:] - raw_t[:-1])

        mon_raw_t = self._raw_monitors[0].readX(0)
        delay = mon_raw_t[2] - mon_raw_t[1]
        # The original EVS loader, raw.for/rawb.for, does this. Done here to match results
        mon_raw_t = mon_raw_t - delay
        self.mon_pt_times = mon_raw_t[1:]
        self.delta_tmon = (mon_raw_t[1:] - mon_raw_t[:-1])

#----------------------------------------------------------------------------------------

    def _load_and_sum_runs(self, spectra):
        """Load the input set of runs & sum them if there
        is more than one.
            @param spectra :: The list of spectra to load
            @returns a tuple of length 2 containing (main_detector_ws, monitor_ws)
        """
        runs = self._get_runs()

        self.summed_ws, self.summed_mon = "__loadraw_evs", "__loadraw_evs_monitors"
        spec_inc_mon = self._mon_spectra
        spec_inc_mon.extend(spectra)
        for index, run in enumerate(runs):
            filename = self._get_filename(run)
            self._raise_error_period_scatter(filename, self._back_scattering)
            if index == 0:
                out_name, out_mon = SUMMED_WS, SUMMED_WS + '_monitors'
            else:
                out_name, out_mon = SUMMED_WS + 'tmp', SUMMED_WS + 'tmp_monitors'

            # Load data
            ms.Load(Filename=filename,
                    SpectrumList=spec_inc_mon,
                    OutputWorkspace=out_name,
                    LoadMonitors='Separate',
                    LoadLogFiles=self._load_log_files,
                    EnableLogging=_LOGGING_)

            # Sum
            if index > 0:
                ms.Plus(LHSWorkspace=SUMMED_WS,
                        RHSWorkspace=out_name,
                        OutputWorkspace=SUMMED_WS,
                        EnableLogging=_LOGGING_)
                ms.Plus(LHSWorkspace=SUMMED_WS + '_monitors',
                        RHSWorkspace=out_mon,
                        OutputWorkspace=SUMMED_WS + '_monitors',
                        EnableLogging=_LOGGING_)

                ms.DeleteWorkspace(out_name, EnableLogging=_LOGGING_)
                ms.DeleteWorkspace(out_mon, EnableLogging=_LOGGING_)

        # Check to see if extra data needs to be loaded to normalise in data
        if "Difference" in self._diff_opt:
            x_max = self._tof_max
            if self._foil_out_norm_end > self._tof_max:
                x_max = self._foil_out_norm_end
                self._crop_required = True

            ms.CropWorkspace(Inputworkspace= SUMMED_WS,
                             OutputWorkspace=SUMMED_WS,
                             XMax=x_max,
                             EnableLogging=_LOGGING_)

        summed_data, summed_mon = mtd[SUMMED_WS], mtd[SUMMED_WS + '_monitors']

        # Sum monitors from each period together
        if self._load_monitors:
            mon_out_name = self.getPropertyValue(WKSP_PROP) + "_monitors"
            clone = self.createChildAlgorithm("CloneWorkspace", False)
            clone.setProperty("InputWorkspace",summed_mon.getItem(0))
            clone.setProperty("OutputWorkspace", mon_out_name)
            clone.execute()
            self._load_monitors_workspace = clone.getProperty("OutputWorkspace").value
            self._load_monitors_workspace = self._sum_monitors_in_group(summed_mon,
                                                                        self._load_monitors_workspace)
        if "Difference" in self._diff_opt:
            self._load_diff_mode_parameters(summed_data)
        return summed_data, summed_mon


#----------------------------------------------------------------------------------------

    def _sum_monitors_in_group(self, monitor_group, output_ws):
        """
        Sums together all the monitors for one run
        @param monitor_group    :: All the monitor workspaces for a single run
        @param output_ws        :: The workspace that will contain the summed monitor data
        @return                 :: The workspace containing the summed monitor data
        """

        for mon_index in range(1, monitor_group.getNumberOfEntries()):
            plus = self.createChildAlgorithm("Plus", False)
            plus.setProperty("LHSWorkspace", output_ws)
            plus.setProperty("RHSWorkspace", monitor_group.getItem(mon_index))
            plus.setProperty("OutputWorkspace", output_ws)
            plus.execute()
            output_ws = plus.getProperty("OutputWorkspace").value
        return output_ws


#----------------------------------------------------------------------------------------

    def _get_runs(self):
        """
        Returns the runs as a list of strings
        """
        # String could be a full file path
        user_input = self.getProperty(RUN_PROP).value
        run_str = os.path.basename(user_input)
        # Load is not doing the right thing when summing. The numbers don't look correct
        if "-" in run_str:
            lower, upper = run_str.split("-")
            # Range goes lower to up-1 but we want to include the last number
            runs = list(range(int(lower), int(upper)+1))
        elif "," in run_str:
            runs = run_str.split(",")
        else:
            # Leave it as it is
            runs = [user_input]

        return runs

#----------------------------------------------------------------------------------------

    def _set_spectra_type(self, spectrum_no):
        """
        Set whether this spectrum no is forward/backward scattering
        and set the normalization range appropriately
        @param spectrum_no The current spectrum no
        """
        if self._is_back_scattering(spectrum_no):
            self._spectra_type=BACKWARD
            self._mon_norm_start, self._mon_norm_end = self._back_mon_norm
            self._period_sum1_start, self._period_sum1_end = self._back_period_sum1
            self._period_sum2_start, self._period_sum2_end = self._back_period_sum2
            self._foil_out_norm_start, self._foil_out_norm_end = self._back_foil_out_norm
        else:
            self._spectra_type=FORWARD
            self._mon_norm_start, self._mon_norm_end = self._forw_mon_norm
            self._period_sum1_start, self._period_sum1_end = self._forw_period_sum1
            self._period_sum2_start, self._period_sum2_end = self._forw_period_sum2
            self._foil_out_norm_start, self._foil_out_norm_end = self._forw_foil_out_norm

#----------------------------------------------------------------------------------------

    def _is_back_scattering(self, spectrum_no):
        return self._backward_spectra_list[0] <= spectrum_no <= self._backward_spectra_list[-1]

    #----------------------------------------------------------------------------------------

    def _is_fwd_scattering(self, spectrum_no):
        return self._forward_spectra_list[0] <= spectrum_no <= self._forward_spectra_list[-1]

    #----------------------------------------------------------------------------------------

    def _integrate_periods(self):
        """
            Calculates 2 arrays of sums, 1 per period, of the Y values from
            the raw data between:
               (a) period_sum1_start & period_sum1_end
               (b) period_sum2_start & period_sum2_end.
            It also creates a 3rd blank array that will be filled by calculate_foil_counts_per_us.
            Operates on the current workspace index
        """
        self.sum1 = np.zeros(self._nperiods)
        self.sum2 = np.zeros(self._nperiods)
        self.sum3 = np.zeros(3)
        #sumx_start/end values obtained from VESUVIO parameter file
        sum1_start,sum1_end = self._period_sum1_start, self._period_sum1_end
        sum2_start,sum2_end = self._period_sum2_start,self._period_sum2_end
        xvalues = self.pt_times # values of the raw_grp x axis
        # Array of bin indexes corresponding to bins that lie within start/end range
        sum1_indices = np.where((xvalues > sum1_start) & (xvalues < sum1_end))
        sum2_indices = np.where((xvalues > sum2_start) & (xvalues < sum2_end))

        wsindex = self._ws_index # The current spectra to examine
        for i in range(self._nperiods):
            # Gets the sum(1,2) of the yvalues at the bin indexs
            yvalues = self._raw_grp[i].readY(wsindex)
            self.sum1[i] = np.sum(yvalues[sum1_indices])
            self.sum2[i] = np.sum(yvalues[sum2_indices])
            if self.sum2[i] != 0.0:
                self.sum1[i] /= self.sum2[i]

        # Sort sum1 in increasing order and match the foil map
        self.sum1 = self.foil_map.reorder(self.sum1)


#----------------------------------------------------------------------------------------

    def _create_foil_workspaces(self):
        """
            Create the workspaces that will hold the foil out, thin & thick results
            The output will be a point workspace
        """
        first_ws = self._raw_grp[0]
        ndata_bins = first_ws.blocksize()
        nhists = first_ws.getNumberHistograms()
        data_kwargs = {'NVectors':nhists,'XLength':ndata_bins,'YLength':ndata_bins}

        # This will be used as the result workspace
        self.foil_out = WorkspaceFactory.create(first_ws, **data_kwargs)
        self.foil_out.setDistribution(True)
        self.foil_thin = WorkspaceFactory.create(first_ws, **data_kwargs)

        # Monitors will be a different size
        first_monws = self._raw_monitors[0]
        nmonitor_bins = first_monws.blocksize()
        monitor_kwargs = copy.deepcopy(data_kwargs)
        monitor_kwargs['XLength'] = nmonitor_bins
        monitor_kwargs['YLength'] = nmonitor_bins

        self.mon_out = WorkspaceFactory.create(first_monws, **monitor_kwargs)
        self.mon_thin = WorkspaceFactory.create(first_monws, **monitor_kwargs)

        if self._nperiods == 2:
            self.foil_thick = None
            self.mon_thick = None
        else:
            self.foil_thick = WorkspaceFactory.create(first_ws, **data_kwargs)
            self.mon_thick = WorkspaceFactory.create(first_monws, **monitor_kwargs)

#----------------------------------------------------------------------------------------

    def _sum_foil_periods(self):
        """
        Sums the counts in the different periods to get the total counts
        for the foil out, thin foil & thick foil states for the back scattering detectors for the
        current workspace index & spectrum number
        """
        foil_out_periods, foil_thin_periods, foil_thick_periods = self._get_foil_periods()

        if self._nperiods == 6 and self._spectra_type == FORWARD:
            mon_out_periods = (5,6)
            mon_thin_periods = (3,4)
            mon_thick_periods = foil_thick_periods
        else:
            # None indicates same as standard foil
            mon_out_periods, mon_thin_periods, mon_thick_periods = (None,None,None)

        # Foil out
        self._sum_foils(self.foil_out, self.mon_out, IOUT, foil_out_periods, mon_out_periods)
        # Thin foil
        self._sum_foils(self.foil_thin, self.mon_thin, ITHIN, foil_thin_periods, mon_thin_periods)
        # Thick foil
        if foil_thick_periods is not None:
            self._sum_foils(self.foil_thick, self.mon_thick, ITHICK,
                            foil_thick_periods, mon_thick_periods)

#----------------------------------------------------------------------------------------

    def _get_foil_periods(self):
        """
        Return the period numbers (starting from 1) that contribute to the
        respective foil states
        """
        if self._nperiods == 2:
            foil_out_periods = (2,)
            foil_thin_periods = (1,)
            foil_thick_periods = None
        elif self._nperiods == 3:
            foil_out_periods = (3,)
            foil_thin_periods = (2,)
            foil_thick_periods = (1,)
        elif self._nperiods == 6:
            if self._spectra_type == BACKWARD:
                foil_out_periods = (5,6)
                foil_thin_periods = (3,4)
                foil_thick_periods = (1,2)
            else:
                foil_out_periods = (4,5,6)
                foil_thin_periods = (1,2,3)
                foil_thick_periods = (1,2)
        else:
            pass

        return foil_out_periods, foil_thin_periods, foil_thick_periods

#----------------------------------------------------------------------------------------

    def _sum_foils(self, foil_ws, mon_ws, sum_index, foil_periods, mon_periods=None):
        """
        Sums the counts from the given foil periods in the raw data group
        @param foil_ws :: The workspace that will receive the summed counts
        @param mon_ws :: The monitor workspace that will receive the summed monitor counts
        @param sum_index :: An index into the sum3 array where the integrated counts will be
                            accumulated
        @param foil_periods :: The period numbers that contribute to this sum
        @param mon_periods :: The period numbers of the monitors that contribute to this monitor sum
                              (if None then uses the foil_periods)
        """
        # index that corresponds to workspace in group based on foil state
        raw_grp_indices = self.foil_map.get_indices(self._spectrum_no, foil_periods)
        wsindex = self._ws_index        # Spectra number - monitors(2) - 1
        outY = foil_ws.dataY(wsindex)   # Initialise outY list to correct length with 0s
        delta_t = self.delta_t          # Bin width
        for grp_index in raw_grp_indices:
            raw_ws = self._raw_grp[grp_index]
            outY += raw_ws.readY(wsindex)
            self.sum3[sum_index] += self.sum2[grp_index]

        # Errors are calculated from counts
        eout = np.sqrt(outY)/delta_t
        foil_ws.setE(wsindex,eout)
        outY /= delta_t

        # monitors
        if mon_periods is None:
            mon_periods = foil_periods
        raw_grp_indices = self.foil_map.get_indices(self._spectrum_no, mon_periods)
        outY = mon_ws.dataY(wsindex)
        for grp_index in raw_grp_indices:
            raw_ws = self._raw_monitors[grp_index]
            outY += raw_ws.readY(self._mon_index)

        outY /= self.delta_tmon


#----------------------------------------------------------------------------------------

    def _normalise_by_monitor(self):
        """
            Normalises by the monitor counts between mon_norm_start & mon_norm_end
            instrument parameters for the current workspace index
        """
        indices_in_range = np.where((self.mon_pt_times >= self._mon_norm_start) & (self.mon_pt_times < self._mon_norm_end))

        wsindex = self._ws_index
        # inner function to apply normalization

        def monitor_normalization(foil_ws, mon_ws):
            """
            Applies monitor normalization to the given foil spectrum from the given
            monitor spectrum.
            """
            mon_values = mon_ws.readY(wsindex)
            mon_values_sum = np.sum(mon_values[indices_in_range])
            foil_state = foil_ws.dataY(wsindex)
            foil_state *= (self._mon_scale/mon_values_sum)
            err = foil_ws.dataE(wsindex)
            err *= (self._mon_scale/mon_values_sum)

        monitor_normalization(self.foil_out, self.mon_out)
        monitor_normalization(self.foil_thin, self.mon_thin)
        if self._nperiods != 2:
            monitor_normalization(self.foil_thick, self.mon_thick)

#----------------------------------------------------------------------------------------

    def _normalise_to_foil_out(self):
        """
            Normalises the thin/thick foil counts to the
            foil out counts between (foil_out_norm_start,foil_out_norm_end)
            for the current workspace index
        """
        # Indices where the given condition is true
        range_indices = np.where((self.pt_times >= self._foil_out_norm_start) & (self.pt_times < self._foil_out_norm_end))
        wsindex = self._ws_index
        cout = self.foil_out.readY(wsindex)
        sum_out = np.sum(cout[range_indices])

        def normalise_to_out(foil_ws, foil_type):
            values = foil_ws.dataY(wsindex)
            sum_values = np.sum(values[range_indices])
            if sum_values == 0.0:
                self.getLogger().warning("No counts in %s foil spectrum %d." % (foil_type,self._spectrum_no))
                sum_values = 1.0
            norm_factor = (sum_out/sum_values)
            values *= norm_factor
            errors = foil_ws.dataE(wsindex)
            errors *= norm_factor

        normalise_to_out(self.foil_thin, "thin")
        if self._nperiods != 2:
            normalise_to_out(self.foil_thick, "thick")

#----------------------------------------------------------------------------------------

    def _calculate_diffs(self):
        """
            Based on the DifferenceType property, calculate the final output
            spectra for the current workspace index
        """
        wsindex = self._ws_index
        if self._diff_opt == "SingleDifference":
            self._calculate_thin_difference(wsindex)
        elif self._diff_opt == "DoubleDifference":
            self._calculate_double_difference(wsindex)
        elif self._diff_opt == "ThickDifference":
            self._calculate_thick_difference(wsindex)
        else:
            raise RuntimeError("Unknown difference type requested: %d" % self._diff_opt)

        self.foil_out.setX(wsindex, self.pt_times)

#----------------------------------------------------------------------------------------

    def _calculate_thin_difference(self, ws_index):
        """
           Calculate difference between the foil out & thin foil
           states for the given index. The foil out workspace
           will become the output workspace
            @param ws_index :: The current workspace index
        """
        # Counts
        cout = self.foil_out.dataY(ws_index)
        if self._spectra_type == BACKWARD:
            cout -= self.foil_thin.readY(ws_index)
        else:
            cout *= -1.0
            cout += self.foil_thin.readY(ws_index)

        # Errors
        eout = self.foil_out.dataE(ws_index)
        ethin = self.foil_thin.readE(ws_index)
        np.sqrt((eout**2 + ethin**2), eout) # The second argument makes it happen in place

#----------------------------------------------------------------------------------------

    def _calculate_double_difference(self, ws_index):
        """
            Calculates the difference between the foil out, thin & thick foils
            using the mixing parameter beta. The final counts are:
                y = c_out(i)*(1-\beta) -c_thin(i) + \beta*c_thick(i).
            The output will be stored in cout
            @param ws_index :: The current index being processed
        """
        cout = self.foil_out.dataY(ws_index)
        one_min_beta = (1. - self._beta)
        cout *= one_min_beta
        cout -= self.foil_thin.readY(ws_index)
        cout += self._beta*self.foil_thick.readY(ws_index)

        # Errors
        eout = self.foil_out.dataE(ws_index)
        ethin = self.foil_thin.readE(ws_index)
        ethick = self.foil_thick.readE(ws_index)
        # The second argument makes it happen in place
        np.sqrt((one_min_beta*eout)**2 + ethin**2 + (self._beta**2)*ethick**2, eout)

#----------------------------------------------------------------------------------------

    def _calculate_thick_difference(self, ws_index):
        """
            Calculates the difference between the foil out & thick foils
            The output will be stored in cout
            @param ws_index :: The current index being processed
        """
        # Counts
        cout = self.foil_out.dataY(ws_index)
        cout -= self.foil_thick.readY(ws_index)

        # Errors
        eout = self.foil_out.dataE(ws_index)
        ethick = self.foil_thick.readE(ws_index)
        np.sqrt((eout**2 + ethick**2), eout) # The second argument makes it happen in place

#----------------------------------------------------------------------------------------

    def _sum_all_spectra(self):
        """
            Sum requested sets of spectra together
        """
        nspectra_out = len(self._spectra)
        ws_out = WorkspaceFactory.create(self.foil_out, NVectors=nspectra_out)
        # foil_out has all spectra in order specified by input
        foil_start = 0
        for idx_out in range(len(self._spectra)):
            ws_out.setX(idx_out, self.foil_out.readX(foil_start))
            summed_set = self._spectra[idx_out]
            nsummed = len(summed_set)
            y_out, e_out = ws_out.dataY(idx_out), ws_out.dataE(idx_out)
            spec_out = ws_out.getSpectrum(idx_out)
            spec_out.setSpectrumNo(self.foil_out.getSpectrum(foil_start).getSpectrumNo())
            spec_out.clearDetectorIDs()
            for foil_idx in range(foil_start, foil_start+nsummed):
                y_out += self.foil_out.readY(foil_idx)
                foil_err = self.foil_out.readE(foil_idx)
                e_out += foil_err*foil_err # gaussian errors
                in_ids = self.foil_out.getSpectrum(foil_idx).getDetectorIDs()
                for det_id in in_ids:
                    spec_out.addDetectorID(det_id)
            #endfor
            np.sqrt(e_out, e_out)
            foil_start += nsummed
        #endfor
        self.foil_out = ws_out

#----------------------------------------------------------------------------------------

    def _store_results(self):
        """
           Sets the values of the output workspace properties
        """
        # Crop the data to _tof_max if not already done so
        if self._crop_required:
            crop = self.createChildAlgorithm("CropWorkspace")
            crop.setProperty("InputWorkspace" ,self.foil_out)
            crop.setProperty("OutputWorkspace",self.foil_out)
            crop.setProperty("XMax", self._tof_max)
            crop.execute()
            self.foil_out = crop.getProperty("OutputWorkspace").value

        self.setProperty(WKSP_PROP, self.foil_out)
        # Add OutputWorkspace property for Monitors
        if self._load_monitors:
            # Check property is not being re-declared
            if not self.existsProperty(WKSP_PROP_LOAD_MON):
                mon_out_name = self.getPropertyValue(WKSP_PROP) + '_monitors'
                self.declareProperty(WorkspaceProperty(WKSP_PROP_LOAD_MON, mon_out_name, Direction.Output),
                                     doc="The output workspace that contains the monitor spectra.")
            self.setProperty(WKSP_PROP_LOAD_MON, self._load_monitors_workspace)

    def _cleanup_raw(self):
        """
            Clean up the raw data files
        """
        if SUMMED_WS in mtd:
            ms.DeleteWorkspace(SUMMED_WS,EnableLogging=_LOGGING_)
        if SUMMED_WS + '_monitors' in mtd:
            ms.DeleteWorkspace(SUMMED_WS + '_monitors',EnableLogging=_LOGGING_)

#########################################################################################


class SpectraToFoilPeriodMap(object):
    """Defines the mapping between a spectrum number
    & the period index into a WorkspaceGroup for a foil state.
    2 period :: forward scattering
    3 period :: back scattering
    6 period :: back & forward scattering

    one_to_one          :: Only used in back scattering where there is a single
                           static foil
    odd_even/even_odd   :: Only used in forward scatter models when the foil
                           is/isn't in front of each detector. First bank 135-142
                           is odd_even, second (143-150) is even_odd and so on.
    """

    def __init__(self, nperiods=6):
        """Constructor. For nperiods set up the mappings"""
        if nperiods == 2:
            self._one_to_one = {1:1, 2:2}   # Kept for use in reorder method
            self._odd_even =   {1:1, 2:2}
            self._even_odd =   {1:2, 2:1}
        elif nperiods == 3:
            self._one_to_one = {1:1, 2:2, 3:3}
        elif nperiods == 6:
            self._one_to_one = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6}
            self._odd_even =   {1:1, 2:3, 3:5, 4:2, 5:4, 6:6}
            self._even_odd =   {1:2, 2:4, 3:6, 4:1, 5:3, 6:5}
        else:
            raise RuntimeError("Unsupported number of periods given: " + str(nperiods) +
                               ". Supported number of periods=2,3,6")

#----------------------------------------------------------------------------------------

    def reorder(self, arr):
        """
           Orders the given array by increasing value. At the same time
           it reorders the 1:1 map to match this order
           numpy
        """
        vals = np.array(list(self._one_to_one.values()))
        sorted_indices = arr.argsort()
        vals = vals[sorted_indices]
        arr = arr[sorted_indices]
        self._one_to_one = {}
        for index,val in enumerate(vals):
            self._one_to_one[index+1] = int(val)

        return arr

#----------------------------------------------------------------------------------------

    def get_foilout_periods(self, spectrum_no):
        """Returns a list of the foil-out periods for the given
        spectrum number. Note that these start from 1 not zero
            @param spectrum_no :: A spectrum number (1->nspectra)
            @returns A list of period numbers for foil out state
        """
        return self.get_foil_periods(spectrum_no, state=0)

#----------------------------------------------------------------------------------------

    def get_foilin_periods(self, spectrum_no):
        """Returns a list of the foil-out periods for the given
        spectrum number. Note that these start from 1 not zero
            @param spectrum_no :: A spectrum number (1->nspectra)
            @returns A list of period numbers for foil out state
        """
        return self.get_foil_periods(spectrum_no, state=1)

#----------------------------------------------------------------------------------------

    def get_foil_periods(self, spectrum_no, state):
        """Returns a list of the periods for the given
        spectrum number & foil state. Note that these start from 1 not zero
            @param spectrum_no :: A spectrum number (1->nspectra)
            @param state :: 0 = foil out, 1 = foil in.
            @returns A list of period numbers for foil out state
        """
        self._validate_spectrum_number(spectrum_no)

        foil_out = (state==0)

        if spectrum_no < 135:
            foil_periods = [1,2,3]
        elif (135 <= spectrum_no <= 142) or \
             (151 <= spectrum_no <= 158) or \
             (167 <= spectrum_no <= 174) or \
             (183 <= spectrum_no <= 190):
            foil_periods = [2,4,6] if foil_out else [1,3,5]
        else:
            foil_periods = [1,3,5] if foil_out else [2,4,6]
        return foil_periods

#----------------------------------------------------------------------------------------

    def get_indices(self, spectrum_no, foil_state_numbers):
        """
        Returns a tuple of indices that can be used to access the Workspace within
        a WorkspaceGroup that corresponds to the foil state numbers given
        @param spectrum_no :: A spectrum number (1->nspectra)
        @param foil_state_numbers :: A number between 1 & 6(inclusive) that defines which foil
                                state is required
        @returns A tuple of indices in a WorkspaceGroup that gives the associated Workspace
        """
        indices = []
        for state in foil_state_numbers:
            indices.append(self.get_index(spectrum_no, state))
        return tuple(indices)

#----------------------------------------------------------------------------------------

    def get_index(self, spectrum_no, foil_state_no):
        """Returns an index that can be used to access the Workspace within
        a WorkspaceGroup that corresponds to the foil state given
            @param spectrum_no :: A spectrum number (1->nspectra)
            @param foil_state_no :: A number between 1 & 6(inclusive) that defines which
                                        foil state is required
            @returns The index in a WorkspaceGroup that gives the associated Workspace
        """

        self._validate_foil_number(foil_state_no)
        self._validate_spectrum_number(spectrum_no)

        # For the back scattering banks or foil states > 6 then there is a 1:1 map
        if foil_state_no > 6 or spectrum_no < 135:
            foil_periods = self._one_to_one
        elif (135 <= spectrum_no <= 142) or \
             (151 <= spectrum_no <= 158) or \
             (167 <= spectrum_no <= 174) or \
             (183 <= spectrum_no <= 190):
             # For each alternating forward scattering bank :: foil_in = 1,3,5, foil out = 2,4,6
            foil_periods = self._odd_even
        else:
            # foil_in = 2,4,6 foil out = 1,3,5
            foil_periods = self._even_odd

        foil_period_no = foil_periods[foil_state_no]
        return foil_period_no - 1 # Minus 1 to get to WorkspaceGroup index

#----------------------------------------------------------------------------------------

    def _validate_foil_number(self, foil_number):
        if foil_number < 1 or foil_number > 6:
            raise ValueError("Invalid foil state given, expected a number between "
                             "1 and 6. number=%d" % foil_number)

#----------------------------------------------------------------------------------------

    def _validate_spectrum_number(self, spectrum_no):
        if spectrum_no < 1 or spectrum_no > 198:
            raise ValueError("Invalid spectrum given, expected a number between 3 "
                             "and 198. spectrum=%d" % spectrum_no)

#########################################################################################


# Registration
AlgorithmFactory.subscribe(LoadVesuvio)
