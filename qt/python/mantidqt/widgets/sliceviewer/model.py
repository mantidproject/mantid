# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from mantid.plots.helperfunctions import get_indices
import numpy as np


class SliceViewerModel(object):
    """Store the workspace to be plotted. Can be MatrixWorkspace, MDEventWorkspace or MDHistoWorkspace"""
    def __init__(self, ws):
        if len(ws.getNonIntegratedDimensions()) < 2:
            raise ValueError("workspace must have at least 2 non-integrated dimensions")

        if not ws.isMDHistoWorkspace():
            raise ValueError("currenly only works for MDHistoWorkspace")

        self._ws = ws

    def get_ws(self):
        return self._ws

    def get_data(self, slicepoint, transpose=False):
        indices, _ = get_indices(self.get_ws(), slicepoint=slicepoint)
        if transpose:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices]).T
        else:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices])

    def get_dim_info(self, n):
        """
        returns dict of (minimum, maximun, number_of_bins, width, name, units) for dimension n
        """
        dim = self.get_ws().getDimension(n)
        return {'minimum': dim.getMinimum(),
                'maximum': dim.getMaximum(),
                'number_of_bins': dim.getNBins(),
                'width': dim.getBinWidth(),
                'name': dim.name,
                'units': dim.getUnits()}

    def get_dimensions_info(self):
        """
        returns a list of dict for each dimension conainting dim_info
        """
        return [self.get_dim_info(n) for n in range(self.get_ws().getNumDims())]
