## IndirectEnergyConversionReducer class
from mantidsimple import *

from reducer import Reducer
import inelastic_indirect_reduction_steps as steps

class IndirectReducer(Reducer):
    """Reducer class for Inelastic Indirect Spectroscopy. Curently a work in
    progress.
    
    Example for use:
    >> import inelastic_indirect_reducer as iir
    >> reducer = iir.IndirectReducer()
    >> reducer.set_instrument_name('IRIS')
    >> reducer.set_parameter_file('IRIS_graphite_002_Parameters.xml')
    >> reducer.set_detector_range(2,52)
    >> reducer.append_data_file('IRS21360.raw')
    >> reducer.reduce()
    
    Will perform the same steps as the ConvertToEnergy interface does on the
    default settings (nearly).
    """
    
    _multiple_frames = False
    _instrument_name = None
    _workspace_instrument = None
    _masking_detectors = []
    _monitor_index = -1
    _detector_range_start = -1
    _detector_range_end = -1
    _rebin_string = None
    _grouping_policy = None
    _sum_files = False
    _calib_raw_files = []
    _calibration_workspace = None
    _parameter_file = None
    _background_start = None
    _background_end = None
    _detailed_balance_temp = None
    
    def __init__(self):
        """Constructor function for IndirectReducer.
        Calls base class constructor and initialises data members to known
        starting values.
        """
        super(IndirectReducer, self).__init__()
        self._multiple_frames = False
        self._instrument_name = None
        self._workspace_instrument = None
        self._monitor_index = -1
        self._detector_range_start = -1
        self._detector_range_end = -1
        self._masking_detectors = []
        self._rebin_string = None
        self._grouping_policy = None
        self._sum_files = False
        self._calib_raw_files = []
        self._calibration_workspace = None
        self._parameter_file = None
        self._background_start = None
        self._background_end = None
        self._detailed_balance_temp = None
        
    def pre_process(self):
        """Handles the loading of the data files. This is done once, at the
        beginning rather than for each individually because the user may want
        to sum their files.
        """
        # Clear current steps
        self._reduction_steps = []
        
        loadData = steps.LoadData()
        loadData.set_ws_list(self._data_files)
        loadData.set_sum(self._sum_files)
        loadData.set_monitor_index(self._monitor_index)
        loadData.set_detector_range(self._detector_range_start,
            self._detector_range_end)
        loadData.set_parameter_file(self._parameter_file)
        loadData.execute(self, None)
        
        self._multiple_frames = loadData.is_multiple_frames()
        self._masking_detectors = loadData.get_mask_list()
        
        if ( self._sum_files ):
            self._data_files = loadData.get_ws_list()
        
        if len(self._calib_raw_files) > 0:
            calib = steps.CreateCalibrationWorkspace()
            calib.set_files(self._calib_raw_files)
            calib.set_instrument_workspace(self._workspace_instrument)
            calib.set_detector_range(self._detector_range_start,
                self._detector_range_end)
            calib.execute(self, None)
            self._calibration_workspace = calib.result_workspace()
        
        # Setup the steps for the rest of the reduction, based on selected
        # user settings.
        self.setup_steps()

    def setup_steps(self):
        """Setup the steps for the reduction.
        """
        
        # "HandleMonitor" converts the monitor to Wavelength, possibly Unwraps
        step = steps.HandleMonitor(MultipleFrames=self._multiple_frames)
        self.append_step(step)
        
        # "BackgroundOperations" just does a FlatBackground at the moment,
        # will be extended for SNS stuff
        if (self._background_start is not None and
                self._background_end is not None):
            step = steps.BackgroundOperations(
                MultipleFrames=self._multiple_frames)
            step.set_range(self._background_start, self._background_end)
            self.append_step(step)
            
        # "ApplyCalibration" divides the workspace by the calibration workspace
        if self._calibration_workspace is not None:
            step = steps.ApplyCalibration()
            step.set_is_multiple_frames(self._multiple_frames)
            step.set_calib_workspace(self._calibration_workspace)
            self.append_step(step)
            
        # "CorrectByMonitor" converts the data into Wavelength, then divides by
        # the monitor workspace.
        step = steps.CorrectByMonitor(MultipleFrames=self._multiple_frames)
        self.append_step(step)
        
        # "FoldData" puts workspaces that have been chopped back together.
        if self._multiple_frames:
            self.append_step(steps.FoldData())
        
        # "ConvertToEnergy" runs ConvertUnits to DeltaE, CorrectKiKf, and
        # Rebin if a rebin string has been specified.
        step = steps.ConvertToEnergy()
        step.set_rebin_string(self._rebin_string)
        self.append_step(step)
        
        if self._detailed_balance_temp is not None:
            step = steps.DetailedBalance()
            step.set_temperature(self._detailed_balance_temp)
            self.append_step(step)
            
        step = steps.Grouping()
        step.set_grouping_policy(self._grouping_policy)
        step.set_mask_list(self._masking_detectors)
        self.append_step(step)
    
    def post_process(self):
        """Deletes the calibration workspace (if we created it).
        """
        if ( self._calibration_workspace is not None ) and ( 
                len(self._calib_raw_files) > 0 ):
            DeleteWorkspace(self._calibration_workspace)

    def set_instrument_name(self, instrument):
        self._instrument_name = instrument
        self._load_empty_instrument()
        self._get_monitor_index()
        
    def set_detector_range(self, start, end):
        self._detector_range_start = start
        self._detector_range_end = end
        
    def set_parameter_file(self, file):
        if self._instrument_name is None:
            raise NotImplementedError("Instrument name not set.")
        self._parameter_file = file
        LoadParameterFile(self._workspace_instrument, file)
        
    def set_rebin_string(self, rebin):
        self._rebin_string = rebin

    def set_grouping_policy(self, policy):
        self._grouping_policy = policy

    def set_sum(self, value):
        self._sum_files = value

    def set_calibration_workspace(self, workspace):
        self._calibration_workspace = workspace
        
    def set_background(self, start, end):
        self._background_start = start
        self._background_end = end
        
    def set_detailed_balance(self, temp):
        self._detailed_balance_temp = temp
        
    def get_result_workspaces(self):
        step = self._reduction_steps[len(self._reduction_steps)-1]
        return step.get_result_workspaces()
        
    def _load_empty_instrument(self):
        """Returns an empty workspace for the instrument.
        """
        if self._instrument_name is None:
            raise ValueError('No instrument selected.')
        self._workspace_instrument = '__empty_' + self._instrument_name
        if mtd[self._workspace_instrument] is None:
            idf_dir = mtd.getConfigProperty('instrumentDefinition.directory')
            idf = idf_dir + self._instrument_name + '_Definition.xml'
            try:
                LoadEmptyInstrument(idf, self._workspace_instrument)
            except RuntimeError:
                raise ValueError('Invalid IDF')
        return mtd[self._workspace_instrument]
        
    def _get_monitor_index(self):
        workspace = self._load_empty_instrument()
        for counter in range(0, workspace.getNumberHistograms()):
            try:
                detector = workspace.getDetector(counter)
            except RuntimeError:
                pass
            if detector.isMonitor():
                self._monitor_index = counter
                return
