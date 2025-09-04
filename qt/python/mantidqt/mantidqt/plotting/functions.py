# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Defines a collection of functions to support plotting workspaces with
our custom window.
"""

# std imports

import matplotlib
import numpy as np

# local imports
from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.plots.plotfunctions import manage_workspace_names, figure_title, plot, create_subplots, raise_if_not_sequence, plot_md_histo_ws

from mantid.kernel import Logger, ConfigService
from mantid.plots.datafunctions import add_colorbar_label
from mantid.plots.utility import get_single_workspace_log_value, legend_set_draggable
from mantidqt.plotting.figuretype import figure_type, FigureType
from mantidqt.dialogs.spectraselectorutils import get_spectra_selection
from mantid.api import IMDHistoWorkspace

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
# See https://matplotlib.org/api/_as_gen/matplotlib.figure.SubplotParams.html#matplotlib.figure.SubplotParams
SUBPLOT_WSPACE = 0.5
SUBPLOT_HSPACE = 0.5
COLORPLOT_MIN_WIDTH = 8
COLORPLOT_MIN_HEIGHT = 7
LOGGER = Logger("workspace.plotting.functions")
DEFAULT_CONTOUR_LEVELS = 2
DEFAULT_CONTOUR_COLOUR = "k"
DEFAULT_CONTOUR_WIDTH = 0.5


# -----------------------------------------------------------------------------
# 'Public' Functions
# -----------------------------------------------------------------------------


def can_overplot():
    """
    Checks if overplotting on the current figure can proceed
    with the given options

    :return: A 2-tuple of boolean indicating compatability and
    a string containing an error message if the current figure is not
    compatible.
    """
    compatible = False
    fig = current_figure_or_none()
    if fig is not None:
        figtype = figure_type(fig)
        if figtype in [FigureType.Line, FigureType.Errorbar, FigureType.Waterfall]:
            compatible = True

    return compatible


def current_figure_or_none():
    """If an active figure exists then return it otherwise return None

    :return: An active figure or None
    """
    import matplotlib.pyplot as plt

    if len(plt.get_fignums()) > 0:
        return plt.gcf()
    else:
        return None


def plot_md_ws_from_names(names, errors, overplot, fig=None):
    """
    Given a list of names of 1-dimensional IMDHistoWorkspaces and plot

    :param names: A list of workspaces names
    :param errors: If true then error bars will be plotted on the points
    :param overplot: If true then the add to the current figure if one
                     exists and it is a compatible figure
    :param fig: If not None then use this figure object to plot
    :return: The figure containing the plot or None if selection was cancelled
    """
    # Get workspaces
    workspaces = AnalysisDataService.Instance().retrieveWorkspaces(names, unrollGroups=True)

    # Check input workspace type
    for ws in workspaces:
        if not isinstance(ws, IMDHistoWorkspace):
            raise RuntimeError(f"Workspace {str(ws)} is {type(ws)} but not an IMDHistoWorkspace")

    # Plot for various cases
    if len(workspaces) > 0:
        return plot_md_histo_ws(workspaces, errors=errors, overplot=overplot, fig=fig, ax_properties=None, window_title=None)


def superplot_from_names(names, plot_kwargs):
    """
    Open the superplot with a list of workspaces but no workspace indexes
    selected.

    :param names: A list of workspace names
    """
    plot(names, plot_kwargs=plot_kwargs, wksp_indices=[], superplot=True)


def superplot_with_errors_from_names(names, plot_kwargs):
    """
    Open the superplot with a list of workspaces but no workspace indexes
    selected.

    :param names: A list of workspace names
    """
    plot(names, errors=True, plot_kwargs=plot_kwargs, wksp_indices=[], superplot=True)


def plot_from_names(names, errors, overplot, fig=None, show_colorfill_btn=False, advanced=False, superplot=False):
    """
    Given a list of names of workspaces, raise a dialog asking for the
    a selection of what to plot and then plot it.

    :param names: A list of workspace names
    :param errors: If true then error bars will be plotted on the points
    :param overplot: If true then the add to the current figure if one
                     exists and it is a compatible figure
    :param fig: If not None then use this figure object to plot
    :param advanced: If true then the advanced options will be shown in the spectra selector dialog.
    :return: The figure containing the plot or None if selection was cancelled
    """
    # Get workspaces from ADS with names
    workspaces = AnalysisDataService.Instance().retrieveWorkspaces(names, unrollGroups=True)

    try:
        # Get selected spectra from all MatrixWorkspaces
        selection = get_spectra_selection(workspaces, show_colorfill_btn=show_colorfill_btn, overplot=overplot, advanced=advanced)
    except Exception as exc:
        LOGGER.warning(format(str(exc)))
        selection = None

    if selection is None:
        return None
    elif selection == "colorfill":
        # plot mesh for color fill
        return pcolormesh_from_names(names)

    log_values = None

    if advanced:
        errors = selection.errors

        nums = selection.spectra if selection.spectra is not None else selection.wksp_indices

        if selection.log_name not in ["Workspace name", "Workspace index"]:
            log_values = []
            counter = 0
            for workspace in workspaces:
                for _ in nums:
                    if selection.custom_log_values is not None:
                        log_values.append(get_single_workspace_log_value(counter, log_values=selection.custom_log_values))
                        counter += 1
                    else:
                        log_values.append(get_single_workspace_log_value(1, matrix_ws=workspace, log_name=selection.log_name))

    if selection.plot_type == selection.Surface or selection.plot_type == selection.Contour:
        if selection.spectra is not None:
            plot_index = workspaces[0].getIndexFromSpectrumNumber(selection.spectra[0])
        else:
            plot_index = selection.wksp_indices[0]

        # import here to avoid circular import
        from mantid.plots.surfacecontourplots import plot as plot_surface_or_contour

        return plot_surface_or_contour(
            selection.plot_type, int(plot_index), selection.axis_name, selection.log_name, selection.custom_log_values, workspaces
        )
    else:
        return plot(
            names,
            spectrum_nums=selection.spectra,
            wksp_indices=selection.wksp_indices,
            errors=errors,
            overplot=overplot,
            fig=fig,
            tiled=selection.plot_type == selection.Tiled,
            waterfall=selection.plot_type == selection.Waterfall,
            log_name=selection.log_name,
            log_values=log_values,
            superplot=superplot,
        )


def pcolormesh_from_names(names, fig=None, ax=None):
    """
    Create a figure containing pcolor subplots

    :param names: A list of workspace names
    :param fig: An optional figure to contain the new plots. Its current contents will be cleared
    :param ax: An optional axis instance on which to put new plots. It's current contents will be cleared
    :returns: The figure containing the plots
    """
    try:
        if ax:
            pcolormesh_on_axis(ax, AnalysisDataService.retrieve(names[0]))
            fig.canvas.draw()
            fig.show()
            return fig
        else:
            return pcolormesh(AnalysisDataService.retrieveWorkspaces(names, unrollGroups=True), fig=fig)
    except Exception as exc:
        LOGGER.warning(format(str(exc)))
        return None


def use_imshow(ws):
    y = ws.getAxis(1).extractValues()
    if ws.getAxis(1).isText():
        nhist = ws.getNumberHistograms()
        y = np.arange(nhist)
    difference = np.diff(y)
    try:
        commonLogBins = hasattr(ws, "isCommonLogBins") and ws.isCommonLogBins()
        return np.all(np.isclose(difference[:-1], difference[0])) and not commonLogBins
    except IndexError:
        return False


@manage_workspace_names
def pcolormesh(workspaces, fig=None, color_norm=None, normalize_by_bin_width=None):
    """
    Create a figure containing pcolor subplots

    :param workspaces: A list of workspace handles
    :param fig: An optional figure to contain the new plots. Its current contents will be cleared
    :param normalize_by_bin_width: Optional and only to be used in the event that the function is being called as part
    of a plot restore
    :returns: The figure containing the plots
    """
    # check inputs
    _validate_pcolormesh_inputs(workspaces)
    workspaces = [ws for ws in workspaces if isinstance(ws, MatrixWorkspace)]

    # create a subplot of the appropriate number of dimensions
    # extend in number of columns if the number of plottables is not a square number
    workspaces_len = len(workspaces)
    # constrained layout since adding colour bar later
    fig, axes, nrows, ncols = create_subplots(workspaces_len, fig=fig, layout_engine="constrained")

    plots = []
    row_idx, col_idx = 0, 0
    for subplot_idx in range(nrows * ncols):
        ax = axes[row_idx][col_idx]
        if subplot_idx < workspaces_len:
            ws = workspaces[subplot_idx]
            pcm = pcolormesh_on_axis(ax, ws, color_norm, normalize_by_bin_width)
            plots.append(pcm)
            if col_idx < ncols - 1:
                col_idx += 1
            else:
                row_idx += 1
                col_idx = 0

            if ConfigService.getString("plots.ShowMinorTicks").lower() == "on":
                ax.minorticks_on()

            ax.show_minor_gridlines = ConfigService.getString("plots.ShowMinorGridlines").lower() == "on"
        else:
            # nothing here
            ax.axis("off")

    # If there are multiple plots limits are the min and max of all the plots
    colorbar_min = min(pt.norm.vmin for pt in plots)
    colorbar_max = max(pt.norm.vmax for pt in plots)
    for pt in plots:
        pt.set_clim(colorbar_min, colorbar_max)

    axes = axes.ravel()
    colorbar = fig.colorbar(pcm, ax=axes)
    add_colorbar_label(colorbar, axes)

    if fig.canvas.manager is not None:
        fig.canvas.manager.set_window_title(figure_title(workspaces, fig.number))
    # assert a minimum size, otherwise we can lose axis labels
    size = fig.get_size_inches()
    if (size[0] <= COLORPLOT_MIN_WIDTH) or (size[1] <= COLORPLOT_MIN_HEIGHT):
        fig.set_size_inches(COLORPLOT_MIN_WIDTH, COLORPLOT_MIN_HEIGHT, forward=True)
    fig.canvas.draw()
    fig.show()
    return fig


def pcolormesh_on_axis(ax, ws, color_norm=None, normalize_by_bin_width=None):
    """
    Plot a pcolormesh plot of the given workspace on the given axis
    :param ax: A matplotlib axes instance
    :param ws: A mantid workspace instance
    :param color_norm: A matplotlib.colours Normalize instance (or any of its subclasses)
    :param normalize_by_bin_width: Optional keyword argument to pass to imshow in the event of a plot restoration
    :return:
    """
    ax.clear()
    ax.set_title(ws.name())
    scale = _get_colorbar_scale() if not color_norm else color_norm

    if use_imshow(ws):
        pcm = ax.imshow(
            ws,
            cmap=ConfigService.getString("plots.images.Colormap"),
            aspect="auto",
            origin="lower",
            norm=scale,
            normalize_by_bin_width=normalize_by_bin_width,
        )
        # remove normalize_by_bin_width from cargs if present so that this can be toggled in future
        for cargs in pcm.axes.creation_args:
            cargs.pop("normalize_by_bin_width")
    else:
        pcm = ax.pcolormesh(ws, cmap=ConfigService.getString("plots.images.Colormap"), norm=scale)

    return pcm


def _validate_pcolormesh_inputs(workspaces):
    """Raises a ValueError if any arguments have the incorrect types"""
    raise_if_not_sequence(workspaces, "workspaces", MatrixWorkspace)


def _get_colorbar_scale():
    """Get the scale type (Linear, Log) for the colorbar in image type plots"""
    scale = ConfigService.getString("plots.images.ColorBarScale")
    if scale == "Log":
        return matplotlib.colors.LogNorm()
    else:
        return matplotlib.colors.Normalize()


@manage_workspace_names
def plot_surface(workspaces, fig=None):
    # Imported here to prevent pyplot being imported before matplotlib.use() is called when Workbench is opened.
    import matplotlib.pyplot as plt

    for ws in workspaces:
        if fig:
            fig.clf()
            ax = fig.add_subplot(111, projection="mantid3d")
        else:
            fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d", "auto_add_to_figure": False})
            fig.add_axes(ax)

        surface = ax.plot_surface(ws, cmap=ConfigService.getString("plots.images.Colormap"))
        ax.set_title(ws.name())
        fig.colorbar(surface, pad=0.15)
        fig.show()

    return fig


@manage_workspace_names
def plot_wireframe(workspaces, fig=None):
    import matplotlib.pyplot as plt
    from matplotlib import colormaps

    cmap = colormaps["tab10"]
    colors = [cmap(i / max(1, len(workspaces) - 1)) for i in range(len(workspaces))]

    if fig:
        fig.clf()
        ax = fig.add_subplot(111, projection="mantid3d")
    else:
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})

    for i, ws in enumerate(workspaces):
        try:
            ax.plot_wireframe(ws, color=colors[i], label=ws.name())
        except Exception as e:
            LOGGER.error(f"Failed to plot workspace {ws.name()}: {e}")

    if len(workspaces) > 1:
        legend = ax.legend(loc="upper right", title="Workspaces")
        legend_set_draggable(legend, True)

    workspace_names = ", ".join(ws.name() for ws in workspaces)
    ax.set_title(f"Wireframe Plot: {workspace_names}")

    fig.set_layout_engine(layout="tight")
    fig.show()

    return fig


@manage_workspace_names
def plot_contour(workspaces, fig=None):
    fig = pcolormesh(workspaces, fig)
    for idx, ws in enumerate(workspaces):
        ax = fig.get_axes()[idx]
        try:
            ax.contour(ws, levels=DEFAULT_CONTOUR_LEVELS, colors=DEFAULT_CONTOUR_COLOUR, linewidths=DEFAULT_CONTOUR_WIDTH)
        except TypeError as type_error:
            LOGGER.warning(str(type_error))

    fig.show()
    return fig
