# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import rcParams
from matplotlib.image import AxesImage
from matplotlib.colors import LogNorm, SymLogNorm
from matplotlib.collections import QuadMesh
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string, clean_variable_name
from workbench.plotting.propertiesdialog import LINTHRESH_DEFAULT

BASE_IMSHOW_COMMAND = "imshow"
BASE_PCOLORMESH_COMMAND = "pcolormesh"
CFILL_NAME = "cfill"
PLOT_KWARGS = ["alpha", "label", "zorder"]

mpl_default_kwargs = {"alpha": None, "label": "", "zorder": 0, "interpolation": rcParams["image.interpolation"]}


def generate_plot_2d_command(artist, ax_object_var):
    lines = []
    pos_args = get_plot_command_pos_args(artist)
    kwargs = get_plot_command_kwargs(artist)
    if isinstance(artist, AxesImage):
        base_command = BASE_IMSHOW_COMMAND
    else:
        base_command = BASE_PCOLORMESH_COMMAND
    arg_string = convert_args_to_string(pos_args, kwargs)
    lines.append(f"{CFILL_NAME} = {ax_object_var}.{base_command}({arg_string})")
    cbar_lines, cbar_headers = get_colorbar(artist, CFILL_NAME, ax_object_var)
    lines.extend(cbar_lines)
    return lines, cbar_headers


def get_colorbar(artist, image_name, ax_object_var):
    lines = []
    headers = []
    if artist.colorbar:
        is_log_norm = isinstance(artist.colorbar.norm, LogNorm)
        is_sym_log_norm = isinstance(artist.colorbar.norm, SymLogNorm)
        if is_log_norm or is_sym_log_norm:
            if is_log_norm:
                scale_str = "LogNorm"
                linthresh_str = ""
                loglocator_import_str = "LogLocator"
                loglocator_args_str = "subs=np.arange(1, 10)"
            else:
                scale_str = "SymLogNorm"
                linthresh_str = ", linthresh=2"
                loglocator_import_str = "SymmetricalLogLocator"
                loglocator_args_str = f"subs=np.arange(1, 10), base=10, linthresh={LINTHRESH_DEFAULT}"

            headers.append("import numpy as np")
            headers.append(f"from matplotlib.colors import {scale_str}")
            headers.append(f"from matplotlib.ticker import {loglocator_import_str}")
            lines.append(f"{CFILL_NAME}.set_norm({scale_str}(vmin={artist.colorbar.vmin}, vmax={artist.colorbar.vmax}{linthresh_str}))")
            lines.append("# If no ticks appear on the color bar remove the subs argument inside the LogLocator below")
            lines.append(
                f"cbar = fig.colorbar({image_name}, ax=[{ax_object_var}], ticks={loglocator_import_str}({loglocator_args_str}), pad=0.06)"
            )
        else:
            lines.append(f"{CFILL_NAME}.set_norm(plt.Normalize(vmin={artist.colorbar.vmin}, vmax={artist.colorbar.vmax}))")
            lines.append(f"cbar = fig.colorbar({image_name}, ax=[{ax_object_var}], pad=0.06)")

        try:
            label = artist.colorbar.ax.yaxis.label.get_text()
            if label:
                lines.append(f"cbar.set_label({repr(label)})")
        except:
            # can't access the label it probably does not have one
            pass
    return lines, headers


def get_plot_command_pos_args(artist):
    ax = artist.axes
    ws_name = ax.get_artists_workspace_and_spec_num(artist)[0].name()
    return [clean_variable_name(ws_name)]


def get_plot_command_kwargs(artist):
    kwargs = _get_plot_command_kwargs_from_colorfill(artist)
    kwargs.update(_get_mantid_specific_plot_kwargs(artist))
    return _remove_kwargs_if_default(kwargs)


def _remove_kwargs_if_default(kwargs):
    """Remove kwargs from the given dict if they're the default values"""
    for kwarg, default_value in mpl_default_kwargs.items():
        try:
            if kwargs[kwarg] == default_value:
                kwargs.pop(kwarg)
        except KeyError:
            pass
    return kwargs


def _get_plot_command_kwargs_from_colorfill(artist):
    props = {key: artist.properties()[key] for key in PLOT_KWARGS}
    props["cmap"] = artist.colorbar.cmap.name
    if isinstance(artist, AxesImage):
        props["aspect"] = artist.properties()["aspect"] if "aspect" in artist.properties().keys() else "auto"
        props["origin"] = artist.properties()["origin"] if "origin" in artist.properties().keys() else "lower"
        if not (isinstance(artist, QuadMesh) or isinstance(artist, Poly3DCollection)):
            props["interpolation"] = artist.get_interpolation()
    return props


def _get_mantid_specific_plot_kwargs(artist):
    ax = artist.axes
    if artist not in ax.get_tracked_artists():
        return dict()
    return {"distribution": not ax.get_artist_normalization_state(artist)}
