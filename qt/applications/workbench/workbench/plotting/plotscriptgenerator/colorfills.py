# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import rcParams
from matplotlib.container import ErrorbarContainer

from mantid.kernel import config
from mantid.plots.modest_image import ModestImage
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, get_ax_from_curve
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string, clean_variable_name

BASE_IMSHOW_COMMAND = "imshow"
BASE_PCOLORMESH_COMMAND = "pcolormesh"
PLOT_KWARGS = [
    'alpha', 'label', 'zorder']

mpl_default_kwargs = {
    'alpha': None,
    'label': '',
    'zorder': 2
}


def generate_plot_2d_command(artist,ax_object_var):
    lines = []
    pos_args = get_plot_command_pos_args(artist)
    kwargs = get_plot_command_kwargs(artist)
    if isinstance(artist, ModestImage):
        base_command = BASE_IMSHOW_COMMAND
    else:
        base_command = BASE_PCOLORMESH_COMMAND
    arg_string = convert_args_to_string(pos_args, kwargs)
    lines.append("cfill = {ax_obj}.{cmd}({args})".format(ax_obj=ax_object_var,
                                            cmd=base_command,
                                            args=arg_string))
    if artist.colorbar:
        lines.append("cbar = fig.colorbar(cfill)")
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
    for color in ['markeredgecolor', 'markerfacecolor', 'ecolor']:
        try:
            if kwargs[color] == kwargs['color']:
                kwargs.pop(color)
        except KeyError:
            pass
    return kwargs


def _get_plot_command_kwargs_from_colorfill(artist):
    props = {key: artist.properties()[key] for key in PLOT_KWARGS}
    props ['cmap'] = artist.colorbar.cmap.name
    if isinstance(artist, ModestImage):
        props['aspect'] = 'auto'
        props['origin'] = 'lower'
    return props


def _get_mantid_specific_plot_kwargs(artist):
    ax = artist.axes
    if artist not in ax.get_tracked_artists():
        return dict()
    return {
        'distribution': not ax.get_artist_normalization_state(artist)
    }

