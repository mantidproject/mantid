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

import numpy
import mantid.kernel
import mantid.api
from mantid.plots.helperfunctions import *
import matplotlib
import matplotlib.colors
import matplotlib.dates as mdates
import matplotlib.image as mimage

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

    For matrix workspaces with more than one spectra, either ``specNum`` or ``wkspIndex``
    needs to be specified. Giving both will generate a :class:`RuntimeError`. There is no similar
    keyword for MDHistoWorkspaces. These type of workspaces have to have exactly one non integrated
    dimension
    '''
    if 'LogName' in kwargs:
        (x, y, FullTime, LogName, units, kwargs) = get_sample_log(workspace, **kwargs)
        axes.set_ylabel('{0} ({1})'.format(LogName, units))
        axes.set_xlabel('Time (s)')
        if FullTime:
            axes.xaxis_date()
            axes.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S\n%b-%d'))
            axes.set_xlabel('Time')
        kwargs['linestyle']='steps-post'
        return axes.plot(x, y, *args, **kwargs)
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        (x, y, dy) = get_md_data1d(workspace, normalization)
    else:
        (wkspIndex, distribution, kwargs) = get_wksp_index_dist_and_label(workspace, **kwargs)
        (x, y, dy, dx) = get_spectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False)
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
        (x, y, dy) = get_md_data1d(workspace, normalization)
        dx = None
    else:
        (wkspIndex, distribution, kwargs) = get_wksp_index_dist_and_label(workspace, **kwargs)
        (x, y, dy, dx) = get_spectrum(workspace, wkspIndex, distribution, withDy=True, withDx=True)
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
    Note: the return is the pcolor, pcolormesh, or pcolorfast of the last spectrum
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

    for xi, yi, zi in zip(x, y, z):
        XX, YY = numpy.meshgrid(xi, yi, indexing='ij')
        cm = pcolor(XX, YY, zi.reshape(-1, 1), **kwargs)

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

def _imshow(axes, X, cmap=None, norm=None, aspect=None,
           interpolation=None, alpha=None, vmin=None, vmax=None,
           origin=None, extent=None, shape=None, filternorm=1,
           filterrad=4.0, imlim=None, resample=None, url=None, **kwargs):
    """
    Essentially the same as :meth:`matplotlib.axes.Axes.imshow`.
    
    Copied here in order to replace AxesImage artist with a custom artist.

    Display an image, i.e. data on a 2D regular raster.
    Parameters
    ----------
    X : array-like or PIL image
        The image data. Supported array shapes are:
        - (M, N): an image with scalar data. The data is visualized
          using a colormap.
        - (M, N, 3): an image with RGB values (float or uint8).
        - (M, N, 4): an image with RGBA values (float or uint8), i.e.
          including transparency.
        The first two dimensions (M, N) define the rows and columns of
        the image.
        The RGB(A) values should be in the range [0 .. 1] for floats or
        [0 .. 255] for integers.  Out-of-range values will be clipped to
        these bounds.
    cmap : str or `~matplotlib.colors.Colormap`, optional
        A Colormap instance or registered colormap name. The colormap
        maps scalar data to colors. It is ignored for RGB(A) data.
        Defaults to :rc:`image.cmap`.
    aspect : {'equal', 'auto'} or float, optional
        Controls the aspect ratio of the axes. The aspect is of particular
        relevance for images since it may distort the image, i.e. pixel
        will not be square.
        This parameter is a shortcut for explicitly calling
        `.Axes.set_aspect`. See there for further details.
        - 'equal': Ensures an aspect ratio of 1. Pixels will be square
          (unless pixel sizes are explicitly made non-square in data
          coordinates using *extent*).
        - 'auto': The axes is kept fixed and the aspect is adjusted so
          that the data fit in the axes. In general, this will result in
          non-square pixels.
        If not given, use :rc:`image.aspect` (default: 'equal').
    interpolation : str, optional
        The interpolation method used. If *None*
        :rc:`image.interpolation` is used, which defaults to 'nearest'.
        Supported values are 'none', 'nearest', 'bilinear', 'bicubic',
        'spline16', 'spline36', 'hanning', 'hamming', 'hermite', 'kaiser',
        'quadric', 'catrom', 'gaussian', 'bessel', 'mitchell', 'sinc',
        'lanczos'.
        If *interpolation* is 'none', then no interpolation is performed
        on the Agg, ps, pdf and svg backends. Other backends will fall back
        to 'nearest'. Note that most SVG renders perform interpolation at
        rendering and that the default interpolation method they implement
        may differ.
        See
        :doc:`/gallery/images_contours_and_fields/interpolation_methods`
        for an overview of the supported interpolation methods.
        Some interpolation methods require an additional radius parameter,
        which can be set by *filterrad*. Additionally, the antigrain image
        resize filter is controlled by the parameter *filternorm*.
    norm : `~matplotlib.colors.Normalize`, optional
        If scalar data are used, the Normalize instance scales the
        data values to the canonical colormap range [0,1] for mapping
        to colors. By default, the data range is mapped to the
        colorbar range using linear scaling. This parameter is ignored for
        RGB(A) data.
    vmin, vmax : scalar, optional
        When using scalar data and no explicit *norm*, *vmin* and *vmax*
        define the data range that the colormap covers. By default,
        the colormap covers the complete value range of the supplied
        data. *vmin*, *vmax* are ignored if the *norm* parameter is used.
    alpha : scalar, optional
        The alpha blending value, between 0 (transparent) and 1 (opaque).
        This parameter is ignored for RGBA input data.
    origin : {'upper', 'lower'}, optional
        Place the [0,0] index of the array in the upper left or lower left
        corner of the axes. The convention 'upper' is typically used for
        matrices and images.
        If not given, :rc:`image.origin` is used, defaulting to 'upper'.
        Note that the vertical axes points upward for 'lower'
        but downward for 'upper'.
    extent : scalars (left, right, bottom, top), optional
        The bounding box in data coordinates that the image will fill.
        The image is stretched individually along x and y to fill the box.
        The default extent is determined by the following conditions.
        Pixels have unit size in data coordinates. Their centers are on
        integer coordinates, and their center coordinates range from 0 to
        columns-1 horizontally and from 0 to rows-1 vertically.
        Note that the direction of the vertical axis and thus the default
        values for top and bottom depend on *origin*:
        - For ``origin == 'upper'`` the default is
          ``(-0.5, numcols-0.5, numrows-0.5, -0.5)``.
        - For ``origin == 'lower'`` the default is
          ``(-0.5, numcols-0.5, -0.5, numrows-0.5)``.
        See the example :doc:`/tutorials/intermediate/imshow_extent` for a
        more detailed description.
    shape : scalars (columns, rows), optional, default: None
        For raw buffer images.
    filternorm : bool, optional, default: True
        A parameter for the antigrain image resize filter (see the
        antigrain documentation).  If *filternorm* is set, the filter
        normalizes integer values and corrects the rounding errors. It
        doesn't do anything with the source floating point values, it
        corrects only integers according to the rule of 1.0 which means
        that any sum of pixel weights must be equal to 1.0.  So, the
        filter function must produce a graph of the proper shape.
    filterrad : float > 0, optional, default: 4.0
        The filter radius for filters that have a radius parameter, i.e.
        when interpolation is one of: 'sinc', 'lanczos' or 'blackman'.
    resample : bool, optional
        When *True*, use a full resampling method.  When *False*, only
        resample when the output image is larger than the input image.
    url : str, optional
        Set the url of the created `.AxesImage`. See `.Artist.set_url`.
    Returns
    -------
    image : `~matplotlib.image.AxesImage`
    Other Parameters
    ----------------
    **kwargs : `~matplotlib.artist.Artist` properties
        These parameters are passed on to the constructor of the
        `.AxesImage` artist.
    See also
    --------
    matshow : Plot a matrix or an array as an image.
    Notes
    -----
    Unless *extent* is used, pixel centers will be located at integer
    coordinates. In other words: the origin will coincide with the center
    of pixel (0, 0).
    There are two common representations for RGB images with an alpha
    channel:
    -   Straight (unassociated) alpha: R, G, and B channels represent the
        color of the pixel, disregarding its opacity.
    -   Premultiplied (associated) alpha: R, G, and B channels represent
        the color of the pixel, adjusted for its opacity by multiplication.
    `~matplotlib.pyplot.imshow` expects RGB images adopting the straight
    (unassociated) alpha representation.
    """
    if norm is not None and not isinstance(norm, mcolors.Normalize):
        raise ValueError(
            "'norm' must be an instance of 'mcolors.Normalize'")
    if aspect is None:
        aspect = matplotlib.rcParams['image.aspect']
    axes.set_aspect(aspect)
    im = mimage.AxesImage(axes, cmap, norm, interpolation, origin, extent,
                          filternorm=filternorm, filterrad=filterrad,
                          resample=resample, **kwargs)

    im.set_data(X)
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
    kwargs['extent'] = [x[0,0],x[0,-1],y[0,0],y[-1,0]]
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
