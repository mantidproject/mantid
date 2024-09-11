# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class EnggFitPeaks(PythonAlgorithm):
    EXPECTED_DIM_TYPE = "Time-of-flight"
    PEAK_TYPE = "BackToBackExponential"
    # Max limit on the estimated error of a center for it to be accepted as a good fit
    # (in percentage of the center value)
    CENTER_ERROR_LIMIT = 10

    _expected_peaks_are_in_tof = True

    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def seeAlso(self):
        return ["EnggFitTOFFromPeaks", "GSASIIRefineFitPeaks", "Fit"]

    def name(self):
        return "EnggFitPeaks"

    def summary(self):
        return (
            "The algorithm fits an expected diffraction pattern to a spectrum from a workspace "
            "by fitting one peak at a time (single peak fits)."
        )

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
            "Workspace to fit peaks in. The X units must be time of flight (TOF).",
        )

        self.declareProperty("WorkspaceIndex", 0, "Index of the spectra to fit peaks in")

        self.declareProperty(
            FloatArrayProperty("ExpectedPeaks", (self._get_default_peaks())),
            "A list of peak centre values to be translated into TOF (if required) to find expected " "peaks.",
        )

        self.declareProperty(
            FileProperty(name="ExpectedPeaksFromFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".csv"]),
            "Load from file a list of peak centre values to be translated into TOF (if required) to "
            "find expected peaks. This takes precedence over 'ExpectedPeaks' if both "
            "options are given.",
        )

        peaks_grp = "Peaks to fit"
        self.setPropertyGroup("ExpectedPeaks", peaks_grp)
        self.setPropertyGroup("ExpectedPeaksFromFile", peaks_grp)

        self.declareProperty(
            "OutFittedPeaksTable",
            "",
            direction=Direction.Input,
            doc="Name for a table workspace with the parameters of the peaks found and "
            "fitted. If not given, the table workspace is not created.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("FittedPeaks", "", Direction.Output),
            doc="Information on fitted peaks. The table contains, for every peak fitted "
            "the expected peak value (in d-spacing), and the parameters fitted. The expected "
            "values are given in the column labelled 'dSpacing'. When fitting "
            "back-to-back exponential functions, the 'X0' column has the fitted peak center.",
        )

    def PyExec(self):
        import EnggUtils

        # Get peaks in dSpacing from file
        expected_peaks = EnggUtils.read_in_expected_peaks(
            self.getPropertyValue("ExpectedPeaksFromFile"), self.getProperty("ExpectedPeaks").value
        )

        if len(expected_peaks) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        # Get expected peaks in TOF for the detector
        in_wks = self.getProperty("InputWorkspace").value
        dim_type = in_wks.getXDimension().name
        if self.EXPECTED_DIM_TYPE != dim_type:
            raise ValueError(
                "This algorithm expects a workspace with %s X dimension, but "
                "the X dimension of the input workspace is: '%s'" % (self.EXPECTED_DIM_TYPE, dim_type)
            )

        wks_index = self.getProperty("WorkspaceIndex").value

        if self._any_expected_peaks_in_ws_range(in_wks, expected_peaks):
            expected_peaks_tof = sorted(expected_peaks)
        else:
            expected_peaks_tof = sorted(self._expected_peaks_in_tof(expected_peaks, in_wks, wks_index))
            self._expected_peaks_are_in_tof = False
            if not self._any_expected_peaks_in_ws_range(in_wks, expected_peaks_tof):
                raise ValueError("Expected peak centres lie outside the limits of the workspace x axis")

        found_peaks = self._peaks_from_find_peaks(in_wks, expected_peaks_tof, wks_index)
        if found_peaks.rowCount() < len(expected_peaks_tof):
            txt = "Peaks effectively found: " + str(found_peaks)[1:-1]
            self.log().warning(
                "Some peaks from the list of expected peaks were not found by the algorithm "
                "FindPeaks which this algorithm uses to check that the data has the "
                "expected peaks. " + txt
            )

        peaks_table_name = self.getPropertyValue("OutFittedPeaksTable")
        fitted_peaks = self._fit_all_peaks(in_wks, wks_index, (found_peaks, expected_peaks), peaks_table_name)

        # mandatory output
        self.setProperty("FittedPeaks", fitted_peaks)

    def _any_expected_peaks_in_ws_range(self, input_ws, expected_peaks):
        x_axis = input_ws.readX(0)
        x_min = min(x_axis)
        x_max = max(x_axis)

        for peak_centre in expected_peaks:
            if self._expected_peak_in_ws_range(x_min, x_max, peak_centre):
                return True
        return False

    def _expected_peak_in_ws_range(self, ws_x_min, ws_x_max, expected_peak_centre):
        return ws_x_min <= expected_peak_centre <= ws_x_max

    def _get_default_peaks(self):
        """
        Gets default peaks for Engg algorithms. Values from CeO2
        """
        import EnggUtils

        return EnggUtils.default_ceria_expected_peaks()

    def _estimate_start_end_fitting_range(self, center, width):
        """
        Try to predict a fit window for the peak (using magic numbers). The heuristic
        +-COEF_LEFT/RIGHT sometimes produces ranges that are too narrow and contain too few
        samples (one or a handful) for the fitting to run correctly. A minimum is enforced.

        @Returns :: a tuple with the range (start and end values) for fitting a peak.
        """
        # Magic numbers, approx. represanting the shape/proportions of a B2BExponential peak
        COEF_LEFT = 2
        COEF_RIGHT = 3
        # Current approach: don't force a minimum width. If the width initial guess is too
        # narrow we might miss some peaks.
        # To prevent that, the minimum could be set to for example the arbitrary '175' which
        # seemed to have good effects overall, but that can lead to fitting the wrong
        # (neighbor) peaks.
        MIN_RANGE_WIDTH = 1

        startx = center - (width * COEF_LEFT)
        endx = center + (width * COEF_RIGHT)
        x_diff = endx - startx
        if x_diff < MIN_RANGE_WIDTH:
            inc = (min_width - x_diff) / 5
            endx = endx + 3 * inc
            startx = startx - 2 * inc

        return startx, endx

    def _fit_all_peaks(self, in_wks, wks_index, peaks, peaks_table_name):
        """
        This method is the core of EnggFitPeaks. It tries to fit as many peaks as there are in the list of
        expected peaks passed to the algorithm. This is a single peak fitting, in the sense that peaks
        are fitted separately, one at a time.

        The parameters from the (Gaussian) peaks fitted by FindPeaks elsewhere (before calling this method)
        are used as initial guesses.

        @param in_wks :: input workspace with spectra for fitting
        @param wks_index :: workspace index of the spectrum where the given peaks should be fitted
        @param peaks :: tuple made of two lists: found_peaks (peaks found by FindPeaks or similar
                        algorithm), and expected_peaks_dsp (expected peaks given as input to this algorithm
                        (in dSpacing units)
        @param peaks_table_name :: name of an (output) table with peaks parameters. If empty, the table is anonymous

        @returns a table with parameters for every fitted peak.

        """

        if self._expected_peaks_are_in_tof:
            peaks = (peaks[0], self._expected_peaks_in_d(peaks[1], in_wks))

        found_peaks = peaks[0]
        fitted_peaks = self._create_fitted_peaks_table(peaks_table_name)

        prog = Progress(self, start=0, end=1, nreports=found_peaks.rowCount())

        for i in range(found_peaks.rowCount()):
            prog.report("Fitting peak number " + str(i + 1))

            row = found_peaks.row(i)
            # Peak parameters estimated by FindPeaks
            initial_params = (row["centre"], row["width"], row["height"])
            # Oh oh, this actually happens sometimes for some spectra of the system test dataset
            # and it should be clarified when the role of FindPeaks etc. is fixed (trac ticket #10907)
            width = initial_params[2]
            if width <= 0.0:
                failure_msg = "Cannot fit a peak with these initial parameters from FindPeaks, center: %s " ", width: %s, height: %s" % (
                    initial_params[0],
                    width,
                    initial_params[1],
                )
                self.log().notice(
                    f"For workspace index {wks_index} a peak that is in the list of expected peaks and "
                    f"was found by FindPeaks has not been fitted correctly. It will be ignored. "
                    f"Expected dSpacing: {peaks[1][i]}. Details: {failure_msg}."
                )
                continue

            try:
                param_table, chi_over_dof = self._fit_single_peak(peaks[1][i], initial_params, in_wks, wks_index)
            except RuntimeError:
                self.log().warning(
                    "Problem found when trying to fit a peak centered at {0} (dSpacing), "
                    "for which the initial guess from FindPeaks is at {1} (ToF). Single "
                    "peak fitting failed. Skipping this peak.".format(peaks[1][i], initial_params[0])
                )
                continue

            fitted_params = {}

            fitted_params["dSpacing"] = peaks[1][i]
            fitted_params["Chi"] = chi_over_dof
            self._add_parameters_to_map(fitted_params, param_table)
            if self._peak_is_acceptable(fitted_params, in_wks, wks_index):
                fitted_peaks.addRow(fitted_params)
            else:
                self.log().notice(
                    "Discarding peak found with wrong center and/or excessive or suspicious "
                    "error estimate in the center estimate: {0} (ToF) ({1}, dSpacing), "
                    "with error: {2}, for dSpacing {3}".format(
                        fitted_params["X0"], peaks[1][i], fitted_params["X0_Err"], fitted_params["dSpacing"]
                    )
                )

        # Check if we were able to really fit any peak
        if 0 == fitted_peaks.rowCount():
            failure_msg = (
                "Could find "
                + str(len(found_peaks))
                + " peaks using the algorithm FindPeaks but "
                + "then it was not possible to fit any peak starting from these peaks found and using '"
                + self.PEAK_TYPE
                + "' as peak function."
            )
            self.log().warning(
                "Could not fit any peak. Please check the list of expected peaks, as it does not "
                "seem to be appropriate for the workspace given. More details: " + failure_msg
            )

            raise RuntimeError(
                "Could not fit any peak.  Failed to fit peaks with peak type "
                + self.PEAK_TYPE
                + " even though FindPeaks found "
                + str(found_peaks.rowCount())
                + " peaks in principle. See the logs for further details."
            )

        self.log().information("Fitted {0} peaks in total.".format(fitted_peaks.rowCount()))

        return fitted_peaks

    def _fit_single_peak(self, expected_center, initial_params, wks, wks_index):
        """
        Fits one peak, given an initial guess of parameters (center, width, height).

        @param expected_center :: expected peak position
        @param initial_params :: tuple with initial guess of the peak center, width and height
        @param wks :: workspace with data (spectra) to fit
        @param wks_index :: index of the spectrum to fit

        @return parameters from Fit, and the goodness of fit estimation from Fit (as Chi^2/DoF)
        """

        center, width, height = initial_params
        # Sigma value of the peak, assuming Gaussian shape
        sigma = width / (2 * math.sqrt(2 * math.log(2)))

        # Approximate peak intensity, assuming Gaussian shape
        intensity = height * sigma * math.sqrt(2 * math.pi)

        peak = FunctionFactory.createFunction(self.PEAK_TYPE)
        peak.setParameter("X0", center)
        peak.setParameter("S", sigma)
        peak.setParameter("I", intensity)

        # Fit using predicted window and a proper function with approximated initial values
        fit_function = "name=LinearBackground;{0}".format(peak)
        (startx, endx) = self._estimate_start_end_fitting_range(center, width)
        self.log().debug(
            "Fitting for peak expected in (d-spacing): {0}, Fitting peak function: {1}, with startx: {2}, endx: {3}".format(
                expected_center, fit_function, startx, endx
            )
        )

        fit_output = Fit(
            Function=fit_function,
            InputWorkspace=wks,
            WorkspaceIndex=wks_index,
            CreateOutput=True,
            StartX=startx,
            EndX=endx,
            StoreInADS=False,
        )

        return fit_output.OutputParameters, fit_output.OutputChi2overDoF

    def _peaks_from_find_peaks(self, in_wks, expected_peaks_tof, wks_index):
        """
        Use the algorithm FindPeaks to check that the expected peaks are there.

        @param in_wks data workspace
        @param expected_peaks_tof vector/list of expected peak values
        @param wks_index workspace index

        @return list of peaks found by FindPeaks. If there are no issues, the length
        of this list should be the same as the number of expected peaks received.
        """

        # Find approximate peak positions, assuming Gaussian shapes
        found_peaks = FindPeaks(
            InputWorkspace=in_wks, PeakPositions=expected_peaks_tof, PeakFunction="Gaussian", WorkspaceIndex=wks_index, StoreInADS=False
        )
        return found_peaks

    def _expected_peaks_in_d(self, expected_peaks, input_ws):
        run = input_ws.getRun()

        if run.hasProperty("difc"):
            difc = run.getLogData("difc").value
            return self._gsas_convert_to_d(expected_peaks, run, difc)

        return self._convert_peaks_to_d_using_convert_units(expected_peaks, input_ws)

    def _gsas_convert_to_d(self, expected_peaks, run, difc):
        tzero = run.getLogData("tzero").value if run.hasProperty("tzero") else 0
        difa = run.getLogData("difa").value if run.hasProperty("difa") else 0
        return [self._gsas_convert_single_peak_to_d(peak, difa, difc, tzero) for peak in expected_peaks]

    def _gsas_convert_single_peak_to_d(self, peak_tof, difa, difc, tzero):
        if difa < 0:
            return (-difc / (2 * difa)) - math.sqrt(peak_tof / difa + math.pow(difc / 2 * difa, 2) - tzero / difa)
        if difa > 0:
            return (-difc / (2 * difa)) + math.sqrt(peak_tof / difa + math.pow(difc / 2 * difa, 2) - tzero / difa)
        return (peak_tof - tzero) / difc

    def _convert_peaks_to_d_using_convert_units(self, expected_peaks, input_ws):
        y_values = [1] * (len(expected_peaks) - 1)
        ws_tof = CreateWorkspace(UnitX="TOF", DataX=expected_peaks, DataY=y_values, ParentWorkspace=input_ws)
        ws_d = ConvertUnits(InputWorkspace=ws_tof, Target="dSpacing")
        return ws_d.readX(0)

    def _expected_peaks_in_tof(self, expected_peaks, in_wks, wks_index):
        """
        Converts expected peak dSpacing values to TOF values for the
        detector. Implemented by using the Mantid algorithm ConvertUnits. A
        simple user script to do what this function does would be
        as follows:

        import mantid.simpleapi as sapi

        yVals = [1] * (len(expected_peaks) - 1)
        ws_from = sapi.CreateWorkspace(UnitX='dSpacing', DataX=expected_peaks, DataY=yVals,
                                      ParentWorkspace=in_wks)
        target_units = 'TOF'
        wsTo = sapi.ConvertUnits(InputWorkspace=ws_from, Target=target_units)
        peaks_ToF = wsTo.dataX(0)
        values = [peaks_ToF[i] for i in range(0,len(peaks_ToF))]

        @param expected_peaks :: vector of expected peaks, in dSpacing units
        @param in_wks :: input workspace with the relevant instrument/geometry
        @param wks_index workspace index

        Returns:
            a vector of ToF values converted from the input (dSpacing) vector.

        """

        # This and the next exception, below, still need revisiting:
        # https://github.com/mantidproject/mantid/issues/12930
        run = in_wks.getRun()
        if 1 == in_wks.getNumberHistograms() and run.hasProperty("difc"):
            difc = run.getLogData("difc").value
            if run.hasProperty("tzero"):
                tzero = run.getLogData("tzero").value
            else:
                tzero = 0
            # If the log difc is present, then use these GSAS calibration parameters from the logs
            return [(epd * difc + tzero) for epd in expected_peaks]

        # When receiving a (for example) focused workspace we still do not know how
        # to properly deal with it. CreateWorkspace won't copy the instrument sample
        # and source even if given the option ParentWorkspace. Resort to old style
        # hard-coded calculation.
        # The present behavior of 'ConvertUnits' is to show an information log:
        # "Unable to calculate sample-detector distance for 1 spectra. Masking spectrum"
        # and silently produce a wrong output workspace. That might need revisiting.
        if 1 == in_wks.getNumberHistograms():
            return self._do_approx_hard_coded_convert_units_to_ToF(expected_peaks, in_wks, wks_index)

        # Create workspace just to convert dSpacing -> ToF, yVals are irrelevant
        # this used to be calculated with:
        # lambda d: 252.816 * 2 * (50 + detL2) * math.sin(detTwoTheta / 2.0) * d
        # which is approximately what ConverUnits will do
        # remember the -1, we must produce a histogram data workspace, which is what
        # for example EnggCalibrate expects
        y_vals = [1] * (len(expected_peaks) - 1)
        # Do like: ws_from = sapi.CreateWorkspace(UnitX='dSpacing', DataX=expected_peaks, DataY=yVals,
        #                                         ParentWorkspace=self.getProperty("InputWorkspace").value)
        ws_from = CreateWorkspace(UnitX="dSpacing", DataX=expected_peaks, DataY=y_vals, ParentWorkspace=in_wks)

        # finally convert units, like: sapi.ConvertUnits(InputWorkspace=ws_from, Target=target_units)
        try:
            # note: this implicitly uses default property "EMode" value 'Elastic'
            target_units = "TOF"
            output_ws = ConvertUnits(InputWorkspace=ws_from, Target=target_units)
        except:
            raise RuntimeError(
                "Conversion of units went wrong. Failed to run ConvertUnits for {0} peaks. Details: {1}".format(
                    len(expected_peaks), expected_peaks
                )
            )

        peaks_tof = output_ws.readX(0)
        if len(peaks_tof) != len(expected_peaks):
            raise RuntimeError(
                "Conversion of units went wrong. Converted {0} peaks from the "
                "original list of {1} peaks. The instrument definition might be "
                "incomplete for the original workspace / file.".format(len(peaks_tof), len(expected_peaks))
            )

        tof_values = [peaks_tof[i] for i in range(0, len(peaks_tof))]
        # catch potential failures because of geometry issues, etc.
        if tof_values == expected_peaks:
            vals = self._do_approx_hard_coded_convert_units_to_ToF(expected_peaks, in_wks, wks_index)
            return vals

        return tof_values

    def _do_approx_hard_coded_convert_units_to_ToF(self, dsp_values, ws, wks_index):
        """
        Converts from dSpacing to Time-of-flight, for one spectrum/detector. This method
        is here for exceptional cases that presently need clarification / further work,
        here and elsewhere in Mantid, and should ideally be removed in favor of the more
        general method that uses the algorithm ConvertUnits.

        @param dsp_values to convert from dSpacing
        @param ws workspace with the appropriate instrument / geometry definition
        @param wks_index index of the spectrum

        Returns:
        input values converted from dSpacing to ToF
        """
        det = ws.getDetector(wks_index)

        # Current detector parameters
        detL2 = det.getDistance(ws.getInstrument().getSample())
        detTwoTheta = ws.detectorTwoTheta(det)

        # hard coded equation to convert dSpacing -> TOF for the single detector
        # Values (in principle, expected peak positions) in TOF for the detector
        tof_values = [252.816 * 2 * (50 + detL2) * math.sin(detTwoTheta / 2.0) * ep for ep in dsp_values]
        return tof_values

    def _create_fitted_peaks_table(self, tbl_name):
        """
        Creates a table where to put peak fitting results to

        @param tbl_name :: name of the table workspace (can be empty)
        """
        if not tbl_name:
            alg = self.createChildAlgorithm("CreateEmptyTableWorkspace")
            alg.execute()
            table = alg.getProperty("OutputWorkspace").value
        else:
            table = CreateEmptyTableWorkspace(OutputWorkspace=tbl_name)

        table.addColumn("double", "dSpacing")

        for param in ["A0", "A1", "X0", "A", "B", "S", "I"]:
            table.addColumn("double", param)
            table.addColumn("double", param + "_Err")

        table.addColumn("double", "Chi")

        return table

    def _peak_is_acceptable(self, fitted_params, wks, wks_index):
        """
        Decide whether a peak fitted looks acceptable, based on the values fitted for the
        parameters of the peak and other metrics from Fit (Chi^2).

        It applies for example a simple rule: if the peak center is
        negative, it is obviously a fit failure. This is sometimes not so straightforward
        from the error estimates and Chi^2 values returned from Fit, as there seem to be
        a small percentage of cases with numberical issues (nan, zeros, etc.).

        @param fitted_params :: parameters fitted from Fit algorithm
        @param in_wks :: input workspace where a spectrum was fitted
        @param wks_index :: workspace index of the spectrum that was fitted

        Returns:
            True if the peak function parameters and error estimates look acceptable
            so the peak should be used.
        """
        spec_x_axis = wks.readX(wks_index)
        center = self._find_peak_center_in_params(fitted_params)
        intensity = self._find_peak_intensity_in_params(fitted_params)
        return (
            spec_x_axis.min() <= center <= spec_x_axis.max()
            and intensity > 0
            and fitted_params["Chi"] < 17
            and self._b2bexp_is_acceptable(fitted_params)
        )

    def _find_peak_center_in_params(self, fitted_params):
        """
        Retrieve the fitted peak center/position from the set of parameters fitted.

        Returns:
            The peak center from the fitted parameters
        """
        if "BackToBackExponential" == self.PEAK_TYPE:
            return fitted_params["X0"]
        else:
            raise ValueError(
                "Inconsistency found. I do not know how to deal with centers of peaks " "of types other than {0}".format(PEAK_TYPE)
            )

    def _find_peak_intensity_in_params(self, fitted_params):
        """
        Retrieve the fitted peak intensity/height/amplitude from the set of parameters fitted.

        Returns:
            The peak intensity from the fitted parameters
        """
        if "BackToBackExponential" == self.PEAK_TYPE:
            return fitted_params["I"]
        else:
            raise ValueError(
                "Inconsistency found. I do not know how to deal with intensities of " "peaks of types other than {0}".format(PEAK_TYPE)
            )

    def _b2bexp_is_acceptable(self, fitted_params):
        """
        Checks specific to Back2BackExponential peak functions.

        @param fitted_params :: parameters fitted, where it is assumed that the
        standard Back2BackExponential parameter names are used

        Returns:
            True if the Bk2BkExponential parameters and error estimates look acceptable
            so the peak should be used.
        """
        # Ban: negative centers, negative left (A) and right (B) exponential coefficient,
        # and Gaussian spread (S).
        # Also ban strange error estimates (nan, all zero error)
        # And make sure that the error on the center (X0) is not too big in relative terms
        return (
            fitted_params["X0"] > 0
            and fitted_params["A"] > 0
            and fitted_params["B"] > 0
            and fitted_params["S"] > 0
            and not math.isnan(fitted_params["X0_Err"])
            and not math.isnan(fitted_params["A_Err"])
            and not math.isnan(fitted_params["B_Err"])
            and fitted_params["X0_Err"] < (fitted_params["X0"] * self.CENTER_ERROR_LIMIT / 100)
            and (0 != fitted_params["X0_Err"] and 0 != fitted_params["S_Err"] and 0 != fitted_params["I_Err"])
        )

    def _add_parameters_to_map(self, param_map, param_table):
        """
        Takes parameters from a table that contains output parameters from a Fit run, and adds
        them as name:value and name_Err:error_value pairs to the map.

        @param param_map :: map where to add the fitting parameters
        @param param_table :: table with parameters from a Fit algorithm run
        """
        for i in range(param_table.rowCount() - 1):  # Skip the last (fit goodness) row
            row = param_table.row(i)

            # Get local func. param name. E.g., not f1.A0, but just A0
            name = (row["Name"].rpartition("."))[2]

            param_map[name] = row["Value"]
            param_map[name + "_Err"] = row["Error"]


AlgorithmFactory.subscribe(EnggFitPeaks)
