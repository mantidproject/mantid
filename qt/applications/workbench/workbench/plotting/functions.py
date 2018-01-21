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

# 3rd party imports
from mantid.api import MatrixWorkspace
import matplotlib.pyplot as plt

# local imports

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
PROJECTION = 'mantid'


# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
def plot(workspaces, spectrum_nums=None, wksp_indices=None, errors=False):
    """
    Create a figure with a single subplot and for each workspace/index add a
    line plot to the new axes. show() is called before returning the figure instance. A legend
    is added.

    :param workspaces: A list of workspace handles
    :param spectrum_nums: A list of spectrum number identifiers (general start from 1)
    :param wksp_indices: A list of workspace indexes (starts from 0)
    :param errors: If true then error bars are added for each plot
    :returns: The figure containing the plots
    """
    # check inputs
    _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices)
    if spectrum_nums is not None:
        kw, nums = 'specNum', spectrum_nums
    else:
        kw, nums = 'wkspIndex', wksp_indices
    # create figure
    fig = plt.figure()
    ax = fig.add_subplot(111, projection=PROJECTION)
    for ws in workspaces:
        for num in nums:
            ax.plot(ws, **{kw:num})
            if errors:
                ax.errorbar(ws, **{kw:num, 'label': '_hidden_on_legend'})
    ax.legend()
    ax.set_title(workspaces[0].getName())
    fig.show()
    return fig


def _validate_plot_inputs(workspaces, spectrum_nums, wksp_indices):
    """Raises a ValueError if any arguments have the incorrect types"""
    def raise_if_not_sequence(seq):
        accepted_types = [list, tuple]
        if type(seq) not in accepted_types:
            raise ValueError("Workspaces should be a list or tuple")

    if spectrum_nums is not None and wksp_indices is not None:
        raise ValueError("Both spectrum_nums and wksp_indices supplied. "
                         "Please supply only 1.")

    if not isinstance(workspaces, MatrixWorkspace):
        raise_if_not_sequence(workspaces)

    if spectrum_nums is not None:
        raise_if_not_sequence(spectrum_nums)

    if wksp_indices is not None:
        raise_if_not_sequence(wksp_indices)


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
    # create figure
    fig = plt.figure()
    ax = fig.add_subplot(111, projection=PROJECTION)
    for ws in workspaces:
        for idx in indices:
            ax.plot(ws, wkspIndex=idx, fmt=fmt)
            if error_bars:
                ax.errorbar(ws, wkspIndex=idx)
    fig.show()
    return fig
