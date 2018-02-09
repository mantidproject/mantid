#  This file is part of the mantid package
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
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, division, print_function)

import numpy
import mantid.kernel
import mantid.api
from mantid.plots.helperfunctions import *
import matplotlib.colors

# ================================================
# Private 2D Helper functions
# ================================================


def _getMatrix2DData(workspace, distribution, histogram2D=False):
    '''
    Get all data from a Matrix workspace that has the same number of bins
    in every spectrum. It is used for 2D plots
    :param workspace: Matrix workspace to extract the data from
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width
    :param histogram2D: flag that specifies if the coordinates in the output are
        -bin centers (such as for contour) for False, or
        -bin edges (such as for pcolor) for True.
    Returns x,y,z 2D arrays
    '''
    try:
        _ = workspace.blocksize()
    except RuntimeError:
        raise ValueError('The spectra are not the same length. Try using pcolor, pcolorfast, or pcolormesh instead')
    x = workspace.extractX()
    y = workspace.getAxis(1).extractValues()
    z = workspace.extractY()

    if workspace.isHistogramData():
        if not distribution:
            z = z / (x[:, 1:] - x[:, 0:-1])
        if histogram2D:
            if len(y) == z.shape[0]:
                y = boundaries_from_points(y)
            x = numpy.vstack((x, x[-1]))
        else:
            x = .5*(x[:, 0:-1]+x[:, 1:])
            if len(y) == z.shape[0]+1:
                y = points_from_boundaries(y)
    else:
        if histogram2D:
            if _commonX(x):
                x = numpy.tile(boundaries_from_points(x[0]), z.shape[0]+1).reshape(z.shape[0]+1, -1)
            else:
                x = numpy.vstack((x, x[-1]))
                x = numpy.array([boundaries_from_points(xi) for xi in x])
            if len(y) == z.shape[0]:
                y = boundaries_from_points(y)
        else:
            if len(y) == z.shape[0]+1:
                y = points_from_boundaries(y)
    y = numpy.tile(y, x.shape[1]).reshape(x.shape[1], x.shape[0]).transpose()
    z = numpy.ma.masked_invalid(z)
    return x, y, z


def _getDataUnevenFlag(workspace, **kwargs):
    '''
    Helper function that allows :meth:`matplotlib.axes.Axes.pcolor`,
    :meth:`matplotlib.axes.Axes.pcolorfast`, and :meth:`matplotlib.axes.Axes.pcolormesh`
    to plot rectangles parallel to the axes even if the data is not
    on a regular grid.
    :param workspace: a workspace2d
    if axisaligned keyword is available and True or if the workspace does
    not have a constant number of bins, it will return true, otherwise false
    '''
    aligned = kwargs.pop('axisaligned', False)
    try:
        _ = workspace.blocksize()
    except RuntimeError:
        aligned = True
    return aligned, kwargs


def _getUnevenData(workspace, distribution):
    '''
    Function to get data for uneven workspace2Ds, such as
    that pcolor, pcolorfast, and pcolormesh will plot axis aligned rectangles
    :param workspace: a workspace2d
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width
    Returns three lists. Each element in the x list is an array of boundaries
    for a spectra. Each element in the y list is a 2 element array with the extents
    of a particular spectra. The z list contains arrays of intensities at bin centers
    '''
    z = []
    x = []
    y = []
    nhist = workspace.getNumberHistograms()
    yvals = workspace.getAxis(1).extractValues()
    if len(yvals) == nhist:
        yvals = boundaries_from_points(yvals)
    for index in range(nhist):
        xvals = workspace.readX(index)
        zvals = workspace.readY(index)
        if workspace.isHistogramData():
            if not distribution:
                zvals = zvals / (xvals[1:] - xvals[0:-1])
        else:
            xvals = boundaries_from_points(xvals)
        zvals = numpy.ma.masked_invalid(zvals)
        z.append(zvals)
        x.append(xvals)
        y.append([yvals[index], yvals[index+1]])
    return x, y, z


def _setLabels1D(axes, workspace):
    '''
    helper function to automatically set axes labels for 1D plots
    '''
    labels = get_axes_labels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[0])


def _setLabels2D(axes, workspace):
    '''
    helper function to automatically set axes labels for 2D plots
    '''
    labels = get_axes_labels(workspace)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[2])


# ========================================================
# Plot functions
# ========================================================

def plot(axes, workspace, *args, **kwargs):
    '''
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

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x, y, _) = _getMDData1D(workspace, normalization)
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, _, _) = _getSpectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False)
    _setLabels1D(axes, workspace)
    return axes.plot(x, y, *args, **kwargs)


def errorbar(axes, workspace, *args, **kwargs):
    '''
    Unpack mantid workspace and render it with matplotlib. ``args`` and
    ``kwargs`` are passed to :py:meth:`matplotlib.axes.Axes.errorbar` after special
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

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x, y, dy) = _getMDData1D(workspace, normalization)
        dx = None
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, dy, dx) = _getSpectrum(workspace, wkspIndex, distribution, withDy=True, withDx=True)
    _setLabels1D(axes, workspace)
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

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x, y, _) = _getMDData1D(workspace, normalization)
    else:
        (wkspIndex, distribution, kwargs) = _getWkspIndexDistAndLabel(workspace, **kwargs)
        (x, y, _, _) = _getSpectrum(workspace, wkspIndex, distribution)
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
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_centers(workspace, normalization)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=False)
    _setLabels2D(axes, workspace)
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
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_centers(workspace, normalization)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=False)
    _setLabels2D(axes, workspace)
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
    Note: the return is the pcolor, pcolormesh, or pcolorfast of the last spectrum
    '''
    (x, y, z) = _getUnevenData(workspace, distribution)
    pcolortype = kwargs.pop('pcolortype', '')
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
    for xi, yi, zi in zip(x, y, z):
        XX, YY = numpy.meshgrid(xi, yi, indexing='ij')
        if 'mesh' in pcolortype.lower():
            cm = axes.pcolormesh(XX, YY, zi.reshape(-1, 1), **kwargs)
        elif 'fast' in pcolortype.lower():
            cm = axes.pcolorfast(XX, YY, zi.reshape(-1, 1), **kwargs)
        else:
            cm = axes.pcolor(XX, YY, zi.reshape(-1, 1), **kwargs)
    return cm


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
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    '''
    _setLabels2D(axes, workspace)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization)
    else:
        (aligned, kwargs) = _getDataUnevenFlag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = ''
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=True)
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
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    '''
    _setLabels2D(axes, workspace)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization)
    else:
        (aligned, kwargs) = _getDataUnevenFlag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'fast'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=True)
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
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    '''
    _setLabels2D(axes, workspace)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization)
    else:
        (aligned, kwargs) = _getDataUnevenFlag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'mesh'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=True)

    return axes.pcolormesh(x, y, z, *args, **kwargs)


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
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization

    See :meth:`matplotlib.axes.Axes.tripcolor` for more information.
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x_temp, y_temp, z = get_md_data2d_bin_centers(workspace, normalization)
        x, y = numpy.meshgrid(x_temp, y_temp)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=False)
    _setLabels2D(axes, workspace)

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
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization

    See :meth:`matplotlib.axes.Axes.tricontour` for more information.
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x_temp, y_temp, z) = get_md_data2d_bin_centers(workspace, normalization)
        (x, y) = numpy.meshgrid(x_temp, y_temp)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=False)
    _setLabels2D(axes, workspace)
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

    See :meth:`matplotlib.axes.Axes.tricontourf` for more information.
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x_temp, y_temp, z) = get_md_data2d_bin_centers(workspace, normalization)
        (x, y) = numpy.meshgrid(x_temp, y_temp)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = _getMatrix2DData(workspace, distribution, histogram2D=False)
    _setLabels2D(axes, workspace)
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