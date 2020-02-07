# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Defines a collection of functions to support plotting workspaces with
our custom window.
"""

# std imports
import numpy as np

# 3rd party imports
try:
    from matplotlib.cm import viridis as DEFAULT_CMAP
except ImportError:
    from matplotlib.cm import jet as DEFAULT_CMAP

# local imports
from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.kernel import Logger
from mantid.plots.plotfunctions import manage_workspace_names, figure_title, plot,\
                                       create_subplots,raise_if_not_sequence
from mantidqt.plotting.figuretype import figure_type, FigureType
from mantidqt.dialogs.spectraselectorutils import get_spectra_selection
# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
# See https://matplotlib.org/api/_as_gen/matplotlib.figure.SubplotParams.html#matplotlib.figure.SubplotParams
SUBPLOT_WSPACE = 0.5
SUBPLOT_HSPACE = 0.5
LOGGER = Logger("workspace.plotting.functions")

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
    msg = "Unable to overplot on currently active plot type.\n" \
          "Please select another plot."
    fig = current_figure_or_none()
    if fig is not None:
        figtype = figure_type(fig)
        if figtype is FigureType.Line or figtype is FigureType.Errorbar:
            compatible, msg = True, None

    return compatible, msg


def current_figure_or_none():
    """If an active figure exists then return it otherwise return None

    :return: An active figure or None
    """
    import matplotlib.pyplot as plt
    if len(plt.get_fignums()) > 0:
        return plt.gcf()
    else:
        return None


def plot_from_names(names, errors, overplot, fig=None, show_colorfill_btn=False):
    """
    Given a list of names of workspaces, raise a dialog asking for the
    a selection of what to plot and then plot it.

    :param names: A list of workspace names
    :param errors: If true then error bars will be plotted on the points
    :param overplot: If true then the add to the current figure if one
                     exists and it is a compatible figure
    :param fig: If not None then use this figure object to plot
    :return: The figure containing the plot or None if selection was cancelled
    """
    workspaces = AnalysisDataService.Instance().retrieveWorkspaces(names, unrollGroups=True)
    try:
        selection = get_spectra_selection(workspaces, show_colorfill_btn=show_colorfill_btn, overplot=overplot)
    except Exception as exc:
        LOGGER.warning(format(str(exc)))
        selection = None

    if selection is None:
        return None
    elif selection == 'colorfill':
        return pcolormesh_from_names(names)

    return plot(selection.workspaces, spectrum_nums=selection.spectra,
                wksp_indices=selection.wksp_indices,
                errors=errors, overplot=overplot, fig=fig, tiled=selection.plot_type==selection.Tiled,
                waterfall=selection.plot_type==selection.Waterfall)


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
            return pcolormesh(AnalysisDataService.retrieveWorkspaces(names, unrollGroups=True),
                              fig=fig)
    except Exception as exc:
        LOGGER.warning(format(str(exc)))
        return None


def use_imshow(ws):
    y = ws.getAxis(1).extractValues()
    difference = np.diff(y)
    try:
        commonLogBins = hasattr(ws, 'isCommonLogBins') and ws.isCommonLogBins()
        return np.all(np.isclose(difference[:-1], difference[0])) and not commonLogBins
    except IndexError:
        return False


@manage_workspace_names
def pcolormesh(workspaces, fig=None):
    """
    Create a figure containing pcolor subplots

    :param workspaces: A list of workspace handles
    :param fig: An optional figure to contain the new plots. Its current contents will be cleared
    :returns: The figure containing the plots
    """
    # check inputs
    _validate_pcolormesh_inputs(workspaces)
    workspaces = [ws for ws in workspaces if isinstance(ws, MatrixWorkspace)]

    # create a subplot of the appropriate number of dimensions
    # extend in number of columns if the number of plottables is not a square number
    workspaces_len = len(workspaces)
    fig, axes, nrows, ncols = create_subplots(workspaces_len, fig=fig)

    row_idx, col_idx = 0, 0
    for subplot_idx in range(nrows * ncols):
        ax = axes[row_idx][col_idx]
        if subplot_idx < workspaces_len:
            ws = workspaces[subplot_idx]
            pcm = pcolormesh_on_axis(ax, ws)
            if col_idx < ncols - 1:
                col_idx += 1
            else:
                row_idx += 1
                col_idx = 0
        else:
            # nothing here
            ax.axis('off')

    # Adjust locations to ensure the plots don't overlap
    fig.subplots_adjust(wspace=SUBPLOT_WSPACE, hspace=SUBPLOT_HSPACE)
    fig.colorbar(pcm, ax=axes.ravel().tolist(), pad=0.06)
    fig.canvas.set_window_title(figure_title(workspaces, fig.number))
    fig.canvas.draw()
    fig.show()
    return fig


def pcolormesh_on_axis(ax, ws):
    """
    Plot a pcolormesh plot of the given workspace on the given axis
    :param ax: A matplotlib axes instance
    :param ws: A mantid workspace instance
    :return:
    """
    ax.clear()
    ax.set_title(ws.name())
    if use_imshow(ws):
        pcm = ax.imshow(ws, cmap=DEFAULT_CMAP, aspect='auto', origin='lower')
    else:
        pcm = ax.pcolormesh(ws, cmap=DEFAULT_CMAP)
    for lbl in ax.get_xticklabels():
        lbl.set_rotation(45)

    return pcm


def _validate_pcolormesh_inputs(workspaces):
    """Raises a ValueError if any arguments have the incorrect types"""
    raise_if_not_sequence(workspaces, 'workspaces', MatrixWorkspace)
