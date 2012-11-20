"""*WIKI* 

A Workflow algorithm to load the data from the VESUVIO instrument at
ISIS.
*WIKI*"""

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (CropWorkspace, LoadEmptyInstrument, LoadRaw, Plus, 
                              DeleteWorkspace)


import numpy as np
import os

RUN_PROP = "RunNumbers"
WKSP_PROP = "OutputWorkspace"
DIFF_PROP = "DifferenceType"
DIFF_PROP_VALUES = ["Single", "Double", "Thick"]
SPECTRA_PROP = "SpectrumList"

# Enumerate the indexes for the different foil state sums
IOUT = 0
ITHIN = 1
ITHICK = 2
# Enumerate the detector types
BACKWARD = 0
FORWARD = 1

# Sub algorithm logging
_LOGGING_ = False

class LoadVesuvio(PythonAlgorithm):
    
    def PyInit(self):
        self.declareProperty(RUN_PROP, "", StringMandatoryValidator(),
                             doc="The run numbers that should be loaded. E.g."
                                 "14188  - for single run"
                                 "14188-14195 - for summed consecutive runs"
                                 "14188,14195 - for summed non-consecutive runs")
        
        self.declareProperty(WorkspaceProperty(WKSP_PROP, "", Direction.Output), 
                             doc="The name of the output workspace.")
        
        self.declareProperty(DIFF_PROP, DIFF_PROP_VALUES[1], StringListValidator(DIFF_PROP_VALUES),
                             doc="The difference option. Valid values: Single, Double, Thick")
        
        self.declareProperty(IntArrayProperty(SPECTRA_PROP, IntArrayMandatoryValidator(),
                                              Direction.Input), 
                             doc="The spectrum numbers to load")

#----------------------------------------------------------------------------------------
    def PyExec(self):
        self._load_inst_parameters()
        self._retrieve_input()

        self._setup_raw(self._spectra)
        self._create_foil_workspaces()

        for ws_index, spectrum_no in enumerate(self._spectra):
            self.foil_map = SpectraToFoilPeriodMap(self._nperiods,self._mon_spectra)
            self._integrate_periods(ws_index)
            self._sum_foil_periods(ws_index, spectrum_no)
            self._apply_dead_time_correction(ws_index, spectrum_no)
            self._normalise_by_monitor(ws_index)
            self._normalise_to_foil_out(ws_index,spectrum_no)
            self._calculate_diffs(ws_index)

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
        self._tau = self.dead_time_fraction
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
        self._diff_opt = self.getProperty(DIFF_PROP).value
        self._spectra = self.getProperty(SPECTRA_PROP).value
        self._spectra_type = self._check_no_bank_overlap(self._spectra)
        
        # Get the correct normalization ranges for this run
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
    def _check_no_bank_overlap(self, spectra):
        """
            Runs a sanity check to ensure that the spectra are from either the
            back-scattering banks or the forward-scattering banks and not a mix.
            Also verifies that the spectra are not just monitors
            @param spectra :: A list of spectra numbers that will be loaded
            @returns An enumeration specifying which set of detectors
        """
        npspectra = np.array(spectra)
        npspectra.sort()
        in_back = np.all((npspectra >= self._backward_spectra_list[0]) & (npspectra <= self._backward_spectra_list[-1]))
        if in_back:
            return BACKWARD
        else:
            in_forward = np.all((npspectra >= self._forward_spectra_list[0]) & (npspectra <= self._forward_spectra_list[-1]))
            if in_forward:
                return FORWARD
            else:
                raise RuntimeError("Invalid spectra range. There is an overlap of the forward/backward spectra ranges")
#----------------------------------------------------------------------------------------

    def _setup_raw(self, spectra):
        self._raw_grp, self._raw_monitors = self._load_and_sum_runs(spectra)
        nperiods = self._raw_grp.size()
        # Sanity check the number of periods
        if self._spectra_type == BACKWARD:
            if nperiods not in (2,3,6,9):
                raise RuntimeError("Invalid number of periods for backward scattering spectra. nperiods=%d" %(nperiods))
        else:
            if nperiods != 6:
                 raise RuntimeError("Invalid number of periods for forward scattering spectra. nperiods=%d" %(nperiods))
        
        self._nperiods = nperiods
        self._goodframes = self._raw_grp[0].getRun().getLogData("goodfrm").value
        
        # X arrays won't change so store a copy of them
        raw_t = self._raw_grp[0].readX(0)
        self.pt_times = 0.5*(raw_t[1:] + raw_t[:-1])
        self.delta_t = raw_t[1:] - raw_t[:-1] # Numpy does difference between successive elements
        self.raw_x = raw_t # For the final workspace
        
        mon_raw_t = self._raw_monitors[0].readX(0)
        self.mon_pt_times = 0.5*(mon_raw_t[1:] + mon_raw_t[:-1])
        self.delta_tmon = mon_raw_t[1:] - mon_raw_t[:-1]

#----------------------------------------------------------------------------------------
    def _load_and_sum_runs(self, spectra):
        """Load the input set of runs & sum them if there
        is more than one.
            @param spectra :: The list of spectra to load
            @returns a tuple of length 2 containing (main_detector_ws, monitor_ws)
        """
        isis = config.getFacility("ISIS")
        inst_prefix = isis.instrument("VESUVIO").shortName()

        run_str = self.getProperty(RUN_PROP).value
        # Load is not doing the right thing when summing. The numbers don't look correct
        if "-" in run_str:
            lower,upper =  run_str.split("-")
            runs = range(int(lower),int(upper)+1) #range goes lower to up-1 but we want to include the last number
            
        elif "," in run_str:
            runs =  run_str.split(",")
        else:
            runs = [run_str]

        self.summed_ws, self.summed_mon = "__loadraw_evs", "__loadraw_evs_monitors"
        for index, run in enumerate(runs):
            run = inst_prefix + str(run)
            if index == 0:
                out_name, out_mon = self.summed_ws, self.summed_mon
            else:
                out_name, out_mon = self.summed_ws+'tmp', self.summed_mon + 'tmp'
            # Load data
            LoadRaw(Filename=run, SpectrumList=spectra, 
                    OutputWorkspace=out_name, LoadMonitors='Exclude',EnableLogging=_LOGGING_)
            LoadRaw(Filename=run,SpectrumList=self._mon_spectra, 
                    OutputWorkspace=out_mon,EnableLogging=_LOGGING_)
            if index > 0: # sum
                Plus(LHSWorkspace=self.summed_ws, RHSWorkspace=out_name,
                     OutputWorkspace=self.summed_ws,EnableLogging=_LOGGING_)
                Plus(LHSWorkspace=self.summed_mon, RHSWorkspace=out_mon,
                     OutputWorkspace=self.summed_mon,EnableLogging=_LOGGING_)
                DeleteWorkspace(out_name,EnableLogging=_LOGGING_)
                DeleteWorkspace(out_mon,EnableLogging=_LOGGING_)

        CropWorkspace(Inputworkspace= self.summed_ws, OutputWorkspace= self.summed_ws, 
                      XMax=self._tof_max,EnableLogging=_LOGGING_)
        CropWorkspace(Inputworkspace= self.summed_mon, OutputWorkspace= self.summed_mon,
                      XMax=self._mon_tof_max,EnableLogging=_LOGGING_)
        return mtd[self.summed_ws], mtd[self.summed_mon]

#----------------------------------------------------------------------------------------
    def _integrate_periods(self, ws_index):
        """
            Calculates 2 arrays of sums, 1 per period, of the Y values from 
            the raw data between:
               (a) period_sum1_start & period_sum1_end 
               (b) period_sum2_start & period_sum2_end. 
            It also creates a 3rd blank array that will be filled by calculate_foil_counts_per_us
            @param ws_index :: The current workspace index
        """
        self.sum1 = np.zeros(self._nperiods)
        self.sum2 = np.zeros(self._nperiods)
        self.sum3 = np.zeros(3)
        sum1_start,sum1_end = self._period_sum1_start, self._period_sum1_end
        sum2_start,sum2_end = self._period_sum2_start,self._period_sum2_end
        xvalues = self.pt_times
        for i in range(self._nperiods):
            yvalues = self._raw_grp[i].readY(ws_index)
            self.sum1[i] = np.sum(yvalues[(xvalues > sum1_start) & (xvalues < sum1_end)])
            self.sum2[i] = np.sum(yvalues[(xvalues > sum2_start) & (xvalues < sum2_end)])
            if self.sum2[i] != 0.0:
                self.sum1[i] /= self.sum2[i]

        # Sort sum1 in increasing order and match the foil map
        self.sum1 = self.foil_map.reorder(self.sum1)

#----------------------------------------------------------------------------------------

    def _create_foil_workspaces(self):
        """
            Create the workspaces that will hold the foil out, thin & thick results
        """
        first_ws = self._raw_grp[0]
        self.foil_out = WorkspaceFactory.create(first_ws) # This will be used as the result workspace
        self.foil_out.setDistribution(True)
        self.foil_thin = WorkspaceFactory.create(first_ws)
        first_monws = self._raw_monitors[0]
        self.mon_out = WorkspaceFactory.create(first_monws, NVectors=first_ws.getNumberHistograms())
        self.mon_thin = WorkspaceFactory.create(first_monws,NVectors=first_ws.getNumberHistograms())

        if self._nperiods == 2:
            self.foil_thick = None
            self.mon_thick = None
        else:
            self.foil_thick = WorkspaceFactory.create(first_ws)
            self.mon_thick = WorkspaceFactory.create(first_monws,NVectors=first_ws.getNumberHistograms())

#----------------------------------------------------------------------------------------
    
    def _sum_foil_periods(self, ws_index, spectrum_no):
        """
        Sums the counts in the different periods to get the total counts
        for the foil out, thin foil & thick foil states for the given workspace index. The workspaces
        are assumed to have been created. NOTE: There is also an implicit conversion to counts/microsecond
            @param ws_index :: The current workspace index
            @param spectrum_no :: Current spectrum number
        """
        if self._nperiods == 2:
            return self._sum_two_periods(ws_index, spectrum_no)
        elif self._nperiods == 6:
            return self._sum_six_periods(ws_index, spectrum_no)
        elif self._nperiods == 9:
            return self._sum_nine_periods(ws_index, spectrum_no)

#----------------------------------------------------------------------------------------
    def _sum_six_periods(self, ws_index, spectrum_no):
        """
            Use the 6 workspaces within the group to calculate  the counts/microsecond for each foil state
            
            @param ws_index :: The current workspace index
            @param spectrum_no :: Current spectrum number
        """
        if self._spectra_type == BACKWARD:
            self._sum_six_periods_backward(ws_index, spectrum_no)
        else:
            self._sum_six_periods_forward(ws_index, spectrum_no)

    def _sum_six_periods_backward(self, ws_index, spectrum_no):
        """
            Use the 6 workspaces within the group to calculate  the counts/microsecond for each foil state
            for the back scattering detectors
            @param ws_index :: The current workspace index
            @param spectrum_no :: Current spectrum number
        """
        # Foil out
        foil_out_index1, foil_out_index2 = self.foil_map.get_indices(spectrum_no,(5,6))
        foil_out_ws1, foil_out_ws2 = self._raw_grp[foil_out_index1], self._raw_grp[foil_out_index2]
        cout = (foil_out_ws1.readY(ws_index) + foil_out_ws2.readY(ws_index))/self.delta_t
        errout = np.sqrt((foil_out_ws1.readE(ws_index) + foil_out_ws2.readE(ws_index)))/self.delta_t
        self.foil_out.setY(ws_index, cout)
        self.foil_out.setE(ws_index, errout)
        # Sum of foil periods
        self.sum3[IOUT] = self.sum2[foil_out_index1] + self.sum2[foil_out_index2]
        # monitors
        foil_out_mon1, foil_out_mon2 = self._raw_monitors[foil_out_index1], self._raw_monitors[foil_out_index2]
        mout = (foil_out_mon1.readY(self._mon_index) + foil_out_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_out.setY(ws_index, mout)

        # Foil thin
        foil_thin_index1, foil_thin_index2 = self.foil_map.get_indices(spectrum_no,(3,4))
        foil_thin_ws1, foil_thin_ws2 = self._raw_grp[foil_thin_index1], self._raw_grp[foil_thin_index2]
        cthin = (foil_thin_ws1.readY(ws_index) + foil_thin_ws2.readY(ws_index))/self.delta_t
        errthin = np.sqrt((foil_thin_ws1.readE(ws_index) + foil_thin_ws2.readE(ws_index)))/self.delta_t
        self.foil_thin.setY(ws_index, cthin)
        self.foil_thin.setE(ws_index, errthin)
        # Sum of foil periods
        self.sum3[ITHIN] = self.sum2[foil_thin_index1] + self.sum2[foil_thin_index2]
        # monitors
        foil_thin_mon1, foil_thin_mon2 = self._raw_monitors[foil_thin_index1], self._raw_monitors[foil_thin_index2]
        mthin = (foil_thin_mon1.readY(self._mon_index) + foil_thin_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_thin.setY(ws_index, mthin)

        # Foil thick
        foil_thick_index1, foil_thick_index2 = self.foil_map.get_indices(spectrum_no,(3,4))
        foil_thick_ws1, foil_thick_ws2 = self._raw_grp[foil_thick_index1], self._raw_grp[foil_thick_index2]
        cthick = (foil_thick_ws1.readY(ws_index) + foil_thick_ws2.readY(ws_index))/self.delta_t
        errthick = np.sqrt((foil_thick_ws1.readE(ws_index) + foil_thick_ws2.readE(ws_index)))/self.delta_t
        self.foil_thick.setY(ws_index, cthick)
        self.foil_thick.setE(ws_index, errthick)
        # Sum foil period counts
        self.sum3[ITHICK] = self.sum2[foil_thick_index1] + self.sum2[foil_thick_index2]
        # monitors
        foil_thick_mon1, foil_thick_mon2 = self._raw_monitors[foil_thick_index1], self._raw_monitors[foil_thick_index2]
        mthick = (foil_thick_mon1.readY(self._mon_index) + foil_thick_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_thick.setY(ws_index, mthick)

    def _sum_six_periods_forward(self, ws_index, spectrum_no):
        """
            Use the 6 workspaces within the group to calculate  the counts/microsecond for each foil state
            for the forward scattering detectors
            @param ws_index :: The current workspace index
            @param spectrum_no :: Current spectrum number
        """
        # Foil out
        foil_out_index1, foil_out_index2, foil_out_index3 = self.foil_map.get_indices(spectrum_no,(4,5,6))
        foil_out_ws1, foil_out_ws2, foil_out_ws3 = self._raw_grp[foil_out_index1], self._raw_grp[foil_out_index2], self._raw_grp[foil_out_index3]
        cout = (foil_out_ws1.readY(ws_index) + foil_out_ws2.readY(ws_index) + foil_out_ws3.readY(ws_index))/self.delta_t
        errout = np.sqrt((foil_out_ws1.readE(ws_index) + foil_out_ws2.readE(ws_index) + foil_out_ws3.readE(ws_index)))/self.delta_t
        self.foil_out.setY(ws_index, cout)
        self.foil_out.setE(ws_index, errout)
        # Sum of foil periods
        self.sum3[IOUT] = self.sum2[foil_out_index1] + self.sum2[foil_out_index2] + self.sum2[foil_out_index3]
        # monitors
        foil_out_mon1, foil_out_mon2 = self._raw_monitors[foil_out_index2], self._raw_monitors[foil_out_index3]
        mout = (foil_out_mon1.readY(self._mon_index) + foil_out_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_out.setY(ws_index, mout)

        # Foil thin
        foil_thin_index1, foil_thin_index2,foil_thin_index3 = self.foil_map.get_indices(spectrum_no,(1,2,3))
        foil_thin_ws1, foil_thin_ws2, foil_thin_ws3 = self._raw_grp[foil_thin_index1], self._raw_grp[foil_thin_index2], self._raw_grp[foil_thin_index3]
        cthin = (foil_thin_ws1.readY(ws_index) + foil_thin_ws2.readY(ws_index) + foil_thin_ws3.readY(ws_index))/self.delta_t
        errthin = np.sqrt((foil_thin_ws1.readE(ws_index) + foil_thin_ws2.readE(ws_index) + foil_thin_ws3.readE(ws_index)))/self.delta_t
        self.foil_thin.setY(ws_index, cthin)
        self.foil_thin.setE(ws_index, errthin)
        # Sum of foil periods
        self.sum3[ITHIN] = self.sum2[foil_thin_index1] + self.sum2[foil_thin_index2] + self.sum2[foil_thin_index3]
        # monitors
        foil_thin_index1, foil_thin_index2 = self.foil_map.get_indices(spectrum_no,(3,4))
        foil_thin_mon1, foil_thin_mon2 = self._raw_monitors[foil_thin_index1], self._raw_monitors[foil_thin_index2]
        mthin = (foil_thin_mon1.readY(self._mon_index) + foil_thin_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_thin.setY(ws_index, mthin)

        # Foil thick
        foil_thick_index1, foil_thick_index2 = self.foil_map.get_indices(spectrum_no,(1,2))
        foil_thick_ws1, foil_thick_ws2 = self._raw_grp[foil_thick_index1], self._raw_grp[foil_thick_index2]
        cthick = (foil_thick_ws1.readY(ws_index) + foil_thick_ws2.readY(ws_index))/self.delta_t
        errthick = np.sqrt((foil_thick_ws1.readE(ws_index) + foil_thick_ws2.readE(ws_index)))/self.delta_t
        self.foil_thick.setY(ws_index, cthick)
        self.foil_thick.setE(ws_index, errthick)
        # Sum foil period counts
        self.sum3[ITHICK] = self.sum2[foil_thick_index1] + self.sum2[foil_thick_index2]
        # monitors
        foil_thick_mon1, foil_thick_mon2 = self._raw_monitors[foil_thick_index1], self._raw_monitors[foil_thick_index2]
        mthick = (foil_thick_mon1.readY(self._mon_index) + foil_thick_mon2.readY(self._mon_index))/self.delta_tmon
        self.mon_thick.setY(ws_index, mthick)

#----------------------------------------------------------------------------------------
    def _apply_dead_time_correction(self, ws_index, spectrum_no):
        """
            Corrects the count arrays for each foil state for dead time in the detectors.
            @param ws_index :: The current ws_index being processed
            @param spectrum_no :: The current spectrum_no (used for error msg)
        """
        # Calculate number of frames
        sum3_total = self.sum3[0] + self.sum3[1] + self.sum3[2]
        nframes = np.ones(3)
        if sum3_total != 0.0:
            for i in range(3):
                nframes[i] = self._goodframes*self.sum3[i]/sum3_total
                if nframes[i] == 0.0:
                    self.getLogger().warning("nframes=0 for period %d in spectrum %d. Using nframes=1" % (i, spectrum_no))
        else:
            self.getLogger().warning("Sum of counts across all foil periods = 0 for spectrum %d. "
                                     "Using nframes=1 for all periods in this spectrum" % spectrum_no)
        
        def dead_time_correct(foil_ws, period):
            """Correct for dead time"""
            for arr_func in (foil_ws.dataY, foil_ws.dataE):
                values = arr_func(ws_index)
                values /= nframes[period]
                values /= (1.0 - self._tau*values)
                values *= nframes[period]

        dead_time_correct(self.foil_out, IOUT)
        dead_time_correct(self.foil_thin, ITHIN)
        if self._nperiods != 2:
            dead_time_correct(self.foil_thick, ITHICK)

#----------------------------------------------------------------------------------------
    def _normalise_by_monitor(self, ws_index):
        """
            Normalises by the monitor counts between mon_norm_start & mon_norm_end
            instrument parameters
            @param ws_index :: The current workspace index being processed
        """
        mon_range_indices = ((self.mon_pt_times >= self._mon_norm_start) & (self.mon_pt_times < self._mon_norm_end))
        def monitor_normalization(foil_ws, mon_ws):
            """Applies monitor normalization to the given foil spectrum from the given monitor spectrum
            """
            mon_values = mon_ws.readY(ws_index)
            mon_values_sum = np.sum(mon_values[mon_range_indices])
            foil_state = foil_ws.dataY(ws_index)
            foil_state /= (self._mon_scale/mon_values_sum)
            err = foil_ws.dataE(ws_index)
            err *= (self._mon_scale/mon_values_sum)

        monitor_normalization(self.foil_out, self.mon_out)
        monitor_normalization(self.foil_thin, self.mon_thin)
        if self._nperiods != 2:
            monitor_normalization(self.foil_thick, self.mon_thick)

    def _normalise_to_foil_out(self, ws_index, spectrum_no):
        """
            Normalises the thin/thick foil counts to the 
            foil out counts between (foil_out_norm_start,foil_out_norm_end)
            @param ws_index :: The current ws_index being processed
            @param spectrum_no :: The current spectrum_no (used for error msg)
        """
        # Indices where the given condition is true
        range_indices = ((self.pt_times >= self._foil_out_norm_start) & (self.pt_times < self._foil_out_norm_end))
        cout = self.foil_out.readY(ws_index)
        sum_out = np.sum(cout[range_indices])

        def normalise_to_out(foil_ws, foil_type):
            values = foil_ws.dataY(ws_index)
            sum_values = np.sum(values[range_indices])
            if sum_values == 0.0:
                self.getLogger().warning("No counts in %s foil spectrum %d." % (foil_type,spectrum_no))
                sum_values = 1.0
            values *= (sum_out/sum_values)
        
        normalise_to_out(self.foil_thin, "thin")
        if self._nperiods != 2:
            normalise_to_out(self.foil_thick, "thick")
#----------------------------------------------------------------------------------------

    def _calculate_diffs(self, ws_index):
        """
            Based on the DifferenceType property, calculate the final output
            spectra
            @param ws_index :: The current workspace index
        """
        if self._diff_opt == 1:
            self._calculate_thin_difference(ws_index)
        elif self._diff_opt == 2:
            self._calculate_double_difference(ws_index)
        else:
            self._calculate_thick_difference(ws_index)

        self.foil_out.setX(ws_index, self.raw_x)
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
        np.sqrt((one_min_beta*eout)**2 + ethin**2 + (self._beta**2)*ethick**2, eout) # The second argument makes it happen in place

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
    def _store_results(self):
        """
           Sets the values of the output workspace properties
        """
        DeleteWorkspace(str(self._raw_grp),EnableLogging=_LOGGING_)
        DeleteWorkspace(str(self._raw_monitors),EnableLogging=_LOGGING_)
        self.setProperty(WKSP_PROP, self.foil_out)

#########################################################################################

class SpectraToFoilPeriodMap(object):
    """Defines the mapping between a spectrum number
    & the period index into a WorkspaceGroup for a foil state.
    """
    
    def __init__(self, nperiods, mon_spectra):
        """Constructor. For nperiods seet up the mappings"""
        self._mon_spectra = mon_spectra
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

    def get_indices(self, spectrum_no, foil_state_numbers):
        """Returns a tuple of indices that can be used to access the Workspace within
        a WorkspaceGroup that corresponds to the foil state numbers given
            @param spectrum_no :: A spectrum number (1->nspectra)
            @param foil_state_no :: A number between 1 & 9(inclusive) that defines which
                                        foil state is required
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
            raise ValueError("Invalid foil state given, expected a number between 1 and 9. number=%d" % foil_number)
        
    def _validate_spectrum_number(self, spectrum_no):
        if spectrum_no in self._mon_spectra:
            raise ValueError("Invalid spectrum given. Spectrum is a monitor not a detector spectrum! spectrum=%d" % spectrum_no)
        if spectrum_no < 1 or spectrum_no > 198:
            raise ValueError("Invalid spectrum given, expected a number between 3 and 198. spectrum=%d" % spectrum_no)
       
#########################################################################################

# Registration
registerAlgorithm(LoadVesuvio)