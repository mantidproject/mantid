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
    >> reducer.set_detector_range(2,52)
    >> reducer.append_data_file('IRS21360.raw')
    >> reducer.reduce()
    
    Will perform the same steps as the ConvertToEnergy interface does on the
    default settings (nearly).
    """
    
    _multiple_frames = False
    _instrument_name = None
    _workspace_instrument = None
    _workspaces_monitor = []
    _workspaces_detectors = []
    _masking_detectors = []
    _monitor_index = -1
    _detector_range_start = -1
    _detector_range_end = -1
    _rebin_string = None
    _grouping_policy = None
    _sum_files = False
    
    def __init__(self):
        """Constructor function for IndirectReducer.
        Calls base class constructor and initialises data members to known
        starting values.
        """
        super(IndirectReducer, self).__init__()
        
        self._multiple_frames = False
        self._instrument_name = None
        self._workspace_instrument = None
        self._workspaces_monitor = []
        self._workspaces_detectors = []
        self._monitor_index = -1
        self._detector_range_start = -1
        self._detector_range_end = -1
        self._masking_detectors = []
        self._rebin_string = None
        self._grouping_policy = None
        self._sum_files = False
        
    def pre_process(self):
        """Handles the loading of the data files. This is done once, at the
        beginning rather than for each individually because the user may want
        to sum their files.
        """
        # Clear current steps
        self._reduction_steps = []
        
        loadData = steps.LoadData()
        loadData.set_sum(self._sum_files)
        loadData.execute(self, None)
        
        # Setup the steps for the rest of the reduction, based on selected
        # user settings.
        self.setup_steps()

    def setup_steps(self):
        """Setup the steps for the reduction.
        """
        self.append_step(steps.HandleMonitor())
        self.append_step(steps.CorrectByMonitor())
        self.append_step(steps.FoldData())
        self.append_step(steps.ConvertToEnergy())
        self.append_step(steps.Grouping())
        
    def set_instrument_name(self, instrument):
        self._instrument_name = instrument
        self._load_empty_instrument()
        
    def set_detector_range(self, start, end):
        self._detector_range_start = start
        self._detector_range_end = end
        
    def set_rebin_string(self, rebin):
        self._rebin_string = rebin

    def set_grouping_policy(self, policy):
        self._grouping_policy = policy

    def set_multiple_frames(self, value):
        self._multiple_frames = value
        
    def set_sum(self, value):
        self._sum_files = value

    def _load_empty_instrument(self):
        """Returns an empty workspace for the instrument.
        """
        if self._instrument_name is None:
            raise ValueError('No instrument selected.')
        self._workspace_instrument = '__empty_' + self._instrument_name
        if mtd[self._workspace_instrument] is None:
            idf_dir = mtd.getConfigProperty('instrumentDefinition.directory')
            idf = idf_dir + self._instrument_name + '_Definition.xml'
            print "Loading IDF for ", self._instrument_name
            try:
                LoadEmptyInstrument(idf, self._workspace_instrument)
            except RuntimeError:
                raise ValueError('Invalid IDF')
        return mtd[self._workspace_instrument]