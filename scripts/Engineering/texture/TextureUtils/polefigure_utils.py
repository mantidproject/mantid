# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from os import path
from Engineering.texture.polefigure.polefigure_model import TextureProjection
from mantid.simpleapi import logger
from mantid.api import AnalysisDataService as ADS
from typing import Sequence, Any
from Engineering.texture.xtal_helper import get_xtal_structure
from Engineering.texture.texture_helper import plot_pole_figure


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
    params: Sequence[str] | None = None,
    xtal_input: str | None = None,
    xtal_args: Sequence[str] | None = None,
    hkl: Sequence[int] | None = None,
    readout_column: str | None = None,
    kernel: float | None = None,
    chi2_thresh: float | None = None,
    peak_thresh: float | None = None,
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
    chi2_thresh = chi2_thresh or 0.0
    peak_thresh = peak_thresh or 0.0
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


def make_iterable(param: Any) -> Sequence[Any]:
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
    scatter: str | bool,
    scat_vol_pos: Sequence[float],
    save_root: str,
    exp_name: str,
    projection_method: str,
    xtal_input: str | None = None,
    xtal_args: Sequence[str] | None = None,
    hkls: Sequence[Sequence[int]] | Sequence[int] | None = None,
    readout_columns: str | Sequence[str] | None = None,
    kernel: float | None = None,
    chi2_thresh: float | None = None,
    peak_thresh: float | None = None,
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
