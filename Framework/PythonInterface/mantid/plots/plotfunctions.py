# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# std imports
import math
from typing import List

import numpy as np
from collections.abc import Sequence

# 3rd party imports
from matplotlib.gridspec import GridSpec
from matplotlib.legend import Legend
import matplotlib as mpl

# local imports
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup
from mantid.api import IMDHistoWorkspace
from mantid.kernel import ConfigService
from mantid.plots import datafunctions, MantidAxes
from mantid.plots.utility import MantidAxType, MARKER_MAP, get_plot_specific_properties

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = "mantid"


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
        return w.name() if hasattr(w, "name") else w

    if isinstance(workspaces, str) or not isinstance(workspaces, Sequence):
        # assume a single workspace
        first = workspaces
    else:
        assert len(workspaces) > 0
        first = workspaces[0]

    return wsname(first) + "-" + str(fig_num)


@manage_workspace_names
def plot_md_histo_ws(workspaces, errors=False, overplot=False, fig=None, ax_properties=None, window_title=None):
    """

    :param workspaces:
    :param errors:
    :param overplot:
    :param fig:
    :return:
    """
    # MDHistoWorkspace
    # Get figure and Axes
    num_axes = 1
    fig, axes = get_plot_fig(overplot, ax_properties, window_title, num_axes, fig)
    axes = [MantidAxes.from_mpl_axes(ax, ignore_artists=[Legend]) if not isinstance(ax, MantidAxes) else ax for ax in axes]

    # Plot MD
    _do_single_plot_mdhisto_workspace(axes[0], workspaces, errors)

    return _update_show_figure(fig)


@manage_workspace_names
def plot(
    workspaces,
    spectrum_nums=None,
    wksp_indices=None,
    errors=False,
    overplot=False,
    fig=None,
    plot_kwargs=None,
    ax_properties=None,
    window_title=None,
    tiled=False,
    waterfall=False,
    log_name=None,
    log_values=None,
    superplot=False,
):
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
    :param ax_properties: A dict of axes properties. E.g. {'yscale': 'log', 'xscale': 'linear'}
    :param window_title: A string denoting name of the GUI window which holds the graph
    :param tiled: An optional flag controlling whether to do a tiled or overlayed plot
    :param waterfall: An optional flag controlling whether or not to do a waterfall plot
    :param log_name: The optional log being plotted against.
    :param log_values: An optional list of log values to plot against.
    :return: The figure containing the plots
    """
    plot_font = ConfigService.getString("plots.font")
    if plot_font:
        if len(mpl.rcParams["font.family"]) > 1:
            mpl.rcParams["font.family"][0] = plot_font
        else:
            mpl.rcParams["font.family"].insert(0, plot_font)

    if plot_kwargs is None:
        plot_kwargs = {}

    if any(isinstance(i, WorkspaceGroup) for i in workspaces):
        workspaces = _unpack_grouped_workspaces(workspaces)

    _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices, tiled, overplot)
    workspaces = [ws for ws in workspaces if isinstance(ws, MatrixWorkspace)]

    if spectrum_nums is not None:
        kw, nums = "specNum", spectrum_nums
    else:
        kw, nums = "wkspIndex", wksp_indices

    _add_default_plot_kwargs_from_settings(plot_kwargs, errors)

    num_axes = len(workspaces) * len(nums) if tiled else 1

    fig, axes = get_plot_fig(overplot, ax_properties, window_title, num_axes, fig)

    # Convert to a MantidAxes if it isn't already. Ignore legend since
    # a new one will be drawn later
    axes = [MantidAxes.from_mpl_axes(ax, ignore_artists=[Legend]) if not isinstance(ax, MantidAxes) else ax for ax in axes]

    assert axes, "No axes are associated with this plot"

    if tiled:
        ws_index = [(ws, index) for ws in workspaces for index in nums]
        for index, ax in enumerate(axes):
            if index < len(ws_index):
                _do_single_plot(ax, [ws_index[index][0]], errors, False, [ws_index[index][1]], kw, plot_kwargs)
            else:
                ax.axis("off")
    else:
        show_title = ("on" == ConfigService.getString("plots.ShowTitle").lower()) and not overplot
        ax = overplot if isinstance(overplot, MantidAxes) else axes[0]
        ax.axis("on")
        _do_single_plot(ax, workspaces, errors, show_title, nums, kw, plot_kwargs, log_name, log_values)

    show_legend = "on" == ConfigService.getString("plots.ShowLegend").lower()
    for ax in axes:
        legend = ax.get_legend()
        if legend is not None:
            legend.set_visible(show_legend)

    # Can't have a waterfall plot with only one line.
    if len(nums) * len(workspaces) == 1 and waterfall:
        waterfall = False

    # The plot's initial xlim and ylim are used to offset each curve in a waterfall plot.
    # Need to do this whether the current curve is a waterfall plot or not because it may be converted later.
    if not overplot:
        datafunctions.set_initial_dimensions(ax)

    if waterfall:
        ax.set_waterfall(True)

    if not overplot and fig.canvas.manager is not None:
        fig.canvas.manager.set_window_title(figure_title(workspaces, fig.number))
    elif ax.is_waterfall():
        _overplot_waterfall(ax, len(nums) * len(workspaces))

    if ax.is_waterfall():
        # If axes is waterfall, update axes limits following line offset.
        ax.relim()
        ax.autoscale()
    elif superplot and not tiled:
        fig.canvas.manager.superplot_toggle()
        if not spectrum_nums and not wksp_indices:
            workspace_names = [ws.name() for ws in workspaces]
            fig.canvas.manager.superplot.set_workspaces(workspace_names)
            fig.canvas.manager.superplot.set_bin_mode(plot_kwargs and "axis" in plot_kwargs and plot_kwargs["axis"] == MantidAxType.BIN)
        fig.canvas.manager.superplot.enable_error_bars(errors)

    # update and show figure
    return _update_show_figure(fig)


def _overplot_waterfall(ax, no_of_lines):
    """
    If overplotting onto a waterfall axes, convert lines to waterfall (add x and y offset) before overplotting.

    :param ax: object of MantidAxes type to overplot onto.
    :param no_of_lines: number of lines to overplot onto input axes.
    """
    for i in range(no_of_lines):
        errorbar_cap_lines = datafunctions.remove_and_return_errorbar_cap_lines(ax)
        datafunctions.convert_single_line_to_waterfall(ax, len(ax.get_lines()) - (i + 1))

        if ax.waterfall_has_fill():
            datafunctions.waterfall_update_fill(ax)

        for cap in errorbar_cap_lines:
            ax.add_line(cap)


def _update_show_figure(fig):
    # This updates the toolbar so the home button now takes you back to this point.
    # The try catch is in case the manager does not have a toolbar attached.
    try:
        fig.canvas.manager.toolbar.update()
    except AttributeError:
        pass

    fig.canvas.draw_idle()
    # This displays the figure, but only works if a manager is attached to the figure.
    # The try catch is in case a figure manager is not present
    try:
        fig.show()
    except AttributeError:
        pass

    return fig


def create_subplots(nplots, fig=None, layout_engine="tight"):
    """
    Create a set of subplots suitable for a given number of plots. A stripped down
    version of plt.subplots that can accept an existing figure instance.
    Figure is given a tight layout.

    :param nplots: The number of plots required
    :param fig: An optional figure. It is cleared before plotting the new contents
    :param layout_engine: The string name of the layout engine to be used for the plot
    :return: fig, axes, ncrows, ncols
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

    fig.set_layout_engine(layout=layout_engine)

    # annoyling this repl
    nplots = nrows * ncols
    axes = np.empty(nplots, dtype=object)

    gs = GridSpec(nrows, ncols, fig)
    ax0 = fig.add_subplot(gs[0, 0], projection=PROJECTION)
    axes[0] = ax0
    for i in range(1, nplots):
        axes[i] = fig.add_subplot(gs[i // ncols, i % ncols], projection=PROJECTION)

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
        raise ValueError("{} should be a list or tuple, " "instead found '{}'".format(seq_name, value.__class__.__name__))
    if element_type is not None:

        def raise_if_not_type(x):
            if not isinstance(x, element_type):
                raise ValueError(f"{x} has unexpected type: '{x.__class__.__name__}'")

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
        # The create subplot below assumes no figure was passed in, this is ensured by the elif above
        # but add an assert which prevents a future refactoring from breaking this assumption
        assert not fig
        fig = plt.gcf()
        if not fig.axes:
            plt.close(fig)
            # The user is likely trying to overplot on a non-existent plot, create one for them
            fig, _, _, _ = create_subplots(axes_num)
    else:
        fig, _, _, _ = create_subplots(axes_num)

    if not ax_properties and not overplot:
        ax_properties = {}
        if ConfigService.getString("plots.xAxesScale").lower() == "log":
            ax_properties["xscale"] = "log"
        else:
            ax_properties["xscale"] = "linear"
        if ConfigService.getString("plots.yAxesScale").lower() == "log":
            ax_properties["yscale"] = "log"
        else:
            ax_properties["yscale"] = "linear"
    if ax_properties:
        for axis in fig.axes:
            axis.set(**ax_properties)
    if window_title and fig.canvas.manager is not None:
        fig.canvas.manager.set_window_title(window_title)

    if not overplot:
        for ax in fig.axes:
            ax.tick_params(
                which="both",
                left="on" == ConfigService.getString("plots.showTicksLeft").lower(),
                bottom="on" == ConfigService.getString("plots.showTicksBottom").lower(),
                right="on" == ConfigService.getString("plots.showTicksRight").lower(),
                top="on" == ConfigService.getString("plots.showTicksTop").lower(),
                labelleft="on" == ConfigService.getString("plots.showLabelsLeft").lower(),
                labelbottom="on" == ConfigService.getString("plots.showLabelsBottom").lower(),
                labelright="on" == ConfigService.getString("plots.showLabelsRight").lower(),
                labeltop="on" == ConfigService.getString("plots.showLabelsTop").lower(),
            )
            ax.xaxis.set_tick_params(
                which="major",
                width=int(ConfigService.getString("plots.ticks.major.width")),
                length=int(ConfigService.getString("plots.ticks.major.length")),
                direction=ConfigService.getString("plots.ticks.major.direction").lower(),
            )
            ax.yaxis.set_tick_params(
                which="major",
                width=int(ConfigService.getString("plots.ticks.major.width")),
                length=int(ConfigService.getString("plots.ticks.major.length")),
                direction=ConfigService.getString("plots.ticks.major.direction").lower(),
            )

        if ConfigService.getString("plots.ShowMinorTicks").lower() == "on":
            for ax in fig.axes:
                ax.minorticks_on()

                ax.xaxis.set_tick_params(
                    which="minor",
                    width=int(ConfigService.getString("plots.ticks.minor.width")),
                    length=int(ConfigService.getString("plots.ticks.minor.length")),
                    direction=ConfigService.getString("plots.ticks.minor.direction").lower(),
                )
                ax.yaxis.set_tick_params(
                    which="minor",
                    width=int(ConfigService.getString("plots.ticks.minor.width")),
                    length=int(ConfigService.getString("plots.ticks.minor.length")),
                    direction=ConfigService.getString("plots.ticks.minor.direction").lower(),
                )

        for ax in fig.axes:
            ax.show_minor_gridlines = ConfigService.getString("plots.ShowMinorGridlines").lower() == "on"
            for spine in ["top", "bottom", "left", "right"]:
                ax.spines[spine].set_linewidth(float(ConfigService.getString("plots.axesLineWidth")))

        if ConfigService.getString("plots.enableGrid").lower() == "on":
            try:
                fig.canvas.manager.toolbar.toggle_grid(enable=True)
            except AttributeError:
                # The canvas has no manager, or the manager has no toolbar
                pass

    return fig, fig.axes


# -----------------------------------------------------------------------------
# Private Methods
# -----------------------------------------------------------------------------
def _unpack_grouped_workspaces(mixed_list: List):
    assert isinstance(mixed_list, list), f"Expected list of group + non-group workspaces, got {repr(mixed_list)}"
    ret = []
    for ws in mixed_list:
        ret.extend([i for i in ws]) if isinstance(ws, WorkspaceGroup) else ret.append(ws)
    return ret


def _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices, tiled=False, overplot=False):
    """Raises a ValueError if any arguments have the incorrect types"""
    if spectrum_nums is not None and wksp_indices is not None:
        raise ValueError("Both spectrum_nums and wksp_indices supplied. " "Please supply only 1.")

    if tiled and overplot:
        raise ValueError("Both tiled and overplot flags set to true. " "Please set only one to true.")

    raise_if_not_sequence(workspaces, "workspaces", MatrixWorkspace)

    if spectrum_nums is not None:
        raise_if_not_sequence(spectrum_nums, "spectrum_nums")

    if wksp_indices is not None:
        raise_if_not_sequence(wksp_indices, "wksp_indices")


def _add_default_plot_kwargs_from_settings(plot_kwargs, errors):
    if "linestyle" not in plot_kwargs:
        plot_kwargs["linestyle"] = ConfigService.getString("plots.line.Style")
    if "drawstyle" not in plot_kwargs:
        plot_kwargs["drawstyle"] = ConfigService.getString("plots.line.DrawStyle")
    if "linewidth" not in plot_kwargs:
        plot_kwargs["linewidth"] = float(ConfigService.getString("plots.line.Width"))
    if "marker" not in plot_kwargs:
        plot_kwargs["marker"] = MARKER_MAP[ConfigService.getString("plots.marker.Style")]
    if "markersize" not in plot_kwargs:
        plot_kwargs["markersize"] = float(ConfigService.getString("plots.marker.Size"))
    if errors:
        if "capsize" not in plot_kwargs:
            plot_kwargs["capsize"] = float(ConfigService.getString("plots.errorbar.Capsize"))
        if "capthick" not in plot_kwargs:
            plot_kwargs["capthick"] = float(ConfigService.getString("plots.errorbar.CapThickness"))
        if "errorevery" not in plot_kwargs:
            plot_kwargs["errorevery"] = int(ConfigService.getString("plots.errorbar.errorEvery"))
        if "elinewidth" not in plot_kwargs:
            plot_kwargs["elinewidth"] = float(ConfigService.getString("plots.errorbar.Width"))


def _validate_workspace_names(workspaces):
    """
    Checks if the workspaces passed into a plotting function are workspace names, and
    retrieves the workspaces if they are.
    This function assumes that we do not have a mix of workspaces and workspace names.
    :param workspaces: A list of workspaces or workspace names
    :return: A list of workspaces
    """
    try:
        raise_if_not_sequence(workspaces, "workspaces", str)
    except ValueError:
        return workspaces
    else:
        return AnalysisDataService.Instance().retrieveWorkspaces(workspaces, unrollGroups=True)


def _do_single_plot_mdhisto_workspace(ax, workspaces, errors=False):
    """Plot IMDHistoWorkspace
    :param ax:
    :param workspaces: list of 1D MDHistoWorkspaces
    :return:
    """
    # Define plot function
    plot_fn = ax.errorbar if errors else ax.plot

    for ws in workspaces:
        # Check inputs from non-integral dimension
        if not isinstance(ws, IMDHistoWorkspace):
            raise RuntimeError(f"Workspace {str(ws)} must be an IMDHistoWorkspace but not {type(ws)}")
        num_dim = 0
        for d in range(ws.getNumDims()):
            if ws.getDimension(d).getNBins() > 1:
                num_dim += 1
        if num_dim != 1:
            raise RuntimeError(
                f"Workspace {str(ws)} is an IMDHistoWorkspace with number of non-integral dimension " f"equal to {num_dim} but not 1."
            )

        # Plot
        plot_fn(ws, label=str(ws))
        # set label is not implemented

    # Legend
    ax.make_legend()


def _set_axes_limits_from_properties(ax):
    """
    Set xlim and ylim using the x_min x_max y_min y_max properties
    :param ax:
    """
    x_min_str = ConfigService.getString("plots.x_min")
    x_max_str = ConfigService.getString("plots.x_max")
    y_min_str = ConfigService.getString("plots.y_min")
    y_max_str = ConfigService.getString("plots.y_max")
    xlim = (float(x_min_str) if x_min_str else None, float(x_max_str) if x_max_str else None)
    ylim = (float(y_min_str) if y_min_str else None, float(y_max_str) if y_max_str else None)

    ax.set_xlim(xlim)
    ax.set_ylim(ylim)


def _do_single_plot(ax, workspaces, errors, set_title, nums, kw, plot_kwargs, log_name=None, log_values=None):
    counter = 0
    for ws in workspaces:
        for num in nums:
            plot_fn = ax.errorbar if errors else ax.plot
            if isinstance(ws, MatrixWorkspace):
                plot_type = ws.getPlotType()
                _plot_kwargs = get_plot_specific_properties(ws, plot_type, plot_kwargs)
                if "errorbar" in plot_type or errors:
                    plot_fn = ax.errorbar

            if log_values:
                label = log_values[counter]
                if len(nums) > 1:
                    label = f"spec {num}: {label}"

                _plot_kwargs["label"] = label

                counter += 1

            _plot_kwargs[kw] = num
            plot_fn(ws, **_plot_kwargs)

    _set_axes_limits_from_properties(ax)
    ax.make_legend()
    if set_title:
        workspace_names = [ws.name() for ws in workspaces]
        title = ", ".join(workspace_names)

        if len(title) > 50:
            title = f"{workspace_names[0]} - {workspace_names[-1]}"

        if log_name:
            title += f" ({log_name})"

        ax.set_title(title)
