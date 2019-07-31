# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.container import ErrorbarContainer
from numpy import ndarray

from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, get_ax_from_curve

BASE_CREATE_FIG_COMMAND = "plt.figure({})"
BASE_CREATE_AX_COMMAND = "add_subplot({})"
BASE_CREATE_LINE_COMMAND = "plot({})"
BASE_ERRORBAR_COMMAND = "errorbar({})"

ADD_SUBPLOT_KWARGS = [  # kwargs passed to the "add_subplot" command
    'frame_on', 'label', 'title', 'visible', 'xlabel', 'xlim', 'xscale',
    'ylabel', 'ylim', 'yscale']
PLOT_KWARGS = [
    'alpha', 'color', 'drawstyle', 'fillstyle', 'label', 'linestyle',
    'linewidth', 'marker', 'markeredgecolor', 'markeredgewidth',
    'markerfacecolor', 'markerfacecoloralt', 'markersize', 'markevery',
    'solid_capstyle', 'solid_joinstyle', 'visible', 'zorder'
]


def convert_value_to_arg_string(value):
    """
    Converts a given object into a string representation of that object
    which can be passed to a function. It is recursive so works on objects
    such as lists.
    """
    if isinstance(value, str) or isinstance(value, unicode):
        return "'{}'".format(value)
    if isinstance(value, list) or isinstance(value, ndarray):
        return "[{}]".format(', '.join([convert_value_to_arg_string(v) for v in value]))
    if isinstance(value, dict):
        kv_pairs = []
        for key, val in value.items():
            kv_pairs.append("{}: {}".format(convert_value_to_arg_string(key),
                                            convert_value_to_arg_string(val)))
        return "{{{}}}".format(', '.join(kv_pairs))
    return str(value)


def convert_args_to_string(args, kwargs):
    """
    Given list of args and dict of kwargs, constructs a string that
    would be valid code to pass into a Python function
    """
    arg_strings = [str(arg) for arg in args] if args else []
    for kwarg, value in sorted(kwargs.items()):  # sorting makes this testable
        arg_strings.append("{}={}".format(kwarg, convert_value_to_arg_string(value)))
    return ', '.join(arg_strings)


def get_axes_index(ax):
    """Get the index position of given Axes in its figure"""
    index = ax.rowNum*ax.numCols + ax.colNum + 1
    return index


class PlotScriptGenerator:

    def __init__(self):
        pass

    @staticmethod
    def get_figure_command_kwargs(fig):
        kwargs = {
            'figsize': (fig.get_figwidth(), fig.get_figheight()),
            'dpi': fig.dpi
        }
        return kwargs

    @staticmethod
    def generate_figure_command(fig):
        """Generate command to create figure"""
        kwargs = PlotScriptGenerator.get_figure_command_kwargs(fig)
        return BASE_CREATE_FIG_COMMAND.format(convert_args_to_string(None, kwargs))

    @staticmethod
    def get_add_subplot_pos_args(ax):
        """Get list of positional args to recreate an axes"""
        return [ax.numRows, ax.numCols, get_axes_index(ax)]

    @staticmethod
    def get_add_subplot_kwargs(ax):
        """Get kwargs for recreating an axes"""
        props = {key: ax.properties()[key] for key in ADD_SUBPLOT_KWARGS}
        props['projection'] = 'mantid'
        props['sharex'] = True if ax.get_shared_x_axes()._mapping else None
        props['sharey'] = True if ax.get_shared_y_axes()._mapping else None
        return props

    @staticmethod
    def generate_add_subplot_command(ax):
        """Generate command to create an axes"""
        command = BASE_CREATE_AX_COMMAND.format(
            convert_args_to_string(PlotScriptGenerator.get_add_subplot_pos_args(ax),
                                   PlotScriptGenerator.get_add_subplot_kwargs(ax)))
        return command

    @staticmethod
    def get_plot_command_pos_args(artist):
        ax = get_ax_from_curve(artist)
        return [ax.get_artists_workspace_and_spec_num(artist)[0]]

    @staticmethod
    def get_plot_command_kwargs(artist):
        if isinstance(artist, ErrorbarContainer):
            kwargs = PlotScriptGenerator._get_plot_command_kwargs_from_errorbar_container(artist)
        else:
            kwargs = PlotScriptGenerator._get_plot_command_kwargs_from_line2d(artist)
        kwargs.update(PlotScriptGenerator._get_mantid_specific_plot_kwargs(artist))
        return kwargs

    @staticmethod
    def _get_plot_command_kwargs_from_line2d(line):
        props = {key: line.properties()[key] for key in PLOT_KWARGS}
        props['specNum'] = line.axes.get_artists_workspace_and_spec_num(line)[1]
        return props

    @staticmethod
    def _get_errorbar_specific_plot_kwargs(err_container):
        props = CurveProperties._get_errorbars_props_from_curve(err_container, {})
        props.pop('hide_errors')
        try:
            props['barsabove'] = err_container[2][0].zorder > err_container[0].zorder
        except TypeError:  # Error when indexing err_container[0] when it has no line
            pass
        return props

    @staticmethod
    def _get_plot_command_kwargs_from_errorbar_container(err_cont):
        props = PlotScriptGenerator._get_errorbar_specific_plot_kwargs(err_cont)
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

    @staticmethod
    def _get_mantid_specific_plot_kwargs(artist):
        ax = get_ax_from_curve(artist)
        if artist not in ax.get_tracked_artists():
            return dict()
        return {'specNum': ax.get_artists_workspace_and_spec_num(artist)[1],
                'distribution': not ax.get_artist_normalization_state(artist)}

    @staticmethod
    def generate_plot_command(artist):
        pos_args = PlotScriptGenerator.get_plot_command_pos_args(artist)
        kwargs = PlotScriptGenerator.get_plot_command_kwargs(artist)
        if isinstance(artist, ErrorbarContainer):
            base_command = BASE_ERRORBAR_COMMAND
        else:
            base_command = BASE_CREATE_LINE_COMMAND
        arg_string = convert_args_to_string(pos_args, kwargs)
        return base_command.format(arg_string)
