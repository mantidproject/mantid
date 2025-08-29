# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from os import path, scandir
import sys
from Engineering.texture.correction.correction_model import TextureCorrectionModel
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from mantid.simpleapi import SaveNexus, logger, CreateEmptyTableWorkspace, Fit
from mantid.simpleapi import (
    SaveNexus,
    logger,
    CreateEmptyTableWorkspace,
    LoadCIF,
    Fit,
    CreateSingleValuedWorkspace,
    SmoothData,
    CloneWorkspace,
    ConvertUnits,
)
from pathlib import Path
from Engineering.EnggUtils import GROUP
from Engineering.EnginX import EnginX
from mantid.api import AnalysisDataService as ADS, MultiDomainFunction, FunctionFactory
from typing import Optional, Sequence, Union, Tuple
from mantid.dataobjects import Workspace2D
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from plugins.algorithms.IntegratePeaks1DProfile import PeakFunctionGenerator, calc_intens_and_sigma_arrays
from Engineering.texture.xtal_helper import get_xtal_structure

# import texture helper functions so they can be accessed by users through the TextureUtils namespace
from Engineering.texture.texture_helper import plot_pole_figure

from mantid.kernel import DeltaEModeType, UnitConversion
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
    root_dir: Directory path in which the experiment directory is constructed
    include_div_corr: Flag for whether to include a beam divergence correction
    div_hoz: Value of beam divergence in the horizontal plane
    div_vert: Value of beam divergence in the vertical plane
    det_hoz: Value of divergence on the detector in the horizontal plane
    clear_ads_after: Flag for whether the produced files should be removed from the ADS after they have been saved
    """
    model = TextureCorrectionModel()
    model.set_reference_ws(ref_ws)

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

    model.set_include_abs(include_abs_corr)
    model.set_include_atten(include_atten_table)
    model.set_include_div(include_div_corr)
    model.set_remove_after_processing(clear_ads_after)

    abs_args = {"gauge_vol_preset": gauge_vol_preset, "gauge_vol_file": gauge_vol_shape_file, "mc_param_str": monte_carlo_args}

    atten_args = {"atten_val": eval_point, "atten_units": eval_units}

    div_args = {"hoz": div_hoz, "vert": div_vert, "det_hoz": det_hoz}

    model.calc_all_corrections(wss, out_wss, root_dir=root_dir, abs_args=abs_args, atten_args=atten_args, div_args=div_args)


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


class TexturePeakFunctionGenerator(PeakFunctionGenerator):
    def __init__(self, peak_params_to_fix: Sequence[str], min_width: float = 1e-3):
        super().__init__(peak_params_to_fix)
        self.min_width = min_width

    def get_initial_fit_function_and_kwargs_from_specs(
        self,
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
        si = ws.spectrumInfo()
        ispecs = list(range(si.size()))
        x_start, x_end = x_window
        function = MultiDomainFunction()
        fit_kwargs = {}
        # estimate background
        # init bg func (global)
        bg_func = FunctionFactory.createFunction(bg_func_name)
        # init peak func
        base_peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
        # save parameter names for future ties/constraints
        base_peak_func.setIntensity(1.0)
        self.intens_par_name = next(
            base_peak_func.getParamName(ipar) for ipar in range(base_peak_func.nParams()) if base_peak_func.isExplicitlySet(ipar)
        )
        self.cen_par_name = base_peak_func.getCentreParameterName()
        self.width_par_name = base_peak_func.getWidthParameterName()
        avg_bg = 0
        intensity_estimates = []
        for ispec in ispecs:
            diff_consts = si.diffractometerConstants(ispec)
            tof_peak = UnitConversion.run("dSpacing", "TOF", peak, 0, DeltaEModeType.Elastic, diff_consts)
            tof_start = UnitConversion.run("dSpacing", "TOF", x_start, 0, DeltaEModeType.Elastic, diff_consts)
            tof_end = UnitConversion.run("dSpacing", "TOF", x_end, 0, DeltaEModeType.Elastic, diff_consts)

            istart = ws_tof.yIndexOfX(tof_start, ispec)
            iend = ws_tof.yIndexOfX(tof_end, ispec)

            # get param estimates
            intens, sigma, bg, centre = self._estimate_intensity_background_and_centre(ws_tof, ispec, istart, iend, tof_peak)
            intensity_estimates.append(intens)
            avg_bg += bg
            # add peak, using provided value as guess for centre
            peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
            peak_func.setCentre(centre)
            peak_func.setIntensity(intens)
            # add constraints
            x0_move = (tof_end - tof_start) / 10
            peak_func.addConstraints(f"{centre - x0_move} < {self.cen_par_name} < {centre + x0_move}")
            peak_func.addConstraints(f"{intens / 5} < {self.intens_par_name} < {intens * 5}")
            scale_factor = 2 * np.sqrt(2 * np.log(2))
            self.width_min = 0.5 * ((tof_end - tof_start) / (iend - istart)) * scale_factor  # FWHM > 0.5 * bin-width
            self.width_max = max(
                self.width_min + 1e-10, ((tof_end - tof_start) / 2) * scale_factor
            )  # must be at least 2 FWHM in data range
            peak_func.addConstraints(f"{self.width_min}<{self.width_par_name}<{self.width_max}")
            peak_func.fixParameter("A")
            peak_func.fixParameter("B")
            comp_func = CompositeFunctionWrapper(FunctionWrapper(peak_func), FunctionWrapper(bg_func), NumDeriv=True)
            function.add(comp_func.function)
            function.setDomainIndex(ispec, ispec)
            function.setMatrixWorkspace(ws_tof, ispec, tof_start, tof_end)
            key_suffix = f"_{ispec}" if ispec > 0 else ""
            fit_kwargs["InputWorkspace" + key_suffix] = ws_tof.name()
            fit_kwargs["StartX" + key_suffix] = tof_start
            fit_kwargs["EndX" + key_suffix] = tof_end
            fit_kwargs["WorkspaceIndex" + key_suffix] = int(ispec)
        # set background (background global tied to first domain function)
        function[0][1]["A0"] = avg_bg / len(ispecs)
        available_params = [base_peak_func.getParamName(i) for i in range(base_peak_func.nParams())]
        if parameters_to_tie is not None:
            invalid = [p for p in parameters_to_tie if p not in available_params]
            if invalid:
                raise ValueError(f"Invalid parameter(s) to tie: {invalid}. Available: {available_params}")
        func = self._add_parameter_ties(function, parameters_to_tie) if tie_bkg else function
        return func, fit_kwargs, intensity_estimates, x0_move

    @staticmethod
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

    def _add_parameter_ties(self, function: MultiDomainFunction, pars_to_tie: Sequence[str]) -> str:
        # fix peak params requested
        [function[0][0].fixParameter(par) for par in self.peak_params_to_fix]
        additional_pars_to_fix = set(self.peak_params_to_fix) - set(pars_to_tie)
        ties = []
        for idom in range(1, function.nDomains()):
            # tie global params to first
            for par in pars_to_tie:
                ties.append(f"f{idom}.f0.{par}=f0.f0.{par}")  # global peak pars
            for ipar_bg in range(function[idom][1].nParams()):
                par = function[idom][1].getParamName(ipar_bg)
                ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
            for par in additional_pars_to_fix:
                # pars to be fixed but not global/already tied
                function[idom][0].fixParameter(par)
        # add ties as string (orders of magnitude quicker than self.function.tie)
        return f"{str(function)};ties=({','.join(ties)})"


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


def _rerun_fit_with_new_ws(
    mdf: MultiDomainFunction,
    fit_kwargs: dict,
    md_fit_kwargs: dict,
    new_ws: Workspace2D,
    intensity_parameter: str,
    centre_parameter: str,
    x0_move: float,
    iters: int,
    tie_background: bool = False,
):
    # replace the workspace with the new one
    for k in md_fit_kwargs.keys():
        if "InputWorkspace" in k:
            md_fit_kwargs[k] = new_ws.name()

    # mdf.freeAll()
    bg_ties = []
    for idom in range(mdf.nFunctions()):
        comp = mdf[idom]
        peak = comp[0]
        bg = comp[1]
        peak.freeAll()
        # update constraints around new values
        intens = peak.getParameterValue(intensity_parameter)
        x0 = peak.getParameterValue(centre_parameter)
        A = peak.getParameterValue("A")
        B = peak.getParameterValue("B")
        peak.addConstraints(f"{intens / 2}<{intensity_parameter}<{intens * 2}")
        peak.addConstraints(f"{x0 - (x0_move)}<{centre_parameter}<{x0 + (x0_move)}")
        peak.addConstraints(f"{0.9 * A}<A<{1.1 * A}")
        peak.addConstraints(f"{0.9 * B}<B<{1.1 * B}")
        if tie_background and idom > 0:
            peak.fixParameter("X0")
            peak.fixParameter("A")
            peak.fixParameter("B")
            peak.fixParameter("S")
            for ipar_bg in range(bg.nParams()):
                par = bg.getParamName(ipar_bg)
                bg_ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")

    func = f"{str(mdf)};ties=({','.join(bg_ties)})"

    return Fit(
        Function=func,
        CostFunction="Least squares",
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
    parameters_to_tie: Optional[Sequence[str]] = (),
    override_dir: bool = False,
    i_over_sigma_thresh: float = 2.0,
    nan_replacement: Optional[str] = "zeros",
    no_fit_value_dict: Optional[dict] = None,
    smooth_vals: Sequence[int] = (27, 9),
    tied_bkgs: Sequence[bool] = (False, False),
) -> None:
    """

    Fit all the peaks given in all the spectra of all the workspaces, for use in a texture analysis workflow

    wss: Workspace names of all the workspaces to fit
    peaks: Sequence of peak positions in d-spacing
    peak_window: size of the window to create around the desired peak for purpose of fitting
    save_dir: directory to save the results in
    parameters_to_tie: Sequence of parameters which should be tied across spectra
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    i_over_sigma_thresh: I/sig less than this value will be deemed as no peak and parameter values will be nan or specified value
    nan_replacement: method options are ("zero", "min", "max", "mean") will try to replace the nan values in columns
                     zero - will replace all nans with 0.0
                     min/max/mean - will replace all nans in a column with the min/max/mean non-nan value (otherwise will remain nan)
    no_fit_value_dict: allows the user to specify the unfit default value of parameters as a dict of key:value pairs
    """

    fit_kwargs = {
        "StepSizeMethod": "Default",
        "IgnoreInvalidData": True,
        "CreateOutput": True,
        "OutputCompositeMembers": True,
    }

    for wsname in wss:
        ws = ADS.retrieve(wsname)
        run, prefix = _get_run_and_prefix_from_ws_log(ws, wsname)
        grouping = _get_grouping_from_ws_log(ws)

        # perform fitting in TOF for better parameter estimates
        ws_tof = ConvertUnits(InputWorkspace=ws, OutputWorkspace="ws_tof", Target="TOF")
        # for initial fit smooth data for easier optimisation landscape
        base_fit_wss = []
        base_bkg_is_tied = []
        if len(smooth_vals) > 0:
            for i, smooth_val in enumerate(smooth_vals):
                tmp_ws = CloneWorkspace(InputWorkspace=ws_tof, OutputWorkspace="tmp_ws", StoreInADS=False)
                base_fit_wss.append(SmoothData(InputWorkspace=tmp_ws, OutputWorkspace=f"smooth_ws_{smooth_val}", NPoints=smooth_val))
                base_bkg_is_tied.append(tied_bkgs[i])
        else:
            # if no smoothing values are given, the initial fit should just be on the ws
            base_fit_wss.append(ws_tof)
            base_bkg_is_tied.append(True)
        # final fit should be on original ws as well
        base_fit_wss.append(ws_tof)
        base_bkg_is_tied.append(True)

        for peak in peaks:
            fit_wss = base_fit_wss.copy()
            bkg_is_tied = base_bkg_is_tied.copy()

            logger.information(f"Workspace: {wsname}, Peak: {peak}")
            # change peak window to fraction
            out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
            out_file = out_ws + ".nxs"
            out_path = path.join(save_dir, out_file) if override_dir else path.join(save_dir, grouping, str(peak), out_file)

            xmin, xmax = peak - peak_window, peak + peak_window

            out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)

            peak_func_name = "BackToBackExponential"
            bg_func_name = "LinearBackground"

            func_generator = TexturePeakFunctionGenerator([])  # don't fix any parameters
            parameters_to_tie = () if not parameters_to_tie else parameters_to_tie

            initial_function, md_fit_kwargs, intensity_estimates, x0_move = func_generator.get_initial_fit_function_and_kwargs_from_specs(
                ws, fit_wss.pop(0), peak, (xmin, xmax), parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied.pop(0)
            )

            fit_object = Fit(
                Function=initial_function,
                Minimizer="Levenberg-Marquardt",
                CostFunction="Least squares",
                Output="first_fit",
                MaxIterations=50,  # if it hasn't fit in 50 it is likely because the texture has the peak missing
                **fit_kwargs,
                **md_fit_kwargs,
            )

            while len(fit_wss) > 0:
                mdf = fit_object.Function.function
                fit_object, md_fit_kwargs = _rerun_fit_with_new_ws(
                    mdf,
                    fit_kwargs,
                    md_fit_kwargs,
                    fit_wss.pop(0),
                    func_generator.intens_par_name,
                    func_generator.cen_par_name,
                    x0_move,
                    50,
                    bkg_is_tied.pop(0),
                )

            mdf = fit_object.Function.function
            fit_result = {"Function": mdf, "OutputWorkspace": fit_object.OutputWorkspace.name()}

            # update peak mask based on I/sig from fit
            *_, i_over_sigma, _ = calc_intens_and_sigma_arrays(fit_result, "Summation")
            fit_mask = i_over_sigma > i_over_sigma_thresh

            # populate table
            spec_fit = ADS.retrieve(f"fit_{ws_tof.name()}_Parameters")
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

            default_vals = get_default_values(u_params, no_fit_value_dict)

            table_vals = np.zeros((si.size(), 2 * len(u_params) + 1))  # intensity_est + param_1_val, param_1_err, +...
            for ispec in range(si.size()):
                if fit_mask[ispec]:
                    row = [intensity_estimates[ispec]]
                    for p in u_params:
                        param_name = f"f{ispec}.f0.{p}"
                        pind = all_params.index(param_name)
                        if p != "X0":
                            row += [param_vals[pind], param_errs[pind]]
                        else:
                            diff_consts = si.diffractometerConstants(ispec)
                            d_peak = UnitConversion.run("TOF", "dSpacing", param_vals[pind], 0, DeltaEModeType.Elastic, diff_consts)
                            d_err = (param_errs[pind] / param_vals[pind]) * d_peak  # convert error to equivalent d spacing
                            row += [d_peak, d_err]

                else:
                    row = [default_vals.get("I_est", np.nan)]
                    for p in u_params:
                        row += [default_vals[p], np.nan]
                table_vals[ispec] = row
            if nan_replacement:
                table_vals = replace_nans(table_vals, nan_replacement)
            for i, row in enumerate(table_vals):
                out_tab.addRow([i] + list(row))
            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


# -------- Pole Figure Script Logic--------------------------------


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
    create_combined_output: bool = False,
    debug_info_level: int = 0,
    save_ascii: bool = True,
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
    create_combined_output: flag which controls whether to create a combined workspace which contains every spectra in the pole figure
    debug_info_level: 0 - No debug info; 1 - will label with alpha, beta and value; 2 - will include spectra information in label
    save_ascii: whether to save files as txt as well as nxs
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
    out_ws, combined_ws, grouping = model.get_pf_output_names(wss, params, hkl, readout_column)
    # if the flag
    combined_ws = combined_ws if create_combined_output else None

    dir1, dir2, dir3 = np.asarray(dir1), np.asarray(dir2), np.asarray(dir3)
    ax_transform = np.concatenate((dir1[:, None], dir2[:, None], dir3[:, None]), axis=1)
    ax_labels = dir_names

    save_dirs = (
        [path.join(root_dir, "PoleFigureTables")] if override_dir else model.get_save_dirs(root_dir, "PoleFigureTables", exp_name, grouping)
    )
    chi2_thresh = chi2_thresh if chi2_thresh else 0.0
    peak_thresh = peak_thresh if peak_thresh else 0.0
    include_spec_info = debug_info_level == 2
    include_debug_info = debug_info_level in (1, 2)
    model.make_pole_figure_tables(
        wss=wss,
        peak_wss=params,
        out_ws_name=out_ws,
        combined_ws_name=combined_ws,
        save_dirs=save_dirs,
        hkl=pf_hkl,
        inc_scatt_corr=include_scatt_power,
        scat_vol_pos=scat_vol_pos,
        chi2_thresh=chi2_thresh,
        peak_thresh=peak_thresh,
        ax_transform=ax_transform,
        readout_col=readout_column,
        include_spec_info=include_spec_info,
        save_ascii=save_ascii,
    )

    fig, ax = plot_pole_figure(
        out_ws,
        projection_method,
        fig=None,
        readout_col=readout_column,
        save_dirs=save_dirs,
        plot_exp=scatter,
        ax_labels=ax_labels,
        contour_kernel=kernel,
        display_debug_info=include_debug_info,
    )
    ax.set_title(out_ws)
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
    create_combined_output: bool = False,
    debug_info_level: int = 0,
    save_ascii: bool = True,
    override_dir: bool = False,
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
    create_combined_output: flag which controls whether to create a combined workspace which contains every spectra in the pole figure
    debug_info_level: 0 - No debug info; 1 - will label with alpha, beta and value; 2 - will include spectra information in label
    save_ascii: whether to save files as txt as well as nxs
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
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
                "create_combined_output": create_combined_output,
                "debug_info_level": debug_info_level,
                "save_ascii": save_ascii,
                "override_dir": override_dir,
            }
            if scatter == "both":
                for scat in (True, False):
                    kwargs["scatter"] = scat
                    create_pf(**kwargs)
            else:
                kwargs["scatter"] = scatter
                create_pf(**kwargs)
