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
from skimage.transform import resize

import mantid.api
import mantid.kernel
from mantid.plots.helperfunctions import get_axes_labels, get_bins, get_data_uneven_flag, get_distribution, \
    get_matrix_2d_data, get_md_data1d, get_md_data2d_bin_bounds, get_md_data2d_bin_centers, get_normalization, \
    get_sample_log, get_spectrum, get_uneven_data, get_wksp_index_dist_and_label

# Used for initializing searches of max, min values
_LARGEST, _SMALLEST = float(sys.maxsize), -sys.maxsize

# ================================================
# Private 2D Helper functions
# ================================================


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


def _get_data_for_plot(axes, workspace, kwargs, with_dy=False, with_dx=False):
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x, y, dy) = get_md_data1d(workspace, normalization)
        dx = None
    else:
        axis = kwargs.pop("axis", MantidAxType.SPECTRUM)
        workspace_index, distribution, kwargs = get_wksp_index_dist_and_label(workspace, axis, **kwargs)
        if axis == MantidAxType.BIN:
            # Overwrite any user specified xlabel
            axes.set_xlabel("Spectrum")
            x, y, dy, dx = get_bins(workspace, workspace_index, with_dy)
        elif axis == MantidAxType.SPECTRUM:
            x, y, dy, dx = get_spectrum(workspace, workspace_index, distribution, with_dy, with_dx)
        else:
            raise ValueError("Axis {} is not a valid axis number.".format(axis))
    return x, y, dy, dx, kwargs


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
        x, y, _, _, kwargs = _get_data_for_plot(axes, workspace, kwargs)
        _setLabels1D(axes, workspace)
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
    :param distribution: ``None`` (default) asks the workspace. ``False`` means
                         divide by bin width. ``True`` means do not divide by bin width.
                         Applies only when the the workspace is a MatrixWorkspace histogram.
    :param normalization: ``None`` (default) ask the workspace. Applies to MDHisto workspaces. It can override
                          the value from displayNormalizationHisto. It checks only if
                          the normalization is mantid.api.MDNormalization.NumEventsNormalization
    :param axis: Specify which axis will be plotted. Use axis=MantidAxType.BIN to plot a bin,
                  and axis=MantidAxType.SPECTRUM to plot a spectrum.
                  The default value is axis=1, plotting spectra by default.

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    """
    x, y, dy, dx, kwargs = _get_data_for_plot(axes, workspace, kwargs,
                                              with_dy=True, with_dx=False)
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
        (x, y, _) = get_md_data1d(workspace, normalization)
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
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_centers(workspace, normalization)
    else:
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False)
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
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False)

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
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    '''
    _setLabels2D(axes, workspace)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization)
    else:
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = ''
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True)
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
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'fast'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True)
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
        (aligned, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if aligned:
            kwargs['pcolortype'] = 'mesh'
            return _pcolorpieces(axes, workspace, distribution, *args, **kwargs)
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True)
    return axes.pcolormesh(x, y, z, *args, **kwargs)


def _skimage_version():
    import skimage
    from distutils.version import LooseVersion
    return LooseVersion(skimage.__version__) >= LooseVersion('1.4.0')


class ScalingAxesImage(mimage.AxesImage):
    def __init__(self, ax,
                 cmap=None,
                 norm=None,
                 interpolation=None,
                 origin=None,
                 extent=None,
                 filternorm=1,
                 filterrad=4.0,
                 resample=False,
                 **kwargs):
        self.dx = None
        self.dy = None
        self.unsampled_data = None
        super(ScalingAxesImage, self).__init__(
            ax,
            cmap=cmap,
            norm=norm,
            interpolation=interpolation,
            origin=origin,
            extent=extent,
            filternorm=filternorm,
            filterrad=filterrad,
            resample=resample,
            **kwargs)

    def set_data(self, A):
        dims = A.shape
        max_dims = (3840, 2160)  # 4K resolution
        if dims[0] > max_dims[0] or dims[1] > max_dims[1]:
            new_dims = numpy.minimum(dims, max_dims)
            if (_skimage_version()):
                self.unsampled_data = resize(A, new_dims, mode='constant', cval=numpy.nan, anti_aliasing=True)
            else:
                self.unsampled_data = resize(A, new_dims, mode='constant', cval=numpy.nan)
        else:
            self.unsampled_data = A
        super(ScalingAxesImage, self).set_data(A)

    def draw(self, renderer):
        ax = self.axes
        # might not be calculated before first call
        we = ax.get_window_extent()
        dx = round(we.x1 - we.x0)
        dy = round(we.y1 - we.y0)
        # decide if we should downsample
        dims = self.unsampled_data.shape
        if dx != self.dx or dy != self.dy:
            if dims[0] > dx or dims[1] > dy:
                new_dims = numpy.minimum(dims, [dx, dy])
                if (_skimage_version()):
                    sampled_data = resize(self.unsampled_data, new_dims, mode='constant', cval=numpy.nan,
                                          anti_aliasing=True)
                else:
                    sampled_data = resize(self.unsampled_data, new_dims, mode='constant', cval=numpy.nan)
                self.dx = dx
                self.dy = dy
                super(ScalingAxesImage, self).set_data(sampled_data)
        return super(ScalingAxesImage,self).draw(renderer)


def _imshow(axes, z, cmap=None, norm=None, aspect=None,
            interpolation=None, alpha=None, vmin=None, vmax=None,
            origin=None, extent=None, shape=None, filternorm=1,
            filterrad=4.0, imlim=None, resample=None, url=None, **kwargs):
    """
    Copy of imshow in order to replace AxesImage artist with a custom artist.

    Use :meth:`matplotlib.axes.Axes.imshow` documentation for individual arguments.
    """
    if norm is not None and not isinstance(norm, mcolors.Normalize):
        raise ValueError(
            "'norm' must be an instance of 'mcolors.Normalize'")
    if aspect is None:
        aspect = matplotlib.rcParams['image.aspect']
    axes.set_aspect(aspect)
    im = ScalingAxesImage(axes, cmap, norm, interpolation, origin, extent,
                          filternorm=filternorm, filterrad=filterrad,
                          resample=resample, **kwargs)
    im.set_data(z)
    im.set_alpha(alpha)
    if im.get_clip_path() is None:
        # image does not already have clipping set, clip to axes patch
        im.set_clip_path(axes.patch)
    if vmin is not None or vmax is not None:
        im.set_clim(vmin, vmax)
    else:
        im.autoscale_None()
    im.set_url(url)

    # update ax.dataLim, and, if autoscaling, set viewLim
    # to tightly fit the image, regardless of dataLim.
    im.set_extent(im.get_extent())

    axes.add_image(im)
    return im


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
    :param axisaligned: ``False`` (default). If ``True``, or if the workspace has a variable
                        number of bins, the polygons will be aligned with the axes
    '''
    _setLabels2D(axes, workspace)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        x, y, z = get_md_data2d_bin_bounds(workspace, normalization)
    else:
        (uneven_bins, kwargs) = get_data_uneven_flag(workspace, **kwargs)
        (distribution, kwargs) = get_distribution(workspace, **kwargs)
        if uneven_bins:
            raise Exception('Variable number of bins is not supported by imshow.')
        else:
            (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=True)

    diffs = numpy.diff(x, axis=1)
    x_spacing_equal = numpy.alltrue(diffs == diffs[0])
    diffs = numpy.diff(y, axis=0)
    y_spacing_equal = numpy.alltrue(diffs == diffs[0])
    if not x_spacing_equal or not y_spacing_equal:
        raise Exception('Unevenly spaced bins are not supported by imshow')
    if 'extent' not in kwargs:
        kwargs['extent'] = [x[0, 0], x[0, -1], y[0, 0], y[-1, 0]]
    return _imshow(axes, z, *args, **kwargs)


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
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False)
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
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False)
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
        (x, y, z) = get_matrix_2d_data(workspace, distribution, histogram2D=False)
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
