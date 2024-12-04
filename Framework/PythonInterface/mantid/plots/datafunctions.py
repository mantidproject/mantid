# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
#
#
import datetime
from itertools import tee

import numpy as np
from matplotlib.collections import PolyCollection, QuadMesh
from matplotlib.container import ErrorbarContainer
from matplotlib.colors import LogNorm
from matplotlib.ticker import LogLocator
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from scipy.interpolate import interp1d

import mantid.api
import mantid.kernel
from mantid.api import MultipleExperimentInfos, MatrixWorkspace
from mantid.dataobjects import EventWorkspace, MDHistoWorkspace, Workspace2D
from mantid.plots.utility import convert_color_to_hex, MantidAxType


# Helper functions for data extraction from a Mantid workspace and plot functionality
# These functions are common between axesfunctions.py and axesfunctions3D.py

# ====================================================
# Validation
# ====================================================


def validate_args(*args, **kwargs):
    return len(args) > 0 and (
        isinstance(args[0], EventWorkspace)
        or isinstance(args[0], Workspace2D)
        or isinstance(args[0], MDHistoWorkspace)
        or isinstance(args[0], MultipleExperimentInfos)
        and "LogName" in kwargs
    )


# ====================================================
# Data extraction and manipulation
# ====================================================


def get_distribution(workspace, **kwargs):
    """
    Determine whether or not the data is a distribution.
    If the workspace is a distribution return true,
    else the value in kwargs wins.
    Applies to Matrix workspaces only
    :param workspace: :class:`mantid.api.MatrixWorkspace` to extract the data from
    """
    distribution = kwargs.pop("distribution", False)
    distribution = True if workspace.isDistribution() else bool(distribution)
    return distribution, kwargs


def get_normalize_by_bin_width(workspace, axes, **kwargs):
    """
    Determine whether or not the workspace should be plotted as a
    distribution. If the workspace is a distribution return False, else,
    if there is already a curves on the axes, return according to
    whether those curves are distributions. Else go by the global
    setting.
    :param workspace: :class:`mantid.api.MatrixWorkspace` workspace being plotted
    :param axes: The axes being plotted on
    """
    normalize_by_bin_width = kwargs.pop("normalize_by_bin_width", None)
    if normalize_by_bin_width is not None:
        return normalize_by_bin_width, kwargs
    distribution = kwargs.get("distribution", None)
    axis = kwargs.get("axis", None)
    if axis == MantidAxType.BIN or distribution or (hasattr(workspace, "isDistribution") and workspace.isDistribution()):
        return False, kwargs
    elif distribution is False:
        return True, kwargs
    else:
        try:
            current_artists = axes.tracked_workspaces.values()
        except AttributeError:
            current_artists = None

        if current_artists:
            current_normalization = any([artist[0].is_normalized for artist in current_artists])
            normalization = current_normalization
        else:
            normalization = mantid.kernel.config["graph1d.autodistribution"].lower() == "on"
    return normalization, kwargs


def get_spectrum_normalisation(**kwargs):
    """
    Get the spectrum normalisation flag from the plot keyword arguments.

    :param kwargs: plot kwargs
    :return the normalisation flag and the new kwargs
    """
    norm = False
    if "normalise_spectrum" in kwargs:
        norm = kwargs.pop("normalise_spectrum")
    return norm, kwargs


def get_normalization(md_workspace, **kwargs):
    """
    Gets the normalization flag of an MDHistoWorkspace. For workspaces
    derived similar to MSlice/Horace, one needs to average data, the so-called
    "number of events" normalization.

    :param md_workspace: :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    """
    normalization = kwargs.pop("normalization", md_workspace.displayNormalizationHisto())
    return normalization, kwargs


def get_indices(md_workspace, **kwargs):
    """
    Gets the indices of an MDHistoWorkspace to select the plane to plot.

    Set the legend to provide the selected axes values

    :param md_workspace: :class:`mantid.api.IMDHistoWorkspace` to extract the data from
    """
    if "slicepoint" in kwargs and "indices" in kwargs:
        raise ValueError("Must specify either 'slicepoint' or 'indices', not both")

    if "slicepoint" in kwargs:
        slicepoint = kwargs.pop("slicepoint")
        assert md_workspace.getNumDims() == len(slicepoint), "slicepoint provided do not match the dimensions of the workspace"
        indices = []
        for n, p in enumerate(slicepoint):
            if p is None:
                indices.append(slice(None))
            else:
                indices.append(pointToIndex(md_workspace.getDimension(n), p))
        indices = tuple(indices)
    elif "indices" in kwargs:
        indices = kwargs.pop("indices")
        assert md_workspace.getNumDims() == len(indices), "indices provided do not match the dimensions of the workspace"
    else:
        indices = None

    if indices and "label" not in kwargs:
        ws_name = md_workspace.name()
        labels = "; ".join(
            "{0}={1:.4}".format(
                md_workspace.getDimension(n).name,
                (md_workspace.getDimension(n).getX(indices[n]) + md_workspace.getDimension(n).getX(indices[n] + 1)) / 2,
            )
            for n in range(md_workspace.getNumDims())
            if indices[n] != slice(None)
        )
        if ws_name:
            kwargs["label"] = "{0}: {1}".format(ws_name, labels)
        else:
            kwargs["label"] = labels

    return indices, kwargs


def pointToIndex(dim, point):
    """
    Finds the bin index of which the point falls into.
    """
    i = (point - dim.getX(0)) / dim.getBinWidth()
    return int(min(max(i, 0), dim.getNBins() - 1))


def points_from_boundaries(input_array):
    """
    The function returns bin centers from bin boundaries

    :param input_array: a :class:`numpy.ndarray` of bin boundaries
    """
    assert isinstance(input_array, np.ndarray), "Not a numpy array"
    if len(input_array) < 2:
        raise ValueError("could not get centers from less than two boundaries")
    return 0.5 * (input_array[0:-1] + input_array[1:])


def _dim2array(d):
    """
    Create a numpy array containing bin centers along the dimension d

    :param d: an :class:`mantid.geometry.IMDDimension` object

    returns: bin boundaries for dimension d
    """
    dmin = d.getMinimum()
    dmax = d.getMaximum()
    return np.linspace(dmin, dmax, d.getNBins() + 1)


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
    if "label" not in kwargs:
        ws_name = workspace.name()
        if axis == MantidAxType.SPECTRUM:
            if workspace.getAxis(1).isText() or workspace.getAxis(1).isNumeric():
                kwargs["label"] = "{0}: {1}".format(ws_name, workspace.getAxis(1).label(workspace_index))
            else:
                if ws_name:
                    kwargs["label"] = "{0}: spec {1}".format(ws_name, spectrum_number)
                else:
                    kwargs["label"] = "spec {0}".format(spectrum_number)
        elif axis == MantidAxType.BIN:
            if ws_name:
                kwargs["label"] = "{0}: bin {1}".format(ws_name, workspace_index)
            else:
                kwargs["label"] = "bin {0}".format(workspace_index)

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
    spectrum_number = kwargs.pop("specNum", None)
    workspace_index = kwargs.pop("wkspIndex", None)

    # error check input parameters
    if (spectrum_number is not None) and (workspace_index is not None):
        raise RuntimeError("Must specify only specNum or wkspIndex")
    if (spectrum_number is None) and (workspace_index is None):
        raise RuntimeError("Must specify either specNum or wkspIndex")

    # convert the spectrum number to a workspace index and vice versa
    if spectrum_number is not None:
        try:
            workspace_index = workspace.getIndexFromSpectrumNumber(int(spectrum_number))
        except RuntimeError:
            raise RuntimeError("Spectrum Number {0} not found in workspace {1}".format(spectrum_number, workspace.name()))
    elif axis == MantidAxType.SPECTRUM:  # Only get a spectrum number if we're traversing the spectra
        try:
            spectrum_number = workspace.getSpectrum(workspace_index).getSpectrumNo()
        except RuntimeError:
            raise RuntimeError("Workspace index {0} not found in workspace {1}".format(workspace_index, workspace.name()))

    return workspace_index, spectrum_number, kwargs


def get_md_data1d(workspace, normalization, indices=None):
    """
    Function to transform data in an MDHisto workspace with exactly
    one non-integrated dimension into arrays of bin centers, data,
    and error, to be used in 1D plots (plot, scatter, errorbar)
    """
    coordinate, data, err = get_md_data(workspace, normalization, indices, withError=True)
    assert len(coordinate) == 1, "The workspace is not 1D"
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
        dims = [workspace.getDimension(n) for n in range(workspace.getNumDims()) if indices[n] == slice(None)]
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
            err2 /= nev * nev
        err = np.sqrt(err2)
    data = data.squeeze().T
    data = np.ma.masked_invalid(data)
    if err is not None:
        err = err.squeeze().T
        err = np.ma.masked_invalid(err)
    return dim_arrays, data, err


def get_spectrum(workspace, wkspIndex, normalize_by_bin_width, withDy=False, withDx=False):
    """
    Extract a single spectrum and process the data into a frequency

    :param workspace: a Workspace2D or an EventWorkspace
    :param wkspIndex: workspace index
    :param normalize_by_bin_width: flag to divide the data by bin width. The same
        effect can be obtained by running the :ref:`algm-ConvertToDistribution`
        algorithm
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
        if normalize_by_bin_width and not workspace.isDistribution():
            y = y / (x[1:] - x[0:-1])
            if dy is not None:
                dy = dy / (x[1:] - x[0:-1])
        x = points_from_boundaries(x)
    try:
        specInfo = workspace.spectrumInfo()
        if specInfo.isMasked(wkspIndex):
            y[:] = np.nan
    except:
        pass
    y = np.ma.masked_invalid(y)
    if dy is not None:
        dy = np.ma.masked_invalid(dy)
    return x, y, dy, dx


def get_bin_indices(workspace):
    """
    Find the bins' indices, without those of the monitors if there are some.
    (ie every detector which is not a monitor)

    :param workspace: a Workspace2D or an EventWorkspace
    :return : the bins' indices as a range if possible, else as a numpy array
    """
    total_range = workspace.getNumberHistograms()
    try:
        spectrum_info = workspace.spectrumInfo()
    except:
        return range(total_range)

    monitors_indices = [index for index in range(total_range) if spectrum_info.hasDetectors(index) and spectrum_info.isMonitor(index)]
    monitor_count = len(monitors_indices)

    # If possible, ie the detectors' indices are continuous, we return a range.
    # If not, we return a numpy array
    range_start = -1
    range_end = total_range
    is_range = True
    for index, monitor_index in enumerate(monitors_indices):
        if index == monitor_index:
            range_start = monitor_index
        else:
            if monitor_count - index == total_range - monitor_index and monitors_indices[-1] == total_range - 1:
                range_end = monitor_index
            else:
                is_range = False
            break

    if is_range:
        return range(range_start + 1, range_end)
    else:
        # the following two lines can be replaced by np.isin when > version 1.7.0 is used on RHEL7
        total_range = np.asarray(range(total_range))
        indices = np.where(np.isin(total_range, monitors_indices, invert=True).reshape(total_range.shape))
        # this check is necessary as numpy may return a tuple or a plain array based on platform.
        indices = indices[0] if isinstance(indices, tuple) else indices
        return indices


def get_bins(workspace, bin_index, withDy=False):
    """
    Extract a requested bin from each spectrum, except if they correspond to monitors

    :param workspace: a Workspace2D or an EventWorkspace
    :param bin_index: the index of a bin
    :param withDy: if True, it will return the error in the "counts", otherwise None

    """
    indices = get_bin_indices(workspace)
    x_values, y_values = [], []
    dy = [] if withDy else None
    for row_index in indices:
        y_data = workspace.readY(int(row_index))
        if bin_index < len(y_data):
            x_values.append(row_index)
            y_values.append(y_data[bin_index])
            if withDy:
                dy.append(workspace.readE(int(row_index))[bin_index])
    dx = None
    return x_values, y_values, dy, dx


def get_md_data2d_bin_bounds(workspace, normalization, indices=None, transpose=False):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin boundaries in each
    dimension, and data. To be used in 2D plots (pcolor, pcolorfast, pcolormesh)

    Note: return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    coordinate, data, _ = get_md_data(workspace, normalization, indices, withError=False)
    assert len(coordinate) == 2, "The workspace is not 2D"
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
    """
    The function tries to guess bin boundaries from bin centers

    :param input_array: a :class:`numpy.ndarray` of bin centers
    """
    assert isinstance(input_array, np.ndarray), "Not a numpy array"
    if len(input_array) == 0:
        raise ValueError("could not extend array with no elements")
    if len(input_array) == 1:
        return np.array([input_array[0] - 0.5, input_array[0] + 0.5])
    return np.concatenate(
        (
            [(3 * input_array[0] - input_array[1]) * 0.5],
            (input_array[1:] + input_array[:-1]) * 0.5,
            [(3 * input_array[-1] - input_array[-2]) * 0.5],
        )
    )


def common_x(arr):
    """
    Helper function to check if all rows in a 2d :class:`numpy.ndarray` are identical
    """
    return np.all(arr == arr[0, :], axis=(1, 0))


def get_matrix_2d_ragged(
    workspace,
    normalize_by_bin_width,
    histogram2D=False,
    transpose=False,
    extent=None,
    xbins=100,
    ybins=100,
    spec_info=None,
    maxpooling=False,
):
    if spec_info is None:
        try:
            spec_info = workspace.spectrumInfo()
        except:
            spec_info = None

    if extent is None:
        common_bins = workspace.isCommonBins()
        delta = np.finfo(np.float64).max
        min_value = np.finfo(np.float64).max
        max_value = np.finfo(np.float64).min

        for spectrum_index in range(workspace.getNumberHistograms()):
            if not (spec_info and spec_info.hasDetectors(spectrum_index) and spec_info.isMonitor(spectrum_index)):
                xtmp = workspace.readX(spectrum_index)
                if workspace.isHistogramData():
                    # input x is edges
                    xtmp = mantid.plots.datafunctions.points_from_boundaries(xtmp)
                else:
                    # input x is centers
                    pass
                min_value = min(min_value, xtmp.min())
                max_value = max(max_value, xtmp.max())
                diff = np.diff(xtmp)
                delta = min(delta, diff.min())
                if common_bins:
                    break
                xtmp = workspace.readX(0)
        if delta == np.finfo(np.float64).max:
            delta = np.diff(xtmp).min()
        if min_value == np.finfo(np.float64).max:
            min_value = xtmp.min()
        if max_value == np.finfo(np.float64).min:
            max_value = xtmp.max()
        num_edges = int(np.ceil((max_value - min_value) / delta)) + 1
        x_centers = np.linspace(min_value, max_value, num=num_edges)
        y = mantid.plots.datafunctions.boundaries_from_points(workspace.getAxis(1).extractValues())
    else:
        x_low, x_high, y_low, y_high = extent[0], extent[1], extent[2], extent[3]
        if transpose:
            x_low, x_high, y_low, y_high = extent[2], extent[3], extent[0], extent[1]

        x_edges = np.linspace(x_low, x_high, int(xbins + 1))
        x_centers = mantid.plots.datafunctions.points_from_boundaries(x_edges)
        y = np.linspace(y_low, y_high, int(ybins))

    counts = interpolate_y_data(workspace, x_centers, y, normalize_by_bin_width, spectrum_info=spec_info, maxpooling=maxpooling)

    if histogram2D and extent is not None:
        x = x_edges
    elif histogram2D:
        x = mantid.plots.datafunctions.boundaries_from_points(x_centers)
    else:
        x = x_centers

    if transpose:
        return y.T, x.T, counts.T
    else:
        return x, y, counts


def pairwise(iterable):
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)


def _workspace_indices(y_bins, workspace):
    workspace_indices = []
    for y in y_bins:
        try:
            workspace_index = workspace.getAxis(1).indexOfValue(y)
            workspace_indices.append(workspace_index)
        except IndexError:
            workspace_indices.append(-1)
    return workspace_indices


def _workspace_indices_maxpooling(y_bins, workspace):
    summed_spectra_workspace = _integrate_workspace(workspace)
    summed_spectra = summed_spectra_workspace.extractY()
    workspace_indices = []
    for y_range in pairwise(y_bins):
        try:
            workspace_range = [workspace.getAxis(1).indexOfValue(y_range[0]), workspace.getAxis(1).indexOfValue(y_range[1])]
            # if the range doesn't span more than one spectra just grab the first element
            # else we need to pick the spectra which has the highest intensity
            if np.diff(workspace_range)[0] > 1:
                workspace_range = range(workspace_range[0], workspace_range[1])
                workspace_index = workspace_range[np.argmax(summed_spectra[workspace_range])]
            else:
                workspace_index = workspace_range[0]
            workspace_indices.append(workspace_index)
        except IndexError:
            workspace_indices.append(-1)
    return workspace_indices


def _integrate_workspace(workspace):
    from mantid.api import AlgorithmManager

    integration = AlgorithmManager.createUnmanaged("Integration")
    integration.initialize()
    integration.setAlwaysStoreInADS(False)
    integration.setLogging(False)
    integration.setChild(True)
    integration.setProperty("InputWorkspace", workspace)
    integration.setProperty("OutputWorkspace", "__dummy")
    integration.execute()
    return integration.getProperty("OutputWorkspace").value


def interpolate_y_data(workspace, x, y, normalize_by_bin_width, spectrum_info=None, maxpooling=False):
    workspace_indices = _workspace_indices_maxpooling(y, workspace) if maxpooling else _workspace_indices(y, workspace)
    counts = np.full([len(workspace_indices), x.size], np.nan, dtype=np.float64)
    previous_index = -1
    index = -1
    for workspace_index in workspace_indices:
        index += 1
        # if workspace axis is beyond limits carry on
        if workspace_index == -1:
            continue
        # avoid repeating calculations
        if previous_index == workspace_index:
            counts[index, :] = counts[index - 1]
            continue
        previous_index = workspace_index
        if not (spectrum_info and spectrum_info.hasDetectors(workspace_index) and spectrum_info.isMonitor(workspace_index)):
            centers, ztmp, _, _ = get_spectrum(
                workspace, workspace_index, normalize_by_bin_width=normalize_by_bin_width, withDy=False, withDx=False
            )
            interpolation_function = interp1d(centers, ztmp, kind="nearest", bounds_error=False, fill_value="extrapolate")
            # only set values in the range of workspace
            x_range = np.where((x >= workspace.readX(workspace_index)[0]) & (x <= workspace.readX(workspace_index)[-1]))
            # set values outside x data to nan
            counts[index, x_range] = interpolation_function(x[x_range])
    counts = np.ma.masked_invalid(counts, copy=False)
    return counts


def get_matrix_2d_data(workspace, distribution, histogram2D=False, transpose=False):
    """
    Get all data from a Matrix workspace that has the same number of bins
    in every spectrum. It is used for 2D plots

    :param workspace: Matrix workspace to extract the data from
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width

    :param histogram2D: flag that specifies if the coordinates in the output are
        -bin centers (such as for contour) for False, or
        -bin edges (such as for pcolor) for True.

    Returns x,y,z 2D arrays
    """
    try:
        workspace.blocksize()
    except RuntimeError:
        raise ValueError("The spectra are not the same length. Try using pcolor, pcolorfast, or pcolormesh instead")
    x = workspace.extractX()
    if workspace.getAxis(1).isText():
        nhist = workspace.getNumberHistograms()
        y = np.arange(nhist)
    else:
        y = workspace.getAxis(1).extractValues()
    z = workspace.extractY()

    try:
        specInfo = workspace.spectrumInfo()
        for index in range(workspace.getNumberHistograms()):
            if specInfo.isMasked(index) or specInfo.isMonitor(index):
                z[index, :] = np.nan
    except:
        pass

    if workspace.isHistogramData():
        if not distribution:
            z /= x[:, 1:] - x[:, 0:-1]
        if histogram2D:
            if len(y) == z.shape[0]:
                y = boundaries_from_points(y)
            x = np.vstack((x, x[-1]))
        else:
            x = 0.5 * (x[:, 0:-1] + x[:, 1:])
            if len(y) == z.shape[0] + 1:
                y = points_from_boundaries(y)
    else:
        if histogram2D:
            if common_x(x):
                x = np.tile(boundaries_from_points(x[0]), z.shape[0] + 1).reshape(z.shape[0] + 1, -1)
            else:
                x = np.vstack((x, x[-1]))
                x = np.array([boundaries_from_points(xi) for xi in x])
            if len(y) == z.shape[0]:
                y = boundaries_from_points(y)
        else:
            if len(y) == z.shape[0] + 1:
                y = points_from_boundaries(y)
    y = np.tile(y, x.shape[1]).reshape(x.shape[1], x.shape[0]).transpose()
    z = np.ma.masked_invalid(z)
    if transpose:
        return y.T, x.T, z.T
    else:
        return x, y, z


def get_uneven_data(workspace, distribution):
    """
    Function to get data for uneven workspace2Ds, such as
    that pcolor, pcolorfast, and pcolormesh will plot axis aligned rectangles

    :param workspace: a workspace2d
    :param distribution: if False, and the workspace contains histogram data,
        the intensity will be divided by the x bin width

    Returns three lists. Each element in the x list is an array of boundaries
    for a spectra. Each element in the y list is a 2 element array with the extents
    of a particular spectra. The z list contains arrays of intensities at bin centers
    """
    z = []
    x = []
    y = []
    nhist = workspace.getNumberHistograms()
    yvals = workspace.getAxis(1).extractValues()
    if workspace.getAxis(1).isText():
        yvals = np.arange(nhist)
    if len(yvals) == nhist:
        yvals = boundaries_from_points(yvals)
    try:
        specInfo = workspace.spectrumInfo()
    except:
        specInfo = None
    for index in range(nhist):
        xvals = workspace.readX(index)
        zvals = workspace.readY(index)
        if workspace.isHistogramData():
            if not distribution:
                zvals = zvals / (xvals[1:] - xvals[0:-1])
        else:
            xvals = boundaries_from_points(xvals)
        if specInfo and specInfo.hasDetectors(index) and (specInfo.isMasked(index) or specInfo.isMonitor(index)):
            zvals = np.full_like(zvals, np.nan, dtype=np.double)
        zvals = np.ma.masked_invalid(zvals)
        z.append(zvals)
        x.append(xvals)
        y.append([yvals[index], yvals[index + 1]])
    return x, y, z


def get_data_uneven_flag(workspace, **kwargs):
    """
    Helper function that allows :meth:`matplotlib.axes.Axes.pcolor`,
    :meth:`matplotlib.axes.Axes.pcolorfast`, and :meth:`matplotlib.axes.Axes.pcolormesh`
    to plot rectangles parallel to the axes even if the data is not
    on a regular grid.

    :param workspace: a workspace2d

    if axisaligned keyword is available and True or if the workspace does
    not have a constant number of bins, it will return true, otherwise false
    """
    aligned = kwargs.pop("axisaligned", False)
    try:
        workspace.blocksize()
    except RuntimeError:
        aligned = True
    return aligned, kwargs


def check_resample_to_regular_grid(ws, **kwargs):
    if isinstance(ws, MatrixWorkspace):
        aligned = kwargs.pop("axisaligned", False)
        if aligned or not ws.isCommonBins():
            return True, kwargs

        x = ws.readX(0)
        difference = np.diff(x)
        if x.size > 1 and not np.allclose(difference[:-1], difference[0]):
            return True, kwargs
    return False, kwargs


# ====================================================
# extract logs
# ====================================================


def get_sample_log(workspace, **kwargs):
    LogName = kwargs.pop("LogName")
    ExperimentInfo = kwargs.pop("ExperimentInfo", 0)
    if isinstance(workspace, MultipleExperimentInfos):
        run = workspace.getExperimentInfo(ExperimentInfo).run()
    else:
        run = workspace.run()
    if not run.hasProperty(LogName):
        raise ValueError("The workspace does not contain the {} sample log".format(LogName))
    tsp = run[LogName]

    try:
        units = tsp.units
    except UnicodeDecodeError as exc:
        mantid.kernel.logger.warning("Error retrieving units for log {}: {}".format(LogName, str(exc)))
        units = "unknown"
    if not isinstance(
        tsp, (mantid.kernel.FloatTimeSeriesProperty, mantid.kernel.Int32TimeSeriesProperty, mantid.kernel.Int64TimeSeriesProperty)
    ):
        raise RuntimeError("This function can only plot Float or Int TimeSeriesProperties objects")
    Filtered = kwargs.pop("Filtered", True)
    if not Filtered:
        # these methods access the unfiltered data
        times = tsp.times.astype("datetime64[us]")
        y = tsp.value
    else:
        times = tsp.filtered_times.astype("datetime64[us]")
        y = tsp.filtered_value
    FullTime = kwargs.pop("FullTime", False)
    StartFromLog = kwargs.pop("StartFromLog", False)
    ShowTimeROI = kwargs.pop("ShowTimeROI", not run.getTimeROI().useAll())  # default is not bother when useAll
    if FullTime:
        x = times.astype(datetime.datetime)
        if ShowTimeROI:
            roi = np.full(x.shape, True, dtype=bool)
            intervals = run.getTimeROI().toTimeIntervals()
            for interval in intervals:
                start = np.timedelta64(interval[0].totalNanoseconds(), "ns") + np.datetime64("1990-01-01T00:00")
                stop = np.timedelta64(interval[1].totalNanoseconds(), "ns") + np.datetime64("1990-01-01T00:00")
                roi &= (x.astype("datetime64[ns]") < start) | (x.astype("datetime64[ns]") > stop)
            kwargs["roi"] = roi
    else:
        # Compute relative time, preserving t=0 at run start. Logs can record before
        # run start and will have negative time offset
        try:
            t0 = run.startTime().to_datetime64().astype("datetime64[us]")
        except RuntimeError:
            mantid.kernel.logger.warning("Workspace has no start time. Assume t0 as first log time.")
            t0 = times[0]
        if not StartFromLog:
            try:
                t0 = run["proton_charge"].times.astype("datetime64[us]")[0]
            except:
                pass  # TODO: Maybe raise a warning?

        if ShowTimeROI:
            x = (times - t0).astype(float) * 1e3
            roi = np.full(x.shape, True, dtype=bool)
            intervals = run.getTimeROI().toTimeIntervals()
            for interval in intervals:
                start = np.timedelta64(interval[0].totalNanoseconds(), "ns") + np.datetime64("1990-01-01T00:00")
                start = (start - t0).astype("datetime64[ns]")
                stop = np.timedelta64(interval[1].totalNanoseconds(), "ns") + np.datetime64("1990-01-01T00:00")
                stop = (stop - t0).astype("datetime64[ns]")
                roi &= (x.astype("datetime64[ns]") < start) | (x.astype("datetime64[ns]") > stop)
            kwargs["roi"] = roi
        x = (times - t0).astype(float) * 1e-6
    return x, y, FullTime, LogName, units, kwargs


# ====================================================
# Plot functionality
# ====================================================


def get_axes_labels(workspace, indices=None, normalize_by_bin_width=True, use_latex=True):
    """
    Get axis labels from a Workspace2D or an MDHistoWorkspace
    Returns a tuple. The first element is the quantity label, such as "Intensity" or "Counts".
    All other elements in the tuple are labels for axes.
    Some of them are latex formatted already.

    If MDWorkspace then the last element will be the values selected by the indices, to be set as title.

    :param workspace: :class:`mantid.api.MatrixWorkspace` or :class:`mantid.api.IMDHistoWorkspace`
    :param indices:
    :param normalize_by_bin_width: bool: Plotting workspace normalized by bin width
    :param use_latex: bool: return y-unit label in Latex form
    """
    if isinstance(workspace, MultipleExperimentInfos):
        axes_labels = ["Intensity"]
        title = ""
        if indices is None:
            dims = workspace.getNonIntegratedDimensions()
        else:
            dims = []
            for n in range(workspace.getNumDims()):
                d = workspace.getDimension(n)
                if indices[n] == slice(None):
                    dims.append(d)
                else:
                    title += "{0}={1:.4}; ".format(d.name, (d.getX(indices[n]) + d.getX(indices[n] + 1)) / 2)
        for d in dims:
            axis_title = d.name.replace("DeltaE", r"$\Delta E$")
            axis_unit = d.getUnits().replace("Angstrom^-1", r"$\AA^{-1}$")
            axis_unit = axis_unit.replace("DeltaE", "meV")
            axis_unit = axis_unit.replace("Angstrom", r"$\AA$")
            axis_unit = axis_unit.replace("MomentumTransfer", r"$\AA^{-1}$")
            axes_labels.append("{0} ({1})".format(axis_title, axis_unit))
        axes_labels.append(title.strip())
    else:
        # For matrix workspaces, return a tuple of ``(YUnit, <other units>)``
        axes_labels = [workspace.YUnitLabel(useLatex=use_latex, plotAsDistribution=normalize_by_bin_width)]
        for index in range(workspace.axes()):
            axis = workspace.getAxis(index)
            unit = axis.getUnit()
            if len(str(unit.symbol())) > 0:
                unit = "{} (${}$)".format(unit.caption(), unit.symbol().latex())
            else:
                unit = unit.caption()
            axes_labels.append(unit)
    return tuple(axes_labels)


def get_data_from_errorbar_container(err_cont):
    """Get plot coordinates and errorbar sizes from ErrorbarContainer"""
    x_segments = _get_x_errorbar_segments(err_cont)
    y_segments = _get_y_errorbar_segments(err_cont)
    x, y, x_errs, y_errs = [], [], None, None
    if x_segments:
        x_errs = []
        for vertex in x_segments:
            x_errs.append((vertex[1][0] - vertex[0][0]) / 2)
            x.append((vertex[0][0] + vertex[1][0]) / 2)
            y.append((vertex[0][1] + vertex[1][1]) / 2)
        if y_segments:
            y_errs = [(vertex[1][1] - vertex[0][1]) / 2 for vertex in y_segments]
    else:
        y_errs = []
        for vertex in y_segments:
            y_errs.append((vertex[1][1] - vertex[0][1]) / 2)
            x.append((vertex[0][0] + vertex[1][0]) / 2)
            y.append((vertex[0][1] + vertex[1][1]) / 2)
    return x, y, x_errs, y_errs


def _get_x_errorbar_segments(err_cont):
    if err_cont.has_xerr:
        return err_cont[2][0].get_segments()
    return None


def _get_y_errorbar_segments(err_cont):
    if err_cont.has_yerr and not err_cont.has_xerr:
        return err_cont[2][0].get_segments()
    elif err_cont.has_yerr and err_cont.has_xerr:
        return err_cont[2][1].get_segments()
    else:
        return None


def get_errorbar_bounds(container):
    min_x, max_x, min_y, max_y = None, None, None, None
    x_segments = _get_x_errorbar_segments(container)
    if x_segments:
        coords = [array[:, 0] for array in x_segments if len(array.shape) == 2]
        if coords:
            max_x = np.max(coords)
            min_x = np.min(coords)
    y_segments = _get_y_errorbar_segments(container)
    if y_segments:
        coords = [array[:, 1] for array in y_segments if len(array.shape) == 2]
        if coords:
            max_y = np.max(coords)
            min_y = np.min(coords)
    return min_x, max_x, min_y, max_y


def errorbars_hidden(err_container):
    """
    Return True if errorbars in ErrorbarContainer are not visible
    :param err_container: ErrorbarContainer to find visibility of
    """
    if not isinstance(err_container, ErrorbarContainer):
        return True
    hidden = True
    for lines in err_container[1:]:
        for line in lines:
            hidden = hidden and (not line.get_visible())
    return hidden


def set_errorbars_hidden(container, hide):
    """
    Set the visibility on all lines in an ErrorbarContainer.

    :param hide: Whether or not to hide the errors.
    :type hide: bool
    """
    if not isinstance(container, ErrorbarContainer):
        return
    # hide gets inverted below, as matplotlib uses `visible`, which has the opposite logic:
    # if hide is True, visible must be False, and vice-versa
    for bar_lines in container[1:]:
        if bar_lines:
            for line in bar_lines:
                line.set_visible(not hide)


# ====================================================
# Waterfall plots
# ====================================================


def set_initial_dimensions(ax):
    # Set the width and height which are used to calculate the offset percentage for waterfall plots.
    # This means that the curves in a waterfall plot are always offset by the same amount, even if the
    # plot limits change.
    x_lim, y_lim = ax.get_xlim(), ax.get_ylim()
    ax.width = x_lim[1] - x_lim[0]
    ax.height = y_lim[1] - y_lim[0]


def remove_and_return_errorbar_cap_lines(ax):
    # Matplotlib holds the line objects representing errorbar caps in the same list as the actual curves on a plot.
    # This causes problems for waterfall plots so here they are removed from the list and placed into a different
    # list, which is returned so they can be readded later.
    errorbar_cap_lines = []
    for line in ax.get_lines():
        # The lines with the label "_nolegend_" are either actual curves with errorbars, or errorbar cap lines.
        # To check if it is an actual curve, we attempt to find the ErrorbarContainer that matches the line object.
        if line.get_label() == "_nolegend_":
            line_is_errorbar_cap = True
            for container in ax.containers:
                if isinstance(container, ErrorbarContainer):
                    if container[0] == line:
                        line_is_errorbar_cap = False
                        break

            if line_is_errorbar_cap:
                errorbar_cap_lines.append(line)
                line.remove()

    return errorbar_cap_lines


def set_waterfall_toolbar_options_enabled(ax):
    toolbar = ax.get_figure().canvas.toolbar

    if toolbar:
        toolbar.waterfall_conversion(ax.is_waterfall())


def get_waterfall_fills(ax):
    return [collection for collection in ax.collections if isinstance(collection, PolyCollection)]


def waterfall_update_fill(ax):
    # Get the colours of each fill so they can be reapplied after updating.
    colours = []
    for collection in ax.collections:
        if isinstance(collection, PolyCollection):
            colours.append(collection.get_facecolor())

    waterfall_remove_fill(ax)
    waterfall_create_fill(ax)

    poly_collections = get_waterfall_fills(ax)
    line_colours = True
    # If there are more fill areas than colours, this means that new curves have been added to the plot
    # (overplotting). In which case, we need to determine whether the fill colours are set to match the line
    # colours by checking that the colour of each fill that existed previously is the same as the line it belongs
    # to. If so, the list of colours is appended to with the colours of the new lines. Otherwise the fills are
    # all set to the same colour and so the list of colours is extended with the same colour for each new curve.
    if len(poly_collections) > len(colours):
        for i in range(len(colours) - 1):
            if convert_color_to_hex(colours[i][0]) != ax.get_lines()[i].get_color():
                line_colours = False
                break

        colours_length = len(colours)
        if line_colours:
            for i in range(colours_length, len(poly_collections)):
                colours.append(ax.get_lines()[i].get_color())
        else:
            for i in range(colours_length, len(poly_collections)):
                colours.append(colours[0])

    for i, collection in enumerate(poly_collections):
        collection.set_color(colours[i])


def apply_waterfall_offset_to_errorbars(ax, line, amount_to_move_x, amount_to_move_y, index):
    for container in ax.containers:
        # Find the ErrorbarContainer that corresponds to the current line.
        if isinstance(container, ErrorbarContainer) and container[0] == line:
            # Shift the data line and the errorbar caps
            for line in (container[0],) + container[1]:
                line.set_xdata(line.get_xdata() + amount_to_move_x)
                line.set_ydata(line.get_ydata() + amount_to_move_y)

                if index == 0:
                    line.set_zorder(len(ax.get_lines()))
                else:
                    line.set_zorder(ax.get_lines()[index - 1].get_zorder() - 1)

            # Shift the errorbars
            for bar_line_col in container[2]:
                segments = bar_line_col.get_segments()
                i = 0
                for point in segments:
                    for row in range(2):
                        point[row][1] += amount_to_move_y[i]
                    for column in range(2):
                        point[column][0] += amount_to_move_x[i]
                    i += 1
                bar_line_col.set_segments(segments)
            bar_line_col.set_zorder((len(ax.get_lines()) - index) + 1)
            break


def convert_single_line_to_waterfall(ax, index, x=None, y=None, need_to_update_fill=False):
    line = ax.get_lines()[index]
    x_data = line.get_xdata()
    y_data = line.get_ydata()
    if ax.get_xscale() == "log":
        amount_to_move_x = (
            x_data * ((1 + ax.waterfall_x_offset / 100) ** index - 1)
            if x is None
            else x_data * (((1 + x / 100) / (1 + ax.waterfall_x_offset / 100)) ** index - 1)
        )
    else:
        amount_to_move_x = (
            np.zeros(x_data.size) + index * ax.width * (ax.waterfall_x_offset / 500)
            if x is None
            else np.zeros(x_data.size) + index * ax.width * ((x - ax.waterfall_x_offset) / 500)
        )
    if ax.get_yscale() == "log":
        amount_to_move_y = (
            y_data * ((1 + ax.waterfall_y_offset / 100) ** index - 1)
            if y is None
            else y_data * (((1 + y / 100) / (1 + ax.waterfall_y_offset / 100)) ** index - 1)
        )
    else:
        amount_to_move_y = (
            np.zeros(y_data.size) + index * ax.height * (ax.waterfall_y_offset / 500)
            if y is None
            else np.zeros(y_data.size) + index * ax.height * ((y - ax.waterfall_y_offset) / 500)
        )

    if line.get_label() == "_nolegend_":
        apply_waterfall_offset_to_errorbars(ax, line, amount_to_move_x, amount_to_move_y, index)
    else:
        line.set_xdata(line.get_xdata() + amount_to_move_x)
        line.set_ydata(line.get_ydata() + amount_to_move_y)

        # Ensures the more offset lines are drawn behind the less offset ones
        if index == 0:
            line.set_zorder(len(ax.get_lines()))
        else:
            line.set_zorder(ax.get_lines()[index - 1].get_zorder() - 1)

    # If the curves are filled and the fill has been set to match the line colour and the line colour has changed
    # then the fill's colour is updated.
    if need_to_update_fill:
        fill = get_waterfall_fill_for_curve(ax, index)
        fill.set_color(line.get_color())


def set_waterfall_fill_visible(ax, index):
    if not ax.waterfall_has_fill():
        return

    # Sets the filled area to match the visibility of the line it belongs to.
    line = ax.get_lines()[index]
    fill = get_waterfall_fill_for_curve(ax, index)
    fill.set_visible(line.get_visible())


def get_waterfall_fill_for_curve(ax, index):
    # Takes the index of a curve and returns that curve's filled area.
    i = 0
    for collection in ax.collections:
        if isinstance(collection, PolyCollection):
            if i == index:
                fill = collection
                break
            i += 1

    return fill


def waterfall_fill_is_line_colour(ax):
    i = 0
    # Check that for each line, the fill area is the same colour as the line.
    for collection in ax.collections:
        if isinstance(collection, PolyCollection):
            line_colour = ax.get_lines()[i].get_color()
            poly_colour = convert_color_to_hex(collection.get_facecolor()[0])
            if line_colour != poly_colour:
                return False
            i += 1
    return True


def waterfall_create_fill(ax):
    if ax.waterfall_has_fill():
        return

    errorbar_cap_lines = remove_and_return_errorbar_cap_lines(ax)

    for i, line in enumerate(ax.get_lines()):
        if ax.get_yscale() == "log":
            bottom_line = [min(line.get_ydata())] * len(line.get_ydata())
        else:
            bottom_line = [min(line.get_ydata()) - ((i * ax.height) / 100)] * len(line.get_ydata())
        fill = ax.fill_between(line.get_xdata(), line.get_ydata(), bottom_line)
        fill.set_zorder((len(ax.get_lines()) - i) + 1)
        set_waterfall_fill_visible(ax, i)

    for cap in errorbar_cap_lines:
        ax.add_line(cap)


def waterfall_remove_fill(ax):
    # Use a temporary list to hold a reference to the collections whilst removing them.
    for poly_collection in list(ax.collections):
        if isinstance(poly_collection, PolyCollection):
            poly_collection.remove()
    ax.get_figure().canvas.draw()


def solid_colour_fill(ax, colour):
    # Add the fill areas if there aren't any already.
    if not ax.waterfall_has_fill():
        waterfall_create_fill(ax)

    for i, collection in enumerate(ax.collections):
        if isinstance(collection, PolyCollection):
            # This function is called every time the colour line edit is changed so it's possible
            # that the current input is not a valid colour, such as if the user hasn't finished entering
            # a colour. So if setting the colour fails, the function just stops.
            try:
                collection.set_color(colour)
            except:
                return

    ax.get_figure().canvas.draw()


def line_colour_fill(ax):
    # Add the fill areas if there aren't any already.
    if not ax.waterfall_has_fill():
        waterfall_create_fill(ax)

    i = 0
    for collection in ax.collections:
        if isinstance(collection, PolyCollection):
            colour = ax.get_lines()[i].get_color()
            collection.set_color(colour)
            # Only want the counter to iterate if the current collection is a PolyCollection (the fill areas) since
            # the axes may have other collections which can be ignored.
            i = i + 1

    ax.get_figure().canvas.draw()


def update_colorbar_scale(figure, image, scale, vmin, vmax):
    """ "
    Updates the colorbar to the scale and limits given.

    :param figure: A matplotlib figure instance
    :param image: The matplotlib image containing the colorbar
    :param scale: The norm scale of the colorbar, this should be a matplotlib colormap norm type
    :param vmin: the minimum value on the colorbar
    :param vmax: the maximum value on the colorbar
    """
    if vmin <= 0 and scale == LogNorm:
        vmin = 0.0001  # Avoid 0 log scale error
        mantid.kernel.logger.warning("Scale is set to logarithmic so non-positive min value has been changed to 0.0001.")

    if vmax <= 0 and scale == LogNorm:
        vmax = 1  # Avoid 0 log scale error
        mantid.kernel.logger.warning("Scale is set to logarithmic so non-positive max value has been changed to 1.")

    image.set_norm(scale(vmin=vmin, vmax=vmax))

    if image.colorbar:
        colorbar = image.colorbar
        locator = None
        if scale == LogNorm:
            locator = LogLocator(subs=np.arange(1, 10))
            if locator.tick_values(vmin=vmin, vmax=vmax).size == 0:
                locator = LogLocator()
                mantid.kernel.logger.warning(
                    "Minor ticks on colorbar scale cannot be shown " "as the range between min value and max value is too large"
                )
            colorbar.set_ticks(locator)


def add_colorbar_label(colorbar, axes):
    """
    Adds a label to the colorbar if every axis on the figure has the same label.
    :param colorbar: the colorbar to label.
    :param axes: the axes that the colorbar belongs to.
    """
    colorbar_labels = [ax.colorbar_label for ax in axes if hasattr(ax, "colorbar_label")]
    if colorbar_labels and colorbar_labels.count(colorbar_labels[0]) == len(colorbar_labels):
        colorbar.set_label(colorbar_labels[0])


def get_images_from_figure(figure):
    """Return a list of images in the given figure excluding any colorbar images"""
    axes = figure.get_axes()

    all_images = []
    for ax in axes:
        all_images += ax.images + [col for col in ax.collections if isinstance(col, QuadMesh) or isinstance(col, Poly3DCollection)]

    # remove any colorbar images
    colorbars = [img.colorbar.solids for img in all_images if img.colorbar]
    images = [img for img in all_images if img not in colorbars]
    return images


def get_axes_from_figure(figure):
    """Return a list of axes in the given figure excluding any colorbar axes"""
    images = get_images_from_figure(figure)
    axes = [img.axes for img in images]

    return axes


def get_legend_handles(ax):
    """
    Get a list of the Line2D and ErrorbarContainer objects to be
    included in the legend so that the order is always the same.
    """
    handles = []
    for line in ax.lines:
        if line.get_label() == "_nolegend_":
            # If the line has no label find the ErrorbarContainer that corresponds to it (if one exists)
            for container in ax.containers:
                if isinstance(container, ErrorbarContainer) and container[0] == line:
                    handles.append(container)
                    break
        else:
            handles.append(line)

    return handles
