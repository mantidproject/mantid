# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from os import path

from mantid.simpleapi import SaveNexus, logger, CreateEmptyTableWorkspace, Fit
from mantid.simpleapi import ConvertUnits, Rebunch, Rebin, SumSpectra, AppendSpectra, CloneWorkspace, CropWorkspaceRagged
from mantid.api import AnalysisDataService as ADS, MultiDomainFunction, FunctionFactory
from typing import Sequence, Tuple, List
from mantid.dataobjects import Workspace2D
from plugins.algorithms.IntegratePeaks1DProfile import get_eval_ws, calc_sigma_from_summation
from Engineering.EnggUtils import convert_TOFerror_to_derror
from mantid.api import IFunction, CompositeFunction

from mantid.kernel import DeltaEModeType, UnitConversion
from plugins.algorithms.peakdata_utils import PeakData


def crop_and_rebin(ws: Workspace2D | str, out_ws: str, lower: float, upper: float, rebin_params: str | Sequence[float]) -> None:
    CropWorkspaceRagged(ws, lower, upper, OutputWorkspace="__tmp_peak_window")
    Rebin("__tmp_peak_window", rebin_params, OutputWorkspace=out_ws)


def _get_max_bin(ws: Workspace2D) -> np.ndarray:
    return max(np.diff(ws.readX(i)).max() for i in range(ws.getNumberHistograms()))


def crop_wss_and_combine(
    wss: Sequence[Workspace2D | str], peak: float, lower: float, upper: float, output: str
) -> Tuple[Workspace2D, List[str]]:
    cropped_rebinned_wss = [f"rebin_ws_{peak}_0"]
    peak_window_ws = CropWorkspaceRagged(wss[0], lower, upper, OutputWorkspace="__peak_window_crop")
    rebin_params = (lower, _get_max_bin(peak_window_ws), upper)
    Rebin("__peak_window_crop", rebin_params, OutputWorkspace=f"rebin_ws_{peak}_0")
    CloneWorkspace(InputWorkspace=f"rebin_ws_{peak}_0", OutputWorkspace=f"rebin_ws_{peak}")
    for iws, ws in enumerate(wss[1:]):
        intermediate_ws = f"rebin_ws_{peak}_{iws + 1}"
        cropped_rebinned_wss.append(intermediate_ws)
        crop_and_rebin(ws, intermediate_ws, lower, upper, rebin_params)
        AppendSpectra(f"rebin_ws_{peak}", intermediate_ws, OutputWorkspace=f"rebin_ws_{peak}")
    return SumSpectra(f"rebin_ws_{peak}", OutputWorkspace=output), cropped_rebinned_wss


def _make_composite(peak_func: IFunction, bg_func: IFunction) -> CompositeFunction:
    """Build a CompositeFunction from existing C++ function objects without
    serialising them (which would discard workspace references, fixed-parameter
    state, etc.).  NumDeriv is NOT set so that each member function uses its own
    derivative method (analytical for LinearBackground, numerical fallback for
    IkedaCarpenterPV) — this avoids redundant expensive peak evaluations when
    computing derivatives for background parameters."""
    comp = FunctionFactory.createFunction("CompositeFunction")
    comp.add(peak_func)
    comp.add(bg_func)
    return comp


def fit_initial_summed_spectra(
    wss: Sequence[Workspace2D | str], peaks: Sequence[float], peak_window: float, fit_kwargs: dict, peak_func_name: str
) -> Tuple[Sequence[Tuple[float, float]], Sequence[Workspace2D]]:
    x0_lims = []
    all_peak_crop_wss = []
    for i, peak in enumerate(peaks):
        # set the ws bounds based on the supplied peak window
        low_bound, hi_bound = peak - peak_window, peak + peak_window
        window_ws, peak_crop_wss = crop_wss_and_combine(wss, peak, low_bound, hi_bound, f"peak_window_{i}")

        # the outer list is peak index and the inner list is each ws (str) in wss cropped and rebinned for that peak
        all_peak_crop_wss.append(peak_crop_wss)

        # set up a function to fit
        bg_func = FunctionFactory.createFunction("LinearBackground")
        peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)

        # estimate starting params
        intens, sigma, bg, centre = _estimate_intensity_background_and_centre(window_ws, 0, 0, len(window_ws.readX(0)) - 1, peak)
        bg_func.setParameter("A0", bg)
        intens_par_name = "I"
        peak_func.setMatrixWorkspace(window_ws, 0, low_bound, hi_bound)
        cen_par_name = "X0"
        peak_func.setParameter(cen_par_name, centre)
        peak_func.setParameter(intens_par_name, intens)
        peak_func.addConstraints(f"{low_bound} < {cen_par_name} < {hi_bound}")
        peak_func.addConstraints(f"{intens_par_name} > 0")

        # for IkedaCarpenterPV, fix instrument-dependent parameters during the initial summed fit
        # this fit is only to estimate peak centre - too many free params causes numerical instability
        if peak_func_name == "IkedaCarpenterPV":
            for par in ("Alpha0", "Alpha1", "Beta0", "Kappa"):
                peak_func.fixParameter(par)

        comp_func = _make_composite(peak_func, bg_func)
        fit_kwargs["InputWorkspace"] = window_ws.name()
        fit_kwargs["StartX"] = low_bound
        fit_kwargs["EndX"] = hi_bound

        fit_object = Fit(
            Function=comp_func,
            Output=f"composite_fit_{peak}",
            MaxIterations=50,  # if it hasn't fit in 50 it is likely because the texture has the peak missing
            **fit_kwargs,
        )
        out_peak_func = fit_object.Function.function.getFunction(0)
        x0 = out_peak_func.getParameterValue(cen_par_name)
        x0_lims.append((x0 * (1 - 3e-3), x0 * (1 + 3e-3)))
    return x0_lims, all_peak_crop_wss


def get_initial_fit_function_and_kwargs_from_specs(
    ws: Workspace2D,
    ws_tof: Workspace2D,
    peak: float,
    x_window: tuple[float, float],
    x0_window: tuple[float, float],
    parameters_to_tie: Sequence[str] | None,
    peak_func_name: str,
    bg_func_name: str,
    tie_bkg: bool,
) -> Tuple[str, dict, Sequence[float]]:
    # modification of get_initial_fit_function_and_kwargs to just fit a peak within the x_window

    # get the number of spectra
    si = ws.spectrumInfo()
    ispecs = list(range(si.size()))

    # set up the fit window and data structures
    x_start, x_end = x_window
    fit_kwargs = {}
    approx_bkgs = []
    intensity_estimates = []

    # set up the overall fit wrapper function and base versions of the individual peak and background
    function = MultiDomainFunction()
    bg_func = FunctionFactory.createFunction(bg_func_name)
    base_peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)

    # save parameter names for future ties/constraints
    intens_par_name = "I"
    cen_par_name = "X0"
    width_par_name = base_peak_func.getWidthParameterName()

    # for each of the spectra
    for ispec in ispecs:
        # convert d spacing to tof - better step size for A, B and S refinement
        diff_consts = si.diffractometerConstants(ispec)
        tof_peak = UnitConversion.run("dSpacing", "TOF", peak, 0, DeltaEModeType.Elastic, diff_consts)
        tof_start = UnitConversion.run("dSpacing", "TOF", x_start, 0, DeltaEModeType.Elastic, diff_consts)
        tof_end = UnitConversion.run("dSpacing", "TOF", x_end, 0, DeltaEModeType.Elastic, diff_consts)

        # get the window indices for the spectra
        istart = ws_tof.yIndexOfX(tof_start, ispec)
        iend = ws_tof.yIndexOfX(tof_end, ispec)

        # get param estimates
        intens, sigma, bg, centre = _estimate_intensity_background_and_centre(ws_tof, ispec, istart, iend, tof_peak)
        intensity_estimates.append(intens)
        approx_bkgs.append(bg)

        # create an individual peak, using estimated values as initial guess
        peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
        peak_func.setParameter(cen_par_name, centre)
        peak_func.setParameter(intens_par_name, intens)
        peak_func.setMatrixWorkspace(ws_tof, ispec, tof_start, tof_end)

        # calculate constraint values
        # convert x0 bounds to TOF
        x0_lower = UnitConversion.run("dSpacing", "TOF", x0_window[0], 0, DeltaEModeType.Elastic, diff_consts)
        x0_upper = UnitConversion.run("dSpacing", "TOF", x0_window[1], 0, DeltaEModeType.Elastic, diff_consts)
        # constrain the values of S to be at least half bin width and no more than half the window size
        scale_factor = 2 * np.sqrt(2 * np.log(2))
        width_min = 0.5 * ((tof_end - tof_start) / (iend - istart)) * scale_factor
        width_max = max(width_min + 1e-10, ((tof_end - tof_start) / 2) * scale_factor)

        # add these constraints
        peak_func.addConstraints(f"{x0_lower} < {cen_par_name} < {x0_upper}")
        peak_func.addConstraints(f"0 < {intens_par_name} < {intens * 5}")
        if width_par_name:
            peak_func.addConstraints(f"{width_min}<{width_par_name}<{width_max}")

        if not tie_bkg:
            bg_func.setParameter("A0", bg)

        # package up the spectra fit functions (peak + background) into a composite function
        comp_func = _make_composite(peak_func, bg_func)
        function.add(comp_func)
        function.setDomainIndex(ispec, ispec)
        function.setMatrixWorkspace(ws_tof, ispec, tof_start, tof_end)

        # set the fit kwargs for this spectra
        key_suffix = f"_{ispec}" if ispec > 0 else ""
        fit_kwargs["InputWorkspace" + key_suffix] = ws_tof.name()
        fit_kwargs["StartX" + key_suffix] = tof_start
        fit_kwargs["EndX" + key_suffix] = tof_end
        fit_kwargs["WorkspaceIndex" + key_suffix] = int(ispec)

    # add parameter ties
    ties = []

    # first tie background
    if tie_bkg:
        function, ties = _tie_bkg(function, approx_bkgs, ties)

    # then any other nominated parameters
    available_params = [base_peak_func.getParamName(i) for i in range(base_peak_func.nParams())]
    if parameters_to_tie is not None:
        invalid = [p for p in parameters_to_tie if p not in available_params]
        if invalid:
            raise ValueError(f"Invalid parameter(s) to tie: {invalid}. Available: {available_params}")
        else:
            for idom in range(1, function.nDomains()):
                # tie global params to first
                for par in parameters_to_tie:
                    ties.append(f"f{idom}.f0.{par}=f0.f0.{par}")  # global peak pars

    # add ties directly to function object to preserve workspace references on member functions
    if ties:
        function.addTies(",".join(ties))
    return function, fit_kwargs, intensity_estimates


def rerun_fit_with_new_ws(
    mdf: MultiDomainFunction,
    fit_kwargs: dict,
    md_fit_kwargs: dict,
    new_ws: Workspace2D,
    x0_frac_move: float,
    iters: int,
    parameters_to_fix: Sequence[str] | None = None,
    parameters_to_tie: Sequence[str] | None = None,
    tie_background: bool = False,
    is_final: bool = False,
    last_fit_ic: bool = False,
) -> Tuple[Fit, dict]:
    # update the input workspace in the fitting kwargs
    for k in md_fit_kwargs.keys():
        if "InputWorkspace" in k:
            md_fit_kwargs[k] = new_ws.name()

    ties = []
    new_func = MultiDomainFunction()
    for idom in range(mdf.nFunctions()):
        comp = mdf[idom]
        peak = comp[0]
        bg = comp[1]

        peak_name = "IkedaCarpenterPV" if last_fit_ic and is_final else peak.name()

        new_peak = FunctionFactory.Instance().createPeakFunction(peak_name)

        intens_par_name = "I"
        cen_par_name = "X0"

        intens = max(peak.getParameterValue(intens_par_name), 1)
        x0 = peak.getParameterValue(cen_par_name)
        key_suffix = f"_{idom}" if idom > 0 else ""

        # create fresh peak as ties are causing problems
        if last_fit_ic and is_final:
            # set X0 and I BEFORE setMatrixWorkspace so that the instrument
            # parameter file formulas for SigmaSquared and Gamma are evaluated
            # at the correct peak centre rather than at X0=0 (default)
            new_peak.setParameter(cen_par_name, x0)
            new_peak.setParameter(intens_par_name, intens)
            new_peak.setMatrixWorkspace(new_ws, idom, md_fit_kwargs["StartX" + key_suffix], md_fit_kwargs["EndX" + key_suffix])
            # if we have changed to IC for the last fit we will just use default parameter ties
            if peak.name() != "IkedaCarpenter":
                parameters_to_tie = _get_default_param_ties("IkedaCarpenterPV", None)
            # fit_kwargs = {**fit_kwargs, "Minimizer": "Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08"}

        else:
            [
                new_peak.setParameter(param, peak.getParameterValue(param))
                for param in [new_peak.getParamName(i) for i in range(new_peak.nParams())]
            ]
        # set workspace on peak function directly so IkedaCarpenterPV can calculate wavelengths
        new_peak.setMatrixWorkspace(new_ws, idom, md_fit_kwargs["StartX" + key_suffix], md_fit_kwargs["EndX" + key_suffix])

        # update constraints around new values

        if not is_final:
            # don't constrain the intensity on the final fit
            new_peak.addConstraints(f"{max(intens / 2, 1e-6)}<{intens_par_name}<{intens * 2}")
        new_peak.addConstraints(f"{x0 * (1 - x0_frac_move)}<{cen_par_name}<{x0 * (1 + x0_frac_move)}")
        # apply ties to the first domain, if ties are required
        if idom > 0:
            if tie_background:
                for ipar_bg in range(bg.nParams()):
                    par = bg.getParamName(ipar_bg)
                    ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
            if parameters_to_tie:
                for par in parameters_to_tie:
                    ties.append(f"f{idom}.f0.{par}=f0.f0.{par}")
        # fix parameters if required
        if parameters_to_fix:
            for param in parameters_to_fix:
                new_peak.fixParameter(param)

        # for IkedaCarpenterPV, fix instrument parameters during non-final fits to improve speed and stability
        if not is_final and new_peak.name() == "IkedaCarpenterPV":
            for par in ("Alpha0", "Alpha1", "Beta0", "Kappa"):
                new_peak.fixParameter(par)

        comp_func = _make_composite(new_peak, bg)
        new_func.add(comp_func)
        new_func.setDomainIndex(idom, idom)

    # add ties directly to function object to preserve workspace references on member functions
    if ties:
        new_func.addTies(",".join(ties))

    return Fit(
        Function=new_func,
        Output=f"fit_{new_ws.name()}",
        MaxIterations=iters,
        **fit_kwargs,
        **md_fit_kwargs,
    ), md_fit_kwargs


def _get_default_param_ties(peak_func_name: str, parameters_to_tie: Sequence[str] | None) -> Sequence[str]:
    if not parameters_to_tie:
        match peak_func_name:
            case "BackToBackExponential":
                parameters_to_tie = ("A", "B")
            case "IkedaCarpenterPV":
                parameters_to_tie = ("Alpha0", "Alpha1", "Beta0", "Kappa")
    return parameters_to_tie


def calc_intens_and_sigma_arrays(fit_result: Fit) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    function = fit_result["Function"]
    ndoms = function.nDomains()
    intens = np.zeros(ndoms)
    sigma = np.zeros(intens.shape)
    intens_over_sig = np.zeros(intens.shape)
    peak_limits = np.full(intens.shape, None)
    for idom, comp_func in enumerate(function):
        intens[idom] = comp_func.getParameterValue("f0.I")
        ws_fit = get_eval_ws(fit_result["OutputWorkspace"], idom, ndoms)
        sigma[idom], peak_limits[idom] = calc_sigma_from_summation(ws_fit.readX(0), ws_fit.readE(0) ** 2, ws_fit.readY(3))
    ivalid = ~np.isclose(sigma, 0)
    intens_over_sig[ivalid] = intens[ivalid] / sigma[ivalid]
    return intens, sigma, intens_over_sig, peak_limits


def fit_all_peaks(
    wss: Sequence[str],
    peaks: Sequence[float],
    peak_window: float,
    save_dir: str,
    override_dir: bool = False,
    i_over_sigma_thresh: float = 2.0,
    nan_replacement: str | None = "zeros",
    no_fit_value_dict: dict | None = None,
    smooth_vals: Sequence[int] = (3, 2),
    tied_bkgs: Sequence[bool] = (False, True),
    final_fit_raw: bool = True,
    parameters_to_tie: Sequence[str] | None = None,
    subsequent_fit_param_fix: Sequence[str] | None = None,
    peak_func_name: str = "BackToBackExponential",
    last_fit_ic: bool = False,
    max_fit_iters: int = 50,
) -> None:
    """
    Fit all the peaks given in all the spectra of all the workspaces, for use in a texture analysis workflow
    wss: Workspace names of all the workspaces to fit
    peaks: Sequence of peak positions in d-spacing
    peak_window: size of the window to create around the desired peak for purpose of fitting
    save_dir: directory to save the results in
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    i_over_sigma_thresh: I/sig less than this value will be deemed as no peak and parameter values will be nan or specified value
    nan_replacement: method options are ("zero", "min", "max", "mean") will try to replace the nan values in columns
                     zero - will replace all nans with 0.0
                     min/max/mean - will replace all nans in a column with the min/max/mean non-nan value (otherwise will remain nan)
    no_fit_value_dict: allows the user to specify the unfit default value of parameters as a dict of key:value pairs
    smooth_vals: the number of bins which should be combined together to improve SNR stats
    tied_bkgs: a bool flag for each of the subsequent fits whether the background fits should be independent for spectra
    final_fit_raw: flag for whether the final fit should be done with no smoothing
    parameters_to_tie: parameters which should be tied across spectra. If None, defaults are used based on peak function:
                       BackToBackExponential: ("A", "B"), IkedaCarpenterPV: ("Alpha0", "Alpha1", "Beta0", "Kappa")
    subsequent_fit_param_fix: parameters which should be fixed after the initial fit (Default is None)
    peak_func_name: peak function to use, should be either BackToBackExponential or IkedaCarpenterPV
    max_fit_iters: maximum number of iterations for a single fit
    """

    # currently the only fit functions intended to be used - less flexibility here allows for less user input
    supported_peaks = ("BackToBackExponential", "IkedaCarpenterPV")
    bg_func_name = "LinearBackground"

    if peak_func_name not in supported_peaks:
        logger.warning(
            f"Provided peak function: '{peak_func_name}' not one of the supported peak functions: ({', '.join(supported_peaks)})."
            f" Behaviour may be unreliable."
        )

    # define some parameters for the fit
    fit_kwargs = {
        "StepSizeMethod": "Sqrt epsilon",
        "IgnoreInvalidData": False,
        "CreateOutput": True,
        "OutputCompositeMembers": True,
        "Minimizer": "Levenberg-Marquardt",
        "CostFunction": "Unweighted least squares",
    }

    parameters_to_tie = _get_default_param_ties(peak_func_name, parameters_to_tie)

    # we are initially going to fit a summed spectra to get a good starting point for the peak centre
    # we will then fix the amount this can change in the individual fits

    x0_lims, all_cropped_rebinned_wss = fit_initial_summed_spectra(wss, peaks, peak_window, fit_kwargs.copy(), peak_func_name)
    for iws, wsname in enumerate(wss):
        # notice user how far through the fitting they are (useful if any fits fail)
        logger.notice(f"Fitting Workspace: {wsname} ({iws + 1}/{len(wss)})")

        # obtain the ws and metadata about ws
        ws = ADS.retrieve(wsname)
        run, prefix = _get_run_and_prefix_from_ws_log(ws, wsname)
        grouping = _get_grouping_from_ws_log(ws)

        # loop over the peaks
        for ipeak, peak in enumerate(peaks):
            # perform fitting in TOF as the parameter magnitudes are better for fitting
            ws_tof = ConvertUnits(InputWorkspace=all_cropped_rebinned_wss[ipeak][iws], OutputWorkspace="ws_tof", Target="TOF")

            # approach will be to use iterative fits and these iteration can have optionally 'rebunched' data to improve SNR
            fit_wss = []
            bkg_is_tied = []
            if len(smooth_vals) > 0:
                for i, smooth_val in enumerate(smooth_vals):
                    fit_wss.append(Rebunch(InputWorkspace=ws_tof, OutputWorkspace=f"smooth_ws_{smooth_val}", NBunch=smooth_val))
                    bkg_is_tied.append(tied_bkgs[i])
            # if no smoothing values are given, the initial fit should just be on ws_tof
            # if final_fit_raw flagged, ws_tof should be added to the end of the fit_wss stack
            if final_fit_raw or len(smooth_vals) == 0:
                fit_wss.append(ws_tof)
                bkg_is_tied.append(True)

            # if the peak func isn't already Ikeda Carpenter, and the last_fit_ic is true, add another ws_tof to be fit
            if last_fit_ic and peak_func_name != "IkedaCarpenterPV":
                fit_wss.append(CloneWorkspace(ws_tof, OutputWorkspace="ws_tof_IkedaCarpenter"))
                bkg_is_tied.append(True)

            # low level information
            logger.information(f"Workspace: {wsname}, Peak: {peak}")

            # set up an index of which of the fit iterations we are on
            fit_num = 0
            # set up final ws and file paths
            out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
            out_file = out_ws + ".nxs"
            out_path = path.join(save_dir, out_file) if override_dir else path.join(save_dir, grouping, str(peak), out_file)
            out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)

            # get window bounds
            xmin, xmax = peak - peak_window, peak + peak_window

            # perform initial fit set up and fit
            fit_ws = fit_wss[fit_num]
            initial_function, md_fit_kwargs, intensity_estimates = get_initial_fit_function_and_kwargs_from_specs(
                ws, fit_ws, peak, (xmin, xmax), x0_lims[ipeak], parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied[fit_num]
            )
            fit_object = Fit(
                Function=initial_function,
                Output=f"fit_{fit_ws}",
                MaxIterations=max_fit_iters,
                **fit_kwargs,
                **md_fit_kwargs,
            )

            # perform subsequent fits
            while len(fit_wss) - 1 > fit_num:
                fit_num += 1
                mdf = fit_object.Function.function
                fit_ws = fit_wss[fit_num]
                fit_object, md_fit_kwargs = rerun_fit_with_new_ws(
                    mdf,
                    fit_kwargs,
                    md_fit_kwargs,
                    fit_ws,
                    0.02,  # allow x0 to only vary by 2% from previous fit
                    max_fit_iters,
                    subsequent_fit_param_fix,
                    parameters_to_tie,
                    bkg_is_tied[fit_num],
                    fit_num == len(fit_wss) - 1,
                    last_fit_ic,
                )

            # establish which detectors have sufficient I over sigma
            mdf = fit_object.Function.function
            fit_result = {"Function": mdf, "OutputWorkspace": fit_object.OutputWorkspace.name()}
            # update peak mask based on I/sig from fit
            *_, i_over_sigma, _ = calc_intens_and_sigma_arrays(fit_result)
            fit_mask = i_over_sigma > i_over_sigma_thresh

            # setup output table columns
            spec_fit = ADS.retrieve(f"fit_{fit_ws}_Parameters")
            si = ws.spectrumInfo()
            out_tab.addColumn("int", "wsindex")
            out_tab.addColumn("double", "I_est")
            all_params = spec_fit.column("Name")[:-1]  # last row is cost function
            param_vals = spec_fit.column("Value")[:-1]
            param_errs = spec_fit.column("Error")[:-1]
            u_params = []
            for col in all_params:  # last col is cost of whole fit
                spec_num, func_num, param = col.split(".")
                # assume first function is the peak
                if func_num == "f0" and param not in u_params:
                    u_params.append(param)
                    out_tab.addColumn("double", param)
                    out_tab.addColumn("double", f"{param}_err")
                    out_tab.addColumn("double", f"{param}/{param}_err")

            # get user defined default vals for unsuccessful fit parameters
            default_vals = get_default_values(u_params, no_fit_value_dict)

            # populate the rows of the table
            table_vals = np.zeros((si.size(), 3 * len(u_params) + 1))  # intens_est + p_1_val, p_1_err, pm_1_val/err +...
            for ispec in range(si.size()):
                # logic for spectra which HAVE been fit successfully
                if fit_mask[ispec]:
                    # add estimate of I
                    row = [intensity_estimates[ispec]]
                    for p in u_params:
                        param_name = f"f{ispec}.f0.{p}"
                        pind = all_params.index(param_name)
                        if p != "X0":
                            row += [param_vals[pind], param_errs[pind], np.divide(param_vals[pind], param_errs[pind])]
                        else:
                            # for x0, convert back to d spacing
                            diff_consts = si.diffractometerConstants(ispec)
                            d_peak = UnitConversion.run("TOF", "dSpacing", param_vals[pind], 0, DeltaEModeType.Elastic, diff_consts)
                            d_err = convert_TOFerror_to_derror(diff_consts, param_errs[pind], d_peak)
                            row += [d_peak, d_err, np.divide(d_peak, d_err)]
                # logic for spectra which HAVE NOT been fit successfully
                else:
                    row = [default_vals.get("I_est", np.nan)]
                    for p in u_params:
                        row += [default_vals[p], np.inf, 0.0]
                table_vals[ispec] = row
            if nan_replacement:
                table_vals = replace_nans(table_vals, nan_replacement)
            for i, row in enumerate(table_vals):
                out_tab.addRow([i] + list(row))

            # save the final table
            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


# ~fitting utility functions~


def _tie_bkg(function: MultiDomainFunction, approx_bkgs: np.ndarray, ties: List[str]) -> Tuple[MultiDomainFunction, List[str]]:
    function[0][1]["A0"] = np.mean(approx_bkgs)
    for idom in range(1, function.nDomains()):
        for ipar_bg in range(function[idom][1].nParams()):
            par = function[idom][1].getParamName(ipar_bg)
            ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
    return function, ties


def _estimate_intensity_background_and_centre(
    ws: Workspace2D, ispec: int, istart: int, iend: int, peak: float
) -> Tuple[float, float, float, float]:
    xdat = ws.readX(ispec)[istart:iend]
    bin_width = np.diff(xdat)
    bin_width = np.hstack((bin_width, bin_width[-1]))  # easier than checking iend and istart not out of bounds
    y = ws.readY(ispec)[istart:iend]
    if not np.any(y > 0):
        return 0.0, 0.0, 0.0, peak
    e = ws.readE(ispec)[istart:iend]
    ibg, _ = PeakData.find_bg_pts_seed_skew(y)
    bg = np.mean(y[ibg])
    intensity = np.trapezoid((y - bg), xdat)
    sigma = np.sqrt(np.sum((e * bin_width) ** 2))
    centre_arg = np.argmax(y)
    centre = xdat[centre_arg]
    return intensity, sigma, bg, centre


def _get_run_and_prefix_from_ws_log(ws: Workspace2D, wsname: str) -> Tuple[str, str]:
    try:
        run = str(ws.getRun().getLogData("run_number").value)
        prefix = wsname.split(run)[0]
    except:
        run = "unknown"
        prefix = ""
    return run, prefix


def _get_grouping_from_ws_log(ws: Workspace2D) -> str:
    try:
        grouping = str(ws.getRun().getLogData("Grouping").value)
    except RuntimeError:
        grouping = "GROUP"
    return grouping


def get_default_values(params: Sequence[str], no_fit_dict: dict) -> dict:
    defaults = dict(zip(params, [np.nan for _ in params]))
    if isinstance(no_fit_dict, dict):
        for k, v in no_fit_dict.items():
            defaults[k] = v
    return defaults


def replace_nans(vals: np.ndarray, method: str | None = None) -> np.ndarray:
    if not method:
        return vals
    if method == "zeros":
        return np.nan_to_num(vals, nan=0)
    func = {"mean": np.nanmean, "max": np.nanmax, "min": np.nanmin}[method]
    out = vals.copy()
    col_stat = func(out, axis=0)
    nan_mask = np.isnan(out)
    out[nan_mask] = col_stat[np.where(nan_mask)[1]]
    return out
