# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-many-branches
from mantid.api import (
    AlgorithmFactory,
    AlgorithmManager,
    CommonBinsValidator,
    IEventWorkspace,
    MatrixWorkspaceProperty,
    Progress,
    PythonAlgorithm,
)
from mantid.kernel import Direction, IntArrayBoundedValidator, IntArrayProperty, Logger
import numpy as np


class SANSDarkRunBackgroundCorrection(PythonAlgorithm):
    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSDarkRunBackgroundCorrection"

    def summary(self):
        return "Correct SANS data with a dark run measurement."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", validator=CommonBinsValidator(), direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("DarkRun", "", validator=CommonBinsValidator(), direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), "The corrected SANS workspace.")
        self.declareProperty("NormalizationRatio", 1.0, "Number to scale the dark run in order" "to make it comparable to the SANS run")
        self.declareProperty("Mean", False, "If True then a mean value of all spectra is used to " "calculate the value to subtract")
        self.declareProperty("Uniform", True, "If True then we treat the treat the tim ebins a")
        self.declareProperty("ApplyToDetectors", True, "If True then we apply the correction to the detector pixels")
        self.declareProperty("ApplyToMonitors", False, "If True then we apply the correction to the monitors")

        arrvalidator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(
            IntArrayProperty("SelectedMonitors", values=[], validator=arrvalidator, direction=Direction.Input),
            "List of selected detector IDs of monitors to which the "
            "correction should be applied. If empty, all monitors will "
            "be corrected, if ApplyToMonitors has been selected.",
        )

    def PyExec(self):
        # Get the workspaces
        workspace = self.getProperty("InputWorkspace").value
        dark_run = self.getProperty("DarkRun").value

        # Provide progress reporting
        progress = Progress(self, 0, 1, 4)

        # Get other properties
        do_mean = self.getProperty("Mean").value
        do_uniform = self.getProperty("Uniform").value
        normalization_ratio = self.getProperty("NormalizationRatio").value
        progress.report("SANSDarkRunBackgroundCorrection: Preparing the dark run for background correction...")

        dark_run_normalized = None
        # Apply normalization. Uniform means here that the time over which the data was measured is uniform, there are
        # no particular spikes to be expected. In the non-uniform case we assume that it matters, when the data was taken
        if do_uniform:
            dark_run_normalized = self._prepare_uniform_correction(
                workspace=workspace, dark_run=dark_run, normalization_ratio=normalization_ratio, do_mean=do_mean
            )
        else:
            dark_run_normalized = self._prepare_non_uniform_correction(
                workspace=workspace, dark_run=dark_run, normalization_ratio=normalization_ratio
            )

        progress.report("SANSDarkRunBackgroundCorrection: Removing unwanted detectors...")
        # Remove the detectors which are unwanted
        dark_run_normalized = self._remove_unwanted_detectors_and_monitors(dark_run_normalized)

        progress.report("SANSDarkRunBackgroundCorrection: Subtracting the background...")
        # Subtract the normalizaed dark run from the SANS workspace
        output_ws = self._subtract_dark_run_from_sans_data(workspace, dark_run_normalized)

        self.setProperty("OutputWorkspace", output_ws)

    def validateInputs(self):
        issues = dict()

        # Either the detectors and/or the monitors need to be selected
        applyToDetectors = self.getProperty("ApplyToDetectors").value
        applyToMonitors = self.getProperty("ApplyToMonitors").value

        if not applyToDetectors and not applyToMonitors:
            error_msg = "Must provide either ApplyToDetectors or ApplyToMonitors or both"
            issues["ApplyToDetectors"] = error_msg

        # We only allow Workspace2D, ie not IEventWorkspaces
        ws1 = self.getProperty("InputWorkspace").value
        ws2 = self.getProperty("DarkRun").value

        if isinstance(ws1, IEventWorkspace):
            error_msg = "The InputWorkspace must be a Workspace2D."
            issues["InputWorkspace"] = error_msg

        if isinstance(ws2, IEventWorkspace):
            error_msg = "The DarkRun worksapce must be a Workspace2D."
            issues["DarkRun"] = error_msg

        return issues

    def _subtract_dark_run_from_sans_data(self, workspace, dark_run):
        # Subtract the dark_run from the workspace
        subtracted_ws_name = "_dark_run_corrected_ws"
        alg_minus = AlgorithmManager.createUnmanaged("Minus")
        alg_minus.initialize()
        alg_minus.setChild(True)
        alg_minus.setProperty("LHSWorkspace", workspace)
        alg_minus.setProperty("RHSWorkspace", dark_run)
        alg_minus.setProperty("OutputWorkspace", subtracted_ws_name)
        alg_minus.execute()
        return alg_minus.getProperty("OutputWorkspace").value

    def _prepare_non_uniform_correction(self, workspace, dark_run, normalization_ratio):
        # Make sure that the binning is the same for the scattering data and the dark run
        dark_run = self._get_cloned(dark_run)
        dark_run = self._rebin_dark_run(dark_run, workspace)
        # Scale with the normalization factor
        return self._scale_dark_run(dark_run, normalization_ratio)

    def _rebin_dark_run(self, dark_run, workspace):
        dark_run_rebin_name = "_dark_run_rebinned"
        alg_rebin = AlgorithmManager.createUnmanaged("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", dark_run)
        alg_rebin.setProperty("WorkspaceToMatch", workspace)
        alg_rebin.setProperty("OutputWorkspace", dark_run_rebin_name)
        alg_rebin.execute()
        return alg_rebin.getProperty("OutputWorkspace").value

    def _get_cloned(self, dark_run):
        dark_run_clone_name = dark_run.name() + "_cloned"
        alg_clone = AlgorithmManager.createUnmanaged("CloneWorkspace")
        alg_clone.initialize()
        alg_clone.setChild(True)
        alg_clone.setProperty("InputWorkspace", dark_run)
        alg_clone.setProperty("OutputWorkspace", dark_run_clone_name)
        alg_clone.execute()
        return alg_clone.getProperty("OutputWorkspace").value

    def _prepare_uniform_correction(self, workspace, dark_run, normalization_ratio, do_mean):
        # First we need to integrate from the dark_run. This happens in each bin
        dark_run_integrated = self._integarate_dark_run(dark_run)

        # If the mean of all detectors is required then we need to average them as well
        if do_mean:
            dark_run_integrated = self._perform_average_over_all_pixels(dark_run_integrated)

        # The workspace needs to be scaled to match the SANS data. This is done by the normalization_factor
        # In addition we need to spread the integrated signal evenly over all bins of the SANS data set.
        # Note that we assume here a workspace with common bins.
        num_bins = len(workspace.dataY(0))
        scale_factor = normalization_ratio / float(num_bins)

        return self._scale_dark_run(dark_run_integrated, scale_factor)

    def _integarate_dark_run(self, dark_run):
        """
        Sum up all bins for each pixel
        @param dark_run: a bare dark run
        @returns an integrated dark run
        """
        dark_run_integrated_name = "_dark_run_integrated"
        alg_integrate = AlgorithmManager.createUnmanaged("Integration")
        alg_integrate.initialize()
        alg_integrate.setChild(True)
        alg_integrate.setProperty("InputWorkspace", dark_run)
        alg_integrate.setProperty("OutputWorkspace", dark_run_integrated_name)
        alg_integrate.execute()
        return alg_integrate.getProperty("OutputWorkspace").value

    def _scale_dark_run(self, dark_run, scale_factor):
        """
        Scales the dark run.
        @param dark_run: The dark run to be scaled
        @param scale_factor: The scaling factor
        @returns a scaled dark run
        """
        dark_run_scaled_name = "_dark_run_scaled"
        alg_scale = AlgorithmManager.createUnmanaged("Scale")
        alg_scale.initialize()
        alg_scale.setChild(True)
        alg_scale.setProperty("InputWorkspace", dark_run)
        alg_scale.setProperty("OutputWorkspace", dark_run_scaled_name)
        alg_scale.setProperty("Operation", "Multiply")
        alg_scale.setProperty("Factor", scale_factor)
        alg_scale.execute()
        return alg_scale.getProperty("OutputWorkspace").value

    def _perform_average_over_all_pixels(self, dark_run_integrated):
        """
        At this point we expect a dark run workspace with one entry for each pixel,ie
        after integration. The average value of all pixels is calculated. This value
        replaces the current value
        @param dark_run_integrated: a dark run with integrated pixels
        @returns an averaged, integrated dark run
        """
        dark_run_summed_name = "_summed_spectra"
        alg_sum = AlgorithmManager.createUnmanaged("SumSpectra")
        alg_sum.initialize()
        alg_sum.setChild(True)
        alg_sum.setProperty("InputWorkspace", dark_run_integrated)
        alg_sum.setProperty("OutputWorkspace", dark_run_summed_name)
        alg_sum.execute()
        dark_run_summed = alg_sum.getProperty("OutputWorkspace").value

        # Get the single value out of the summed workspace and divide it
        # by the number of pixels
        summed_value = dark_run_summed.dataY(0)[0]
        num_pixels = dark_run_integrated.getNumberHistograms()
        averaged_value = summed_value / float(num_pixels)

        # Apply the averaged value to all pixels. Set values to unity. Don't
        # divide workspaces as this will alter the y unit.
        for index in range(0, dark_run_integrated.getNumberHistograms()):
            dark_run_integrated.dataY(index)[0] = 1.0
            dark_run_integrated.dataE(index)[0] = 1.0

        # Now that we have a unity workspace multiply with the unit value
        return self._scale_dark_run(dark_run_integrated, averaged_value)

    def _remove_unwanted_detectors_and_monitors(self, dark_run):
        # If we want both the monitors and the detectors, then we don't have to do anything
        applyToDetectors = self.getProperty("ApplyToDetectors").value
        applyToMonitors = self.getProperty("ApplyToMonitors").value
        selected_monitors = self.getProperty("SelectedMonitors").value

        detector_cleaned_dark_run = None
        remover = DarkRunMonitorAndDetectorRemover()
        if applyToDetectors and applyToMonitors and len(selected_monitors) == 0:
            # If the user wants everything, then we don't have to do anything here
            detector_cleaned_dark_run = dark_run
        elif applyToDetectors and not applyToMonitors:
            # We want to set the monitors to 0
            detector_cleaned_dark_run = remover.set_pure_detector_dark_run(dark_run)
        elif applyToMonitors and not applyToDetectors:
            # We want to set the detectors to 0
            detector_cleaned_dark_run = remover.set_pure_monitor_dark_run(dark_run, selected_monitors)
        elif applyToDetectors and applyToMonitors and len(selected_monitors) > 0:
            # We only want to set the detecors to 0 which are not sepecifically mentioned
            detector_cleaned_dark_run = remover.set_mixed_monitor_detector_dark_run(dark_run, selected_monitors)
        else:
            raise RuntimeError("SANSDarkRunBackgroundCorrection: Must provide either " "ApplyToDetectors or ApplyToMonitors or both")

        return detector_cleaned_dark_run


class DarkRunMonitorAndDetectorRemover(object):
    """
    This class can set detecors or monitors to 0. Either all monitors can be seletected or only
    a single one.
    """

    def __init__(self):
        super(DarkRunMonitorAndDetectorRemover, self).__init__()

    def set_pure_detector_dark_run(self, dark_run):
        """
        Sets all monitors on the dark run workspace to 0.
        @param dark_run: the dark run workspace
        """
        # Get the list of monitor workspace indices
        monitor_list = self.find_monitor_workspace_indices(dark_run)

        # Since we only have around 10 or so monitors
        # we set them manually to 0
        for ws_index, dummy_det_id in monitor_list:
            data = dark_run.dataY(ws_index)
            error = dark_run.dataE(ws_index)
            data = data * 0
            error = error * 0
            dark_run.setY(ws_index, data)
            dark_run.setE(ws_index, error)

        return dark_run

    def find_monitor_workspace_indices(self, dark_run):
        """
        Finds all monitor workspace indices
        @param dark_run: the dark run workspace
        @returns a zipped list of workspace/detids
        """
        monitor_list = []
        det_id_list = []
        # pylint: disable=bare-except
        try:
            num_histograms = dark_run.getNumberHistograms()
            spectrumInfo = dark_run.spectrumInfo()
            for index in range(0, num_histograms):
                if spectrumInfo.isMonitor(index):
                    det = dark_run.getDetector(index)
                    det_id_list.append(det.getID())
                    monitor_list.append(index)
        except:
            Logger("DarkRunMonitorAndDetectorRemover").information(
                "There was an issue when trying " "to extract the monitor list from workspace"
            )
        return list(zip(monitor_list, det_id_list))

    def set_pure_monitor_dark_run(self, dark_run, monitor_selection):
        """
        We copy the monitors, set everything to 0 and reset the monitors.
        Since there are only  a few monitors, this should not be very costly.
        @param dark_run: the dark run
        @param monitor_selection: the monitors which are selected
        @raise RuntimeError: If the selected monitor workspace index does not exist.
        """
        # Get the list of monitor workspace indices
        monitor_list = self.find_monitor_workspace_indices(dark_run)

        # Get the monitor selection
        selected_monitors = self._get_selected_monitors(monitor_selection, monitor_list)

        # Grab the monitor Y and E values
        list_dataY, list_dataE = self._get_monitor_values(dark_run, monitor_list)

        # Set everything to 0
        scale_factor = 0.0
        dark_run_scaled_name = "dark_run_scaled"

        alg_scale = AlgorithmManager.createUnmanaged("Scale")
        alg_scale.initialize()
        alg_scale.setChild(True)
        alg_scale.setProperty("InputWorkspace", dark_run)
        alg_scale.setProperty("OutputWorkspace", dark_run_scaled_name)
        alg_scale.setProperty("Operation", "Multiply")
        alg_scale.setProperty("Factor", scale_factor)
        alg_scale.execute()
        dark_run = alg_scale.getProperty("OutputWorkspace").value

        # Reset the monitors which are required. Either we reset all monitors
        # or only a specific set of monitors which was selected by the user.
        if len(selected_monitors) > 0:
            dark_run = self._set_only_selected_monitors(dark_run, list_dataY, list_dataE, monitor_list, selected_monitors)
        else:
            dark_run = self._set_all_monitors(dark_run, list_dataY, list_dataE, monitor_list)
        return dark_run

    def set_mixed_monitor_detector_dark_run(self, dark_run, monitor_selection):
        """
        We only unset the monitors which are not sepcifically listed
        @param dark_run: the dark run
        @param monitor_selection: the monitors which are selected
        @raise RuntimeError: If the selected monitor workspace index does not exist.
        """
        # Get the list of monitor workspace indices
        monitor_list = self.find_monitor_workspace_indices(dark_run)

        # Get the monitor selection
        selection = self._get_selected_monitors(monitor_selection, monitor_list)

        # Grab the monitor Y and E values
        list_dataY, list_dataE = self._get_monitor_values(dark_run, monitor_list)

        # Now set the monitors to zero and apply leave the detectors as they are
        dark_run = self.set_pure_detector_dark_run(dark_run)

        # Reset the selected monitors
        return self._set_only_selected_monitors(dark_run, list_dataY, list_dataE, monitor_list, selection)

    def _get_selected_monitors(self, monitor_selection, monitor_list):
        """
        Checks and gets the monitor selection, ie checks for sanity and removes duplicates
        @param monitor_selection: the monitors which are selected
        @param monitor_list: the list of monitors
        @raise RuntimeError: If the selected monitor workspace index does not exist.
        """
        det_id_list = []
        if len(monitor_list) != 0:
            det_id_list = list(zip(*monitor_list))[1]

        selected_monitors = []
        if len(monitor_selection) > 0:
            selected_monitors = set(monitor_selection)
            if not selected_monitors.issubset(set(det_id_list)):
                raise RuntimeError(
                    "DarkRunMonitorAndDetectorRemover: "
                    "The selected monitors are not part of the workspace. "
                    "Make sure you have selected a monitor workspace index "
                    "which is part of the workspace"
                )
        return selected_monitors

    def _get_monitor_values(self, dark_run, monitor_list):
        """
        Gets the Y and E values of the monitors of the dark run
        @param dark_run: the dark run
        @param monitor_list: the list of monitors
        @returns one array with y values and one array with e values
        """
        list_dataY = []
        list_dataE = []
        for ws_index, dummy_det_id in monitor_list:
            list_dataY.append(np.copy(dark_run.dataY(ws_index)))
            list_dataE.append(np.copy(dark_run.dataE(ws_index)))
        return list_dataY, list_dataE

    def _set_all_monitors(self, dark_run, list_dataY, list_dataE, monitor_list):
        """
        We reset all monitors back to the old values
        @param dark_run: the dark run workspace
        @param list_dataY: the old Y data
        @param list_dataE: the old E data
        @param monitor_list: a colleciton of monitors
        @returns the reset dark run workspace
        """
        counter = 0
        for ws_index, dummy_det_id in monitor_list:
            dark_run.setY(ws_index, list_dataY[counter])
            dark_run.setE(ws_index, list_dataE[counter])
            counter += 1
        return dark_run

    # pylint: disable=too-many-arguments

    def _set_only_selected_monitors(self, dark_run, list_dataY, list_dataE, monitor_list, selected_monitors):
        """
        Resets indivisual monitors
        @param dark_run: the dark run workspace
        @param list_dataY: the old Y data
        @param list_dataE: the old E data
        @param monitor_list: a colleciton of monitors
        @param selected_monitors: a collection of monitors which need to be reset
        @returns the reset dark run workspace
        """
        # The selected monitors is a detector ID, hence we need to compare it with
        # a detector ID, but we use the assoicated workspace index to correct the data
        counter = 0
        for ws_index, det_id in monitor_list:
            # Only add the data back for the specified monitors
            if det_id in selected_monitors:
                dark_run.setY(ws_index, list_dataY[counter])
                dark_run.setE(ws_index, list_dataE[counter])
            counter += 1
        return dark_run


#############################################################################################


AlgorithmFactory.subscribe(SANSDarkRunBackgroundCorrection)
