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

from mantid.plots.helperfunctions import (get_axes_labels, get_normalization, get_distribution,
                                          get_md_data2d_bin_centers, get_matrix_2d_data, get_md_data1d,
                                          get_wksp_index_dist_and_label, get_spectrum, get_indices)
import mantid.dataobjects


def _set_labels_3d(axes, workspace, indices=None):
    """
    Helper function to automatically set axis labels for 3D plots
    """
    labels = get_axes_labels(workspace, indices)
    axes.set_xlabel(labels[1])
    axes.set_ylabel(labels[2])
    axes.set_zlabel(labels[0])


def _extract_3d_data(workspace, **kwargs):
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        normalization, kwargs = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        x_temp, y_temp, z = get_md_data2d_bin_centers(workspace, normalization, indices)
        x, y = numpy.meshgrid(x_temp, y_temp)
    else:
        distribution, kwargs = get_distribution(workspace, **kwargs)
        x, y, z = get_matrix_2d_data(workspace, distribution, histogram2D=False)
        indices = None
    return x, y, z, indices


def plot(axes, workspace, *args, **kwargs):
    '''
    3D plots - line plots

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or
                      :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    :param zdir: Which direction to use as z ('x', 'y' or 'z') when plotting a 2D set.
    '''
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
        (normalization, kwargs) = get_normalization(workspace, **kwargs)
        indices, kwargs = get_indices(workspace, **kwargs)
        (x, y, z) = get_md_data1d(workspace, normalization, indices)
    else:
        (wksp_index, distribution, kwargs) = get_wksp_index_dist_and_label(workspace, **kwargs)
        (x, z, _, _) = get_spectrum(workspace, wksp_index, distribution, withDy=False, withDx=False)
        y_val = workspace.getAxis(1).extractValues()[wksp_index]
        y = [y_val for _ in range(len(x))]  # fill x size array with y value
        _set_labels_3d(axes, workspace)
    return axes.plot(x, y, z, *args, **kwargs)


def scatter(axes, workspace, *args, **kwargs):
    '''
    Scatter plots

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
                      to extract the data from
    :param zdir:	Which direction to use as z ('x', 'y' or 'z') when plotting a 2D set.
    :param s:	Size in points^2. It is a scalar or an array of the same length as x and y.
    :param c:	A color. c can be a single color format string, or a sequence of color
                specifications of length N, or a sequence of N numbers to be mapped to
                colors using the cmap and norm specified via kwargs (see below). Note
                that c should not be a single numeric RGB or RGBA sequence because that
                is indistinguishable from an array of values to be colormapped.
                c can be a 2-D array in which the rows are RGB or RGBA, however, including
                the case of a single row to specify the same color for all points.
    :param depthshade:	Whether or not to shade the scatter markers to give the appearance
                        of depth. Default is True.
    '''
    x, y, z, indices = _extract_3d_data(workspace, **kwargs)
    _set_labels_3d(axes, workspace, indices)
    return axes.scatter(x, y, z, *args, **kwargs)


def plot_wireframe(axes, workspace, *args, **kwargs):
    '''
    Wire-frame plot

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or
                      :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    :param rstride: Array row stride (step size), defaults to 1
    :param cstride: Array column stride (step size), defaults to 1
    :param rcount:	Use at most this many rows, defaults to 50
    :param ccount:	Use at most this many columns, defaults to 50
    '''
    x, y, z, indices = _extract_3d_data(workspace, **kwargs)
    _set_labels_3d(axes, workspace, indices)
    return axes.plot_wireframe(x, y, z, *args, **kwargs)


def plot_surface(axes, workspace, *args, **kwargs):
    '''
    Surface plots

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or
                      :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    :param rstride: Array row stride (step size), defaults to 1
    :param cstride: Array column stride (step size), defaults to 1
    :param rcount:	Use at most this many rows, defaults to 50
    :param ccount:	Use at most this many columns, defaults to 50
    :param color:	Color of the surface patches
    :param cmap:	A colormap for the surface patches.
    :param norm:	An instance of Normalize to map values to colors
    :param vmin:	Minimum value to map
    :param vmax:	Maximum value to map
    :param shade:	Whether to shade the facecolors
    :param facecolors:	Face colors for the individual patches
    '''
    x, y, z, indices = _extract_3d_data(workspace, **kwargs)
    _set_labels_3d(axes, workspace, indices)
    return axes.plot_surface(x, y, z, *args, **kwargs)


def contour(axes, workspace, *args, **kwargs):
    '''
    Contour plots

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or
                      :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    :param extend3d:	Whether to extend contour in 3D (default: False)
    :param stride:	Stride (step size) for extending contour
    :param zdir:	The direction to use: x, y or z (default)
    :param offset:	If specified plot a projection of the contour lines
                    on this position in plane normal to zdir
    '''
    x, y, z, indices = _extract_3d_data(workspace, **kwargs)
    _set_labels_3d(axes, workspace, indices)
    return axes.contour(x, y, z, *args, **kwargs)


def contourf(axes, workspace, *args, **kwargs):
    '''
    Filled Contour plots

    :param axes: class:`matplotlib.axes.Axes3D` object that will do the plotting
    :param workspace: :class:`mantid.api.MatrixWorkspace` or
                      :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    :param zdir:	The direction to use: x, y or z (default)
    :param offset:	If specified plot a projection of the filled contour on this
                    position in plane normal to zdir
    '''
    x, y, z, indices = _extract_3d_data(workspace, **kwargs)
    _set_labels_3d(axes, workspace, indices)
    return axes.contourf(x, y, z, *args, **kwargs)
