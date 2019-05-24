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

import datetime

import numpy

import mantid.api
import mantid.kernel
from mantid.api import MultipleExperimentInfos
from mantid.dataobjects import EventWorkspace, MDHistoWorkspace, Workspace2D
from mantid.plots.utility import MantidAxType
from scipy.interpolate import interp1d

# Helper functions for data extraction from a Mantid workspace and plot functionality
# These functions are common between plotfunctions.py and plotfunctions3D.py

# ====================================================
# Validation
# ====================================================


def validate_args(*args, **kwargs):
    return len(args) > 0 and (isinstance(args[0], EventWorkspace) or
                              isinstance(args[0], Workspace2D) or
                              isinstance(args[0], MDHistoWorkspace) or
                              isinstance(args[0], MultipleExperimentInfos) and "LogName" in kwargs)


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


def get_indices(md_workspace, **kwargs):
    """
    Gets the indices of an MDHistoWorkspace to select the plane to plot.

    Set the legend to provide the selected axes values

    :param md_workspace: :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    """
    if 'slicepoint' in kwargs and 'indices' in kwargs:
        raise ValueError("Must specify either 'slicepoint' or 'indices', not both")

    if 'slicepoint' in kwargs:
        slicepoint = kwargs.pop('slicepoint')
        assert md_workspace.getNumDims() == len(slicepoint), "slicepoint provided do not match the dimensions of the workspace"
        indices = []
        for n, p in enumerate(slicepoint):
            if p is None:
                indices.append(slice(None))
            else:
                indices.append(pointToIndex(md_workspace.getDimension(n), p))
        indices = tuple(indices)
    elif 'indices' in kwargs:
        indices = kwargs.pop('indices')
        assert md_workspace.getNumDims() == len(indices), "indices provided do not match the dimensions of the workspace"
    else:
        indices = None

    if indices and 'label' not in kwargs:
        ws_name = md_workspace.name()
        labels = '; '.join('{0}={1:.4}'.format(md_workspace.getDimension(n).name,
                                               (md_workspace.getDimension(n).getX(indices[n]) +
                                                md_workspace.getDimension(n).getX(indices[n]+1))/2)
                           for n in range(md_workspace.getNumDims()) if indices[n] != slice(None))
        if ws_name:
            kwargs['label'] = '{0}: {1}'.format(ws_name, labels)
        else:
            kwargs['label'] = labels

    return indices, kwargs


def pointToIndex(dim, point):
    """
    Finds the bin index of which the point falls into.
    """
    i = (point-dim.getX(0))/dim.getBinWidth()
    return int(min(max(i, 0), dim.getNBins()-1))


def points_from_boundaries(input_array):
    """
    The function returns bin centers from bin boundaries

    :param input_array: a :class:`numpy.ndarray` of bin boundaries
    """
    assert isinstance(input_array, numpy.ndarray), 'Not a numpy array'
    if len(input_array) < 2:
        raise ValueError('could not get centers from less than two boundaries')
    return .5 * (input_array[0:-1] + input_array[1:])


def _dim2array(d):
    """
    Create a numpy array containing bin centers along the dimension d

    :param d: an :class:`mantid.geometry.IMDDimension` object

    returns: bin boundaries for dimension d
    """
    dmin = d.getMinimum()
    dmax = d.getMaximum()
    return numpy.linspace(dmin, dmax, d.getNBins() + 1)


def get_wksp_index_dist_and_label(workspace, axis=MantidAxType.SPECTRUM, **kwargs):
    """
    Get workspace index, whether the workspace is a distribution,
    and label for the spectrum

    :param workspace: a Workspace2D or an EventWorkspace
    :param axis: The axis on which we're operating
    :param kwargs: Keyword arguments passed to the plot function, passed by reference as it is mutated
    """
    # get the special arguments out of kwargs
    workspace_index, spectrum_number, kwargs = _get_wksp_index_and_spec_num(workspace, axis, **kwargs)

    # create a label if it isn't already specified
    if 'label' not in kwargs:
        ws_name = workspace.name()
        if axis == MantidAxType.SPECTRUM:
            if ws_name:
                kwargs['label'] = '{0}: spec {1}'.format(ws_name, spectrum_number)
            else:
                kwargs['label'] = 'spec {0}'.format(spectrum_number)
        elif axis == MantidAxType.BIN:
            if ws_name:
                kwargs['label'] = '{0}: bin {1}'.format(ws_name, workspace_index)
            else:
                kwargs['label'] = 'bin {0}'.format(workspace_index)

    (distribution, kwargs) = get_distribution(workspace, **kwargs)

    return workspace_index, distribution, kwargs


def _get_wksp_index_and_spec_num(workspace, axis, **kwargs):
    """
    Get the workspace index and the spectrum number from the kwargs provided
    :param workspace: a Workspace2D or an EventWorkspace
    :param axis: The axis on which the workspace is being traversed,
                 can be either MantidAxType.BIN or MantidAxType.SPECTRUM,
                 default is MantidAxType.SPECTRUM
    :param kwargs: Dict of keyword arguments that should contain either spectrum number
                   or workspace index
    :return The workspace index and the spectrum number
    """
    spectrum_number = kwargs.pop('specNum', None)
    workspace_index = kwargs.pop('wkspIndex', None)

    # don't worry if there is only one spectrum
    if workspace.getNumberHistograms() == 1:
        spectrum_number = None
        workspace_index = 0

    # error check input parameters
    if (spectrum_number is not None) and (workspace_index is not None):
        raise RuntimeError('Must specify only specNum or wkspIndex')
    if (spectrum_number is None) and (workspace_index is None):
        raise RuntimeError('Must specify either specNum or wkspIndex')

    # convert the spectrum number to a workspace index and vice versa
    if spectrum_number is not None:
        workspace_index = workspace.getIndexFromSpectrumNumber(int(spectrum_number))
    elif axis == MantidAxType.SPECTRUM:  # Only get a spectrum number if we're traversing the spectra
        spectrum_number = workspace.getSpectrum(workspace_index).getSpectrumNo()

    return workspace_index, spectrum_number, kwargs


def get_md_data1d(workspace, normalization, indices=None):
    """
    Function to transform data in an MDHisto workspace with exactly
    one non-integrated dimension into arrays of bin centers, data,
    and error, to be used in 1D plots (plot, scatter, errorbar)
    """
    coordinate, data, err = get_md_data(workspace, normalization, indices, withError=True)
    assert len(coordinate) == 1, 'The workspace is not 1D'
    coordinate = points_from_boundaries(coordinate[0])
    return coordinate, data, err


def get_md_data(workspace, normalization, indices=None, withError=False):
    """
    Generic function to extract data from an MDHisto workspace

    :param workspace: :class:`mantid.api.IMDHistoWorkspace` containing data
    :param normalization: if :class:`mantid.api.MDNormalization.NumEventsNormalization`
        it will divide intensity by the number of corresponding MDEvents
    :param indices: slice indices to select data
    :param withError: flag for if the error is calculated. If False, err is returned as None

    returns a tuple containing bin boundaries for each dimension, the (maybe normalized)
    signal and error arrays
    """
    if indices is None:
        dims = workspace.getNonIntegratedDimensions()
        indices = Ellipsis
    else:
        dims =  [workspace.getDimension(n) for n in range(workspace.getNumDims()) if indices[n] == slice(None)]
    dim_arrays = [_dim2array(d) for d in dims]
    # get data
    data = workspace.getSignalArray()[indices].copy()
    if normalization == mantid.api.MDNormalization.NumEventsNormalization:
        nev = workspace.getNumEventsArray()[indices]
        data /= nev
    err = None
    if withError:
        err2 = workspace.getErrorSquaredArray()[indices].copy()
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


def get_bins(workspace, wkspIndex, withDy=False):
    """
    Extract all the bins for a spectrum

    :param workspace: a Workspace2D or an EventWorkspace
    :param wkspIndex: workspace index
    :param withDy: if True, it will return the error in the "counts", otherwise None

    """
    num_hist = workspace.getNumberHistograms()
    x = range(0, num_hist)
    y = []
    dy = [] if withDy else None
    for i in x:
        y.append(workspace.readY(i)[wkspIndex])
        if withDy:
            dy.append(workspace.readE(i)[wkspIndex])

    dx = None
    return x, y, dy, dx


def get_md_data2d_bin_bounds(workspace, normalization, indices=None, transpose=False):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin boundaries in each
    dimension, and data. To be used in 2D plots (pcolor, pcolorfast, pcolormesh)

    Note: return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    coordinate, data, _ = get_md_data(workspace, normalization, indices, withError=False)
    assert len(coordinate) == 2, 'The workspace is not 2D'
    if transpose:
        return coordinate[1], coordinate[0], data.T
    else:
        return coordinate[0], coordinate[1], data


def get_md_data2d_bin_centers(workspace, normalization, indices=None, transpose=False):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin centers in each
    dimension, and data. To be used in 2D plots (contour, contourf,
    tricontour, tricontourf, tripcolor)

    Note: return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    x, y, data = get_md_data2d_bin_bounds(workspace, normalization, indices, transpose)
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
        return numpy.array([input_array[0] - 0.5, input_array[0] + 0.5])
    return numpy.concatenate(([(3 * input_array[0] - input_array[1]) * 0.5],
                              (input_array[1:] + input_array[:-1]) * 0.5,
                              [(3 * input_array[-1] - input_array[-2]) * 0.5]))


def common_x(arr):
    """
    Helper function to check if all rows in a 2d :class:`numpy.ndarray` are identical
    """
    return numpy.all(arr == arr[0, :], axis=(1, 0))


def get_matrix_2d_ragged(workspace, distribution, histogram2D=False, transpose=False):
    num_hist = workspace.getNumberHistograms()
    delta = numpy.finfo(numpy.float64).max
    min_value = numpy.finfo(numpy.float64).max
    max_value = numpy.finfo(numpy.float64).min
    for i in range(num_hist):
        xtmp = workspace.readX(i)
        if workspace.isHistogramData():
            #input x is edges
            xtmp = mantid.plots.helperfunctions.points_from_boundaries(xtmp)
        else:
            #input x is centers
            pass
        min_value = min(min_value, xtmp.min())
        max_value = max(max_value, xtmp.max())
        diff = xtmp[1:] - xtmp[:-1]
        delta = min(delta, diff.min())
    num_edges = int(numpy.ceil((max_value - min_value)/delta)) + 1
    x_centers = numpy.linspace(min_value, max_value, num=num_edges)
    y = mantid.plots.helperfunctions.boundaries_from_points(workspace.getAxis(1).extractValues())
    z = numpy.empty([num_hist, num_edges], dtype=numpy.float64)
    for i in range(num_hist):
        centers, ztmp, _, _ = mantid.plots.helperfunctions.get_spectrum(workspace, i, distribution=distribution, withDy=False, withDx=False)
        f = interp1d(centers, ztmp, kind='nearest', bounds_error=False, fill_value=numpy.nan)
        z[i] = f(x_centers)
    if histogram2D:
        x = mantid.plots.helperfunctions.boundaries_from_points(x_centers)
    else:
        x = x_centers
    if transpose:
        return y.T,x.T,z.T
    else:
        return x,y,z


def get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=False):
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
        workspace.blocksize()
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
            x = .5 * (x[:, 0:-1] + x[:, 1:])
            if len(y) == z.shape[0] + 1:
                y = points_from_boundaries(y)
    else:
        if histogram2D:
            if common_x(x):
                x = numpy.tile(boundaries_from_points(x[0]), z.shape[0] + 1).reshape(z.shape[0] + 1, -1)
            else:
                x = numpy.vstack((x, x[-1]))
                x = numpy.array([boundaries_from_points(xi) for xi in x])
            if len(y) == z.shape[0]:
                y = boundaries_from_points(y)
        else:
            if len(y) == z.shape[0] + 1:
                y = points_from_boundaries(y)
    y = numpy.tile(y, x.shape[1]).reshape(x.shape[1], x.shape[0]).transpose()
    z = numpy.ma.masked_invalid(z)
    if transpose:
        return y.T,x.T,z.T
    else:
        return x,y,z


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
        y.append([yvals[index], yvals[index + 1]])
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
        workspace.blocksize()
    except RuntimeError:
        aligned = True
    return aligned, kwargs


def check_resample_to_regular_grid(ws):
    if not ws.isCommonBins():
        return True

    x = ws.dataX(0)
    difference = numpy.diff(x)
    if not numpy.all(numpy.isclose(difference[:-1], difference[0])):
        return True

    return False


# ====================================================
# extract logs
# ====================================================

def get_sample_log(workspace, **kwargs):
    LogName = kwargs.pop('LogName')
    ExperimentInfo = kwargs.pop('ExperimentInfo', 0)
    if isinstance(workspace, MultipleExperimentInfos):
        run = workspace.getExperimentInfo(ExperimentInfo).run()
    else:
        run = workspace.run()
    if not run.hasProperty(LogName):
        raise ValueError('The workspace does not contain the {} sample log'.format(LogName))
    tsp = run[LogName]
    units = tsp.units
    if not isinstance(tsp, (mantid.kernel.FloatTimeSeriesProperty,
                            mantid.kernel.Int32TimeSeriesProperty,
                            mantid.kernel.Int64TimeSeriesProperty)):
        raise RuntimeError('This function can only plot Float or Int TimeSeriesProperties objects')
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
                pass  # TODO: Maybe raise a warning?
        x = (times - t0).astype(float) * 1e-6
    return x, y, FullTime, LogName, units, kwargs


# ====================================================
# Plot functionality
# ====================================================


def get_axes_labels(workspace, indices=None, plot_as_dist=True, use_latex=True):
    """
    Get axis labels from a Workspace2D or an MDHistoWorkspace
    Returns a tuple. The first element is the quantity label, such as "Intensity" or "Counts".
    All other elements in the tuple are labels for axes.
    Some of them are latex formatted already.

    If MDWorkspace then the last element will be the values selected by the indices, to be set as title.

    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
    :param indices:
    :param plot_as_dist: bool: plotting as distribution
    :param use_latex: bool: return y-unit label in Latex form
    """
    if isinstance(workspace, MultipleExperimentInfos):
        axes_labels = ['Intensity']
        title = ''
        if indices is None:
            dims = workspace.getNonIntegratedDimensions()
        else:
            dims = []
            for n in range(workspace.getNumDims()):
                d = workspace.getDimension(n)
                if indices[n] == slice(None):
                    dims.append(d)
                else:
                    title += '{0}={1:.4}; '.format(d.name,
                                                   (d.getX(indices[n])+d.getX(indices[n]+1))/2)
        for d in dims:
            axis_title = d.name.replace('DeltaE', r'$\Delta E$')
            axis_unit = d.getUnits().replace('Angstrom^-1', r'$\AA^{-1}$')
            axis_unit = axis_unit.replace('DeltaE', 'meV')
            axis_unit = axis_unit.replace('Angstrom', r'$\AA$')
            axis_unit = axis_unit.replace('MomentumTransfer', r'$\AA^{-1}$')
            axes_labels.append('{0} ({1})'.format(axis_title, axis_unit))
        axes_labels.append(title.strip())
    else:
        # For matrix workspaces, return a tuple of ``(YUnit, <other units>)``
        axes_labels = [workspace.YUnitLabel(useLatex=use_latex,
                                            plotAsDistribution=plot_as_dist)]
        for index in range(workspace.axes()):
            axis = workspace.getAxis(index)
            unit = axis.getUnit()
            if len(str(unit.symbol())) > 0:
                unit = '{} (${}$)'.format(unit.caption(), unit.symbol().latex())
            else:
                unit = unit.caption()
            axes_labels.append(unit)
    return tuple(axes_labels)
