# import mantid algorithms, numpy and matplotlib
import numpy as np
from os import path, scandir
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from Engineering.texture.correction.correction_model import TextureCorrectionModel
from mantid.simpleapi import SaveNexus, Load, logger, CreateEmptyTableWorkspace, CropWorkspace, LoadCIF, Fit
from pathlib import Path
from Engineering.EnggUtils import GROUP
from Engineering.EnginX import EnginX
from mantid.geometry import CrystalStructure
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence, Union

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
    def __init__(
        self,
        vanadium_run: str,
        focus_runs: Sequence[str],
        save_dir: str,
        full_inst_calib_path: str,
        prm_path: Optional[str] = None,
        ceria_run: Optional[str] = None,
        group: Optional[GROUP] = None,
        groupingfile_path: Optional[str] = None,
        spectrum_num: Optional[str] = None,
    ) -> None:
        super().__init__(
            vanadium_run, focus_runs, save_dir, full_inst_calib_path, prm_path, ceria_run, group, groupingfile_path, spectrum_num
        )


# -------- Focus Script Logic--------------------------------


def run_focus_script(
    wss: Sequence[str],
    focus_dir: str,
    van_run: str,
    ceria_run: str,
    full_instr_calib: str,
    grouping: Optional[str] = None,
    prm_path: Optional[str] = None,
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
    )

    mk(focus_dir)
    model.main()


# -------- Absorption Script Logic--------------------------------


def run_abs_corr(
    wss: Sequence[str],
    ref_ws: str,
    orientation_file: str,
    orient_file_is_euler: bool,
    euler_scheme: str,
    euler_axes_sense: str,
    copy_ref: bool,
    include_abs_corr: bool,
    monte_carlo_args: str,
    gauge_vol_preset: str,
    gauge_vol_shape_file: str,
    include_atten_table: bool,
    eval_point: str,
    eval_units: str,
    exp_name: str,
    root_dir: str,
    include_div_corr: bool,
    div_hoz: float,
    div_vert: float,
    det_hoz: float,
    clear_ads_after: bool,
):
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


# -------- Fitting Script Logic--------------------------------


def get_numerical_integ(ws: str, ispec: int):
    """
    Perform a numerical integration of the whole peak window after a linear background has been subtracted

    ws: workspace
    ispec: spectra id
    """
    lin_back = "name=LinearBackground"

    Fit(lin_back, InputWorkspace=ws, WorkspaceIndex=ispec, Output="fit_result")

    fit_ws = ADS.retrieve("fit_result_Workspace")
    diffx, diffy = fit_ws.extractX()[-1], fit_ws.extractY()[-1]

    return np.trapezoid(diffy, np.convolve(diffx, np.ones(2) / 2, "valid"))


def fit_all_peaks(wss: Sequence[str], peaks: Sequence[float], peak_window: float, save_dir: str):
    """
    Fit all the peaks given in all the spectra of all the workspaces, for use in a texture analysis workflow

    wss: Workspace names of all the workspaces to fit
    peaks: Sequence of peak positions in d-spacing
    peak_window: size of the window to create around the desired peak for purpose of fitting
    save_dir: directory to save the results in
    """
    for wsname in wss:
        ws = ADS.retrieve(wsname)
        try:
            run = str(ws.getRun().getLogData("run_number").value)
            prefix = wsname.split(run)[0]
        except:
            run = "unknown"
            prefix = ""
        try:
            grouping = str(ws.getRun().getLogData("Grouping").value)
        except RuntimeError:
            grouping = ""
        for peak in peaks:
            # change peak window to fraction
            out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
            out_file = out_ws + ".nxs"
            out_path = path.join(save_dir, grouping, str(peak), out_file)

            fit_range_min = peak - peak_window
            fit_range_max = peak + peak_window

            func = f"""
                    composite=CompositeFunction;
                      name=LinearBackground;
                      name=BackToBackExponential,X0={peak};
                      constraints=({fit_range_min} < f1.X0 < {fit_range_max}, 0<f1.I)
                    """
            out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)

            num_spec = ws.getNumberHistograms()

            crop_ws = CropWorkspace(InputWorkspace=ws, XMin=peak - peak_window, XMax=peak + peak_window, OutputWorkspace="crop_ws")

            for ispec in range(num_spec):
                Fit(
                    Function=func,
                    InputWorkspace=ws,
                    WorkspaceIndex=ispec,
                    StartX=fit_range_min,
                    EndX=fit_range_max,
                    Output="fit",
                    CreateOutput=True,
                    OutputCompositeMembers=True,
                    OutputParametersOnly=True,
                    OutputWorkspace="focussed_peak_centres",
                    CostFunction="Least squares",
                )

                spec_fit = ADS.retrieve("fit_Parameters")

                if ispec == 0:
                    out_tab.addColumn("int", "wsindex")
                    for param in spec_fit.column("Name"):
                        param_name = param.split(".")[-1]
                        col_name = "chi2" if param_name == "Cost function value" else param_name
                        out_tab.addColumn("double", col_name)
                    out_tab.addColumn("double", "I_est")

                intensity_est = get_numerical_integ(crop_ws, ispec)
                out_tab.addRow([ispec] + spec_fit.column("Value") + [intensity_est])

            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


# -------- Pole Figure Script Logic--------------------------------


def create_pf(
    wss: Sequence[str],
    params: Optional[Sequence[str]],
    include_scatt_power: bool,
    cif: Optional[str],
    lattice: Optional[str],
    space_group: Optional[str],
    basis: Optional[str],
    hkl: Optional[Sequence[int]],
    readout_column: Optional[str],
    dir1: Sequence[float],
    dir2: Sequence[float],
    dir3: Sequence[float],
    dir_names: Sequence[str],
    scatter: bool,
    kernel: Optional[float],
    scat_vol_pos: Sequence[float],
    chi2_thresh: Optional[float],
    peak_thresh: Optional[float],
    root_dir: str,
    exp_name: str,
    projection_method: str,
):
    """
    Create a single pole figure, for use in texture analysis workflow

    wss: Workspace names of the ws with the orientation information present as a goniometer matrix
    params: Parameter Workspaces if you want to read a column of this table to each point in the pole figure
    include_scatt_power: flag for whether to adjust the value by a scattering power calculation
    cif: path to CIF file for the crystal structure
    lattice: String representation of Lattice for CrystalStructure
    space_group: String representation of Space Group for CrystalStructure
    basis: String representation of Basis for CrystalStructure
    hkl: H,K,L reflection of the peak fit in the param workspaces
    readout_column: column of the param ws that should be attached to the pole figure table
    dir1: vector of the first principle direction of the sample
    dir2: vector of the second (projection) principle direction of the sample
    dir3: vector of the third principle direction of the sample
    dir_names: Names of the first, second and third principle directions
    scatter: flag as to whether the plotted pole figure should be a scatter plot of experimental points or a fitted contour plot
    kernel: if scatter == False, the kernel size of the gaussian filter applied to smooth the contour plot
    scat_vol_pos: position of the centre of mass of the scattering gauge volume
    chi2_thresh: if chi2 column present in params, the maximum value which will still get added to the pole figure table
    peak_thresh: if X0 present in params, the maximum allowable difference between a spectra's X0 and the mean X0/
                 X0 corresponding to the provided HKL
    root_dir: root of the directory to which the data should be saved
    exp_name: experiment name, which provides the overarching folder within the root directory
    projection_method: the type of projection to use to create the pole figure ("Azimuthal", "Stereographic")
    """
    model = TextureProjection()
    if include_scatt_power:
        if cif:
            for ws in wss:
                LoadCIF(Workspace=ws, InputFile=cif)
        else:
            for ws in wss:
                ws = ADS.retrieve(ws)
                ws.sample().setCrystalStructure(CrystalStructure(lattice, space_group, basis))

    out_ws, grouping = model.get_pf_table_name(wss, params, hkl, readout_column)
    ax_transform = np.concatenate((dir1[:, None], dir2[:, None], dir3[:, None]), axis=1)
    ax_labels = dir_names

    save_dirs = model.get_save_dirs(root_dir, "PoleFigureTables", exp_name, grouping)
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
        logger.notice("Ignoring a problem with the plt.get_edgecolor. This is probably fine")


def make_iterable(param):
    """
    take a single parameter and make a single value in a list

    param: parameter value
    """
    return (
        param
        if (isinstance(param, list) or isinstance(param, tuple))
        else [
            param,
        ]
    )


def create_pf_loop(
    root_dir: str,
    ws_folder: str,
    fit_folder: Optional[str],
    peaks: Optional[Union[float, Sequence[float]]],
    grouping: str,
    include_scatt_power: bool,
    cif: Optional[str],
    lattice: Optional[str],
    space_group: Optional[str],
    basis: Optional[str],
    hkls: Optional[Union[Sequence[Sequence[int]], Sequence[int]]],
    readout_columns: Optional[Union[str, Sequence[str]]],
    dir1: Sequence[float],
    dir2: Sequence[float],
    dir3: Sequence[float],
    dir_names: Sequence[str],
    scatter: Union[str, bool],
    kernel: Optional[float],
    scat_vol_pos: Sequence[float],
    chi2_thresh: Optional[float],
    peak_thresh: Optional[float],
    save_root: str,
    exp_name: str,
    projection_method: str,
):
    """
    Create a series of pole figures, for use in texture analysis workflow

    root_dir: root of the directory to which the data should be loaded from
    ws_folder: folder name, within which the workspaces will be found
    fit_folder: folder name, within which the fit parameters will be found
    peaks: either a single peak or sequence of peaks which should be fit
           (must correspond exactly to a sub folder in the fit folder directory)
    grouping: name of the detector grouping used
    include_scatt_power: flag for whether to adjust the value by a scattering power calculation
    cif: path to CIF file for the crystal structure
    lattice: String representation of Lattice for CrystalStructure
    space_group: String representation of Space Group for CrystalStructure
    basis: String representation of Basis for CrystalStructure
    hkls: H,K,L reflection of each peak fitted by the param workspaces
    readout_columns: each column of the param ws that should be attached to its own pole figure table
    dir1: vector of the first principle direction of the sample
    dir2: vector of the second (projection) principle direction of the sample
    dir3: vector of the third principle direction of the sample
    dir_names: Names of the first, second and third principle directions
    scatter: flag as to whether the plotted pole figure should be a scatter plot of experimental points or a fitted contour plot.
            the string "both" is also a valid argument and that will create both
    kernel: if scatter == False, the kernel size of the gaussian filter applied to smooth the contour plot
    scat_vol_pos: position of the centre of mass of the scattering gauge volume
    chi2_thresh: if chi2 column present in params, the maximum value which will still get added to the pole figure table
    peak_thresh: if X0 present in params, the maximum allowable difference between a spectra's X0 and the mean X0/
                 X0 corresponding to the provided HKL
    save_root: root of the directory to which the data should be saved
    exp_name: experiment name, which provides the overarching folder within the root directory
    projection_method: the type of projection to use to create the pole figure ("Azimuthal", "Stereographic")
    """
    # get ws paths
    focus_dir = path.join(root_dir, ws_folder, grouping, "CombinedFiles")
    focus_wss = find_all_files(focus_dir)
    wss = [path.splitext(path.basename(fp))[0] for fp in focus_wss]
    peaks = make_iterable(peaks)
    for ipeak, peak in enumerate(peaks):
        # get fit params
        fit_dir = path.join(root_dir, fit_folder, grouping, str(peak))
        fit_wss = find_all_files(fit_dir)
        params = [path.splitext(path.basename(fp))[0] for fp in fit_wss]
        for iws, ws in enumerate(wss):
            if not ADS.doesExist(ws):
                Load(Filename=focus_wss[iws], OutputWorkspace=ws)
            if not ADS.doesExist(params[iws]):
                Load(Filename=fit_wss[iws], OutputWorkspace=params[iws])

        # if multiple peaks are provided, multiple hkls should also be provided
        hkl = hkls if len(peaks) == 1 else hkls[ipeak]

        for readout_column in make_iterable(readout_columns):
            if scatter == "both":
                for scat in (True, False):
                    create_pf(
                        wss=wss,
                        params=params,
                        include_scatt_power=include_scatt_power,
                        cif=cif,
                        lattice=lattice,
                        space_group=space_group,
                        basis=basis,
                        hkl=hkl,
                        readout_column=readout_column,
                        dir1=dir1,
                        dir2=dir2,
                        dir3=dir3,
                        dir_names=dir_names,
                        scatter=scat,
                        kernel=kernel,
                        scat_vol_pos=scat_vol_pos,
                        chi2_thresh=chi2_thresh,
                        peak_thresh=peak_thresh,
                        root_dir=save_root,
                        exp_name=exp_name,
                        projection_method=projection_method,
                    )
            else:
                create_pf(
                    wss=wss,
                    params=params,
                    include_scatt_power=include_scatt_power,
                    cif=cif,
                    lattice=lattice,
                    space_group=space_group,
                    basis=basis,
                    hkl=hkl,
                    readout_column=readout_column,
                    dir1=dir1,
                    dir2=dir2,
                    dir3=dir3,
                    dir_names=dir_names,
                    scatter=scatter,
                    kernel=kernel,
                    scat_vol_pos=scat_vol_pos,
                    chi2_thresh=chi2_thresh,
                    peak_thresh=peak_thresh,
                    root_dir=save_root,
                    exp_name=exp_name,
                    projection_method=projection_method,
                )
