# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#
from __future__ import (absolute_import, division, print_function)

# local imports
from mantid.kernel import Logger
from mantid.plots.utility import MantidAxType
from mantid.plots.plotfunctions import plot

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
LOGGER = Logger("mantid.plots.plotCompatability")
# ================================================
# Compatability functions
# ================================================
def plotSpectrum(workspaces, indices=None, distribution=None, error_bars=False,
                 type=None, window=None, clearWindow=None,
                 waterfall=None, spectrum_nums=None):
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
    :param type: curve style for plot it 1: scatter/dots otherwise line which is default
    :param window: If passed an existing plot then the plot will occur in that plot
    :param clearWindow: If true, and window set then the plot will be cleared before adding these curves
    :param waterfall: If true then a waterfall plot will be produced
    :param spectrum_nums: A single int or list of ints specifying the spectrum numbers to plot
                          Cannot be used at the same time as indices
    """
    _report_deprecated_parameter("distribution", distribution)

    plot_kwargs = {}
    if type==1:
        plot_kwargs["linestyle"] = "None"
        plot_kwargs["marker"] = "."
    return plot(_ensure_object_is_list(workspaces), wksp_indices=_ensure_object_is_list(indices),
                errors=error_bars, spectrum_nums=_ensure_object_is_list(spectrum_nums), waterfall = waterfall,
                fig=window, overplot=((window is not None) and not clearWindow), plot_kwargs=plot_kwargs)


def plotBin(workspaces, indices, error_bars=False, type=None, window=None, clearWindow=None,
            waterfall=None):
    """Create a 1D Plot of bin count vs spectrum in a workspace.

    This puts the spectrum number as the X variable, and the
    count in the particular bin # (in 'indices') as the Y value.

    If indices is a tuple or list, then several curves are created, one
    for each bin index.

    :param workspace or name of a workspace
    :param indices: bin number(s) to plot
    :param error_bars: If true then error bars will be added for each curve
    :param type: curve style for plot it 1: scatter/dots otherwise line, default
    :param window:If passed an existing plot then the plot will occur in that plot
    :param clearWindow: If true, and window set then the plot will be cleared before adding these curves
    :param waterfall: If true then a waterfall plot will be produced

    """

    plot_kwargs = {"axis": MantidAxType.BIN}
    if type==1:
        plot_kwargs["linestyle"] = "None"
        plot_kwargs["marker"] = "."
    return plot(_ensure_object_is_list(workspaces), wksp_indices=_ensure_object_is_list(indices),
                errors=error_bars, plot_kwargs=plot_kwargs, fig = window, waterfall = waterfall,
                overplot = ((window is not None) and not clearWindow))


# -----------------------------------------------------------------------------
# 'Private' Functions
# -----------------------------------------------------------------------------
def _ensure_object_is_list(object):
    """If the object is not a list itself it will be returned in a list"""
    if (object is None) or isinstance(object, list):
        return object
    else:
        return [object]


def _report_deprecated_parameter(param_name,param_value):
    """Logs a warning message if the parameter value is not None"""
    if param_value is not None:
        LOGGER.warning("The argument '{}' is not supported in workbench and has been ignored".format(param_name))

