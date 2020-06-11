# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.image import AxesImage
from matplotlib.colors import LogNorm
from matplotlib.collections import QuadMesh
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string, clean_variable_name

BASE_IMSHOW_COMMAND = "imshow"
BASE_PCOLORMESH_COMMAND = "pcolormesh"
CFILL_NAME = "cfill"
PLOT_KWARGS = [
    'alpha', 'label', 'zorder']

mpl_default_kwargs = {
    'alpha': None,
    'label': '',
    'zorder': 0,
    'interpolation': 'nearest'
}


def generate_plot_2d_command(artist,ax_object_var):
    lines = []
    pos_args = get_plot_command_pos_args(artist)
    kwargs = get_plot_command_kwargs(artist)
    if isinstance(artist, AxesImage):
        base_command = BASE_IMSHOW_COMMAND
    else:
        base_command = BASE_PCOLORMESH_COMMAND
    arg_string = convert_args_to_string(pos_args, kwargs)
    lines.append(f"{CFILL_NAME} = {ax_object_var}.{base_command}({arg_string})")
    lines.extend(get_colorbar(artist, CFILL_NAME, ax_object_var))
    return lines


def get_colorbar(artist, image_name, ax_object_var):
    lines = []
    if artist.colorbar:
        if isinstance(artist.colorbar.norm, LogNorm):
            lines.append('from matplotlib.colors import LogNorm')
            lines.append('from matplotlib.ticker import LogLocator')
            lines.append(f"{CFILL_NAME}.set_norm(LogNorm(vmin={artist.colorbar.vmin}, vmax={artist.colorbar.vmax}))")
            lines.append('# If no ticks appear on the color bar remove the subs argument inside the LogLocator below')
            lines.append(f"cbar = fig.colorbar({image_name}, ax=[{ax_object_var}], ticks=LogLocator(subs=np.arange(1, 10)), pad=0.06)")
        else:
            lines.append(f"{CFILL_NAME}.set_norm(plt.Normalize(vmin={artist.colorbar.vmin}, vmax={artist.colorbar.vmax}))")
            lines.append(f"cbar = fig.colorbar({image_name}, ax=[{ax_object_var}], pad=0.06)")

        try:
            label = artist.colorbar.ax.yaxis.label[2]
            if label:
                lines.append(f"cbar.set_label('{label}')")
        except:
            # can't access the label it probably does not have one
            pass
    return lines


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
    props ['cmap'] = artist.colorbar.cmap.name
    if isinstance(artist, AxesImage):
        props['aspect'] = artist.properties()['aspect'] if 'aspect' in artist.properties().keys() else 'auto'
        props['origin'] = artist.properties()['origin'] if 'origin' in artist.properties().keys() else 'lower'
        if not (isinstance(artist, QuadMesh) or isinstance(artist, Poly3DCollection)):
            props['interpolation'] = artist.get_interpolation()
    return props


def _get_mantid_specific_plot_kwargs(artist):
    ax = artist.axes
    if artist not in ax.get_tracked_artists():
        return dict()
    return {
        'distribution': not ax.get_artist_normalization_state(artist)
    }
