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
import mantid.dataobjects


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


def _get_md_data(workspace, normalization, with_error = False):
    """
    generic function to extract data from an MDHisto workspace
    :param workspace: :class:`mantid.api.IMDHistoWorkspace` containing data
    :param normalization: if :class:`mantid.api.MDNormalization.NumEventsNormalization`
        it will divide intensity by the number of corresponding MDEvents
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
    if with_error:
        err2 = workspace.getErrorSquaredArray()*1.
        if normalization == mantid.api.MDNormalization.NumEventsNormalization:
            err2 /= (nev*nev)
        err = numpy.sqrt(err2)
    data = data.squeeze().T
    data = numpy.ma.masked_invalid(data)
    if err is not None:
        err = err.squeeze().T
        err = numpy.ma.masked_invalid(err)
    return dim_arrays, data, err


def _get_md_data2d_bin_bounds(workspace, normalization):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin boundaries in each
    dimension, and data. To be used in 2D plots (pcolor, pcolorfast, pcolormesh)
    Note return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    coordinate, data, _ = _get_md_data(workspace, normalization, with_error=False)
    assert len(coordinate) == 2, 'The workspace is not 2D'
    return coordinate[0], coordinate[1], data


def _get_md_data2d_bin_centers(workspace, normalization):
    """
    Function to transform data in an MDHisto workspace with exactly
    two non-integrated dimension into arrays of bin centers in each
    dimension, and data. To be used in 2D plots (contour, contourf,
    tricontour, tricontourf, tripcolor)
    Note return coordinates are 1d vectors. Use numpy.meshgrid to generate 2d versions
    """
    x, y, data = _get_md_data2d_bin_bounds(workspace, normalization)
    x = points_from_boundaries(x)
    y = points_from_boundaries(y)
    return x, y, data


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
    if isinstance(workspace, mantid.dataobjects.MDHistoWorkspace):
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
