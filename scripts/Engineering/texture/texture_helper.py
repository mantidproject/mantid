# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from os import path
from mantid.simpleapi import (
    logger,
    SetGoniometer,
    CreatePoleFigureTableWorkspace,
    CreateEmptyTableWorkspace,
    CloneWorkspace,
    CombineTableWorkspaces,
)
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence
from mantid.dataobjects import Workspace2D, TableWorkspace
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from scipy.interpolate import griddata
from scipy.ndimage import gaussian_filter
import matplotlib.pyplot as plt
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.common.xml_shapes import get_cube_xml

# ---------------------------------------------------------#
##### Utility Gauge Volume Setup Functions ################
# ---------------------------------------------------------#


def get_gauge_vol_str(preset: str, custom_file: Optional[str] = None) -> str:
    """
    Create an xml string for a gauge volume

    preset: [4mmCube, No Gauge Volume] If 4mmCube will create a 4mm x 4mm x 4mm gauge volume
    custom: if preset is not one of the defaults, the custom file will be read

    """
    if preset == "4mmCube":
        gauge_str = get_cube_xml("some-gv", 0.004)
    elif preset == "No Gauge Volume":
        gauge_str = None
    else:
        try:
            gauge_str = _read_xml(custom_file)
        except RuntimeError:
            gauge_str = None
    return gauge_str


def _read_xml(file: str) -> str:
    out = None
    if _validate_file(file, ".xml"):
        with open(file, "r") as f:
            out = f.read()
    return out


def _validate_file(file: str, ext: str) -> bool:
    valid = False
    if file:
        root, f_ext = path.splitext(file)
        if f_ext == ext:
            valid = True
    return valid


# ------------------------------------------------------------#
##### Utility Texture Sample Viewer Functions ################
# ------------------------------------------------------------#


# wrapper around the show sample logic that is called from the interface
def show_texture_sample_shape(
    ws: str | Workspace2D,
    ax_transform: np.ndarray = np.eye(3),
    ax_labels: Sequence[str] = ("d1", "d2", "d3"),
    gauge_vol_preset: Optional[str] = None,
    custom_file: Optional[str] = None,
):
    """
    Show the sample present on a workspace along with the texture sample directions, and optionally a gauge volume

    ws: Workspace/ Workspace name which the sample should be shown for
    ax_transform: 3x3 numpy array which defines the sample directions
    ax_labels: length 3 sequence of the names of the corresponding sample directions
    gauge_vol_preset: [4mmCube, No Gauge Volume] If 4mmCube will create a 4mm x 4mm x 4mm gauge volume,
                      if None, no gauge volume will be shown
    custom_file: if preset is not None or one of the defaults, this custom file will be read
    """
    model = ShowSampleModel()
    wsname = ws if isinstance(ws, str) else ws.name()
    model.set_ws_name(wsname)
    model.set_fix_axes_to_sample(True)
    if gauge_vol_preset:
        model.set_gauge_vol_str(get_gauge_vol_str(gauge_vol_preset, custom_file))
    model.show_shape_plot(ax_transform, ax_labels)


# --------------------------------------------------------#
##### Utility Orientation Setup Functions ################
# --------------------------------------------------------#


def load_all_orientations(
    wss: Sequence[Workspace2D | str],
    txt_file: str,
    use_euler: bool,
    euler_scheme: Optional[Sequence[str]] = None,
    euler_sense: Optional[str] = None,
) -> None:
    """
    A method for providing orientation information on a set of workspaces

    wss: Sequence of workspaces for which the orientation data will be provided
    txt_file: file containing orientation data, either as euler angles or flattened matrices,
              each row in the file will be applied to the next workspace in the sequence
    use_euler: flag for whether the data in the text file is euler angles (True) or matrices (False)
    euler_scheme: lab frame alignment of the euler axes (not case-sensitive) eg. `XYZ`, or `yxy` would be acceptable inputs
    euler_sense: comma separated sense of rotation for each euler axis eg. `1,1,1` or `-1,1,-1`
                 (1 is counter-clockwise, -1 is clockwise)
    """
    if _validate_file(txt_file, ".txt"):
        with open(txt_file, "r") as f:
            goniometer_strings = [line.strip().replace("\t", ",") for line in f]
            goniometer_lists = [[float(x) for x in gs.split(",")] for gs in goniometer_strings]
        try:
            n_ws, n_gonios = len(wss), len(goniometer_lists)
            if n_ws == 0:
                logger.warning(
                    "No workspaces have been provided - if you are using the UI, ensure you have selected all desired workspaces"
                )
            elif n_ws < n_gonios:
                logger.warning(
                    f"Fewer Workspaces ({n_ws}) provided than lines of orientation data ({n_gonios}). "
                    f"The last {n_gonios - n_ws} lines of the orientation file will be ignored"
                )
            if not use_euler:
                # if use euler angles not selected then assumes it is a scans output matrix
                for iws, ws in enumerate(wss):
                    SetGoniometer(ws, GoniometerMatrix=goniometer_lists[iws][:9])
            else:
                axis_dict = {"x": "1,0,0", "y": "0,1,0", "z": "0,0,1"}
                rotation_sense = [int(x) for x in euler_sense.split(",")]
                for iws, ws in enumerate(wss):
                    angles = goniometer_strings[iws].split(",")
                    kwargs = {}
                    for iang, angle in enumerate(angles):
                        sense = rotation_sense[iang]
                        kwargs[f"Axis{iang}"] = f"{angle},{axis_dict[(euler_scheme[iang]).lower()]},{sense}"
                    SetGoniometer(ws, **kwargs)
        except BaseException as e:
            logger.error(f"{str(e)}. Failed to set goniometer, are your settings for `use_euler_angles` correct? Currently: {use_euler}")


# -------------------------------------------------------------------#
##### Utility Pole Figure Tables and Plots Functions ################
# -------------------------------------------------------------------#


def create_default_parameter_table_with_value(ws_name: str, val: float, out_ws: str):
    """
    Creates an example parameter table with a row for each spectrum in the supplied workspace and the intensity value provided

    ws_name: Name of the Workspace
    val: Intensity value desired
    out_ws: Name to be used for the Output Workspace
    """
    tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
    tab.addColumn("float", "I")
    ws = ADS.retrieve(ws_name)
    for _ in range(ws.getNumberHistograms()):
        tab.addRow(
            [
                float(val),
            ]
        )


def create_pole_figure_tables(
    wss: Sequence[str],
    peak_wss: Optional[Sequence[str]],
    out_ws: str,
    hkl: Optional[Sequence[int]] = None,
    inc_scatt_corr: bool = False,
    scat_vol_pos: Sequence[float] = (0, 0, 0),
    chi2_thresh: Optional[float] = 0.0,
    peak_thresh: Optional[float] = 0.0,
    ax_transform: Sequence[float] = np.eye(3),
    readout_col: str = "I",
) -> TableWorkspace:
    """
    Create a single pole figure table for a sequence of workspaces and their fit parameters

    wss: Sequence of workspaces
    peak_wss: sequence of table workspaces, one for each of the provided workspaces,
              which should contain the fit parameters for each spectrum in the given workspace
    out_ws: name to give the output workspace
    hkl: if the crystal structure is known and you would like to apply thresholds based on target HKL peak location
         or use a scattering power correction, you can provide that here
    inc_scatt_corr: flag for whether or not to scale the parameter value by scattering power (also requires HKL)
    chi2_thresh: if the peak fit parameters have a chi2 per fit,
                 this can be used to exclude spectra where the chi2 is higher than this supplied value
    peak_thresh: spectra with x0 more than this value from the `target` x0 will be excluded.
                 If HKL and crystal structure are provided then this target x0 is the theoretical peak centre for the given HKL
                 otherwise it is the average x0 of the spectra
    ax_transform: the coordinate change between the lab frame and the sample frame which defines the initial orientation
    readout_col: the parameter column which should be read and used to populate the table

    """
    flat_ax_transform = np.reshape(ax_transform, (9,))
    table_workspaces = []
    if peak_wss and (len(peak_wss) == len(wss)):
        for iws, ws in enumerate(wss):
            ws_str = f"_{iws}_abi_table"
            CreatePoleFigureTableWorkspace(
                InputWorkspace=ws,
                PeakParameterWorkspace=peak_wss[iws],
                OutputWorkspace=ws_str,
                Reflection=hkl,
                Chi2Threshold=chi2_thresh,
                PeakPositionThreshold=peak_thresh,
                ApplyScatteringPowerCorrection=inc_scatt_corr,
                ScatteringVolumePosition=scat_vol_pos,
                AxesTransform=flat_ax_transform,
                ReadoutColumn=readout_col,
            )
            table_workspaces.append(ws_str)
    else:
        for iws, ws in enumerate(wss):
            default_param_vals = "_default_param_table"
            create_default_parameter_table_with_value(ws, iws + 1, default_param_vals)
            ws_str = f"_{iws}_abi_table"
            CreatePoleFigureTableWorkspace(
                InputWorkspace=ws,
                PeakParameterWorkspace=default_param_vals,
                OutputWorkspace=ws_str,
                Reflection=hkl,
                Chi2Threshold=chi2_thresh,
                PeakPositionThreshold=peak_thresh,
                ApplyScatteringPowerCorrection=inc_scatt_corr,
                ScatteringVolumePosition=scat_vol_pos,
                AxesTransform=flat_ax_transform,
            )
            table_workspaces.append(ws_str)
    CloneWorkspace(InputWorkspace=table_workspaces[0], OutputWorkspace=out_ws)
    for tw in table_workspaces[1:]:
        CombineTableWorkspaces(LHSWorkspace=out_ws, RHSWorkspace=tw, OutputWorkspace=out_ws)
    return ADS.retrieve(out_ws)


def plot_pole_figure(
    ws: TableWorkspace | str,
    projection: str,
    fig: Optional[Figure] = None,
    readout_col: str = "I",
    save_dirs: Optional[Sequence[str] | str] = None,
    plot_exp: bool = True,
    ax_labels: Sequence[str] = ("Dir1", "Dir2"),
    contour_kernel: Optional[float] = 2.0,
    **kwargs,
) -> [Figure, Axes]:
    """
    Create a pole figure plot for a pole figure table

    ws: The pole figure TableWorkspace
    projection: The projection method for displaying the detector positions (`stereographic`/`azimuthal`)
    fig: The matplotlib figure that the pole figure plot should be added to (can be None)
    readout_col: The name of the column from the TableWorkspace which will serve as the plotted intensities
    save_dirs: Where to save the resulting figure (can also be None)
    plot_exp: flag for whether the experimental points should be plotted as points (True)
              or a contour interpolation should be calculated (False)
    ax_labels: Sequence for the label names for the two inplane sample directions
    contour_kernel: The sigma value of the gaussian kernel used for a contour interpolation

    """
    ws = _retrieve_ws_object(ws)
    pfi = get_pole_figure_data(ws, projection, readout_col)

    if plot_exp:
        suffix = "scatter"
        fig, ax = plot_exp_pf(pfi, ax_labels, readout_col, fig, **kwargs)
    else:
        suffix = f"contour_{contour_kernel}"
        fig, ax = plot_contour_pf(pfi, ax_labels, readout_col, fig, contour_kernel, **kwargs)
    if save_dirs:
        if isinstance(save_dirs, str):
            save_dirs = [save_dirs]
        for save_dir in save_dirs:
            fig.savefig(str(path.join(save_dir, ws.name() + f"_{suffix}.png")))
    return fig, ax


def plot_exp_pf(
    pole_figure_data: np.ndarray, ax_labels: Sequence[str], column_label: str, fig: Optional[Figure] = None, **kwargs
) -> [Figure, Axes]:
    """
    Create a scatter plot pole figure plot for a pole figure table

    pole_figure_data: Array of (N,3) giving the alpha, beta, and intensity for each N point
    ax_labels: Sequence for the label names for the two inplane sample directions
    column_label: The label to give the intensity colour bar
    fig: The matplotlib figure that the pole figure plot should be added to (can be None)
    """
    u = np.linspace(0, 2 * np.pi, 100)
    x = np.cos(u)
    y = np.sin(u)
    z = np.zeros_like(x)
    eq = np.concatenate((x[None, :], y[None, :], z[None, :]), axis=0)

    fig = plt.figure(layout="constrained") if not fig else fig
    gs = fig.add_gridspec(
        1,
        3,
        width_ratios=[1, 30, 1],
        left=0.05,
        right=0.98,
        top=0.98,
        bottom=0.06,
        wspace=0.05,
    )
    ax = fig.add_subplot(gs[0, 1])
    cax = fig.add_subplot(gs[0, 2])
    scat_plot = ax.scatter(
        pole_figure_data[:, 1], pole_figure_data[:, 0], c=pole_figure_data[:, 2], s=20, cmap="jet", label="poles", **kwargs
    )
    ax.plot(eq[0], eq[1], c="grey", label="plot bounding circle")
    ax.set_aspect("equal")
    ax.set_axis_off()
    ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
    ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)
    ax.text(-0.8, -0.95, ax_labels[-1], fontsize=10)
    ax.text(-0.95, -0.8, ax_labels[0], fontsize=10)
    cbar = fig.colorbar(scat_plot, cax=cax)
    ax.set_label("Pole Figure Plot")
    scat_plot.set_label("pole figure data")
    cbar.set_label(column_label, rotation=0, labelpad=15)
    return fig, ax


def plot_contour_pf(
    pole_figure_data: np.ndarray,
    ax_labels: Sequence[str],
    column_label: str,
    fig: Optional[Figure] = None,
    contour_kernel: float = 2.0,
    **kwargs,
) -> [Figure, Axes]:
    """
    Create an interpolated contour plot pole figure plot for a pole figure table

    pole_figure_data: Array of (N,3) giving the alpha, beta, and intensity for each N point
    ax_labels: Sequence for the label names for the two inplane sample directions
    column_label: The label to give the intensity colour bar
    fig: The matplotlib figure that the pole figure plot should be added to (can be None)
    contour_kernel: The sigma value of the gaussian kernel used for a contour interpolation
    """
    x, y, z = pole_figure_data[:, 1], pole_figure_data[:, 0], np.nan_to_num(pole_figure_data[:, 2])
    # Grid definition
    R = 1
    grid_x, grid_y = np.mgrid[-R:R:200j, -R:R:200j]

    # Mask to keep only points inside the circle of radius R
    mask = grid_x**2 + grid_y**2 <= R**2

    # Interpolate z-values on the grid
    grid_z = np.asarray(griddata((x, y), z, (grid_x, grid_y), method="nearest"))
    grid_z = np.asarray(gaussian_filter(grid_z, sigma=contour_kernel))

    # Apply the mask
    grid_z[~mask] = np.nan

    # Plotting
    fig = plt.figure() if not fig else fig
    gs = fig.add_gridspec(
        1,
        3,
        width_ratios=[1, 30, 1],
        left=0.05,
        right=0.85,
        top=0.98,
        bottom=0.06,
        wspace=0.04,
    )
    ax = fig.add_subplot(gs[0, 1])
    cax = fig.add_subplot(gs[0, 2])
    contour_plot = ax.contourf(grid_x, grid_y, grid_z, levels=10, cmap="jet", **kwargs)
    circle = plt.Circle((0, 0), R, color="grey", fill=False, linestyle="-", label="plot bounding circle")
    ax.add_patch(circle)
    ax.set_aspect("equal")
    ax.set_axis_off()
    ax.quiver(-0.98, -0.98, 0.2, 0, color="blue", scale=1)
    ax.quiver(-0.98, -0.98, 0, 0.2, color="red", scale=1)
    ax.text(-0.78, -0.93, ax_labels[-1], fontsize=10)
    ax.text(-0.93, -0.78, ax_labels[0], fontsize=10)
    ax.set_label("Pole Figure Plot")
    contour_plot.set_label("pole figure data")
    cbar = fig.colorbar(contour_plot, cax=cax)
    cbar.set_label(column_label, rotation=0, labelpad=15)
    return fig, ax


def get_pole_figure_data(ws: TableWorkspace, projection: str, readout_col: str = "I"):
    """
    Convert data in a pole figure table into a data array and project it into two dimensions

    ws: TableWorkspace with the pole figure data
    projection: The projection method for displaying the detector positions (`stereographic`/`azimuthal`)
    readout_col: The name of the column from the TableWorkspace which will serve as the plotted intensities
    """
    if projection.lower() == "stereographic":
        proj = ster_proj
    else:
        proj = azim_proj
    alphas = np.asarray(ws.column("Alpha"))
    betas = np.asarray(ws.column("Beta"))
    i = np.asarray(ws.column(readout_col))
    return proj(alphas, betas, i)


def ster_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    """
    project pole figure data using a stereographic projection

    alphas: array of spherical coordinate alpha angles (in rad)
    betas: array of spherical coordinate beta angles (in rad)
    i: array of intensity values
    """
    betas = np.pi - betas  # this formula projects onto the north-pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 3))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    out[:, 2] = i
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    """
    project pole figure data using an azimuthal projection

    alphas: array of spherical coordinate alpha angles (in rad)
    betas: array of spherical coordinate beta angles (in rad)
    i: array of intensity values
    """
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs, i[:, None]], axis=1)
    return out


def _retrieve_ws_object(ws: str | Workspace2D | TableWorkspace):
    if isinstance(ws, str):
        return ADS.retrieve(ws)
    return ws
