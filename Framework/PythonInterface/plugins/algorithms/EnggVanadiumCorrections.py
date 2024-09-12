# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as mantid

import numpy as np
import EnggUtils


class EnggVanadiumCorrections(PythonAlgorithm):
    # banks (or groups) to which the pixel-by-pixel correction should be applied
    _ENGINX_BANKS_FOR_PIXBYPIX_CORR = [1, 2]

    def category(self):
        return (
            "Diffraction\\Engineering;CorrectionFunctions\\BackgroundCorrections;"
            "CorrectionFunctions\\EfficiencyCorrections;CorrectionFunctions\\NormalisationCorrections"
        )

    def name(self):
        return "EnggVanadiumCorrections"

    def summary(self):
        return (
            "This algorithm is deprecated as of May 2021, use a workflow with the Integration algorithm instead."
            "Calculates correction features and / or uses them to correct diffraction data "
            "with respect to reference Vanadium data."
        )

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("Workspace", "", Direction.InOut, PropertyMode.Optional),
            "Workspace with the diffraction data to correct. The Vanadium corrections " "will be applied on it.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Workspace with the reference Vanadium diffraction data.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("OutIntegrationWorkspace", "", Direction.Output, PropertyMode.Optional),
            "Output integration workspace produced when given an input Vanadium workspace",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutCurvesWorkspace", "", Direction.Output, PropertyMode.Optional),
            "Output curves workspace produced when given an input Vanadium workspace",
        )

        # ~10 break points is still poor, there is no point in using less than that
        self.declareProperty(
            "SplineBreakPoints",
            defaultValue=50,
            validator=IntBoundedValidator(10),
            doc="Number of break points used when fitting the bank profiles with a spline " "function.",
        )

        out_vana_grp = "Output parameters (for when calculating corrections)"
        self.setPropertyGroup("OutIntegrationWorkspace", out_vana_grp)
        self.setPropertyGroup("OutCurvesWorkspace", out_vana_grp)
        self.setPropertyGroup("SplineBreakPoints", out_vana_grp)

        self.declareProperty(
            ITableWorkspaceProperty("IntegrationWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Workspace with the integrated values for every spectra of the reference " "Vanadium diffraction data. One row per spectrum.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CurvesWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Workspace with the curves fitted on bank-aggregated Vanadium diffraction "
            "data, one per bank. This workspace has three spectra per bank, as produced "
            "by the algorithm Fit. This is meant to be used as an alternative input "
            "VanadiumWorkspace",
        )

        in_vana_grp = "Input parameters (for when applying pre-calculated corrections)"
        self.setPropertyGroup("IntegrationWorkspace", in_vana_grp)
        self.setPropertyGroup("CurvesWorkspace", in_vana_grp)

    def PyExec(self):
        """
        Vanadium correction as done for the EnginX instrument. This is in principle meant to be used
        in EnginXFocus and EnginXCalibrateFull. The process consists of two steps:
        1. sensitivity correction
        2. pixel-by-pixel correction

        1. is performed for very pixel/detector/spectrum: scale every spectrum/curve by a
        scale factor proportional to the number of neutrons in that spectrum

        2. Correct every pixel by dividing by a curve (spline) fitted on the summed spectra
        (summing banks separately).

        The sums and fits are done in d-spacing.
        """
        mantid.logger.warning(
            "EnggVanadiumCorrections is deprecated as of May 2021. Please use a workflow with the " "Integration algorithm instead."
        )
        ws = self.getProperty("Workspace").value
        vanadium_ws = self.getProperty("VanadiumWorkspace").value
        van_integration_ws = self.getProperty("IntegrationWorkspace").value
        van_curves_ws = self.getProperty("CurvesWorkspace").value
        spline_breaks = self.getProperty("SplineBreakPoints").value

        max_reports = 30
        prog = Progress(self, start=0, end=1, nreports=max_reports)

        prog.report("Checking availability of vanadium correction features")
        # figure out if we are calculating or re-using pre-calculated corrections
        if vanadium_ws:
            self.log().information("A workspace with reference Vanadium data was passed. Calculating corrections")
            van_integration_ws, van_curves_ws = self._calculate_van_correction(vanadium_ws, spline_breaks, prog)
            self.setProperty("OutIntegrationWorkspace", van_integration_ws)
            self.setProperty("OutCurvesWorkspace", van_curves_ws)

        elif not van_integration_ws or not van_curves_ws:
            raise ValueError(
                "When a VanadiumWorkspace is not passed, both the IntegrationWorkspace and "
                "the CurvesWorkspace are required inputs. One or both of them were not given"
            )

        prog.report("Applying corrections on the input workspace")
        if ws:
            self._apply_vanadium_corrections(ws, van_integration_ws, van_curves_ws, prog)
            self.setProperty("Workspace", ws)

        prog.report(max_reports, "Finished")

    def _apply_vanadium_corrections(self, ws, van_integration_ws, van_curves_ws, prog):
        """
        Applies the corrections on a workspace. The integration and curves workspaces may have
        been calculated from a Vanadium run or may have been passed from a previous calculation.

        @param ws :: workspace to correct (modified in-place)
        @param van_integration_ws :: table workspace with spectra integration values, one row per spectra
        @param van_curves_ws ::workspace with "Vanadium curves" for every bank
        @param prog :: progress reporter
        """
        num_integration_spectra = van_integration_ws.rowCount()
        spectra = ws.getNumberHistograms()
        if num_integration_spectra < spectra:
            raise ValueError(
                "The number of histograms in the input data workspace (%d) is bigger "
                "than the number of spectra (rows) in the integration workspace (%d)" % (spectra, num_integration_spectra)
            )

        prog.report("Applying sensitivity correction")
        self._apply_sensitivity_correction(ws, van_integration_ws, van_curves_ws)
        prog.report("Applying pixel-by-pixel correction")
        self._apply_pix_by_pix_correction(ws, van_curves_ws)

    def _apply_sensitivity_correction(self, ws, van_integration_ws, van_curves_ws):
        """
        Applies the first step of the Vanadium corrections on the given workspace.
        Operations are done in ToF

        @param ws :: workspace (in/out)
        @param van_integration_ws :: pre-calculated integral of every spectrum for the Vanadium data
        @param van_curves_ws :: pre-calculated per-bank curves from the Vanadium data
        """
        blocksize = van_curves_ws.blocksize()
        for i in range(0, ws.getNumberHistograms()):
            scale_factor = van_integration_ws.cell(i, 0) / blocksize
            ws.setY(i, np.divide(ws.dataY(i), scale_factor))

    def _apply_pix_by_pix_correction(self, ws, van_curves_ws):
        """
        Applies the second step of the Vanadium correction on the given workspace: pixel by pixel
        divides by a curve fitted to the sum of the set of spectra of the corresponding bank.

        @param ws :: workspace to work on / correct
        @param van_curves_ws :: a workspace with the per-bank curves for Vanadium data,
                this will contain 3 histograms per instrument bank
        """
        curves_dict = self._fit_results_to_dict(van_curves_ws)

        self._divide_by_curves(ws, curves_dict)

    def _calculate_van_correction(self, vanadium_ws, spline_breaks, prog):
        """
        Calculates the features that are required to perform vanadium corrections: integration
        of the vanadium data spectra, and per-bank curves fitted to the summed spectra

        @param vanadium_ws :: workspace with data from a Vanadium run
        @param spline_breaks :: number of break points when fitting spline functions
        @param prog :: progress reporter

        @returns two workspaces: the integration and the curves. The integration workspace is a
        matrix workspace as produced by the algotithm 'Integration'. The curves workspace is a
        matrix workspace as produced in the outputs of the algorithm 'Fit'
        """
        prog.report("Calculating integration spectra")
        # Integration of every spectra, as a matrix workspace
        van_integration_ws = self._calculate_integration_spectra(vanadium_ws)

        # Have to calculate curves. get one curve per bank, in d-spacing
        van_curves_ws = self._fit_curves_per_bank(vanadium_ws, self._ENGINX_BANKS_FOR_PIXBYPIX_CORR, spline_breaks, prog)
        return van_integration_ws, van_curves_ws

    def _calculate_integration_spectra(self, vanadium_ws):
        """
        This does the real calculations behind _applySensitivityCorrection(), essentially a call to
        the 'Integration' algorithm, for when we are given raw data from a Vanadium run.

        @param vanadium_ws :: workspace with data from a Vanadium run

        @returns Integration workspace with Vanadium spectra integration values, as a table workspace
        with one row per spectrum
        """
        expected_dim = "Time-of-flight"
        dim_type = vanadium_ws.getXDimension().name
        if expected_dim != dim_type:
            raise ValueError(
                "This algorithm expects a workspace with %s X dimension, but "
                "the X dimension of the input workspace is: '%s'" % (expected_dim, dim_type)
            )

        vanadium_integration_ws = mantid.Integration(InputWorkspace=vanadium_ws, StoreInADS=False)
        if 1 != vanadium_integration_ws.blocksize() or vanadium_integration_ws.getNumberHistograms() < vanadium_ws.getNumberHistograms():
            raise RuntimeError(
                "Error while integrating vanadium workspace, the Integration algorithm "
                "produced a workspace with %d bins and %d spectra. The workspace "
                "being integrated has %d spectra."
                % (vanadium_integration_ws.blocksize(), vanadium_integration_ws.getNumberHistograms(), vanadium_ws.getNumberHistograms())
            )

        integration_spectra = mantid.CreateEmptyTableWorkspace(OutputWorkspace="__vanIntegTbl")
        integration_spectra.addColumn("double", "Spectra Integration")
        for i in range(vanadium_integration_ws.getNumberHistograms()):
            integration_spectra.addRow([vanadium_integration_ws.readY(i)[0]])

        return integration_spectra

    def _fit_curves_per_bank(self, vanadium_ws, banks, spline_breaks, prog):
        """
        Fits one curve to every bank (where for every bank the data fitted is the result of
        summing up all the spectra of the bank). The fitting is done in d-spacing.

        @param vanadium_ws :: Vanadium run workspace to fit, expected in TOF units as they are archived
        @param banks :: list of banks to consider which is normally all the banks of the instrument
        @param spline_breaks :: number of break points when fitting spline functions
        @param prog :: progress reporter

        @returns a workspace with fitting results for all banks (3 spectra per bank). The spectra
        are in dSpacing units.
        """
        curves = {}
        for bank_number, bank in enumerate(banks):
            prog.report("Fitting bank {} of {}".format(bank_number + 1, len(banks)))
            indices = EnggUtils.get_ws_indices_for_bank(vanadium_ws, bank)
            if not indices:
                # no indices at all for this bank, not interested in it, don't add it to the dictionary
                # (as when doing Calibrate (not-full)) which does CropData() the original workspace
                continue

            prog.report("Cropping")
            ws_to_fit = EnggUtils.crop_data(self, vanadium_ws, indices)
            prog.report("Converting to d-spacing")
            ws_to_fit = EnggUtils.convert_to_d_spacing(self, ws_to_fit)
            prog.report("Summing spectra")
            ws_to_fit = EnggUtils.sum_spectra(self, ws_to_fit)

            prog.report("Fitting bank {} to curve".format(bank_number))
            fit_ws = self._fit_bank_curve(ws_to_fit, bank, spline_breaks, prog)
            curves.update({bank: fit_ws})

        curves_ws = self._prepare_curves_ws(curves)

        return curves_ws

    def _fit_bank_curve(self, vanadium_ws, bank, spline_breaks, prog):
        """
        Fits a spline to a single-spectrum workspace (in d-spacing)

        @param vanadium_ws :: Vanadium workspace to fit (normally this contains spectra for a single bank)
        @param bank :: instrument bank this is fitting is done for
        @param spline_breaks :: number of break points when fitting spline functions
        @param prog :: progress reporter

        @returns fit workspace (MatrixWorkspace), with the same number of bins as the input
        workspace, and the Y values simulated from the fitted curve
        """
        expected_dim = "d-Spacing"
        dim_type = vanadium_ws.getXDimension().name
        if expected_dim != dim_type:
            raise ValueError(
                "This algorithm expects a workspace with %s X dimension, but "
                "the X dimension of the input workspace is: '%s'" % (expected_dim, dim_type)
            )

        if 1 != vanadium_ws.getNumberHistograms():
            raise ValueError("The workspace does not have exactly one histogram. Inconsistency found.")

        # without these min/max parameters 'BSpline' would completely misbehave
        x_values = vanadium_ws.readX(0)
        start_x = min(x_values)
        end_x = max(x_values)

        function_descriptor = "name=BSpline, Order=3, StartX={0}, EndX={1}, NBreak={2}".format(start_x, end_x, spline_breaks)
        # WorkspaceIndex is left to default '0' for 1D function fits
        # StartX, EndX could in principle be left to default start/end of the spectrum, but apparently
        # not safe for 'BSpline'
        prog.report("Performing fit")
        fit_output = mantid.Fit(InputWorkspace=vanadium_ws, Function=function_descriptor, CreateOutput=True, StoreInADS=False)
        prog.report("Fit complete")

        success = fit_output.OutputStatus
        self.log().information("Fitting Vanadium curve for bank %s, using function '%s', result: %s" % (bank, function_descriptor, success))

        failure_msg = (
            "It seems that this algorithm failed to to fit a function to the summed " "spectra of a bank. The function definiton was: '%s'"
        ) % function_descriptor

        output_params_prop_name = "OutputParameters"
        if not hasattr(fit_output, output_params_prop_name):
            raise RuntimeError(
                "Could not find the parameters workspace expected in the output property "
                + output_params_prop_name
                + " from the algorithm Fit. It seems that this algorithm failed."
                + failure_msg
            )

        try:
            fit_ws = fit_output.OutputWorkspace
        except AttributeError:
            raise RuntimeError(
                "Could not find the data workspace expected in the output property " + "OutputWorkspace" + ". " + failure_msg
            )

        mtd["engg_van_ws_dsp"] = vanadium_ws
        mtd["engg_fit_ws_dsp"] = fit_ws

        return fit_ws

    def _prepare_curves_ws(self, curves_dict):
        """
        Simply concantenates or appends fitting output workspaces as produced by the algorithm Fit (with 3
        spectra each). The groups of 3 spectra are added sorted by the bank ID (number). This could also
        produce a workspace group with the individual workspaces in it, but the AppendSpectra solution
        seems simpler.

        @param curves_dict :: dictionary with fitting workspaces produced by 'Fit'

        @returns a workspace where all the input workspaces have been concatenated, with 3 spectra per
        workspace / bank
        """
        if 0 == len(curves_dict):
            raise RuntimeError("Expecting a dictionary with fitting workspaces from 'Fit' but got an " "empty dictionary")
        if 1 == len(curves_dict):
            return list(curves_dict.values())[0]

        keys = sorted(curves_dict)
        ws = curves_dict[keys[0]]
        for idx in range(1, len(keys)):
            next_ws = curves_dict[keys[idx]]
            ws = self._append_spectra(ws, next_ws)

        return ws

    def _divide_by_curves(self, ws, curves):
        """
        Expects a workspace in ToF units. All operations are done in-place (the workspace is
        input/output). For every bank-curve pair, divides the corresponding spectra in the
        workspace by the (simulated) fitted curve. The division is done in d-spacing (the
        input workspace is converted to d-spacing inside this method, but results are converted
        back to ToF before returning from this method). The curves workspace is expected in
        d-spacing units (since it comes from fitting a sum of spectra for a bank or group of
        detectors).

        This method is capable of dealing with workspaces with range and bin size different from
        the range and bin size of the curves. It will rebin the curves workspace to match the
        input 'ws' workspace (using the algorithm RebinToWorkspace).

        @param ws :: workspace with (sample) spectra to divide by curves fitted to Vanadium spectra

        @param curves :: dictionary of fitting workspaces (in d-spacing), one per bank. The keys are
        the bank identifier and the values are their fitting workspaces. The fitting workspaces are
        expected as returned by the algorithm 'Fit': 3 spectra: original data, simulated data with fit,
        difference between original and simulated data.
        """
        # Note that this division could use the algorithm 'Divide'
        # This is simple and more efficient than using divide workspace, which requires
        # cropping separate workspaces, dividing them separately, then appending them
        # with AppendSpectra, etc.
        ws = EnggUtils.convert_to_d_spacing(self, ws)
        for b in curves:
            # process all the spectra (indices) in one bank
            fitted_curve = curves[b]
            idxs = EnggUtils.get_ws_indices_for_bank(ws, b)

            if not idxs:
                pass

            # This RebinToWorkspace is required here: normal runs will have narrower range of X values,
            # and possibly different bin size, as compared to (long) Vanadium runs. Same applies to short
            # Ceria runs (for Calibrate -non-full) and even long Ceria runs (for Calibrate-Full).
            rebinned_fit_curve = mantid.RebinToWorkspace(WorkspaceToRebin=fitted_curve, WorkspaceToMatch=ws, StoreInADS=False)

            for i in idxs:
                # take values of the second spectrum of the workspace (fit simulation - fitted curve)
                ws.setY(i, np.divide(ws.dataY(i), rebinned_fit_curve.readY(1)))

        # finally, convert back to ToF
        EnggUtils.convert_to_TOF(self, ws)

    def _fit_results_to_dict(self, ws):
        """
        Turn a workspace with one or more fitting results (3 spectra per curve fitted), that comes
        with all the spectra appended, into a dictionary of individual fitting results.

        @param ws workspace with fitting results (3 spectra per curve fitted) as provided by Fit for
        all banks.

        @returns dictionary with every individual fitting result (3 spectra)
        """
        curves = {}

        if 0 != (ws.getNumberHistograms() % 3):
            raise RuntimeError(
                "A workspace without instrument definition has been passed, so it is "
                "expected to have fitting results, but it does not have a number of "
                "histograms multiple of 3. Number of hsitograms found: %d" % ws.getNumberHistograms()
            )

        for wi in range(0, int(ws.getNumberHistograms() / 3)):
            indiv = EnggUtils.crop_data(self, ws, [wi, wi + 2])
            # the bank id is +1 because wi starts from 0
            bankid = wi + 1
            curves.update({bankid: indiv})

        return curves

    def _append_spectra(self, ws1, ws2):
        """
        Uses the algorithm 'AppendSpectra' to append the spectra of ws1 and ws2

        @param ws1 :: first workspace
        @param ws2 :: second workspace

        @returns workspace with the concatenation of the spectra ws1+ws2
        """
        alg = self.createChildAlgorithm("AppendSpectra")
        alg.setProperty("InputWorkspace1", ws1)
        alg.setProperty("InputWorkspace2", ws2)
        alg.execute()

        result = alg.getProperty("OutputWorkspace").value
        return result


AlgorithmFactory.subscribe(EnggVanadiumCorrections)
