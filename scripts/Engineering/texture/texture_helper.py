# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from os import path
from mantid.simpleapi import logger, SetGoniometer
from mantid.api import AnalysisDataService as ADS
from typing import Optional, Sequence
from mantid.dataobjects import Workspace2D
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from scipy.interpolate import griddata
from scipy.ndimage import gaussian_filter
import matplotlib.pyplot as plt
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel
from Engineering.common.xml_shapes import get_cube_xml


def get_gauge_vol_str(preset: str, custom_file: Optional[str]) -> str:
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


def load_all_orientations(
    wss: Sequence[Workspace2D | str],
    txt_file: str,
    use_euler: bool,
    euler_scheme: Optional[Sequence[str]] = None,
    euler_sense: Optional[str] = None,
) -> None:
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
            if n_ws < n_gonios:
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


def plot_pole_figure(
    ws_name: str,
    projection: str,
    fig: Optional[Figure] = None,
    readout_col: str = "I",
    save_dirs: Optional[Sequence[str] | str] = None,
    plot_exp: bool = True,
    ax_labels: Sequence[str] = ("Dir1", "Dir2"),
    contour_kernel: Optional[float] = 2.0,
    **kwargs,
) -> [Figure, Axes]:
    pfi = get_pole_figure_data(ws_name, projection, readout_col)

    if plot_exp:
        suffix = "scatter"
        fig, ax = plot_exp_pf(pfi, ax_labels, readout_col, fig, **kwargs)
    else:
        suffix = f"contour_{contour_kernel}"
        fig, ax = plot_contour_pf(pfi, ax_labels, readout_col, fig, contour_kernel, **kwargs)
    if save_dirs:
        for save_dir in save_dirs:
            fig.savefig(str(path.join(save_dir, ws_name + f"_{suffix}.png")))

    return fig, ax


def plot_exp_pf(pfi: np.ndarray, ax_labels: Sequence[str], column_label: str, fig: Optional[Figure] = None, **kwargs) -> [Figure, Axes]:
    u = np.linspace(0, 2 * np.pi, 100)
    x = np.cos(u)
    y = np.sin(u)
    z = np.zeros_like(x)
    eq = np.concatenate((x[None, :], y[None, :], z[None, :]), axis=0)

    fig = plt.figure(layout="constrained") if not fig else fig
    gs = fig.add_gridspec(
        1,
        3,
        width_ratios=[1, 30, 1],  # tweak 30 to control plot vs cbar width
        left=0.05,
        right=0.98,
        top=0.98,
        bottom=0.06,
        wspace=0.05,
    )
    # left spacer: gs[0, 0] (we don't use it)
    ax = fig.add_subplot(gs[0, 1])
    cax = fig.add_subplot(gs[0, 2])
    scat_plot = ax.scatter(pfi[:, 1], pfi[:, 0], c=pfi[:, 2], s=20, cmap="jet", label="poles", **kwargs)
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
    pfi: np.ndarray, ax_labels: Sequence[str], column_label: str, fig: Optional[Figure] = None, contour_kernel: float = 2.0, **kwargs
) -> [Figure, Axes]:
    x, y, z = pfi[:, 1], pfi[:, 0], np.nan_to_num(pfi[:, 2])
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
        width_ratios=[1, 30, 1],  # tweak 30 to control plot vs cbar width
        left=0.05,
        right=0.98,
        top=0.98,
        bottom=0.06,
        wspace=0.05,
    )
    # left spacer: gs[0, 0] (we don't use it)
    ax = fig.add_subplot(gs[0, 1])
    cax = fig.add_subplot(gs[0, 2])
    contour_plot = ax.contourf(grid_x, grid_y, grid_z, levels=10, cmap="jet", **kwargs)
    circle = plt.Circle((0, 0), R, color="grey", fill=False, linestyle="-", label="plot bounding circle")
    ax.add_patch(circle)
    ax.set_aspect("equal")
    ax.set_axis_off()
    ax.quiver(-1, -1, 0.2, 0, color="blue", scale=1)
    ax.quiver(-1, -1, 0, 0.2, color="red", scale=1)
    ax.text(-0.8, -0.95, ax_labels[-1], fontsize=10)
    ax.text(-0.95, -0.8, ax_labels[0], fontsize=10)
    ax.set_label("Pole Figure Plot")
    contour_plot.set_label("pole figure data")
    cbar = fig.colorbar(contour_plot, cax=cax)
    cbar.set_label(column_label, rotation=0, labelpad=15)
    return fig, ax


def get_pole_figure_data(ws_name: str, projection: str, readout_col: str = "I"):
    if projection.lower() == "stereographic":
        proj = ster_proj
    else:
        proj = azim_proj
    ws = ADS.retrieve(ws_name)
    alphas = np.asarray(ws.column("Alpha"))
    betas = np.asarray(ws.column("Beta"))
    i = np.asarray(ws.column(readout_col))
    return proj(alphas, betas, i)


def ster_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = np.pi - betas  # this formula projects onto the north-pole, and beta is taken from the south
    r = np.sin(betas) / (1 - np.cos(betas))
    out = np.zeros((len(alphas), 3))
    out[:, 0] = r * np.cos(alphas)
    out[:, 1] = r * np.sin(alphas)
    out[:, 2] = i
    return out


def azim_proj(alphas: np.ndarray, betas: np.ndarray, i: np.ndarray) -> np.ndarray:
    betas = betas / (np.pi / 2)
    xs = (betas * np.cos(alphas))[:, None]
    zs = (betas * np.sin(alphas))[:, None]
    out = np.concatenate([xs, zs, i[:, None]], axis=1)
    return out
