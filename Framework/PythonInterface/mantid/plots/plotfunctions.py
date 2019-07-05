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

from mantid.plots.utility import MantidAxType
import collections
import sys

import matplotlib
import matplotlib.collections as mcoll
import matplotlib.colors
import matplotlib.dates as mdates
import matplotlib.image as mimage
import numpy

import mantid.api
import mantid.kernel
from mantid.plots.helperfunctions import get_axes_labels, get_bins, get_data_uneven_flag, get_distribution, \
    get_matrix_2d_ragged, get_matrix_2d_data, get_md_data1d, get_md_data2d_bin_bounds, \
    get_md_data2d_bin_centers, get_normalization, get_sample_log, get_spectrum, get_uneven_data, \
    get_wksp_index_dist_and_label, check_resample_to_regular_grid, get_indices, get_normalize_by_bin_width

import mantid.plots.modest_image

# Used for initializing searches of max, min values
_LARGEST, _SMALLEST = float(sys.maxsize), -sys.maxsize


# ================================================
# Private 2D Helper functions
# ================================================

def _setLabels1D(axes, workspace, indices=None, normalize_by_bin_width=True,
                 axis=MantidAxType.SPECTRUM):
    '''
    helper function to automatically set axes labels for 1D plots
    '''
    labels = get_axes_labels(workspace, indices, normalize_by_bin_width)
    # We assume that previous checking has ensured axis can only be 1 of 2 types
    axes.set_xlabel(labels[1 if axis == MantidAxType.SPECTRUM else 2])
    axes.set_ylabel(labels[0])


def _setLabels2D(axes, workspace, indices=None, transpose=False, xscale=None):
    '''
    helper function to automatically set axes labels for 2D plots
    '''
    labels = get_axes_labels(workspace, indices)
    if transpose:
        axes.set_xlabel(labels[2])
        axes.set_ylabel(labels[1])
    else:
        axes.set_xlabel(labels[1])
        axes.set_ylabel(labels[2])
    axes.set_title(labels[-1])
    if xscale is None and hasattr(workspace, 'isCommonLogBins') and workspace.isCommonLogBins():
        axes.set_xscale('log')
    elif xscale is not None:
        axes.set_xscale(xscale)


def _get_data_for_plot(axes, workspace, kwargs, with_dy=False, with_dx=False):
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        (x, y, dy) = get_md_data1d(workspace, normalization, indices)
        dx = None
        axis = None
    else:
        axis = MantidAxType(kwargs.pop("axis", MantidAxType.SPECTRUM))
        normalize_by_bin_width, kwargs = get_normalize_by_bin_width(
            workspace, axes, **kwargs)
        workspace_index, distribution, kwargs = get_wksp_index_dist_and_label(workspace, axis, **kwargs)
        if axis == MantidAxType.BIN:
            # Overwrite any user specified xlabel
            axes.set_xlabel("Spectrum")
            x, y, dy, dx = get_bins(workspace, workspace_index, with_dy)
        elif axis == MantidAxType.SPECTRUM:
            x, y, dy, dx = get_spectrum(workspace, workspace_index,
                                        normalize_by_bin_width, with_dy, with_dx)
        else:
            raise ValueError("Axis {} is not a valid axis number.".format(axis))
        indices = None
    return x, y, dy, dx, indices, axis, kwargs


# ========================================================
# Plot functions
# ========================================================

def _plot_impl(axes, workspace, args, kwargs):
    """
    Compute data and labels for plot. Used by workspace
    replacement handlers to recompute data. See plot for
    argument details
    """
    if 'LogName' in kwargs:
        (x, y, FullTime, LogName, units, kwargs) = get_sample_log(workspace, **kwargs)
        axes.set_ylabel('{0} ({1})'.format(LogName, units))
        axes.set_xlabel('Time (s)')
        if FullTime:
            axes.xaxis_date()
            axes.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S\n%b-%d'))
            axes.set_xlabel('Time')
        kwargs['linestyle'] = 'steps-post'
    else:
        normalize_by_bin_width, kwargs = get_normalize_by_bin_width(
            workspace, axes, **kwargs)
        x, y, _, _, indices, axis, kwargs = _get_data_for_plot(axes, workspace, kwargs)
        _setLabels1D(axes, workspace, indices, normalize_by_bin_width=normalize_by_bin_width,
                     axis=axis)
    return x, y, args, kwargs


def plot(axes, workspace, *args, **kwargs):
    """
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.plot` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param specNum:   spectrum number to plot if MatrixWorkspace
    :param wkspIndex: workspace index to plot if MatrixWorkspace
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param LogName:   if specified, it will plot the corresponding sample log. The x-axis
                      of the plot is the time difference between the log time and the first
                      value of the `proton_charge` log (if available) or the sample log's
                      first time.
    :param StartFromLog: False by default. If True the time difference will be from the sample log's
                      first time, even if `proton_charge` log is available.
    :param FullTime:  False by default. If true, the full date and time will be plotted on the axis
                      instead of the time difference
    :param ExperimentInfo: for MD Workspaces with multiple :class:`mantid.api.ExperimentInfo` is the
                           ExperimentInfo object from which to extract the log. It's 0 by default
    :param axis: Specify which axis will be plotted. Use axis=MantidAxType.BIN to plot a bin,
                 and axis=MantidAxType.SPECTRUM to plot a spectrum.
                 The default value is axis=1, plotting spectra by default.
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimension to plot. *e.g.* to select the second axis to plot from a
                    3D volume use ``indices=(5, slice(None), 10)`` where the 5/10 are the bins selected
                    for the other 2 axes.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the second
                       axis to plot from a 3D volume use ``slicepoint=(1.0, None, 2.0)`` where the 1.0/2.0 are
                       the dimension selected for the other 2 axes.

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    """
    x, y, args, kwargs = _plot_impl(axes, workspace, args, kwargs)
    return axes.plot(x, y, *args, **kwargs)


def errorbar(axes, workspace, *args, **kwargs):
    """
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.errorbar` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param specNum:   spectrum number to plot if MatrixWorkspace
    :param wkspIndex: workspace index to plot if MatrixWorkspace
    :param distribution: ``None`` (default) asks the global setting. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.
    :param normalize_by_bin_width: Plot the workspace as a distribution. If None default to global
                                   setting: config['graph1d.autodistribution']
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param axis: Specify which axis will be plotted. Use axis=MantidAxType.BIN to plot a bin,
                  and axis=MantidAxType.SPECTRUM to plot a spectrum.
                  The default value is axis=1, plotting spectra by default.
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimension to plot. *e.g.* to select the second axis to plot from a
                    3D volume use ``indices=(5, slice(None), 10)`` where the 5/10 are the bins selected
                    for the other 2 axes.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the second
                       axis to plot from a 3D volume use ``slicepoint=(1.0, None, 2.0)`` where the 1.0/2.0 are
                       the dimension selected for the other 2 axes.

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    """
    normalize_by_bin_width, kwargs = get_normalize_by_bin_width(
        workspace, axes, **kwargs)
    x, y, dy, dx, indices, axis, kwargs = _get_data_for_plot(
        axes, workspace, kwargs, with_dy=True, with_dx=False)
    _setLabels1D(axes, workspace, indices, normalize_by_bin_width=normalize_by_bin_width,
                 axis=axis)

    return axes.errorbar(x, y, dy, dx, *args, **kwargs)


def scatter(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.scatter` after special
    keyword arguments are removed. This will automatically label the
    line according to the spectrum number unless specified otherwise.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param specNum:   spectrum number to plot if MatrixWorkspace
    :param wkspIndex: workspace index to plot if MatrixWorkspace
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimension to plot. *e.g.* to select the second axis to plot from a
                    3D volume use ``indices=(5, slice(None), 10)`` where the 5/10 are the bins selected
                    for the other 2 axes.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the second
                       axis to plot from a 3D volume use ``slicepoint=(1.0, None, 2.0)`` where the 1.0/2.0 are
                       the dimension selected for the other 2 axes.

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        (x, y, _) = get_md_data1d(workspace, normalization, indices)
        _setLabels1D(axes, workspace, indices)
    else:
        (wkspIndex, distribution, kwargs) = get_wksp_index_dist_and_label(workspace, **kwargs)
        (x, y, _, _) = get_spectrum(workspace, wkspIndex, distribution)
        _setLabels1D(axes, workspace)
    return axes.scatter(x, y, *args, **kwargs)


def contour(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.contour`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_centers(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    return axes.contour(x, y, z, *args, **kwargs)


def contourf(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.contourf`
    but calculates the countour levels. Currently this only works with
    workspaces that have a constant number of bins between spectra.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_centers(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    return axes.contourf(x, y, z, *args, **kwargs)


def _pcolorpieces(axes, workspace, distribution, *args, **kwargs):
    '''
    Helper function for pcolor, pcolorfast, and pcolormesh that will
    plot a 2d representation of each spectra. The polycollections or meshes
    will be normalized to the same intensity limits.
    :param axes: :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param pcolortype: this keyword allows the plotting to be one of pcolormesh or
        pcolorfast if there is "mesh" or "fast" in the value of the keyword, or
        pcolor by default
    :return: A list of the pcolor pieces created
    '''
    (x, y, z) = get_uneven_data(workspace, distribution)
    mini = numpy.min([numpy.min(i) for i in z])
    maxi = numpy.max([numpy.max(i) for i in z])
    if 'vmin' in kwargs:
        mini = kwargs['vmin']
    if 'vmax' in kwargs:
        maxi = kwargs['vmax']
    if 'norm' not in kwargs:
        kwargs['norm'] = matplotlib.colors.Normalize(vmin=mini, vmax=maxi)
    else:
        if kwargs['norm'].vmin is None:
            kwargs['norm'].vmin = mini
        if kwargs['norm'].vmax is None:
            kwargs['norm'].vmax = maxi

    # setup the particular pcolor to use
    pcolortype = kwargs.pop('pcolortype', '').lower()
    if 'mesh' in pcolortype:
        pcolor = axes.pcolormesh
    elif 'fast' in pcolortype:
        pcolor = axes.pcolorfast
    else:
        pcolor = axes.pcolor

    pieces = []
    for xi, yi, zi in zip(x, y, z):
        XX, YY = numpy.meshgrid(xi, yi, indexing='ij')
        pieces.append(pcolor(XX, YY, zi.reshape(-1, 1), **kwargs))

    return pieces


def pcolor(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolor`

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = ''
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True, transpose=transpose)
            _setLabels2D(axes, workspace, transpose=transpose)
    return axes.pcolor(x, y, z, *args, **kwargs)


def pcolorfast(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolorfast`

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'fast'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    return axes.pcolorfast(x, y, z, *args, **kwargs)


def pcolormesh(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.pcolormesh`.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'mesh'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    return axes.pcolormesh(x, y, z, *args, **kwargs)


def imshow(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`matplotlib.axes.Axes.imshow`.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization, indices, transpose)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (uneven_bins, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if check_resample_to_regular_grid(workspace):
            (x, y, z) = get_matrix_2d_ragged(workspace, distribution, histogram2D=True, transpose=transpose)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    if 'extent' not in kwargs:
        if x.ndim == 2 and y.ndim == 2:
            kwargs['extent'] = [x[0, 0], x[0, -1], y[0, 0], y[-1, 0]]
        else:
            kwargs['extent'] = [x[0], x[-1], y[0], y[-1]]
    return mantid.plots.modest_image.imshow(axes, z, *args, **kwargs)


def tripcolor(axes, workspace, *args, **kwargs):
    '''
    To be used with non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra or with
    MDHistoWorkspaces.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace

    See :meth:`matplotlib.axes.Axes.tripcolor` for more information.
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x_temp, y_temp, z = get_md_data2d_bin_centers(workspace, normalization, indices, transpose)
        x, y = numpy.meshgrid(x_temp, y_temp)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    return axes.tripcolor(x.ravel(), y.ravel(), z.ravel(), *args, **kwargs)


def tricontour(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.contour`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra or with
    MDHistoWorkspaces.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace

    See :meth:`matplotlib.axes.Axes.tricontour` for more information.
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x_temp, y_temp, z = get_md_data2d_bin_centers(workspace, normalization, indices, transpose)
        x, y = numpy.meshgrid(x_temp, y_temp)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    # tricontour segfaults if many z values are not finite
    # https://github.com/matplotlib/matplotlib/issues/10167
    x = x.ravel()
    y = y.ravel()
    z = z.ravel()
    condition = numpy.isfinite(z)
    x = x[condition]
    y = y[condition]
    z = z[condition]
    return axes.tricontour(x, y, z, *args, **kwargs)


def tricontourf(axes, workspace, *args, **kwargs):
    '''
    Essentially the same as :meth:`mantid.plots.contourf`, but works
    for non-uniform grids. Currently this only works with workspaces
    that have a constant number of bins between spectra or with
    MDHistoWorkspaces.

    :param axes:      :class:`matplotlib.axes.Axes` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the matrix workspace is a histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param indices: Specify which slice of an MDHistoWorkspace to use when plotting. Needs to be a tuple
                    and will be interpreted as a list of indices. You need to use ``slice(None)`` to
                    select which dimensions to plot. *e.g.* to select the last two axes to plot from a
                    3D volume use ``indices=(5, slice(None), slice(None))`` where the 5 is the bin selected
                    for the first axis.
    :param slicepoint: Specify which slice of an MDHistoWorkspace to use when plotting in the dimension units.
                       You need to use ``None`` to select which dimension to plot. *e.g.* to select the last
                       two axes to plot from a 3D volume use ``slicepoint=(1.0, None, None)`` where the 1.0 is
                       the value of the dimension selected for the first axis.
    :param transpose: ``bool`` to transpose the x and y axes of the plotted dimensions of an MDHistoWorkspace

    See :meth:`matplotlib.axes.Axes.tricontourf` for more information.
    '''
    transpose = kwargs.pop('transpose', False)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x_temp, y_temp, z = get_md_data2d_bin_centers(workspace, normalization, indices, transpose)
        x, y = numpy.meshgrid(x_temp, y_temp)
        _setLabels2D(axes, workspace, indices, transpose)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=transpose)
        _setLabels2D(axes, workspace, transpose=transpose)
    # tricontourf segfaults if many z values are not finite
    # https://github.com/matplotlib/matplotlib/issues/10167
    x = x.ravel()
    y = y.ravel()
    z = z.ravel()
    condition = numpy.isfinite(z)
    x = x[condition]
    y = y[condition]
    z = z[condition]
    return axes.tricontourf(x, y, z, *args, **kwargs)


def update_colorplot_datalimits(axes, mappables):
    """
    For an colorplot (imshow, pcolor*) plots update the data limits on the axes
    to circumvent bugs in matplotlib
    :param mappables: An iterable of mappable for this axes
    """
    # ax.relim in matplotlib < 2.2 doesn't take into account of images
    # and it doesn't support collections at all as of verison 3 so we'll take
    # over
    if not isinstance(mappables, collections.Iterable):
        mappables = [mappables]
    xmin_all, xmax_all, ymin_all, ymax_all = _LARGEST, _SMALLEST, _LARGEST, _SMALLEST
    for mappable in mappables:
        xmin, xmax, ymin, ymax = get_colorplot_extents(mappable)
        xmin_all, xmax_all = min(xmin_all, xmin), max(xmax_all, xmax)
        ymin_all, ymax_all = min(ymin_all, ymin), max(ymax_all, ymax)
    axes.update_datalim(((xmin_all, ymin_all), (xmax_all, ymax_all)))
    axes.autoscale()


def get_colorplot_extents(mappable):
    """
    Return the extent of the given mappable
    :param mappable: A 2D mappable object
    :return: (left, right, bottom, top)
    """
    if isinstance(mappable, mimage.AxesImage):
        xmin, xmax, ymin, ymax = mappable.get_extent()
    elif isinstance(mappable, mcoll.QuadMesh):
        # coordinates are vertices of the grid
        coords = mappable._coordinates
        xmin, ymin = coords[0][0]
        xmax, ymax = coords[-1][-1]
    elif isinstance(mappable, mcoll.PolyCollection):
        xmin, ymin = mappable._paths[0].get_extents().min
        xmax, ymax = mappable._paths[-1].get_extents().max
    else:
        raise ValueError("Unknown mappable type '{}'".format(type(mappable)))

    return xmin, xmax, ymin, ymax
