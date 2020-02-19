# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from __future__ import absolute_import

# std imports
import math
import numpy as np
import collections

# 3rd party imports
from matplotlib.gridspec import GridSpec
from matplotlib.legend import Legend

# local imports
from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.kernel import ConfigService
from mantid.plots import datafunctions, MantidAxes
from mantid.py3compat import is_text_string, string_types

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = 'mantid'

MARKER_MAP = {'square': 's', 'plus (filled)': 'P', 'point': '.', 'tickdown': 3,
              'triangle_right': '>', 'tickup': 2, 'hline': '_', 'vline': '|',
              'pentagon': 'p', 'tri_left': '3', 'caretdown': 7,
              'caretright (centered at base)': 9, 'tickright': 1,
              'caretright': 5, 'caretleft': 4, 'tickleft': 0, 'tri_up': '2',
              'circle': 'o', 'pixel': ',', 'caretleft (centered at base)': 8,
              'diamond': 'D', 'star': '*', 'hexagon1': 'h', 'octagon': '8',
              'hexagon2': 'H', 'tri_right': '4', 'x (filled)': 'X',
              'thin_diamond': 'd', 'tri_down': '1', 'triangle_left': '<',
              'plus': '+', 'triangle_down': 'v', 'triangle_up': '^', 'x': 'x',
              'caretup': 6, 'caretup (centered at base)': 10,
              'caretdown (centered at base)': 11, 'None': 'None'}

# -----------------------------------------------------------------------------
# Decorators
# -----------------------------------------------------------------------------
def manage_workspace_names(func):
    """
    A decorator to go around plotting functions.
    This will retrieve workspaces from workspace names before
    calling the plotting function
    :param func: A plotting function
    :return:
    """

    def inner_func(workspaces, *args, **kwargs):
        workspaces = _validate_workspace_names(workspaces)
        return func(workspaces, *args, **kwargs)

    return inner_func


# -----------------------------------------------------------------------------
# Public Methods
# -----------------------------------------------------------------------------
def figure_title(workspaces, fig_num):
    """Create a default figure title from a single workspace, list of workspaces or
    workspace names and a figure number. The name of the first workspace in the list
    is concatenated with the figure number.

    :param workspaces: A single workspace, list of workspaces or workspace name/list of workspace names
    :param fig_num: An integer denoting the figure number
    :return: A title for the figure
    """

    def wsname(w):
        return w.name() if hasattr(w, 'name') else w

    if is_text_string(workspaces) or not isinstance(workspaces, collections.Sequence):
        # assume a single workspace
        first = workspaces
    else:
        assert len(workspaces) > 0
        first = workspaces[0]

    return wsname(first) + '-' + str(fig_num)

@manage_workspace_names
def plot(workspaces, spectrum_nums=None, wksp_indices=None, errors=False,
         overplot=False, fig=None, plot_kwargs=None, ax_properties=None,
         window_title=None, tiled=False, waterfall=False):
    """
    Create a figure with a single subplot and for each workspace/index add a
    line plot to the new axes. show() is called before returning the figure instance. A legend
    is added.

    :param workspaces: A list of workspace handles or strings
    :param spectrum_nums: A list of spectrum number identifiers (general start from 1)
    :param wksp_indices: A list of workspace indexes (starts from 0)
    :param errors: If true then error bars are added for each plot
    :param overplot: If true then overplot over the current figure if one exists. If an axis object the overplotting
    will be done on the axis passed in
    :param fig: If not None then use this Figure object to plot
    :param plot_kwargs: Arguments that will be passed onto the plot function
    :param ax_properties: A dict of axes properties. E.g. {'yscale': 'log'}
    :param window_title: A string denoting name of the GUI window which holds the graph
    :param tiled: An optional flag controlling whether to do a tiled or overlayed plot
    :param waterfall: An optional flag controlling whether or not to do a waterfall plot
    :return: The figure containing the plots
    """
    if plot_kwargs is None:
        plot_kwargs = {}
    _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices, tiled, overplot)
    workspaces = [ws for ws in workspaces if isinstance(ws, MatrixWorkspace)]

    if spectrum_nums is not None:
        kw, nums = 'specNum', spectrum_nums
    else:
        kw, nums = 'wkspIndex', wksp_indices

    _add_default_plot_kwargs_from_settings(plot_kwargs, errors)

    num_axes = len(workspaces) * len(nums) if tiled else 1

    fig, axes = get_plot_fig(overplot, ax_properties, window_title, num_axes, fig)

    # Convert to a MantidAxes if it isn't already. Ignore legend since
    # a new one will be drawn later
    axes = [MantidAxes.from_mpl_axes(ax, ignore_artists=[Legend]) if not isinstance(ax, MantidAxes) else ax
            for ax in axes]

    if tiled:
        ws_index = [(ws, index) for ws in workspaces for index in nums]
        for index, ax in enumerate(axes):
            if index < len(ws_index):
                _do_single_plot(ax, [ws_index[index][0]], errors, False, [ws_index[index][1]], kw, plot_kwargs)
            else:
                ax.axis('off')
    else:
        show_title = ("on" == ConfigService.getString("plots.ShowTitle").lower()) and not overplot
        ax = overplot if isinstance(overplot, MantidAxes) else axes[0]
        ax.axis('on')
        _do_single_plot(ax, workspaces, errors, show_title, nums, kw, plot_kwargs)

    # Can't have a waterfall plot with only one line.
    if len(nums) == 1 and waterfall:
        waterfall = False

    # The plot's initial xlim and ylim are used to offset each curve in a waterfall plot.
    # Need to do this whether the current curve is a waterfall plot or not because it may be converted later.
    if not overplot:
        datafunctions.set_initial_dimensions(ax)

    if waterfall:
        ax.set_waterfall(True)

    if not overplot:
        fig.canvas.set_window_title(figure_title(workspaces, fig.number))
    else:
        if ax.is_waterfall():
            for i in range(len(nums)):
                errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(ax)
                datafunctions.convert_single_line_to_waterfall(ax, len(ax.get_lines()) - (i + 1))

                if ax.waterfall_has_fill():
                    datafunctions.waterfall_update_fill(ax)

                ax.lines += errorbar_cap_lines

    # This updates the toolbar so the home button now takes you back to this point.
    # The try catch is in case the manager does not have a toolbar attached.
    try:
        fig.canvas.manager.toolbar.update()
    except AttributeError:
        pass

    fig.canvas.draw()
    # This displays the figure, but only works if a manager is attached to the figure.
    # The try catch is in case a figure manager is not present
    try:
        fig.show()
    except AttributeError:
        pass

    return fig


def create_subplots(nplots, fig=None):
    """
    Create a set of subplots suitable for a given number of plots. A stripped down
    version of plt.subplots that can accept an existing figure instance.

    :param nplots: The number of plots required
    :param fig: An optional figure. It is cleared before plotting the new contents
    :return: A 2-tuple of (fig, axes)
    """
    import matplotlib.pyplot as plt
    square_side_len = int(math.ceil(math.sqrt(nplots)))
    nrows, ncols = square_side_len, square_side_len
    if square_side_len * square_side_len != nplots:
        # not a square number - square_side_len x square_side_len
        # will be large enough but we could end up with an empty
        # row so chop that off
        if nplots <= (nrows - 1) * ncols:
            nrows -= 1

    if fig is None:
        fig = plt.figure()
    else:
        fig.clf()
    # annoyling this repl
    nplots = nrows * ncols
    gs = GridSpec(nrows, ncols)
    axes = np.empty(nplots, dtype=object)
    ax0 = fig.add_subplot(gs[0, 0], projection=PROJECTION)
    axes[0] = ax0
    for i in range(1, nplots):
        axes[i] = fig.add_subplot(gs[i // ncols, i % ncols],
                                  projection=PROJECTION)
    axes = axes.reshape(nrows, ncols)

    return fig, axes, nrows, ncols


def raise_if_not_sequence(value, seq_name, element_type=None):
    """
    Raise a ValueError if the given object is not a sequence

    :param value: The value object to validate
    :param seq_name: The variable name of the sequence for the error message
    :param element_type: An optional type to provide to check that each element
    is an instance of this type
    :raises ValueError: if the conditions are not met
    """
    accepted_types = (list, tuple, range)
    if type(value) not in accepted_types:
        raise ValueError("{} should be a list or tuple, "
                         "instead found '{}'".format(seq_name,
                                                     value.__class__.__name__))
    if element_type is not None:
        def raise_if_not_type(x):
            if not isinstance(x, element_type):
                if element_type == MatrixWorkspace:
                    # If the workspace is the wrong type, log the error and remove it from the list so that the other
                    # workspaces can still be plotted.
                    LOGGER.warning("{} has unexpected type '{}'".format(x, x.__class__.__name__))
                else:
                    raise ValueError("Unexpected type: '{}'".format(x.__class__.__name__))

        # Map in Python3 is an iterator, so ValueError will not be raised unless the values are yielded.
        # converting to a list forces yielding
        list(map(raise_if_not_type, value))


def get_plot_fig(overplot=None, ax_properties=None, window_title=None, axes_num=1, fig=None):
    """
    Create a blank figure and axes, with configurable properties.
    :param overplot: If true then plotting on figure will plot over previous plotting. If an axis object the overplotting
    will be done on the axis passed in
    :param ax_properties: A dict of axes properties. E.g. {'yscale': 'log'} for log y-axis
    :param window_title: A string denoting the name of the GUI window which holds the graph
    :param axes_num: The number of axes to create on the figure
    :param fig: An optional figure object
    :return: Matplotlib fig and axes objects
    """
    import matplotlib.pyplot as plt
    if fig and overplot:
        fig = fig
    elif fig:
        fig, _, _, _ = create_subplots(axes_num, fig)
    elif overplot:
        fig = plt.gcf()
    else:
        fig, _, _, _ = create_subplots(axes_num)

    if not ax_properties:
        ax_properties = {}
        if ConfigService.getString("plots.xAxesScale").lower() == 'log':
            ax_properties['xscale'] = 'log'
        else:
            ax_properties['xscale'] = 'linear'
        if ConfigService.getString("plots.yAxesScale").lower() == 'log':
            ax_properties['yscale'] = 'log'
        else:
            ax_properties['yscale'] = 'linear'
    if ax_properties:
        for axis in fig.axes:
            axis.set(**ax_properties)
    if window_title:
        fig.canvas.set_window_title(window_title)

    return fig, fig.axes


# -----------------------------------------------------------------------------
# Pricate Methods
# -----------------------------------------------------------------------------
def _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices, tiled=False, overplot=False):
    """Raises a ValueError if any arguments have the incorrect types"""
    if spectrum_nums is not None and wksp_indices is not None:
        raise ValueError("Both spectrum_nums and wksp_indices supplied. "
                         "Please supply only 1.")

    if tiled and overplot:
        raise ValueError("Both tiled and overplot flags set to true. "
                         "Please set only one to true.")

    raise_if_not_sequence(workspaces, 'workspaces', MatrixWorkspace)

    if spectrum_nums is not None:
        raise_if_not_sequence(spectrum_nums, 'spectrum_nums')

    if wksp_indices is not None:
        raise_if_not_sequence(wksp_indices, 'wksp_indices')


def _add_default_plot_kwargs_from_settings(plot_kwargs, errors):
    if 'linestyle' not in plot_kwargs:
        plot_kwargs['linestyle'] = ConfigService.getString("plots.line.Style")
    if 'linewidth' not in plot_kwargs:
        plot_kwargs['linewidth'] = float(ConfigService.getString("plots.line.Width"))
    if 'marker' not in plot_kwargs:
        plot_kwargs['marker'] = MARKER_MAP[ConfigService.getString("plots.marker.Style")]
    if 'markersize' not in plot_kwargs:
        plot_kwargs['markersize'] = float(ConfigService.getString("plots.marker.Size"))
    if errors:
        if 'capsize' not in plot_kwargs:
            plot_kwargs['capsize'] = float(ConfigService.getString("plots.errorbar.Capsize"))
        if 'capthick' not in plot_kwargs:
            plot_kwargs['capthick'] = float(ConfigService.getString("plots.errorbar.CapThickness"))
        if 'errorevery' not in plot_kwargs:
            plot_kwargs['errorevery'] = int(ConfigService.getString("plots.errorbar.errorEvery"))
        if 'elinewidth' not in plot_kwargs:
            plot_kwargs['elinewidth'] = float(ConfigService.getString("plots.errorbar.Width"))


def _validate_workspace_names(workspaces):
    """
    Checks if the workspaces passed into a plotting function are workspace names, and
    retrieves the workspaces if they are.
    This function assumes that we do not have a mix of workspaces and workspace names.
    :param workspaces: A list of workspaces or workspace names
    :return: A list of workspaces
    """
    try:
        raise_if_not_sequence(workspaces, 'workspaces', string_types)
    except ValueError:
        return workspaces
    else:
        return AnalysisDataService.Instance().retrieveWorkspaces(workspaces, unrollGroups=True)


def _do_single_plot(ax, workspaces, errors, set_title, nums, kw, plot_kwargs):
    # do the plotting
    plot_fn = ax.errorbar if errors else ax.plot
    for ws in workspaces:
        for num in nums:
            plot_kwargs[kw] = num
            plot_fn(ws, **plot_kwargs)
    ax.make_legend()
    if set_title:
        title = workspaces[0].name()
        ax.set_title(title)


