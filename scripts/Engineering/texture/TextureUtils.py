# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from os import path, scandir
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from Engineering.texture.correction.correction_model import TextureCorrectionModel
from mantid.simpleapi import (
    SaveNexus,
    logger,
    CreateEmptyTableWorkspace,
    LoadCIF,
    Fit,
    CreateSingleValuedWorkspace,
    ConvertUnits,
    Rebunch,
)
from pathlib import Path
from Engineering.EnggUtils import GROUP
from Engineering.EnginX import EnginX
from mantid.geometry import CrystalStructure
from mantid.api import AnalysisDataService as ADS, MultiDomainFunction, FunctionFactory
from typing import Optional, Sequence, Union, Tuple
from mantid.dataobjects import Workspace2D
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from plugins.algorithms.IntegratePeaks1DProfile import calc_intens_and_sigma_arrays
from mantid.kernel import DeltaEModeType, UnitConversion, UnitParams
from plugins.algorithms.peakdata_utils import PeakData
# -------- Utility --------------------------------


def find_all_files(directory):
    """
    find all the files in a directory

    directory: directory to iterate over
    """
    with scandir(directory) as entries:
        return [entry.path for entry in entries if entry.is_file()]


def mk(dir_path: str):
    """
    make a directory

    dir_path: path to make a directory at
    """
    p = Path(dir_path)
    if not p.exists():
        p.mkdir()


class TextureInstrument(EnginX):
    # for now, just a wrapper to set up for inheriting from different instruments
    pass


# -------- Focus Script Logic--------------------------------


def run_focus_script(
    wss: Sequence[str],
    focus_dir: str,
    van_run: str,
    ceria_run: str,
    full_instr_calib: str,
    grouping: Optional[str] = None,
    prm_path: Optional[str] = None,
    spectrum_num: Optional[str] = None,
    groupingfile_path: Optional[str] = None,
) -> None:
    """
    Focus data for use in a texture analysis pipeline. Currently only ENGIN-X is supported,
    but TextureInstrument class should grow to include others.

    wss: Sequence of workspaces to be focused, can be paths to files or ws names
    focus_dir: directory of where the focused data should be saved
    van_run: the run number/ file path of the vanadium calibration run
    ceria_run: the run number/ file path of the latest ceria calibration run at time of experiment
    full_instr_calib: path to the full instrument calibration file (can be found in settings of Engineering Diffraction Interface)
    grouping: key for desired detector grouping, if standard, otherwise use the prm path
    prm_path: optional path to the grouping prm file (produced during calibration), if using a standard detector grouping,
              just use the grouping argument
    spectrum_num: optional string of spectra numbers if desired to define custom grouping by specifying the spectra
    groupingfile_path: optional path to a grouping ".cal" or ".xml" file, alternative to prm_path
    """
    group = GROUP(grouping) if grouping else None
    model = TextureInstrument(
        vanadium_run=van_run,
        ceria_run=ceria_run,
        focus_runs=wss,
        save_dir=focus_dir,
        prm_path=prm_path,
        full_inst_calib_path=full_instr_calib,
        group=group,
        spectrum_num=spectrum_num,
        groupingfile_path=groupingfile_path,
    )

    mk(focus_dir)
    model.main()


# -------- Absorption Script Logic--------------------------------


def run_abs_corr(
    wss: Sequence[str],
    ref_ws: Optional[str] = None,
    orientation_file: Optional[str] = None,
    orient_file_is_euler: Optional[bool] = None,
    euler_scheme: Optional[str] = None,
    euler_axes_sense: Optional[str] = None,
    copy_ref: bool = False,
    include_abs_corr: bool = False,
    monte_carlo_args: Optional[str] = None,
    gauge_vol_preset: Optional[str] = None,
    gauge_vol_shape_file: Optional[str] = None,
    include_atten_table: bool = False,
    eval_point: Optional[Union[str, float]] = None,
    eval_units: Optional[str] = None,
    exp_name: Optional[str] = None,
    root_dir: str = ".",
    include_div_corr: bool = False,
    div_hoz: Optional[float] = None,
    div_vert: Optional[float] = None,
    det_hoz: Optional[float] = None,
    clear_ads_after: bool = True,
) -> None:
    """
    Apply absorption correction to data for use in texture analysis pipeline
    wss: Sequence of workspace names to have corrections calculated and applied
    ref_ws: Name of the reference workspace, if one required
    orientation_file: path to the orientation file (should be .txt with one line per run)
    orient_file_is_euler: flag for whether the file provides euler goniometer angles or direct rotation matrices
    euler_scheme: the lab frame directions along which each axis of the goniometer initially lies
    euler_axes_sense: the sense of the rotation around each of the axes (1 being CCW, -1 being CW)
    copy_ref: Whether the reference sample should be copied to each ws
    include_abs_corr: Whether the workspaces should have the absorption correction applied
    monte_carlo_args: String of arguments to supply to the MonteCarloAbsorption alg e.g. "Arg1: val1, Arg2: val2"
    gauge_vol_preset: Name of the preset to use for the gauge volume, currently ("4mmCube"), otherwise should be Custom or No Gauge Volume
    gauge_vol_shape_file: Path to custom gauge volume shape file
    include_atten_table: flag for whether a table of attenuation values at a specified point should be created
    eval_point: point to calculate the attenuation coefficient at
    eval_units: units which the eval_point is given in
    exp_name: Name of the experiment to act as main folder name
    root_dir: Directory path in which the experiment directory is constructed
    include_div_corr: Flag for whether to include a beam divergence correction
    div_hoz: Value of beam divergence in the horizontal plane
    div_vert: Value of beam divergence in the vertical plane
    det_hoz: Value of divergence on the detector in the horizontal plane
    clear_ads_after: Flag for whether the produced files should be removed from the ADS after they have been saved
    """
    model = TextureCorrectionModel()
    model.set_reference_ws = ref_ws

    valid_inputs, error_msg = validate_abs_corr_inputs(
        ref_ws,
        orientation_file,
        orient_file_is_euler,
        euler_scheme,
        euler_axes_sense,
        copy_ref,
        include_abs_corr,
        gauge_vol_preset,
        gauge_vol_shape_file,
        include_atten_table,
        eval_point,
        eval_units,
        include_div_corr,
        div_hoz,
        div_vert,
        det_hoz,
    )
    if not valid_inputs:
        logger.error(error_msg)
        return
    # otherwise run script
    if orientation_file:
        model.load_all_orientations(wss, orientation_file, orient_file_is_euler, euler_scheme, euler_axes_sense)

    out_wss = [f"Corrected_{ws}" for ws in wss]

    if copy_ref:
        model.copy_sample_info(ref_ws, wss)

    for i, ws in enumerate(wss):
        abs_corr = 1.0
        div_corr = 1.0

        if include_abs_corr:
            model.define_gauge_volume(ws, gauge_vol_preset, gauge_vol_shape_file)
            model.calc_absorption(ws, monte_carlo_args)
            abs_corr = "_abs_corr"
            if include_atten_table:
                atten_vals = model.read_attenuation_coefficient_at_value(abs_corr, eval_point, eval_units)
                model.write_atten_val_table(
                    ws,
                    atten_vals,
                    eval_point,
                    eval_units,
                    exp_name,
                    None,
                    root_dir,
                )

        if include_div_corr:
            model.calc_divergence(ws, div_hoz, div_vert, det_hoz)
            div_corr = "_div_corr"

        model.apply_corrections(ws, out_wss[i], None, root_dir, abs_corr, div_corr, None, clear_ads_after)


def validate_abs_corr_inputs(
    ref_ws: Optional[str] = None,
    orientation_file: Optional[str] = None,
    orient_file_is_euler: Optional[bool] = None,
    euler_scheme: Optional[str] = None,
    euler_axes_sense: Optional[str] = None,
    copy_ref: bool = False,
    include_abs_corr: bool = False,
    gauge_vol_preset: Optional[str] = None,
    gauge_vol_shape_file: Optional[str] = None,
    include_atten_table: bool = False,
    eval_point: Optional[Union[str, float]] = None,
    eval_units: Optional[str] = None,
    include_div_corr: bool = False,
    div_hoz: Optional[float] = None,
    div_vert: Optional[float] = None,
    det_hoz: Optional[float] = None,
) -> Tuple[bool, str]:
    error_msg = ""
    # validate inputs
    if orientation_file:
        valid_orientation_inputs = isinstance(orient_file_is_euler, bool)
        if not valid_orientation_inputs:
            error_msg += r"If orientation file is specified, must flag orient_file_is_euler.\n"
        if valid_orientation_inputs and orient_file_is_euler:
            # if is euler flag, require euler_scheme and euler_axes_sense
            valid_orientation_inputs = isinstance(euler_scheme, str) and isinstance(euler_axes_sense, str)
            if not valid_orientation_inputs:
                error_msg += r"If orientation file is euler, must provide scheme and sense.\n"

    if copy_ref:
        if not isinstance(ref_ws, str):
            error_msg += r"If copy_ref is True, must provide ref_ws.\n"

    if include_abs_corr:
        if gauge_vol_preset == "Custom":
            if not isinstance(gauge_vol_shape_file, str):
                error_msg += r"If custom gauge volume required, must provide shape xml as file.\n"

    if include_atten_table:
        if not (isinstance(eval_point, Union[str, float]) and isinstance(eval_units, str)):
            error_msg += r"If attenuation table required, must provide valid point and units.\n"

    if include_div_corr:
        if not (isinstance(div_hoz, float) and isinstance(div_vert, float) and isinstance(det_hoz, float)):
            error_msg += r"If divergence correction required, must provide valid values.\n"
    # if error_msg is still empty string, the inputs are assumed to be valid
    return error_msg == "", error_msg


# -------- Fitting Script Logic--------------------------------


def get_initial_fit_function_and_kwargs_from_specs(
    ws: Workspace2D,
    ws_tof: Workspace2D,
    peak: float,
    x_window: tuple[float, float],
    parameters_to_tie: Sequence[str],
    peak_func_name: str,
    bg_func_name: str,
    tie_bkg: bool,
) -> Tuple[str, dict, Sequence[float], float]:
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
    base_peak_func.setIntensity(1.0)

    # save parameter names for future ties/constraints
    intens_par_name = next(
        base_peak_func.getParamName(ipar) for ipar in range(base_peak_func.nParams()) if base_peak_func.isExplicitlySet(ipar)
    )
    cen_par_name = base_peak_func.getCentreParameterName()
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
        peak_func.setCentre(centre)
        peak_func.setIntensity(intens)

        # calculate constraint values
        # allow x0 to move 10% of the total fit window from the estimate (highest point)
        x0_move = (tof_end - tof_start) / 10
        # constrain the values of S to be at least half bin width and no more than half the window size
        scale_factor = 2 * np.sqrt(2 * np.log(2))
        width_min = 0.5 * ((tof_end - tof_start) / (iend - istart)) * scale_factor
        width_max = max(width_min + 1e-10, ((tof_end - tof_start) / 2) * scale_factor)

        # add these constraints
        peak_func.addConstraints(f"{centre - x0_move} < {cen_par_name} < {centre + x0_move}")
        peak_func.addConstraints(f"{intens / 5} < {intens_par_name} < {intens * 5}")
        peak_func.addConstraints(f"{width_min}<{width_par_name}<{width_max}")

        if not tie_bkg:
            bg_func.setParameter("A0", bg)

        # package up the spectra fit functions (peak + background) into a composite function
        comp_func = CompositeFunctionWrapper(FunctionWrapper(peak_func), FunctionWrapper(bg_func), NumDeriv=True)
        function.add(comp_func.function)
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

    # first background
    if tie_bkg:
        function[0][1]["A0"] = np.mean(approx_bkgs)
        for idom in range(1, function.nDomains()):
            for ipar_bg in range(function[idom][1].nParams()):
                par = function[idom][1].getParamName(ipar_bg)
                ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")

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

    # if ties are to be added, do so as a string as it is faster
    func = f"{str(function)};ties=({','.join(ties)})" if ties else function
    return func, fit_kwargs, intensity_estimates, x0_move


def rerun_fit_with_new_ws(
    mdf: MultiDomainFunction,
    fit_kwargs: dict,
    md_fit_kwargs: dict,
    new_ws: Workspace2D,
    x0_move: float,
    iters: int,
    parameters_to_fix: Sequence[str],
    tie_background: bool = False,
):
    # update the input workspace in the fitting kwargs
    for k in md_fit_kwargs.keys():
        if "InputWorkspace" in k:
            md_fit_kwargs[k] = new_ws.name()

    bg_ties = []
    new_func = MultiDomainFunction()
    for idom in range(mdf.nFunctions()):
        comp = mdf[idom]
        peak = comp[0]
        bg = comp[1]
        # create fresh peak as ties are causing problems
        new_peak = FunctionFactory.Instance().createPeakFunction(peak.name())
        [new_peak.setParameter(param, peak.getParameterValue(param)) for param in ("I", "X0", "A", "B", "S")]
        # update constraints around new values
        intens = max(peak.getParameterValue("I"), 1)
        x0 = peak.getParameterValue("X0")
        new_peak.addConstraints(f"{max(intens / 2, 1e-6)}<I<{intens * 2}")
        new_peak.addConstraints(f"{x0 - x0_move}<X0<{x0 + x0_move}")
        if tie_background and idom > 0:
            for ipar_bg in range(bg.nParams()):
                par = bg.getParamName(ipar_bg)
                bg_ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
        for param in parameters_to_fix:
            new_peak.fixParameter(param)

        comp_func = CompositeFunctionWrapper(FunctionWrapper(new_peak), FunctionWrapper(bg), NumDeriv=True)
        new_func.add(comp_func.function)
        new_func.setDomainIndex(idom, idom)
        key_suffix = f"_{idom}" if idom > 0 else ""
        new_func.setMatrixWorkspace(new_ws, idom, md_fit_kwargs["StartX" + key_suffix], md_fit_kwargs["EndX" + key_suffix])

    # if ties are required add them
    func = f"{str(new_func)};ties=({','.join(bg_ties)})" if tie_background else new_func

    return Fit(
        Function=func,
        Output=f"fit_{new_ws.name()}",
        MaxIterations=iters,
        **fit_kwargs,
        **md_fit_kwargs,
    ), md_fit_kwargs


def fit_all_peaks(
    wss: Sequence[str],
    peaks: Sequence[float],
    peak_window: float,
    save_dir: str,
    override_dir: bool = False,
    i_over_sigma_thresh: float = 2.0,
    nan_replacement: Optional[str] = "zeros",
    no_fit_value_dict: Optional[dict] = None,
    smooth_vals: Sequence[int] = (3, 2),
    tied_bkgs: Sequence[bool] = (False, True),
    final_fit_raw: bool = False,
    parameters_to_tie: Sequence[str] = ("A", "B"),
    subsequent_fit_param_fix: Sequence[str] = ("A", "B"),
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
    parameters_to_tie: parameters which should be tied across spectra for the initial fit (Default is A and B)
    subsequent_fit_param_fix: parameters which should be fixed after the initial fit (Default is A and B)
    """

    # currently the only fit functions intended to be used - less flexibility here allows for less user input
    peak_func_name = "BackToBackExponential"
    bg_func_name = "LinearBackground"

    # define some parameters for the fit
    fit_kwargs = {
        "StepSizeMethod": "Sqrt epsilon",
        "IgnoreInvalidData": False,
        "CreateOutput": True,
        "OutputCompositeMembers": True,
        "Minimizer": "Levenberg-Marquardt",
        "CostFunction": "Unweighted least squares",
    }

    for iws, wsname in enumerate(wss):
        # notice user how far through the fitting they are (useful if any fits fail)
        logger.notice(f"Fitting Workspace: {wsname} ({iws + 1}/{len(wss)})")

        # obtain the ws and metadata about ws
        ws = ADS.retrieve(wsname)
        run, prefix = _get_run_and_prefix_from_ws_log(ws, wsname)
        grouping = _get_grouping_from_ws_log(ws)

        # perform fitting in TOF as the parameter magnitudes are better for fitting (A, B, and S)
        ws_tof = ConvertUnits(InputWorkspace=ws, OutputWorkspace="ws_tof", Target="TOF")

        # approach will be to use interative fits and these iteration can have optionally 'rebunched' data to improve SNR
        fit_wss = []
        bkg_is_tied = []
        if len(smooth_vals) > 0:
            for i, smooth_val in enumerate(smooth_vals):
                fit_wss.append(Rebunch(InputWorkspace=ws_tof, OutputWorkspace=f"smooth_ws_{smooth_val}", NBunch=smooth_val))
                bkg_is_tied.append(tied_bkgs[i])
        else:
            # if no smoothing values are given, the initial fit should just be on the ws
            fit_wss.append(ws_tof)
            bkg_is_tied.append(True)
        # if flagged, there will be a final unbunched fit (with the background tied)
        if final_fit_raw:
            fit_wss.append(ws_tof)
            bkg_is_tied.append(True)

        # loop over the peaks
        for peak in peaks:
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
            initial_function, md_fit_kwargs, intensity_estimates, x0_move = get_initial_fit_function_and_kwargs_from_specs(
                ws, fit_ws, peak, (xmin, xmax), parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied[fit_num]
            )
            fit_object = Fit(
                Function=initial_function,
                Output=f"fit_{fit_ws}",
                MaxIterations=50,  # if it hasn't fit in 50 it is likely because the texture has the peak missing
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
                    x0_move,
                    50,
                    subsequent_fit_param_fix,
                    bkg_is_tied[fit_num],
                )

            # establish which detectors have sufficient I over sigma
            mdf = fit_object.Function.function
            fit_result = {"Function": mdf, "OutputWorkspace": fit_object.OutputWorkspace.name()}
            # update peak mask based on I/sig from fit
            *_, i_over_sigma, _ = calc_intens_and_sigma_arrays(fit_result, "Summation")
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

            # get user defined default vals for unsuccessful fit parameters
            default_vals = get_default_values(u_params, no_fit_value_dict)

            # populate the rows of the table
            table_vals = np.zeros((si.size(), 2 * len(u_params) + 1))  # intensity_est + param_1_val, param_1_err, +...
            for ispec in range(si.size()):
                # logic for spectra which HAVE been fit successfully
                if fit_mask[ispec]:
                    row = [intensity_estimates[ispec]]
                    for p in u_params:
                        param_name = f"f{ispec}.f0.{p}"
                        pind = all_params.index(param_name)
                        if p != "X0":
                            row += [param_vals[pind], param_errs[pind]]
                        else:
                            # for x0, convert back to d spacing
                            diff_consts = si.diffractometerConstants(ispec)
                            d_peak = UnitConversion.run("TOF", "dSpacing", param_vals[pind], 0, DeltaEModeType.Elastic, diff_consts)
                            d_err = _convert_TOFerror_to_derror(diff_consts, param_errs[pind], d_peak)
                            row += [d_peak, d_err]
                # logic for spectra which HAVE NOT been fit successfully
                else:
                    row = [default_vals.get("I_est", np.nan)]
                    for p in u_params:
                        row += [default_vals[p], np.nan]
                table_vals[ispec] = row
            if nan_replacement:
                table_vals = replace_nans(table_vals, nan_replacement)
            for i, row in enumerate(table_vals):
                out_tab.addRow([i] + list(row))

            # save the final table
            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


# ~fitting utility functions~


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
    intensity = np.sum((y - bg) * bin_width)
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


def get_default_values(params, no_fit_dict):
    defaults = dict(zip(params, [np.nan for _ in params]))
    if isinstance(no_fit_dict, dict):
        for k, v in no_fit_dict.items():
            defaults[k] = v
    return defaults


def replace_nans(vals, method: str):
    new_vals = np.zeros_like(vals.T)  # want to iterate over table columns
    # if method is zero, replace all nans with zero
    if method == "zeros":
        return np.nan_to_num(vals)
    # otherwise, if there is at least 1 value per row that isn't nan, replace the nans with the min/max/mean of these
    elif method == "mean":
        func = np.mean
    elif method == "max":
        func = np.max
    else:
        func = np.min
    for i, col in enumerate(vals.T):
        non_nan = col[~np.isnan(col)]
        if len(non_nan) > 0:
            new_vals[i] = np.nan_to_num(col, nan=func(non_nan))
        else:
            # if the row is all nan, then we leave it as is
            new_vals[i] = col
    return new_vals.T


def _convert_TOFerror_to_derror(diff_consts, tof_error, d):
    difc = diff_consts[UnitParams.difc]
    difa = diff_consts[UnitParams.difa] if UnitParams.difa in diff_consts else 0
    return tof_error / (2 * difa * d + difc)


# -------- Pole Figure Script Logic--------------------------------


class CrystalPhase:
    def __init__(self, crystal_structure: CrystalStructure):
        self.xtal = crystal_structure

    @classmethod
    def from_cif(cls, cif_file: str):
        ws = CreateSingleValuedWorkspace(StoreInADS=False, EnableLogging=False)
        LoadCIF(ws, cif_file, StoreInADS=False)
        return CrystalPhase(ws.sample().getCrystalStructure())

    @classmethod
    def from_alatt(cls, alatt: np.ndarray, space_group: str = "P 1", basis: str = ""):
        alatt_str = " ".join([str(par) for par in alatt])
        xtal = CrystalStructure(alatt_str, space_group, basis)
        return CrystalPhase(xtal)

    @classmethod
    def from_string(cls, lattice: str, space_group: str, basis: str):
        xtal = CrystalStructure(lattice, space_group, basis)
        return CrystalPhase(xtal)


def get_xtal_structure(input_method: str, *args, **kwargs) -> CrystalStructure:
    match input_method:
        case "cif":
            phase = CrystalPhase.from_cif(*args, **kwargs)
            return phase.xtal
        case "array":
            phase = CrystalPhase.from_alatt(*args, **kwargs)
            return phase.xtal
        case "string":
            phase = CrystalPhase.from_string(*args, **kwargs)
            return phase.xtal
        case _:
            raise ValueError(f"input_method must be: 'cif', 'array', or 'string', '{input_method}' was provided")


def create_pf(
    wss: Sequence[str],
    root_dir: str,
    exp_name: str,
    dir1: Sequence[float] = (1.0, 0.0, 0.0),
    dir2: Sequence[float] = (0.0, 1.0, 0.0),
    dir3: Sequence[float] = (0.0, 0.0, 1.0),
    dir_names: Sequence[str] = ("D1", "D2", "D3"),
    include_scatt_power: bool = False,
    scatter: bool = True,
    scat_vol_pos: Sequence[float] = (0.0, 0.0, 0.0),
    projection_method: str = "Azimuthal",
    params: Optional[Sequence[str]] = None,
    xtal_input: Optional[str] = None,
    xtal_args: Optional[Sequence[str]] = None,
    hkl: Optional[Sequence[int]] = None,
    readout_column: Optional[str] = None,
    kernel: Optional[float] = None,
    chi2_thresh: Optional[float] = None,
    peak_thresh: Optional[float] = None,
    override_dir: bool = False,
) -> None:
    """
    Create a single pole figure, for use in texture analysis workflow

    wss: Workspace names of the ws with the orientation information present as a goniometer matrix
    root_dir: root of the directory to which the data should be saved
    exp_name: experiment name, which provides the overarching folder within the root directory
    dir1: vector of the first principle direction of the sample
    dir2: vector of the second (projection) principle direction of the sample
    dir3: vector of the third principle direction of the sample
    dir_names: Names of the first, second and third principle directions
    include_scatt_power: flag for whether to adjust the value by a scattering power calculation
    scatter: flag for whether the plotted pole figure should be a scatter plot of experimental points, a fitted contour plot, or both
    scat_vol_pos: position of the centre of mass of the scattering gauge volume
    projection_method: the type of projection to use to create the pole figure ("Azimuthal", "Stereographic")
    params: Parameter Workspaces if you want to read a column of this table to each point in the pole figure
    xtal_input: method by which the crystal structure will be input, options are ("cif", "array", "string")
    xtal_args: list of arguments for the specified crystal input:
                for input "cif", require the cif filepath, example: ["C:/User/Fe.cif",],
                for "array" array of lattice parameters, space group, basis, example: [(1.0,1.0,1.0), "P1", "Fe 0 0 0 1 0"]
                for "string" lattice parameter string, space group and basis, example: ["1.0 1.0 1.0", "P1", "Fe 0 0 0 1 0"]
    hkl: H,K,L reflection of the peak fit in the param workspaces
    readout_column: column of the param ws that should be attached to the pole figure table
    kernel: if scatter == False, the kernel size of the gaussian filter applied to smooth the contour plot
    chi2_thresh: if chi2 column present in params, the maximum value which will still get added to the pole figure table
    peak_thresh: if X0 present in params, the maximum allowable difference between a spectra's X0 and the mean X0/
                 X0 corresponding to the provided HKL
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    """
    model = TextureProjection()
    has_xtal = False
    if xtal_input:
        has_xtal = True
        for ws in wss:
            ws = ADS.retrieve(ws)
            xtal = get_xtal_structure(xtal_input, *xtal_args) if xtal_input else None
            ws.sample().setCrystalStructure(xtal)
    # only pass the HKL to CreatePoleFigureTable if crystal structure has been defined
    # otherwise `hkl` is just used for naming the output workspace
    pf_hkl = hkl if has_xtal else None
    out_ws, grouping = model.get_pf_table_name(wss, params, hkl, readout_column)

    dir1, dir2, dir3 = np.asarray(dir1), np.asarray(dir2), np.asarray(dir3)
    ax_transform = np.concatenate((dir1[:, None], dir2[:, None], dir3[:, None]), axis=1)
    ax_labels = dir_names

    save_dirs = (
        [path.join(root_dir, "PoleFigureTables")] if override_dir else model.get_save_dirs(root_dir, "PoleFigureTables", exp_name, grouping)
    )
    chi2_thresh = chi2_thresh if chi2_thresh else 0.0
    peak_thresh = peak_thresh if peak_thresh else 0.0
    model.make_pole_figure_tables(
        wss, params, out_ws, pf_hkl, include_scatt_power, scat_vol_pos, chi2_thresh, peak_thresh, save_dirs, ax_transform, readout_column
    )

    fig = model.plot_pole_figure(
        out_ws,
        projection_method,
        fig=None,
        readout_col=readout_column,
        save_dirs=save_dirs,
        plot_exp=scatter,
        ax_labels=ax_labels,
        contour_kernel=kernel,
    )
    fig.gca().set_title(out_ws)
    try:
        fig.show()
    except IndexError:
        logger.debug("Ignoring a problem with the plt.get_edgecolor. This is (probably) fine")


def make_iterable(param):
    """
    take a single parameter and make a single value in a list

    param: parameter value
    """
    return param if isinstance(param, tuple) or isinstance(param, list) else [param]


def create_pf_loop(
    wss: Sequence[str],
    param_wss: Sequence[Sequence[str]],
    include_scatt_power: bool,
    dir1: Sequence[float],
    dir2: Sequence[float],
    dir3: Sequence[float],
    dir_names: Sequence[str],
    scatter: Union[str, bool],
    scat_vol_pos: Sequence[float],
    save_root: str,
    exp_name: str,
    projection_method: str,
    xtal_input: Optional[str] = None,
    xtal_args: Optional[Sequence[str]] = None,
    hkls: Optional[Union[Sequence[Sequence[int]], Sequence[int]]] = None,
    readout_columns: Optional[Union[str, Sequence[str]]] = None,
    kernel: Optional[float] = None,
    chi2_thresh: Optional[float] = None,
    peak_thresh: Optional[float] = None,
) -> None:
    """
    Create a series of pole figures, for use in texture analysis workflow

    wss: Workspace names of the ws with the orientation information present as a goniometer matrix
    param_wss: Sequence of Parameter Workspaces if you want to read a column of each table to each point in the pole figure
    include_scatt_power: flag for whether to adjust the value by a scattering power calculation
    dir1: vector of the first principle direction of the sample
    dir2: vector of the second (projection) principle direction of the sample
    dir3: vector of the third principle direction of the sample
    dir_names: Names of the first, second and third principle directions
    scatter: flag as to whether the plotted pole figure should be a scatter plot of experimental points or a fitted contour plot.
            the string "both" is also a valid argument and that will create both
    scat_vol_pos: position of the centre of mass of the scattering gauge volume
    save_root: root of the directory to which the data should be saved
    exp_name: experiment name, which provides the overarching folder within the root directory
    projection_method: the type of projection to use to create the pole figure ("Azimuthal", "Stereographic")
    xtal_input: method by which the crystal structure will be input, options are ("cif", "array", "string")
    xtal_args: list of arguments for the specified crystal input:
                for input "cif", require the cif filepath, example: ["C:/User/Fe.cif",],
                for "array" array of lattice parameters, space group, basis, example: [(1.0,1.0,1.0), "P1", "Fe 0 0 0 1 0"]
                for "string" lattice parameter string, space group and basis, example: ["1.0 1.0 1.0", "P1", "Fe 0 0 0 1 0"]
    hkls: H,K,L reflection of each peak fitted by the param workspaces
    readout_columns: each column of the param ws that should be attached to its own pole figure table
    kernel: if scatter == False, the kernel size of the gaussian filter applied to smooth the contour plot
    chi2_thresh: if chi2 column present in params, the maximum value which will still get added to the pole figure table
    peak_thresh: if X0 present in params, the maximum allowable difference between a spectra's X0 and the mean X0/
                 X0 corresponding to the provided HKL
    """
    # get ws paths
    for iparam, params in enumerate(param_wss):
        # if multiple peaks are provided, multiple hkls should also be provided
        hkl = hkls if len(param_wss) == 1 else hkls[iparam] if hkls else None

        for readout_column in make_iterable(readout_columns):
            kwargs = {
                "wss": wss,
                "params": params,
                "include_scatt_power": include_scatt_power,
                "xtal_input": xtal_input,
                "xtal_args": xtal_args,
                "hkl": hkl,
                "readout_column": readout_column,
                "dir1": dir1,
                "dir2": dir2,
                "dir3": dir3,
                "dir_names": dir_names,
                "kernel": kernel,
                "scat_vol_pos": scat_vol_pos,
                "chi2_thresh": chi2_thresh,
                "peak_thresh": peak_thresh,
                "root_dir": save_root,
                "exp_name": exp_name,
                "projection_method": projection_method,
            }
            if scatter == "both":
                for scat in (True, False):
                    kwargs["scatter"] = scat
                    create_pf(**kwargs)
            else:
                kwargs["scatter"] = scatter
                create_pf(**kwargs)
