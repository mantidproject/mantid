# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std
from unittest.mock import MagicMock

# 3rd party imports
from mantid.api import (MatrixWorkspace, IMDEventWorkspace, IMDHistoWorkspace,
                        SpecialCoordinateSystem)
from mantid.geometry import IMDDimension
import numpy as np


def create_mock_histoworkspace(ndims: int, coords: SpecialCoordinateSystem, extents: tuple,
                               signal: np.array, error: np.array, nbins: tuple, names: tuple,
                               units: tuple, isq: tuple):
    """
    :param ndims: The number of dimensions
    :param coords: MD coordinate system
    :param extents: Extents of each dimension
    :param signal: Array to be returned as signal
    :param error: Array to be returned as errors
    :param nbins: Number of bins in each dimension
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param isq: Boolean for each dimension defining if Q or not
    """
    ws = create_mock_workspace(IMDHistoWorkspace, coords, has_oriented_lattice=False, ndims=ndims)
    ws.getSignalArray.return_value = signal
    ws.getErrorSquaredArray.return_value = error
    return add_dimensions(ws, names, isq, extents, nbins, units)


def create_mock_mdeventworkspace(ndims: int, coords: SpecialCoordinateSystem, extents: tuple,
                                 names: tuple, units: tuple, isq: tuple):
    """
    :param ndims: The number of dimensions
    :param coords: MD coordinate system
    :param extents: Extents of each dimension
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    :param isq: Boolean for each dimension defining if Q or not
    """
    ws = create_mock_workspace(IMDEventWorkspace, coords, has_oriented_lattice=False, ndims=ndims)
    return add_dimensions(ws, names, isq, extents, nbins=(1, ) * ndims, units=units)


def create_mock_matrixworkspace(x_axis: tuple,
                                y_axis: tuple,
                                distribution: bool,
                                names: tuple,
                                units: tuple = None):
    """
    :param x_axis: X axis values
    :param y_axis: Y axis values
    :param distribution: Value of distribution flag
    :param names: The name of each dimension
    :param units: Unit labels for each dimension
    """
    ws = MagicMock(MatrixWorkspace)
    ws.getNumDims.return_value = 2
    ws.getNumberHistograms.return_value = len(y_axis)
    ws.isDistribution.return_value = distribution
    extents = (x_axis[0], x_axis[-1], y_axis[0], y_axis[-1])
    nbins = (len(x_axis) - 1, len(y_axis) - 1)
    return add_dimensions(ws, names, (False, False), extents, nbins, units)


def create_mock_workspace(ws_type,
                          coords: SpecialCoordinateSystem = None,
                          has_oriented_lattice: bool = None,
                          ndims: int = 2):
    """
    :param ws_type: Used this as spec for Mock
    :param coords: MD coordinate system for MD workspaces
    :param has_oriented_lattice: If the mock should claim to have an attached a lattice
    :param ndims: The number of dimensions
    """
    ws = MagicMock(spec=ws_type)
    if hasattr(ws, 'getExperimentInfo'):
        ws.getNumDims.return_value = ndims
        if ws_type == IMDHistoWorkspace:
            ws.isMDHistoWorkspace.return_value = True
            ws.getNonIntegratedDimensions.return_value = [MagicMock(), MagicMock()]
        else:
            ws.isMDHistoWorkspace.return_value = False

        ws.getSpecialCoordinateSystem.return_value = coords
        expt_info = MagicMock()
        sample = MagicMock()
        sample.hasOrientedLattice.return_value = has_oriented_lattice
        expt_info.sample.return_value = sample
        ws.getExperimentInfo.return_value = expt_info
    elif hasattr(ws, 'getNumberHistograms'):
        ws.getNumDims.return_value = 2
        ws.getNumberHistograms.return_value = 3
        mock_dimension = MagicMock()
        mock_dimension.getNBins.return_value = 3
        ws.getDimension.return_value = mock_dimension
    return ws


def add_dimensions(mock_ws,
                   names,
                   isq,
                   extents: tuple = None,
                   nbins: tuple = None,
                   units: tuple = None):
    """
    :param mock_ws: An existing mock workspace object
    :param names: The name of each dimension
    :param isq: Boolean for each dimension defining if Q or not
    :param extents: Extents of each dimension
    :param nbins: Number of bins in each dimension
    :param units: Unit labels for each dimension
    """
    def create_dimension(index):
        dimension = MagicMock(spec=IMDDimension)
        dimension.name = names[index]
        mdframe = MagicMock()
        mdframe.isQ.return_value = isq[index]
        dimension.getMDFrame.return_value = mdframe
        if units is not None:
            dimension.getUnits.return_value = units[index]
        if extents is not None:
            dim_min, dim_max = extents[2 * index], extents[2 * index + 1]
            dimension.getMinimum.return_value = dim_min
            dimension.getMaximum.return_value = dim_max
            if nbins is not None:
                bin_width = (dim_max - dim_min) / nbins[index]
                dimension.getBinWidth.return_value = bin_width
                dimension.getX.side_effect = lambda i: dim_min + bin_width * i
                dimension.getNBins.return_value = nbins[index]
        return dimension

    dimensions = [create_dimension(index) for index in range(len(names))]
    mock_ws.getDimension.side_effect = lambda index: dimensions[index]
    return mock_ws
