# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib import rcParams
from matplotlib.container import ErrorbarContainer

from mantid.kernel import config
from mantid.plots.helperfunctions import errorbars_hidden
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, get_ax_from_curve
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string, clean_variable_name

BASE_CREATE_LINE_COMMAND = "plot({})"
BASE_ERRORBAR_COMMAND = "errorbar({})"
PLOT_KWARGS = [
    'alpha', 'color', 'drawstyle', 'fillstyle', 'label', 'linestyle', 'linewidth', 'marker',
    'markeredgecolor', 'markeredgewidth', 'markerfacecolor', 'markerfacecoloralt', 'markersize',
    'markevery', 'solid_capstyle', 'solid_joinstyle', 'visible', 'zorder']

mpl_default_kwargs = {
    'alpha': None,
    'barsabove': False,
    'capsize': rcParams['errorbar.capsize'],
    'capthick': rcParams['lines.markeredgewidth'],
    'color': rcParams['lines.color'],
    'distribution': config['graph1d.autodistribution'] != 'On',
    'drawstyle': 'default',
    'ecolor': rcParams['lines.color'],
    'elinewidth': rcParams['lines.linewidth'],
    'errorevery': 1,
    'fillstyle': rcParams['markers.fillstyle'],
    'label': '',
    'linestyle': rcParams['lines.linestyle'],
    'linewidth': rcParams['lines.linewidth'],
    'marker': rcParams['lines.marker'],
    'markeredgewidth': rcParams['lines.markeredgewidth'],
    'markerfacecoloralt': 'none',
    'markersize': rcParams['lines.markersize'],
    'markevery': None,
    'solid_capstyle': rcParams['lines.solid_capstyle'],
    'solid_joinstyle': rcParams['lines.solid_joinstyle'],
    'visible': True,
    'zorder': 2
}


def generate_plot_command(artist):
    pos_args = get_plot_command_pos_args(artist)
    kwargs = get_plot_command_kwargs(artist)
    if isinstance(artist, ErrorbarContainer) and not errorbars_hidden(artist):
        base_command = BASE_ERRORBAR_COMMAND
    else:
        base_command = BASE_CREATE_LINE_COMMAND
    arg_string = convert_args_to_string(pos_args, kwargs)
    return base_command.format(arg_string)


def get_plot_command_pos_args(artist):
    ax = get_ax_from_curve(artist)
    ws_name = ax.get_artists_workspace_and_spec_num(artist)[0].name()
    return [clean_variable_name(ws_name)]


def get_plot_command_kwargs(artist):
    if isinstance(artist, ErrorbarContainer):
        # We can't hide error bars using an argument in ax.errorbar() (visible
        # only affects the connecting line) so use the ax.plot command if
        # errorbars are hidden
        if errorbars_hidden(artist):
            kwargs = _get_plot_command_kwargs_from_line2d(artist[0])
            kwargs['label'] = artist.get_label()
        else:
            kwargs = _get_plot_command_kwargs_from_errorbar_container(artist)
    else:
        kwargs = _get_plot_command_kwargs_from_line2d(artist)
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


def _get_plot_command_kwargs_from_line2d(line):
    props = {key: line.properties()[key] for key in PLOT_KWARGS}
    return props


def _get_errorbar_specific_plot_kwargs(err_container):
    props = CurveProperties._get_errorbars_props_from_curve(err_container, {})
    props.pop('hide_errors')
    try:
        props['barsabove'] = err_container[2][0].zorder >= err_container[0].zorder
    except TypeError:  # Error when indexing err_container[0] when it has no line
        pass
    return props


def _get_plot_command_kwargs_from_errorbar_container(err_cont):
    props = _get_errorbar_specific_plot_kwargs(err_cont)
    if not err_cont[0]:
        props['fmt'] = None
    else:
        props.update({key: err_cont[0].properties()[key] for key in PLOT_KWARGS})
        # The capthick kwarg overwrites markeredgewidth so there's no point
        # having both
        props.pop('markeredgewidth')
    props['label'] = err_cont.get_label()
    props['zorder'] = err_cont[2][0].zorder
    return props


def _get_mantid_specific_plot_kwargs(artist):
    ax = get_ax_from_curve(artist)
    if artist not in ax.get_tracked_artists():
        return dict()
    return {
        'specNum': ax.get_artists_workspace_and_spec_num(artist)[1],
        'distribution': not ax.get_artist_normalization_state(artist)
    }
