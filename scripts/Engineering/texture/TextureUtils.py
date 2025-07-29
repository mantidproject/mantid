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
from mantid.simpleapi import SaveNexus, logger, CreateEmptyTableWorkspace, LoadCIF, Fit, CreateSingleValuedWorkspace
from pathlib import Path
from Engineering.EnggUtils import GROUP
from Engineering.EnginX import EnginX
from mantid.geometry import CrystalStructure
from mantid.api import AnalysisDataService as ADS, MultiDomainFunction, FunctionFactory
from typing import Optional, Sequence, Union, Tuple
from mantid.dataobjects import Workspace2D
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from plugins.algorithms.IntegratePeaks1DProfile import PeakFunctionGenerator
# -------- Utility --------------------------------


def find_all_files(directory):
    """
    find all the files in a directory

    directory: directory to iterate over
    """
    files = []
    with scandir(directory) as entries:
        for entry in entries:
            if entry.is_file():
                files.append(entry.path)
    return files


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
    # otherwise run script
    else:
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
    valid_inputs = True
    error_msg = ""
    # validate inputs
    if orientation_file:
        valid_orientation_inputs = isinstance(orient_file_is_euler, bool)
        if not valid_orientation_inputs:
            valid_inputs = False
            error_msg += r"If orientation file is specified, must flag orient_file_is_euler.\n"
        if valid_orientation_inputs and orient_file_is_euler:
            # if is euler flag, require euler_scheme and euler_axes_sense
            valid_orientation_inputs = isinstance(euler_scheme, str) and isinstance(euler_axes_sense, str)
            if not valid_orientation_inputs:
                valid_inputs = False
                error_msg += r"If orientation file is euler, must provide scheme and sense.\n"

    if copy_ref:
        if not isinstance(ref_ws, str):
            valid_inputs = False
            error_msg += r"If copy_ref is True, must provide ref_ws.\n"

    if include_abs_corr:
        if gauge_vol_preset == "Custom":
            if not isinstance(gauge_vol_shape_file, str):
                valid_inputs = False
                error_msg += r"If custom gauge volume required, must provide shape xml as file.\n"

    if include_atten_table:
        if not (isinstance(eval_point, Union[str, float]) and isinstance(eval_units, str)):
            valid_inputs = False
            error_msg += r"If attenuation table required, must provide valid point and units.\n"

    if include_div_corr:
        if not (isinstance(div_hoz, float) and isinstance(div_vert, float) and isinstance(det_hoz, float)):
            valid_inputs = False
            error_msg += r"If divergence correction required, must provide valid values.\n"
    return valid_inputs, error_msg


# -------- Fitting Script Logic--------------------------------


class TexturePeakFunctionGenerator(PeakFunctionGenerator):
    def __init__(self, peak_params_to_fix: Sequence[str], min_width: float = 1e-3):
        super().__init__(peak_params_to_fix)
        self.min_width = min_width

    def get_initial_fit_function_and_kwargs_from_specs(
        self,
        ws: Workspace2D,
        peak: float,
        x_window: tuple[float, float],
        parameters_to_tie: Sequence[str],
        peak_func_name: str,
        bg_func_name: str,
    ) -> Tuple[str, dict, Sequence[float]]:
        # modification of get_initial_fit_function_and_kwargs to just fit a peak within the x_window
        si = ws.spectrumInfo()
        ispecs = list(range(si.size()))
        x_start, x_end = x_window
        function = MultiDomainFunction()
        fit_kwargs = {}
        # estimate background
        istart = ws.yIndexOfX(x_start)
        iend = ws.yIndexOfX(x_end)
        # init bg func (global)
        bg_func = FunctionFactory.createFunction(bg_func_name)
        # init peak func
        peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
        # save parameter names for future ties/constraints
        peak_func.setIntensity(1.0)
        self.intens_par_name = next(peak_func.getParamName(ipar) for ipar in range(peak_func.nParams()) if peak_func.isExplicitlySet(ipar))
        self.cen_par_name = peak_func.getCentreParameterName()
        self.width_par_name = peak_func.getWidthParameterName()
        avg_bg = 0
        intensity_estimates = []
        for ispec in ispecs:
            # get param estimates
            intens, sigma, bg = self._estimate_intensity_and_background(ws, ispec, istart, iend)
            intensity_estimates.append(intens)
            avg_bg += bg
            # add peak, using provided value as guess for centre
            peak_func.setCentre(peak)
            peak_func.setIntensity(intens)
            # add constraints
            peak_func.addConstraints(f"{self.cen_par_name} > {x_start}")
            peak_func.addConstraints(f"{self.cen_par_name} < {x_end}")
            peak_func.addConstraints(f"{self.intens_par_name} > 0")
            comp_func = CompositeFunctionWrapper(FunctionWrapper(peak_func), FunctionWrapper(bg_func), NumDeriv=True)
            function.add(comp_func.function)
            function.setDomainIndex(ispec, ispec)
            key_suffix = f"_{ispec}" if ispec > 0 else ""
            fit_kwargs["InputWorkspace" + key_suffix] = ws.name()
            fit_kwargs["StartX" + key_suffix] = x_start
            fit_kwargs["EndX" + key_suffix] = x_end
            fit_kwargs["WorkspaceIndex" + key_suffix] = int(ispec)
        # set background (background global tied to first domain function)
        function[0][1]["A0"] = avg_bg / len(ispecs)

        available_params = [peak_func.getParamName(i) for i in range(peak_func.nParams())]
        if parameters_to_tie is not None:
            invalid = [p for p in parameters_to_tie if p not in available_params]
            if invalid:
                raise ValueError(f"Invalid parameter(s) to tie: {invalid}. Available: {available_params}")

        # set constraint on FWHM (to avoid peak fitting to noise or background)
        self._add_fwhm_constraints(function, peak_func, fit_range=x_end - x_start, nbins=iend - istart)
        return self._add_parameter_ties(function, parameters_to_tie), fit_kwargs, intensity_estimates

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


def fit_all_peaks(wss: Sequence[str], peaks: Sequence[float], peak_window: float, save_dir: str, override_dir: bool = False) -> None:
    """
    Fit all the peaks given in all the spectra of all the workspaces, for use in a texture analysis workflow

    wss: Workspace names of all the workspaces to fit
    peaks: Sequence of peak positions in d-spacing
    peak_window: size of the window to create around the desired peak for purpose of fitting
    save_dir: directory to save the results in
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    """
    for wsname in wss:
        ws = ADS.retrieve(wsname)
        run, prefix = _get_run_and_prefix_from_ws_log(ws, wsname)
        grouping = _get_grouping_from_ws_log(ws)
        for peak in peaks:
            # change peak window to fraction
            out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
            out_file = out_ws + ".nxs"
            out_path = path.join(save_dir, out_file) if override_dir else path.join(save_dir, grouping, str(peak), out_file)

            xmin, xmax = peak - peak_window, peak + peak_window

            out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)

            peak_func_name = "BackToBackExponential"
            bg_func_name = "LinearBackground"

            func_generator = TexturePeakFunctionGenerator([])  # don't fix any parameters
            initial_function, md_fit_kwargs, intensity_estimates = func_generator.get_initial_fit_function_and_kwargs_from_specs(
                ws, peak, (xmin, xmax), ("A", "B"), peak_func_name, bg_func_name
            )

            Fit(
                Function=initial_function,
                CostFunction="Least squares",
                Output="fit",
                MaxIterations=50,  # if it hasn't fit in 50 it is likely because the texture has the peak missing
                **md_fit_kwargs,
            )

            spec_fit = ADS.retrieve("fit_Parameters")
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

            for ispec in range(si.size()):
                row = [ispec, intensity_estimates[ispec]]
                for p in u_params:
                    param_name = f"f{ispec}.f0.{p}"
                    pind = all_params.index(param_name)
                    row += [param_vals[pind], param_errs[pind]]
                out_tab.addRow(row)
            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


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
    xtal: Optional[CrystalStructure] = None,
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
    xtal: Crystal Structure of the sample
    hkl: H,K,L reflection of the peak fit in the param workspaces
    readout_column: column of the param ws that should be attached to the pole figure table
    kernel: if scatter == False, the kernel size of the gaussian filter applied to smooth the contour plot
    chi2_thresh: if chi2 column present in params, the maximum value which will still get added to the pole figure table
    peak_thresh: if X0 present in params, the maximum allowable difference between a spectra's X0 and the mean X0/
                 X0 corresponding to the provided HKL
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    """
    model = TextureProjection()
    if xtal:
        for ws in wss:
            ws = ADS.retrieve(ws)
            ws.sample().setCrystalStructure(xtal)

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
        wss, params, out_ws, hkl, include_scatt_power, scat_vol_pos, chi2_thresh, peak_thresh, save_dirs, ax_transform, readout_column
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
    xtal: Optional[CrystalStructure] = None,
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
    xtal: Crystal Structure of the sample
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
                "xtal": xtal,
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
