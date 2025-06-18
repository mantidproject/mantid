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
from typing import Optional, Sequence

# -------- Utility --------------------------------


def find_all_files(directory):
    files = []
    with scandir(directory) as entries:
        for entry in entries:
            if entry.is_file():
                files.append(entry.path)
    return files


def mk(p):
    p = Path(p)
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
        instr_prefix: str = "engggui",
        data_prefix: str = "ENGINX00",
    ) -> None:
        super().__init__(
            vanadium_run, focus_runs, save_dir, full_inst_calib_path, prm_path, ceria_run, group, groupingfile_path, spectrum_num
        )
        self.instr_prefix = instr_prefix
        self.data_prefix = data_prefix


# -------- Focus Script Logic--------------------------------


def run_focus_script(wss, focus_dir, van_run, ceria_run, prm_path, full_instr_calib, instr_prefix, data_prefix, grouping, save_focus):
    group = GROUP(grouping)
    model = TextureInstrument(
        vanadium_run=van_run,
        ceria_run=ceria_run,
        focus_runs=wss,
        save_dir=focus_dir,
        prm_path=prm_path,
        full_inst_calib_path=full_instr_calib,
        instr_prefix=instr_prefix,
        data_prefix=data_prefix,
        group=group,
    )

    mk(focus_dir)
    model.main()


# -------- Absorption Script Logic--------------------------------


def run_abs_corr(
    wss,
    ref_ws,
    orientation_file,
    orient_file_is_euler,
    euler_scheme,
    euler_axes_sense,
    copy_ref,
    include_abs_corr,
    monte_carlo_args,
    gauge_vol_preset,
    gauge_vol_shape_file,
    include_atten_table,
    eval_point,
    eval_units,
    exp_name,
    root_dir,
    include_div_corr,
    div_hoz,
    div_vert,
    det_hoz,
    clear_ads_after,
):
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


def get_numerical_integ(ws, ispec, peak, peak_window):
    lin_back = "name=LinearBackground"

    Fit(lin_back, InputWorkspace=ws, WorkspaceIndex=ispec, Output="fit_result")

    fit_ws = ADS.retrieve("fit_result_Workspace")
    diffx, diffy = fit_ws.extractX()[-1], fit_ws.extractY()[-1]

    return np.trapezoid(diffy, np.convolve(diffx, np.ones(2) / 2, "valid"))


def fit_all_peaks(wss, peaks, peak_window, save_dir):
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

                intensity_est = get_numerical_integ(crop_ws, ispec, peak, peak_window)
                out_tab.addRow([ispec] + spec_fit.column("Value") + [intensity_est])

            SaveNexus(InputWorkspace=out_ws, Filename=out_path)


# -------- Pole Figure Script Logic--------------------------------


def create_pf(
    wss,
    params,
    include_scatt_power,
    cif,
    lattice,
    space_group,
    basis,
    hkl,
    readout_column,
    dir1,
    dir2,
    dir3,
    dir_names,
    scatter,
    kernel,
    scat_vol_pos,
    chi2_thresh,
    peak_thresh,
    root_dir,
    exp_name,
    projection_method,
):
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
    return (
        param
        if (isinstance(param, list) or isinstance(param, tuple))
        else [
            param,
        ]
    )


def create_pf_loop(
    root_dir,
    ws_folder,
    fit_folder,
    peaks,
    grouping,
    include_scatt_power,
    cif,
    lattice,
    space_group,
    basis,
    hkl,
    readout_columns,
    dir1,
    dir2,
    dir3,
    dir_names,
    scatter,
    kernel,
    scat_vol_pos,
    chi2_thresh,
    peak_thresh,
    save_root,
    exp_name,
    projection_method,
):
    # get ws paths
    focus_dir = path.join(root_dir, ws_folder, grouping, "CombinedFiles")
    focus_wss = find_all_files(focus_dir)
    wss = [path.splitext(path.basename(fp))[0] for fp in focus_wss]

    for peak in peaks:
        # get fit params
        fit_dir = path.join(root_dir, fit_folder, grouping, str(peak))
        fit_wss = find_all_files(fit_dir)
        params = [path.splitext(path.basename(fp))[0] for fp in fit_wss]
        for iws, ws in enumerate(wss):
            if not ADS.doesExist(ws):
                Load(Filename=focus_wss[iws], OutputWorkspace=ws)
            if not ADS.doesExist(params[iws]):
                Load(Filename=fit_wss[iws], OutputWorkspace=params[iws])

        for readout_column in make_iterable(readout_columns):
            create_pf(
                wss,
                params,
                include_scatt_power,
                cif,
                lattice,
                space_group,
                basis,
                hkl,
                readout_column,
                dir1,
                dir2,
                dir3,
                dir_names,
                scatter,
                kernel,
                scat_vol_pos,
                chi2_thresh,
                peak_thresh,
                save_root,
                exp_name,
                projection_method,
            )
