#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""Defines a collection of functions to support plotting workspaces with
our custom window.
"""

# std imports
import math

# 3rd party imports
from mantid.api import MatrixWorkspace
import matplotlib.pyplot as plt

# local imports

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = 'mantid'
DEFAULT_COLORMAP = 'viridis'
# See https://matplotlib.org/api/_as_gen/matplotlib.figure.SubplotParams.html#matplotlib.figure.SubplotParams
SUBPLOT_WSPACE = 0.5
SUBPLOT_HSPACE = 0.5


# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def raise_if_not_sequence(seq, seq_name):
    accepted_types = [list, tuple]
    if type(seq) not in accepted_types:
        raise ValueError("{} should be a list or tuple".format(seq_name))


def _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices):
    """Raises a ValueError if any arguments have the incorrect types"""
    if spectrum_nums is not None and wksp_indices is not None:
        raise ValueError("Both spectrum_nums and wksp_indices supplied. "
                         "Please supply only 1.")

    if not isinstance(workspaces, MatrixWorkspace):
        raise_if_not_sequence(workspaces, 'Workspaces')

    if spectrum_nums is not None:
        raise_if_not_sequence(spectrum_nums, 'spectrum_nums')

    if wksp_indices is not None:
        raise_if_not_sequence(wksp_indices, 'wksp_indices')


def _validate_pcolormesh_inputs(workspaces):
    """Raises a ValueError if any arguments have the incorrect types"""
    if not isinstance(workspaces, MatrixWorkspace):
        raise_if_not_sequence(workspaces, 'Workspaces')


def plot(workspaces, spectrum_nums=None, wksp_indices=None, errors=False,
         overplot=False):
    """
    Create a figure with a single subplot and for each workspace/index add a
    line plot to the new axes. show() is called before returning the figure instance. A legend
    is added.

    :param workspaces: A list of workspace handles
    :param spectrum_nums: A list of spectrum number identifiers (general start from 1)
    :param wksp_indices: A list of workspace indexes (starts from 0)
    :param errors: If true then error bars are added for each plot
    :param overplot: If true then overplot over the current figure if one exists
    :returns: The figure containing the plots
    """
    # check inputs
    _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices)
    if spectrum_nums is not None:
        kw, nums = 'specNum', spectrum_nums
    else:
        kw, nums = 'wkspIndex', wksp_indices

    # get/create the axes to hold the plot
    if overplot:
        ax = plt.gca(projection=PROJECTION)
        fig = ax.figure
    else:
        fig = plt.figure()
        ax = fig.add_subplot(111, projection=PROJECTION)

    # do the plotting
    plot_fn = ax.errorbar if errors else ax.plot
    for ws in workspaces:
        for num in nums:
            plot_fn(ws, **{kw: num})

    ax.legend()
    title = workspaces[0].name()
    ax.set_title(title)
    fig.canvas.set_window_title(title)
    fig.canvas.draw()
    fig.show()
    return fig


def pcolormesh(workspaces):
    """
    Create a figure containing subplots

    :param workspaces: A list of workspace handles
    :param spectrum_nums: A list of spectrum number identifiers (general start from 1)
    :param wksp_indices: A list of workspace indexes (starts from 0)
    :param errors: If true then error bars are added for each plot
    :returns: The figure containing the plots
    """
    # check inputs
    _validate_pcolormesh_inputs(workspaces)

    # create a subplot of the appropriate number of dimensions
    # extend in number of columns if the number of plottables is not a square number
    workspaces_len = len(workspaces)
    square_side_len = int(math.ceil(math.sqrt(workspaces_len)))
    nrows, ncols = square_side_len, square_side_len
    if square_side_len*square_side_len != workspaces_len:
        # not a square number - square_side_len x square_side_len
        # will be large enough but we could end up with an empty
        # row so chop that off
        if workspaces_len <= (nrows-1)*ncols:
            nrows -= 1

    fig, axes = plt.subplots(nrows, ncols, squeeze=False,
                             subplot_kw=dict(projection=PROJECTION))
    row_idx, col_idx = 0, 0
    for subplot_idx in range(nrows*ncols):
        ax = axes[row_idx][col_idx]
        if subplot_idx < workspaces_len:
            ws = workspaces[subplot_idx]
            ax.set_title(ws.name())
            pcm = ax.pcolormesh(ws, cmap=DEFAULT_COLORMAP)
            xticks = ax.get_xticklabels()
            map(lambda lbl: lbl.set_rotation(45), xticks)
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
    fig.canvas.draw()
    fig.show()
    return fig


# Compatibility function for existing MantidPlot functionality
def plotSpectrum(workspaces, indices, distribution=None, error_bars=False,
                 type=None, window=None, clearWindow=None,
                 waterfall=False):
    """
    Create a figure with a single subplot and for each workspace/index add a
    line plot to the new axes. show() is called before returning the figure instance

    :param workspaces: Workspace/workspaces to plot as a string, workspace handle, list of strings or list of
    workspaces handles.
    :param indices: A single int or list of ints specifying the workspace indices to plot
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.
    :param error_bars: If true then error bars will be added for each curve
    :param type: curve style for plot (-1: unspecified; 0: line, default; 1: scatter/dots)
    :param window: Ignored. Here to preserve backwards compatibility
    :param clearWindow: Ignored. Here to preserve backwards compatibility
    :param waterfall:
    """
    if type == 1:
        fmt = 'o'
    else:
        fmt = '-'

    return plot(workspaces, wksp_indices=indices,
                errors=error_bars, fmt=fmt)
