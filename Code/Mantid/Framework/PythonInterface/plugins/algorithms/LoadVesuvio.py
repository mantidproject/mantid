#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (CropWorkspace, LoadEmptyInstrument, LoadRaw, Plus,
                              DeleteWorkspace)

import copy
import numpy as np
import os

RUN_PROP = "Filename"
WKSP_PROP = "OutputWorkspace"
MODE_PROP = "Mode"
MODES=["SingleDifference", "DoubleDifference", "ThickDifference", "FoilOut", "FoilIn", "FoilInOut"]
SPECTRA_PROP = "SpectrumList"
INST_PAR_PROP = "InstrumentParFile"
SUM_PROP = "SumSpectra"

# Raw workspace names which are necessary at the moment
SUMMED_WS, SUMMED_MON = "__loadraw_evs", "__loadraw_evs_monitors"
# Enumerate the indexes for the different foil state sums
IOUT = 0
ITHIN = 1
ITHICK = 2
# Enumerate the detector types
BACKWARD = 0
FORWARD = 1

# Instrument parameter headers. Key in the dictionary is the number of columns in the file
IP_HEADERS = {5:"spectrum,theta,t0,-,R", 6:"spectrum,-,theta,t0,-,R"}

# Child Algorithm logging
_LOGGING_ = False

class LoadVesuvio(PythonAlgorithm):

    _ws_index = None
    _spectrum_no = None
    foil_map = None
    _inst_prefix = None
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
    _goodframes = None
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

    def summary(self):
        return "Loads raw data produced by the Vesuvio instrument at ISIS."

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

        self.declareProperty(FileProperty(INST_PAR_PROP,"",action=FileAction.OptionalLoad,
                                          extensions=["dat"]),
                             doc="An optional IP file. If provided the values are used to correct "
                                 "the default instrument values and attach the t0 values to each "
                                 "detector")

        self.declareProperty(SUM_PROP, False,
                             doc="If true then the final output is a single spectrum containing "
                                 "the sum of all of the requested spectra. All detector angles/"
                                 "parameters are averaged over the individual inputs")

        self.declareProperty(WorkspaceProperty(WKSP_PROP, "", Direction.Output),
                             doc="The name of the output workspace.")

#----------------------------------------------------------------------------------------
    def PyExec(self):
        self._load_inst_parameters()
        self._retrieve_input()

        if "Difference" in self._diff_opt:
            self._exec_difference_mode()
        else:
            self._exec_single_foil_state_mode()

#----------------------------------------------------------------------------------------

    def _exec_difference_mode(self):
        """
           Execution path when a difference mode is selected
        """
        try:
            all_spectra = [item for sublist in self._spectra for item in sublist]
            self._setup_raw(all_spectra)
            self._create_foil_workspaces()

            for ws_index, spectrum_no in enumerate(all_spectra):
                self._ws_index, self._spectrum_no = ws_index, spectrum_no
                self._set_spectra_type(spectrum_no)
                self.foil_map = SpectraToFoilPeriodMap(self._nperiods)

                self._integrate_periods()
                self._sum_foil_periods()
                self._normalise_by_monitor()
                self._normalise_to_foil_out()
                self._calculate_diffs()
            # end of differencing loop

            ip_file = self.getPropertyValue(INST_PAR_PROP)
            if len(ip_file) > 0:
                self._load_ip_file(ip_file)

            if self._sumspectra:
                self._sum_all_spectra()

            self._store_results()
        finally: # Ensures it happens whether there was an error or not
            self._cleanup_raw()

#----------------------------------------------------------------------------------------

    def _exec_single_foil_state_mode(self):
        """
        Execution path when a single foil state is requested
        """
        runs = self._get_runs()
        if len(runs) > 1:
            raise RuntimeError("Single soil state mode does not currently support summing "
                               "multiple files")

        isis = config.getFacility("ISIS")
        inst_prefix = isis.instrument("VESUVIO").shortName()

        try:
            run_str = inst_prefix + runs[0]
        except ValueError:
            run_str = runs[0]

        all_spectra = [item for sublist in self._spectra for item in sublist]
        LoadRaw(Filename=run_str, OutputWorkspace=SUMMED_WS, SpectrumList=all_spectra,
                EnableLogging=_LOGGING_)
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
                raw_grp_indices = range(0, self._nperiods)
            else:
                raise RuntimeError("Unknown single foil mode: %s." % (self._diff_opt))

            dataY = foil_out.dataY(ws_index)
            dataE = foil_out.dataE(ws_index)
            for group_index in raw_grp_indices:
                dataY += raw_group[group_index].readY(ws_index)
                dataE += np.square(raw_group[group_index].readE(ws_index))
            np.sqrt(dataE, dataE)
            foil_out.setX(ws_index, x_values)

        if self._sumspectra:
            self._sum_all_spectra()

        DeleteWorkspace(Workspace=SUMMED_WS)
        self._store_results()

#----------------------------------------------------------------------------------------

    def _load_inst_parameters(self):
        """
            Loads an empty VESUVIO instrument and attaches the necessary
            parameters as attributes
        """
        isis = config.getFacility("ISIS")
        inst_name = "VESUVIO"
        inst_dir = config.getInstrumentDirectory()
        inst_file = os.path.join(inst_dir, inst_name + "_Definition.xml")
        __empty_vesuvio_ws = LoadEmptyInstrument(Filename=inst_file, EnableLogging=_LOGGING_)
        empty_vesuvio = __empty_vesuvio_ws.getInstrument()

        def to_int_list(str_param):
            """Return the list of numbers described by the string range"""
            elements = str_param.split("-")
            return range(int(elements[0]),int(elements[1]) + 1) # range goes x_l,x_h-1

        # Attach parameters as attributes
        self._inst_prefix = isis.instrument(inst_name).shortName()
        parnames = empty_vesuvio.getParameterNames(False)
        for name in parnames:
            # Irritating parameter access doesn't let you query the type
            # so resort to trying
            try:
                parvalue = empty_vesuvio.getNumberParameter(name)
            except RuntimeError:
                parvalue = empty_vesuvio.getStringParameter(name)
            setattr(self, name, parvalue[0])

        self._mon_spectra = [int(self.monitor_spectrum)]
        self._mon_index = self._mon_spectra[0] - 1

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
            return (float(elements[0]),float(elements[1]))

        self._back_mon_norm = to_range_tuple(self.backward_monitor_norm)
        self._back_period_sum1 = to_range_tuple(self.backward_period_sum1)
        self._back_period_sum2 = to_range_tuple(self.backward_period_sum2)
        self._back_foil_out_norm = to_range_tuple(self.backward_foil_out_norm)

        self._forw_mon_norm = to_range_tuple(self.forward_monitor_norm)
        self._forw_period_sum1 = to_range_tuple(self.forward_period_sum1)
        self._forw_period_sum2 = to_range_tuple(self.forward_period_sum2)
        self._forw_foil_out_norm = to_range_tuple(self.forward_foil_out_norm)

        DeleteWorkspace(__empty_vesuvio_ws,EnableLogging=_LOGGING_)

#----------------------------------------------------------------------------------------
    def _retrieve_input(self):
        self._diff_opt = self.getProperty(MODE_PROP).value

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
            self._spectra.append(numbers)
        #endfor
        self._sumspectra = self.getProperty(SUM_PROP).value

#----------------------------------------------------------------------------------------

    def _setup_raw(self, spectra):
        self._raw_grp, self._raw_monitors = self._load_and_sum_runs(spectra)
        nperiods = self._raw_grp.size()

        first_ws = self._raw_grp[0]
        self._nperiods = nperiods
        self._goodframes = first_ws.getRun().getLogData("goodfrm").value

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
        isis = config.getFacility("ISIS")
        inst_prefix = isis.instrument("VESUVIO").shortName()

        runs = self._get_runs()

        self.summed_ws, self.summed_mon = "__loadraw_evs", "__loadraw_evs_monitors"
        for index, run in enumerate(runs):
            run = inst_prefix + str(run)
            if index == 0:
                out_name, out_mon = SUMMED_WS, SUMMED_MON
            else:
                out_name, out_mon = SUMMED_WS+'tmp', SUMMED_MON + 'tmp'
            # Load data
            LoadRaw(Filename=run, SpectrumList=spectra,
                    OutputWorkspace=out_name, LoadMonitors='Exclude',EnableLogging=_LOGGING_)
            LoadRaw(Filename=run,SpectrumList=self._mon_spectra,
                    OutputWorkspace=out_mon,EnableLogging=_LOGGING_)
            if index > 0: # sum
                Plus(LHSWorkspace=SUMMED_WS, RHSWorkspace=out_name,
                     OutputWorkspace=SUMMED_WS,EnableLogging=_LOGGING_)
                Plus(LHSWorkspace=SUMMED_MON, RHSWorkspace=out_mon,
                     OutputWorkspace=SUMMED_MON,EnableLogging=_LOGGING_)
                DeleteWorkspace(out_name,EnableLogging=_LOGGING_)
                DeleteWorkspace(out_mon,EnableLogging=_LOGGING_)

        CropWorkspace(Inputworkspace= SUMMED_WS, OutputWorkspace= SUMMED_WS,
                      XMax=self._tof_max,EnableLogging=_LOGGING_)
        CropWorkspace(Inputworkspace= SUMMED_MON, OutputWorkspace= SUMMED_MON,
                      XMax=self._mon_tof_max, EnableLogging=_LOGGING_)
        return mtd[SUMMED_WS], mtd[SUMMED_MON]

#----------------------------------------------------------------------------------------

    def _get_runs(self):
        """
        Returns the runs as a list of strings
        """
        run_str = self.getProperty(RUN_PROP).value
        # Load is not doing the right thing when summing. The numbers don't look correct
        if "-" in run_str:
            lower,upper =  run_str.split("-")
            # Range goes lower to up-1 but we want to include the last number
            runs = range(int(lower),int(upper)+1)

        elif "," in run_str:
            runs =  run_str.split(",")
        else:
            runs = [run_str]

        return runs
#----------------------------------------------------------------------------------------

    def _set_spectra_type(self, spectrum_no):
        """
        Set whether this spectrum no is forward/backward scattering
        and set the normalization range appropriately
        @param spectrum_no The current spectrum no
        """
        if spectrum_no >= self._backward_spectra_list[0] and spectrum_no <= self._backward_spectra_list[-1]:
            self._spectra_type=BACKWARD
        else:
            self._spectra_type=FORWARD

        if self._spectra_type == BACKWARD:
            self._mon_norm_start, self._mon_norm_end = self._back_mon_norm
            self._period_sum1_start, self._period_sum1_end = self._back_period_sum1
            self._period_sum2_start, self._period_sum2_end = self._back_period_sum2
            self._foil_out_norm_start, self._foil_out_norm_end = self._back_foil_out_norm
        else:
            self._mon_norm_start, self._mon_norm_end = self._forw_mon_norm
            self._period_sum1_start, self._period_sum1_end = self._forw_period_sum1
            self._period_sum2_start, self._period_sum2_end = self._forw_period_sum2
            self._foil_out_norm_start, self._foil_out_norm_end = self._forw_foil_out_norm


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
        sum1_start,sum1_end = self._period_sum1_start, self._period_sum1_end
        sum2_start,sum2_end = self._period_sum2_start,self._period_sum2_end
        xvalues = self.pt_times
        sum1_indices = np.where((xvalues > sum1_start) & (xvalues < sum1_end))
        sum2_indices = np.where((xvalues > sum2_start) & (xvalues < sum2_end))

        wsindex = self._ws_index
        for i in range(self._nperiods):
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
            foil_out_periods = (2)
            foil_thin_periods = (1)
            foil_thick_periods = None
        elif self._nperiods == 3:
            foil_out_periods = (3)
            foil_thin_periods = (2)
            foil_thick_periods = (1)
        elif self._nperiods == 6:
            if self._spectra_type == BACKWARD:
                foil_out_periods = (5,6)
                foil_thin_periods = (3,4)
                foil_thick_periods = (1,2)
            else:
                foil_out_periods = (4,5,6)
                foil_thin_periods = (1,2,3)
                foil_thick_periods = (1,2)
        elif self._nperiods == 9:
            foil_out_periods = (7,8,9)
            foil_thin_periods = (4,5,6)
            foil_thick_periods = (1,2,3)
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
        raw_grp_indices = self.foil_map.get_indices(self._spectrum_no, foil_periods)
        wsindex = self._ws_index
        outY = foil_ws.dataY(wsindex)
        delta_t = self.delta_t
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
                self.getLogger().warning("No counts in %s foil spectrum %d." % (
                                          foil_type,self._spectrum_no))
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
    def _load_ip_file(self, ip_file):
        """
            If provided, load the instrument parameter file into the result
            workspace
            @param ip_file A string containing the full path to an IP file
        """
        if ip_file == "":
            raise ValueError("Empty filename string for IP file")

        ip_header = self._get_header_format(ip_file)

        # More verbose until the child algorithm stuff is sorted
        update_inst = self.createChildAlgorithm("UpdateInstrumentFromFile")
        update_inst.setLogging(_LOGGING_)
        update_inst.setProperty("Workspace", self.foil_out)
        update_inst.setProperty("Filename", ip_file)
        update_inst.setProperty("MoveMonitors", False)
        update_inst.setProperty("IgnorePhi", True)
        update_inst.setProperty("AsciiHeader", ip_header)
        update_inst.execute()

        self.foil_out = update_inst.getProperty("Workspace").value

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
            ws_out.setX(idx_out, self.foil_out.readX(idx_out))
            summed_set = self._spectra[idx_out]
            nsummed = len(summed_set)
            y_out, e_out = ws_out.dataY(idx_out), ws_out.dataE(idx_out)
            for foil_idx in range(foil_start, foil_start+nsummed):
                y_out += self.foil_out.readY(foil_idx)
                foil_err = self.foil_out.readE(foil_idx)
                e_out += foil_err*foil_err # gaussian errors
            #endfor
            np.sqrt(e_out, e_out)
            foil_start += nsummed
        #endfor
        self.foil_out = ws_out

#----------------------------------------------------------------------------------------
    def _get_header_format(self, ip_filename):
        """
            Returns the header format to be used for the given
            IP file. Currently supports 5/6 column files.
            Raises ValueError if anything other than a 5/6 column
            file is found.
            @filename ip_filename :: Full path to the IP file.
            @returns The header format string for use with UpdateInstrumentFromFile
        """
        ipfile = open(ip_filename, "r")
        first_line = ipfile.readline()
        columns = first_line.split() # splits on whitespace characters
        try:
            return IP_HEADERS[len(columns)]
        except KeyError:
            raise ValueError("Unknown format for IP file. Currently support 5/6 column "
                             "variants. ncols=%d" % (len(columns)))

#----------------------------------------------------------------------------------------
    def _store_results(self):
        """
           Sets the values of the output workspace properties
        """
        self.setProperty(WKSP_PROP, self.foil_out)

    def _cleanup_raw(self):
        """
            Clean up the raw data files
        """
        if SUMMED_WS in mtd:
            DeleteWorkspace(SUMMED_WS,EnableLogging=_LOGGING_)
        if SUMMED_MON in mtd:
            DeleteWorkspace(SUMMED_MON,EnableLogging=_LOGGING_)

#########################################################################################

class SpectraToFoilPeriodMap(object):
    """Defines the mapping between a spectrum number
    & the period index into a WorkspaceGroup for a foil state.
    """

    def __init__(self, nperiods=6):
        """Constructor. For nperiods seet up the mappings"""
        if nperiods == 2:
            self._one_to_one = {1:1, 2:2}
            self._odd_even =  {1:1, 2:3}
            self._even_odd =  {1:2, 2:4}
        elif nperiods == 3:
            self._one_to_one = {1:1, 2:2, 3:3}
            self._odd_even =   {1:1, 2:3, 3:5}
            self._even_odd =   {1:2, 2:4, 3:6}
        elif nperiods == 6:
            self._one_to_one = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6}
            self._odd_even =   {1:1, 2:3, 3:5, 4:2, 5:4, 6:6}
            self._even_odd =   {1:2, 2:4, 3:6, 4:1, 5:3, 6:5}
        elif nperiods == 9:
            self._one_to_one = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9}
            self._odd_even =   {1:1, 2:3, 3:5, 4:2, 5:4, 6:6, 7:7, 8:8, 9:9}
            self._even_odd =   {1:2, 2:4, 3:6, 4:1, 5:3, 6:5, 7:7, 8:8, 9:9}
        else:
            raise RuntimeError("Unsupported number of periods given: " + str(nperiods) +
                               ". Supported number of periods=2,3,6,9")

    def reorder(self, arr):
        """
           Orders the given array by increasing value. At the same time
           it reorders the 1:1 map to match this order
           numpy
        """
        vals = np.array(self._one_to_one.values())
        sorted_indices = arr.argsort()
        vals = vals[sorted_indices]
        arr = arr[sorted_indices]
        self._one_to_one = {}
        for index,val in enumerate(vals):
            self._one_to_one[index+1] = int(val)
        return arr

    def get_foilout_periods(self, spectrum_no):
        """Returns a list of the foil-out periods for the given
        spectrum number. Note that these start from 1 not zero
            @param spectrum_no :: A spectrum number (1->nspectra)
            @returns A list of period numbers for foil out state
        """
        return self.get_foil_periods(spectrum_no, state=0)

    def get_foilin_periods(self, spectrum_no):
        """Returns a list of the foil-out periods for the given
        spectrum number. Note that these start from 1 not zero
            @param spectrum_no :: A spectrum number (1->nspectra)
            @returns A list of period numbers for foil out state
        """
        return self.get_foil_periods(spectrum_no, state=1)

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
        elif (spectrum_no >= 135 and spectrum_no <= 142) or \
             (spectrum_no >= 151 and spectrum_no <= 158) or \
             (spectrum_no >= 167 and spectrum_no <= 174) or \
             (spectrum_no >= 183 and spectrum_no <= 190):
            foil_periods = [2,4,6] if foil_out else [1,3,5]
        else:
            foil_periods = [1,3,5] if foil_out else [2,4,6]
        return foil_periods

    def get_indices(self, spectrum_no, foil_state_numbers):
        """
        Returns a tuple of indices that can be used to access the Workspace within
        a WorkspaceGroup that corresponds to the foil state numbers given
        @param spectrum_no :: A spectrum number (1->nspectra)
        @param foil_state_no :: A number between 1 & 9(inclusive) that defines which foil
                                state is required
        @returns A tuple of indices in a WorkspaceGroup that gives the associated Workspace
        """
        indices = []
        for state in foil_state_numbers:
            indices.append(self.get_index(spectrum_no, state))
        return tuple(indices)

    def get_index(self, spectrum_no, foil_state_no):
        """Returns an index that can be used to access the Workspace within
        a WorkspaceGroup that corresponds to the foil state given
            @param spectrum_no :: A spectrum number (1->nspectra)
            @param foil_state_no :: A number between 1 & 9(inclusive) that defines which
                                        foil state is required
            @returns The index in a WorkspaceGroup that gives the associated Workspace
        """
        self._validate_foil_number(foil_state_no)
        self._validate_spectrum_number(spectrum_no)

        # For the back scattering banks or foil states > 6 then there is a 1:1 map
        if foil_state_no > 6 or spectrum_no < 135:
            foil_periods = self._one_to_one
        elif (spectrum_no >= 135 and spectrum_no <= 142) or \
             (spectrum_no >= 151 and spectrum_no <= 158) or \
             (spectrum_no >= 167 and spectrum_no <= 174) or \
             (spectrum_no >= 183 and spectrum_no <= 190):
             # foil_in = 1,3,5, foil out = 2,4,6
            foil_periods = self._odd_even
        else:
            # foil_in = 2,4,6 foil out = 1,3,5
            foil_periods = self._even_odd

        foil_period_no = foil_periods[foil_state_no]
        return foil_period_no - 1 # Minus 1 to get to WorkspaceGroup index

    def _validate_foil_number(self, foil_number):
        if foil_number < 1 or foil_number > 9:
            raise ValueError("Invalid foil state given, expected a number between "
                             "1 and 9. number=%d" % foil_number)

    def _validate_spectrum_number(self, spectrum_no):
        if spectrum_no < 1 or spectrum_no > 198:
            raise ValueError("Invalid spectrum given, expected a number between 3 "
                             "and 198. spectrum=%d" % spectrum_no)

#########################################################################################

# Registration
AlgorithmFactory.subscribe(LoadVesuvio)
