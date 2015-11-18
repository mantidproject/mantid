#pylint: disable=invalid-name
import mantid
from mantid.simpleapi import *
from mantid.kernel import time_duration

class DarkRunCorrection(object):
    '''
    This class performs the dark run correction for ISIS SANS instruments
    '''
    def __init__(self):
        super(DarkRunCorrection, self).__init__()
        self._normalization_extractor = DarkRunNormalizationExtractor()

        # Should we look at a mean value of the dark count over all pixels.
        # Only applicable if the data is uniform
        self._use_mean = False

        # Should we use time logs or uamph logs to calculat the normalization ratio.
        # In the former case we treat the dark run signal as uniform, ie constant 
        # (excpt for stat. fluctuations) over time. In the latter case it is treated
        # as non-uniform
        self._use_time = True

        # Should we use the detectors
        self._use_detectors = True

        # Should we use the monitors = False
        self._use_monitors = False

        # Which monitor numbers should be used
        self._mon_numbers = []

    def _reset_settings(self):
        self._use_mean = False
        self._use_time = True
        self._use_detectors = True
        self._use_monitors = False
        self._mon_numbers = []

    def set_use_mean(self, use_mean):
        self._use_mean = use_mean

    def set_use_time(self, use_time):
        self._use_time = use_time

    def set_use_detectors(self, use_detectors):
        self._use_detectors = use_detectors

    def set_use_monitors(self, use_monitors):
        self._use_monitors = use_monitors

    def set_mon_numbers(self, mon_numbers):
        self._mon_numbers = mon_numbers

    def execute(self, scatter_workspace, dark_run):
        '''
        Perform the dark run correction.
        @param scatter_workspace: the workspace which needs correcting
        @param dark_run: the dark run
        '''
        # Get the normalization ratio from the workspaces
        normalization_ratio = self._normalization_extractor.extract_normalization(scatter_workspace,
                                                                                  dark_run, self._use_time)

        # Run the correction algorithm with the user settings
        corrected_ws_name = scatter_workspace.name() + "_dark_workspace_corrected"
        alg_dark = AlgorithmManager.create("SANSDarkRunBackgroundCorrection")
        alg_dark.initialize()
        alg_dark.setChild(True)
        alg_dark.setProperty("InputWorkspace", scatter_workspace)
        alg_dark.setProperty("DarkRun", dark_run)
        alg_dark.setProperty("Mean", self._use_mean)
        alg_dark.setProperty("Uniform", self._use_time) # If we use time, then it is uniform
        alg_dark.setProperty("NormalizationRatio", normalization_ratio)
        alg_dark.setProperty("ApplyToDetectors", self._use_detectors)
        alg_dark.setProperty("ApplyToMonitors", self._use_monitors)
        alg_dark.setProperty("SelectedMonitors", self._mon_numbers)
        alg_dark.setProperty("OutputWorkspace", corrected_ws_name)
        alg_dark.execute()

        # Make sure that we forget about the original settings
        self._reset_settings()
        return alg_dark.getProperty("OutputWorkspace").value

class DarkRunNormalizationExtractor(object):
    '''
    Extrats the normalization ratio from the scatter workspace
    and the dark run workspace depending. The normalization ratio
    can be either calculated as a ratio of good proton charges or
    a ratio of measurement times
    '''
    def __init__(self):
        super(DarkRunNormalizationExtractor, self).__init__()

    def extract_normalization(self, scatter_workspace, dark_run, use_time = True):
        '''
        Extract the normalization by either looking at the time duration of the measurement (good_frames)
        or by looking at the time of the good charge (good_uah_log)
        '''
        log_entry = None
        if use_time:
            log_entry = "good_frames"
        else:
            log_entry = "good_uah_log"
        return self._extract_normalization(scatter_workspace, dark_run, log_entry)

    def _extract_normalization(self, scatter_workspace, dark_run, log_entry):
        '''
        Create a normalization ratio based on the duration.
        '''
        scatter_duration = self._get_time_duration_from_logs(scatter_workspace, log_entry)
        dark_run_duration = self._get_time_duration_from_logs(dark_run, log_entry)
        return scatter_duration/dark_run_duration

    def _get_time_duration_from_logs(self, workspace, log_entry):
        '''
        Extract the time duration from the logs.
        '''
        run = workspace.getRun()
        if not run.hasProperty(log_entry):
            raise RuntimeError("DarkRunCorrection: The workspace does not have a " + log_entry +
                               "log entry. This is required for calculating the noramlization"
                               "of the dark run.")

        entry = run.getProperty(log_entry)
        first_time = entry.firstTime()
        last_time = entry.lastTime()
        return time_duration.total_nanoseconds(last_time- first_time)/1e9
