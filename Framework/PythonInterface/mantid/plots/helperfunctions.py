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
import datetime
from mantid.dataobjects import EventWorkspace, Workspace2D, MDHistoWorkspace
import mantid.kernel, mantid.api


# Helper functions for data extraction from a Mantid workspace and plot functionality
# These functions are common between plotfunctions.py and plotfunctions3D.py

# ====================================================
# Validation
# ====================================================
def validate_args(*args):
    return len(args) > 0 and (isinstance(args[0], EventWorkspace) or
                              isinstance(args[0], Workspace2D) or
                              isinstance(args[0], MDHistoWorkspace))


# ====================================================
# Data extraction and manipulation
# ====================================================

def get_distribution(workspace, **kwargs):
    """
    Determine whether or not the data is a distribution. The value in
    the kwargs wins. Applies to Matrix workspaces only

    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    """
    distribution = kwargs.pop('distribution', workspace.isDistribution())
    return bool(distribution), kwargs


def get_normalization(md_workspace, **kwargs):
    """
    Gets the normalization flag of an MDHistoWorkspace. For workspaces
    derived similar to MSlice/Horace, one needs to average data, the so-called
    "number of events" normalization.

    :param md_workspace: :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    """
    normalization = kwargs.pop('normalization', md_workspace.displayNormalizationHisto())
    return normalization, kwargs


def points_from_boundaries(input_array):
    """
    The function returns bin centers from bin boundaries

    :param input_array: a :class:`numpy.ndarray` of bin boundaries
    """
    assert isinstance(input_array, numpy.ndarray), 'Not a numpy array'
    if len(input_array) < 2:
        raise ValueError('could not get centers from less than two boundaries')
    return .5*(input_array[0:-1]+input_array[1:])


def _dim2array(d):
    """
    Create a numpy array containing bin centers along the dimension d

    :param d: an :class:`mantid.geometry.IMDDimension` object

    returns: bin boundaries for dimension d
    """
    dmin = d.getMinimum()
    dmax = d.getMaximum()
    return numpy.linspace(dmin, dmax, d.getNBins()+1)


def get_wksp_index_dist_and_label(workspace, **kwargs):
    """
    Get workspace index, whether the workspace is a distribution,
    and label for the spectrum

    :param workspace: a Workspace2D or an EventWorkspace
    """
    # get the special arguments out of kwargs
    specNum = kwargs.pop('specNum', None)
    wkspIndex = kwargs.pop('wkspIndex', None)

    # don't worry if there is only one spectrum
    if workspace.getNumberHistograms() == 1:
        specNum = None
        wkspIndex = 0

    # error check input parameters
    if (specNum is not None) and (wkspIndex is not None):
        raise RuntimeError('Must specify only specNum or wkspIndex')
    if (specNum is None) and (wkspIndex is None):
        raise RuntimeError('Must specify either specNum or wkspIndex')

    # convert the spectrum number to a workspace index and vice versa
    if specNum is not None:
        wkspIndex = workspace.getIndexFromSpectrumNumber(int(specNum))
    else:
        specNum = workspace.getSpectrum(wkspIndex).getSpectrumNo()

    # create a label if it isn't already specified
    if 'label' not in kwargs:
        wsName = workspace.name()
        if wsName:
            kwargs['label'] = '{0}: spec {1}'.format(wsName, specNum)
        else:
            kwargs['label'] = 'spec {0}'.format(specNum)

    (distribution, kwargs) = get_distribution(workspace, **kwargs)

    return wkspIndex, distribution, kwargs


def get_md_data1d(workspace, normalization):
    """
    Function to transform data in an MDHisto workspace with exactly
    one non-integrated dimension into arrays of bin centers, data,
    and error, to be used in 1D plots (plot, scatter, errorbar)
    """
    coordinate, data, err = get_md_data(workspace, normalization, withError=True)
    assert len(coordinate) == 1, 'The workspace is not 1D'
    coordinate = points_from_boundaries(coordinate[0])
    return coordinate, data, err


def get_md_data(workspace, normalization, withError=False):
    """
    Generic function to extract data from an MDHisto workspace

    :param workspace: :class:`mantid.api.IMDHistoWorkspace` containing data
    :param normalization: if :class:`mantid.api.MDNormalization.NumEventsNormalization`
    it will divide intensity by the number of corresponding MDEvents
    :param withError: flag for if the error is calculated. If False, err is returned as None

    returns a tuple containing bin boundaries for each dimension, the (maybe normalized)
    signal and error arrays
    """
    dims = workspace.getNonIntegratedDimensions()
    dim_arrays = [_dim2array(d) for d in dims]
    # get data
    data = workspace.getSignalArray()*1.
    if normalization == mantid.api.MDNormalization.NumEventsNormalization:
        nev = workspace.getNumEventsArray()
        data /= nev
    err = None
    if withError:
        err2 = workspace.getErrorSquaredArray()*1.
        if normalization == mantid.api.MDNormalization.NumEventsNormalization:
            err2 /= (nev * nev)
        err = numpy.sqrt(err2)
    data = data.squeeze().T
    data = numpy.ma.masked_invalid(data)
    if err is not None:
        err = err.squeeze().T
        err = numpy.ma.masked_invalid(err)
    return dim_arrays, data, err


def get_spectrum(workspace, wkspIndex, distribution, withDy=False, withDx=False):
    """
    Extract a single spectrum and process the data into a frequency

    :param workspace: a Workspace2D or an EventWorkspace
    :param wkspIndex: workspace index
    :param distribution: flag to divide the data by bin width. It happens only
        when this flag is False, the workspace contains histogram data, and
        the mantid configuration is set up to divide such workspaces by bin
        width. The same effect can be obtained by running the
        :ref:`algm-ConvertToDistribution` algorithm

    :param withDy: if True, it will return the error in the "counts", otherwise None
    :param with Dx: if True, and workspace has them, it will return errors
        in the x coordinate, otherwise None

    Note that for workspaces containing bin boundaries, this function will return
    the bin centers for x.
    To be used in 1D plots (plot, scatter, errorbar)
    """
    x = workspace.readX(wkspIndex)
    y = workspace.readY(wkspIndex)
    dy = None
    dx = None

    if withDy:
        dy = workspace.readE(wkspIndex)
    if withDx and workspace.getSpectrum(wkspIndex).hasDx():
        dx = workspace.readDx(wkspIndex)

    if workspace.isHistogramData():
        if (not distribution) and (mantid.kernel.config['graph1d.autodistribution'] == 'On'):
            y = y / (x[1:] - x[0:-1])
            if dy is not None:
                dy = dy / (x[1:] - x[0:-1])
        x = points_from_boundaries(x)
    y = numpy.ma.masked_invalid(y)
    if dy is not None:
        dy = numpy.ma.masked_invalid(dy)
    return x, y, dy, dx


def get_md_data2d_bin_bounds(workspace, normalization):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin boundaries in each
    dimension, and data. To be used in 2D plots (pcolor, pcolorfast, pcolormesh)

    Note: return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    coordinate, data, _ = get_md_data(workspace, normalization, withError=False)
    assert len(coordinate) == 2, 'The workspace is not 2D'
    return coordinate[0], coordinate[1], data


def get_md_data2d_bin_centers(workspace, normalization):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin centers in each
    dimension, and data. To be used in 2D plots (contour, contourf,
    tricontour, tricontourf, tripcolor)

    Note: return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    x, y, data = get_md_data2d_bin_bounds(workspace, normalization)
    x = points_from_boundaries(x)
    y = points_from_boundaries(y)
    return x, y, data


def boundaries_from_points(input_array):
    """"
    The function tries to guess bin boundaries from bin centers

    :param input_array: a :class:`numpy.ndarray` of bin centers
    """
    assert isinstance(input_array, numpy.ndarray), 'Not a numpy array'
    if len(input_array) == 0:
        raise ValueError('could not extend array with no elements')
    if len(input_array) == 1:
        return numpy.array([input_array[0]-0.5, input_array[0]+0.5])
    return numpy.concatenate(([(3*input_array[0]-input_array[1])*0.5],
                             (input_array[1:]+input_array[:-1])*0.5,
                             [(3*input_array[-1]-input_array[-2])*0.5]))


def common_x(arr):
    """
    Helper function to check if all rows in a 2d :class:`numpy.ndarray` are identical
    """
    return numpy.all(arr == arr[0, :], axis=(1, 0))


def get_matrix_2d_data(workspace, distribution, histogram2D=False):
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
            z /= x[:, 1:] - x[:, 0:-1]
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
            if common_x(x):
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


def get_uneven_data(workspace, distribution):
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


def get_data_uneven_flag(workspace, **kwargs):
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

# ====================================================
# extract logs
# ====================================================

def get_sample_log(workspace, **kwargs):
    LogName = kwargs.pop('LogName')
    ExperimentInfo = kwargs.pop('ExperimentInfo',0)
    if isinstance(workspace, MDHistoWorkspace):
        run = workspace.getExperimentInfo(ExperimentInfo).run()
    else:
        run = workspace.run()
    if not run.hasProperty(LogName):
        raise ValueError('The workspace does not contain the {} sample log'.format(LogName))
    tsp = run[LogName]
    units = tsp.units
    if not isinstance(tsp, mantid.kernel.FloatTimeSeriesProperty):
        raise RuntimeError('This function can only plot FloatTimeSeriesProperty objects')
    times = tsp.times.astype('datetime64[us]')
    y = tsp.value
    FullTime = kwargs.pop('FullTime', False)
    StartFromLog = kwargs.pop('StartFromLog', False)
    if FullTime:
        x = times.astype(datetime.datetime)
    else:
        t0 = times[0]
        if not StartFromLog:
            try:
                t0 = run['proton_charge'].times.astype('datetime64[us]')[0]
            except:
                pass #TODO: Maybe raise a warning?
        x = (times - t0).astype(float) * 1e-6
    return x, y, FullTime, LogName, units, kwargs


# ====================================================
# Plot functionality
# ====================================================


def get_axes_labels(workspace):
    """
    Get axis labels from a Workspace2D or an MDHistoWorkspace
    Returns a tuple. The first element is the quantity label, such as "Intensity" or "Counts".
    All other elements in the tuple are labels for axes.
    Some of them are latex formatted already.

    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
    """
    if isinstance(workspace, MDHistoWorkspace):
        axes = ['Intensity']
        dims = workspace.getNonIntegratedDimensions()
        for d in dims:
            axis_title = d.name.replace('DeltaE', '$\Delta E$')
            axis_unit = d.getUnits().replace('Angstrom^-1', '$\AA^{-1}$')
            axis_unit = axis_unit.replace('DeltaE', 'meV')
            axis_unit = axis_unit.replace('Angstrom', '$\AA$')
            axis_unit = axis_unit.replace('MomentumTransfer', '$\AA^{-1}$')
            axes.append('{0} ({1})'.format(axis_title, axis_unit))
    else:
        '''For matrix workspaces, return a tuple of ``(YUnit, <other units>)``'''
        axes = [workspace.YUnit()]  # TODO: deal with distribution
        for index in range(workspace.axes()):
            axis = workspace.getAxis(index)
            unit = axis.getUnit()
            if len(str(unit.symbol())) > 0:
                unit = '{} (${}$)'.format(unit.caption(), unit.symbol().latex())
            else:
                unit = unit.caption()
            axes.append(unit)
    return tuple(axes)
