#pylint: disable=no-init,invalid-name,too-many-locals,too-many-branches
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

class SANSDarkRunBackgroundCorrection(PythonAlgorithm):

    def category(self):
        return "Workflow\\SANS\\UsesPropertyManager"

    def name(self):
        return "SANSDarkRunBackgroundCorrection"

    def summary(self):
        return "Correct SANS data with a dark run measurement."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     validator=CommonBinsValidator(),
                                                     direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("DarkRun", "",
                                                     validator=CommonBinsValidator(),
                                                     direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output),
                                                     "The corrected SANS workspace.")
        self.declareProperty("NormalizationRatio", 1.0, "Number to scale the dark run in order"
                                                       "to make it comparable to the SANS run")
        self.declareProperty("Mean", False, "If True then a mean value of all spectra is used to" 
                                             "calculate the value to subtract")
        self.declareProperty("Uniform", True, "If True then we treat the treat the tim ebins a")
        self.declareProperty("ApplyToDetectors", True, "If True then we apply the correction to the detector pixels")
        self.declareProperty("ApplyToMonitors", False, "If True then we apply the correction to the monitors")

    def PyExec(self):
        # Get the workspaces
        workspace = self.getProperty("InputWorkspace").value
        dark_run = self.getProperty("DarkRun").value
        output_ws_name = self.getPropertyValue("OutputWorkspace")

        # Get other properties
        do_mean = self.getProperty("Mean").value
        do_uniform = self.getProperty("Uniform").value
        normalization_ratio = self.getProperty("NormalizationRatio").value

        dark_run_normalized = None
        # Apply normalization. Uniform means here that the time over which the data was measured is uniform, there are
        # no particular spikes to be expected. In the non-uniform case we assume that it matters, when the data was taken
        if do_uniform:
            dark_run_normalized = self._prepare_uniform_correction(workspace = workspace,
                                                                   dark_run = dark_run,
                                                                   normalization_ratio = normalization_ratio,
                                                                   do_mean = do_mean)
        else:
            dark_run_normalized = self._prepare_non_uniform_correction(workspace = workspace,
                                                                       dark_run = dark_run,
                                                                       normalization_ratio = normalization_ratio)

        # Subtract the normalizaed dark run from the SANS workspace
        output_ws = self._subtract_dark_run_from_sans_data(workspace, dark_run_normalized)

        self.setProperty("OutputWorkspace", output_ws)

    def validateInputs(self):
        issues = dict()

        # Either the detectors and/or the monitors need to be selected
        applyToDetectors = self.getProperty("ApplyToDetectors").value
        applyToMonitors = self.getProperty("ApplyToMonitors").value

        if not applyToDetectors and not applyToMonitors:
            error_msg = 'Must provide either ApplyToDetectors or ApplyToMonitors or both'
            issues['ApplyToDetectors'] = error_msg

    def _subtract_dark_run_from_sans_data(self, workspace, dark_run):
        # Subtract the dark_run from the workspace
        subtracted_ws_name = "_dark_run_corrected_ws"
        alg_minus = AlgorithmManager.create("Minus")
        alg_minus.initialize()
        alg_minus.setChild(True)
        alg_minus.setProperty("LHSWorkspace", workspace)
        alg_minus.setProperty("RHSWorkspace", dark_run)
        alg_minus.setProperty("OutputWorkspace", subtracted_ws_name)
        alg_minus.execute()
        ws = alg_minus.getProperty("OutputWorkspace").value
        return alg_minus.getProperty("OutputWorkspace").value

    def _prepare_non_uniform_correction(self, workspace, dark_run, normalization_ratio):
        # Make sure that the binning is the same for the scattering data and the dark run
        dark_run_clone_name = dark_run.name() + "_cloned"
        alg_clone = AlgorithmManager.create("CloneWorkspace")
        alg_clone.initialize()
        alg_clone.setChild(True)
        alg_clone.setProperty("InputWorkspace", dark_run)
        alg_clone.setProperty("OutputWorkspace", dark_run_clone_name)
        alg_clone.execute()
        dark_run_clone = alg_clone.getProperty("OutputWorkspace").value

        # Remove the detectors which are unwanted
        dark_run_monitor_corrected = self._remove_unwanted_detectors(dark_run_clone)

        dark_run_rebin_name = "_dark_run_rebinned"
        alg_rebin = AlgorithmManager.create("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", dark_run_clone)
        alg_rebin.setProperty("WorkspaceToMatch", workspace)
        alg_rebin.setProperty("OutputWorkspace", dark_run_rebin_name)
        alg_rebin.execute()
        dark_run_rebinned = alg_rebin.getProperty("OutputWorkspace").value

        # Scale with the normalization factor
        return self._scale_dark_run(dark_run_rebinned, normalization_ratio)

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
        scale_factor = normalization_ratio/float(num_bins)

        return self._scale_dark_run(dark_run_integrated, scale_factor)

    def _integarate_dark_run(self, dark_run):
        '''
        Sum up all bins for each pixel
        @param dark_run: a bare dark run
        @returns an integrated dark run
        '''
        dark_run_integrated_name = "_dark_run_integrated"
        alg_integrate = AlgorithmManager.create("Integration")
        alg_integrate.initialize()
        alg_integrate.setChild(True)
        alg_integrate.setProperty("InputWorkspace", dark_run)
        alg_integrate.setProperty("OutputWorkspace", dark_run_integrated_name)
        alg_integrate.execute()
        return alg_integrate.getProperty("OutputWorkspace").value

    def _scale_dark_run(self, dark_run, scale_factor):
        '''
        Scales the dark run.
        @param dark_run: The dark run to be scaled
        @param scale_factor: The scaling factor
        @returns a scaled dark run
        '''
        dark_run_scaled_name = "_dark_run_scaled"
        alg_scale  = AlgorithmManager.create("Scale")
        alg_scale.initialize()
        alg_scale.setChild(True)
        alg_scale.setProperty("InputWorkspace", dark_run)
        alg_scale.setProperty("OutputWorkspace", dark_run_scaled_name)
        alg_scale.setProperty("Operation", "Multiply")
        alg_scale.setProperty("Factor", scale_factor)
        alg_scale.execute()
        return alg_scale.getProperty("OutputWorkspace").value

    def _perform_average_over_all_pixels(self, dark_run_integrated):
        '''
        At this point we expect a dark run workspace with one entry for each pixel,ie
        after integration. The average value of all pixels is calculated. This value
        replaces the current value
        @param dark_run_integrated: a dark run with integrated pixels
        @returns an averaged, integrated dark run
        '''
        dark_run_summed_name= "_summed_spectra"
        alg_sum  = AlgorithmManager.create("SumSpectra")
        alg_sum.initialize()
        alg_sum.setChild(True)
        alg_sum.setProperty("InputWorkspace",  dark_run_integrated)
        alg_sum.setProperty("OutputWorkspace", dark_run_summed_name)
        alg_sum.execute()
        dark_run_summed = alg_sum.getProperty("OutputWorkspace").value

        # Get the single value out of the summed workspace and divide it
        # by the number of pixels
        summed_value = dark_run_summed.dataY(0)[0]
        num_pixels = dark_run_integrated.getNumberHistograms()
        averaged_value = summed_value/float(num_pixels)

        # Apply the averaged value to all pixels. Set values to zero and 
        # multiply by the averaged value.
        dark_run_divide_name = "_unity_spectra"
        alg_divide  = AlgorithmManager.create("Divide")
        alg_divide.initialize()
        alg_divide.setChild(True)
        alg_divide.setProperty("LHSWorkspace",  dark_run_integrated)
        alg_divide.setProperty("RHSWorkspace",  dark_run_integrated)
        alg_divide.setProperty("OutputWorkspace", dark_run_divide_name)
        alg_divide.execute()
        dark_run_unity= alg_divide.getProperty("OutputWorkspace").value

        # Now that we have a unity workspace multiply with the unit value
        return self._scale_dark_run(dark_run_unity,averaged_value)

    def _remove_unwanted_detectors(self, dark_run):
        # If we want both the monitors and the detectors, then we don't have to do anything
        applyToDetectors = self.getProperty("ApplyToDetectors").value
        applyToMonitors = self.getProperty("ApplyToMonitors").value

        if applyToDetectors and applyToMonitors:
            return dark_run

        # Find all monitors
        monitor_list = self._find_monitor_workspace_indices(dark_run)

        # If the user only wants the detector selection. Then we we cannot subtract
        # anything from the monitor spectra. This is true the other way around of course.
        # The minus algorithm will need to map all spectra from the LHS to some spectra
        # of the RHS, hence it does not quite do what we want. In order to circumvent the
        # issue here, we set all spectra to 0 which should not contribute to the dark run
        # correction.
        detector_cleaned_dark_run = None
        if applyToDetectors:
            detector_cleaned_dark_run = self._set_pure_detector_dark_run(dark_run, monitor_list)
        elif applyToMonitors:
            detector_cleaned_dark_run = self._set_pure_monitor_dark_run(dark_run, monitor_list)
        else:
            raise RuntimeError("SANSDarkRunBackgroundCorrection: Must provide either "
                               "ApplyToDetectors or ApplyToMonitors or both")

    def _set_pure_detector_dark_run(monitor_list):
        # Since we only have around 1 monitors we set them manually to 0
        for index in monitor_list:
            data = dark_run.dataX(index) # TODO: Continue here

    def _find_monitor_workspace_indices(dark_run):
        monitor_list = []
        try:
            num_histograms = dark_run.getNumHistograms()
            for index in range(0, num_histograms):
                det = dark_run.getDetector(index)
                if det.isMonitor():
                    monitor_list.append(index)
        except:
            # TODO: PRODUCE WARNING
            pass
        return monitor_list

#############################################################################################

AlgorithmFactory.subscribe(SANSDarkRunBackgroundCorrection)
