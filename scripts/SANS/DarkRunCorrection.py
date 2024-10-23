# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantid.api import AlgorithmManager


class DarkRunCorrection(object):
    """
    This class performs the dark run correction for ISIS SANS instruments
    """

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
        if mon_numbers is None:
            self._mon_numbers = []
        else:
            self._mon_numbers = mon_numbers

    def execute(self, scatter_workspace, dark_run):
        """
        Perform the dark run correction.
        @param scatter_workspace: the workspace which needs correcting
        @param dark_run: the dark run
        """
        # Get the normalization ratio from the workspaces
        normalization_ratio = self._normalization_extractor.extract_normalization(scatter_workspace, dark_run, self._use_time)
        # Run the correction algorithm with the user settings
        corrected_ws_name = scatter_workspace.name() + "_dark_workspace_corrected"
        alg_dark = AlgorithmManager.createUnmanaged("SANSDarkRunBackgroundCorrection")
        alg_dark.initialize()
        alg_dark.setChild(True)
        alg_dark.setProperty("InputWorkspace", scatter_workspace)
        alg_dark.setProperty("DarkRun", dark_run)
        alg_dark.setProperty("Mean", self._use_mean)
        alg_dark.setProperty("Uniform", self._use_time)  # If we use time, then it is uniform
        alg_dark.setProperty("NormalizationRatio", normalization_ratio)
        alg_dark.setProperty("ApplyToDetectors", self._use_detectors)
        alg_dark.setProperty("ApplyToMonitors", self._use_monitors)
        alg_dark.setProperty("SelectedMonitors", self._mon_numbers)
        alg_dark.setProperty("OutputWorkspace", corrected_ws_name)
        alg_dark.execute()

        # Make sure that we forget about the original settings
        self._reset_settings()
        return alg_dark.getProperty("OutputWorkspace").value


# pylint: disable=too-few-public-methods


class DarkRunNormalizationExtractor(object):
    """
    Extrats the normalization ratio from the scatter workspace
    and the dark run workspace depending. The normalization ratio
    can be either calculated as a ratio of good proton charges or
    a ratio of measurement times
    """

    def __init__(self):
        super(DarkRunNormalizationExtractor, self).__init__()

    def extract_normalization(self, scatter_workspace, dark_run, use_time=True):
        """
        Extract the normalization by either looking at the time duration of the measurement (good_frames)
        or by looking at the time of the good charge (good_uah_log)
        """
        if use_time:
            normalization = self._extract_normalization_from_time(scatter_workspace, dark_run)
        else:
            normalization = self._extract_normalization_from_charge(scatter_workspace, dark_run)
        return normalization

    def _extract_normalization_from_charge(self, scatter_workspace, dark_run):
        """
        We get the get the normalization ration from the gd_prtn_chrg entries.
        @param scatter_workspace: the scatter workspace
        @param dark_run: the dark run
        @returns a normalization factor for good proton charges
        """
        scatter_proton_charge = self._get_good_proton_charge(scatter_workspace)
        dark_proton_charge = self._get_good_proton_charge(dark_run)
        return scatter_proton_charge / dark_proton_charge

    def _get_good_proton_charge(self, workspace):
        """
        Get the good proton charge
        @param workspace: the workspace from which to extract
        @returns the proton charge
        """
        log_entry = "gd_prtn_chrg"
        run = workspace.getRun()
        if not run.hasProperty(log_entry):
            raise RuntimeError(
                "DarkRunCorrection: The workspace does not have a "
                + log_entry
                + "log entry. This is required for calculating the noramlization"
                "of the dark run."
            )
        entry = run.getProperty(log_entry)
        return entry.value

    def _extract_normalization_from_time(self, scatter_workspace, dark_run):
        """
        Create a normalization ratio based on the duration.
        @param scatter_workspace: the scatter workspace
        @param dark_run: the dark run
        @returns a normalization factor based on good frames
        """
        scatter_time = self._get_duration_for_frames(scatter_workspace)
        dark_time = self._get_duration_for_frames(dark_run)
        return scatter_time / dark_time

    def _get_duration_for_frames(self, workspace):
        """
        Extract the time duration from the logs.
        @param workspace: the workspace to extract from
        @returns the duration
        """
        log_entry = "good_frames"
        run = workspace.getRun()
        if not run.hasProperty(log_entry):
            raise RuntimeError(
                "DarkRunCorrection: The workspace does not have a "
                + log_entry
                + "log entry. This is required for calculating the noramlization"
                "of the dark run."
            )
        prop = run.getProperty(log_entry)
        frame_time = self._get_time_for_frame(workspace)
        number_of_frames = self._get_number_of_good_frames(prop)
        return frame_time * number_of_frames

    def _get_time_for_frame(self, workspace):
        """
        Get the time of a frame. Look into the first histogram only.
        @param workspace: the workspace from which extract the frame time
        """
        return workspace.dataX(0)[-1] - workspace.dataX(0)[0]

    def _get_number_of_good_frames(self, prop):
        """
        Get the number of good frames.
        @param prop: the property from which we extract the frames
        @returns the number of good frames
        """
        # Since we are dealing with a cumulative sample log, we can extract
        # the total number of good frames by looking at the last frame
        frames = prop.value
        return frames[-1]
