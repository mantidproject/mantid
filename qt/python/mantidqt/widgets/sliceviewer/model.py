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
from mantid.api import MatrixWorkspace, MultipleExperimentInfos
from mantid.simpleapi import BinMD
from mantid.py3compat.enum import Enum
import numpy as np


class WS_TYPE(Enum):
    MDE = 0
    MDH = 1
    MATRIX = 2


class SliceViewerModel(object):
    """Store the workspace to be plotted. Can be MatrixWorkspace, MDEventWorkspace or MDHistoWorkspace"""
    def __init__(self, ws):
        if isinstance(ws, MatrixWorkspace):
            if ws.getNumberHistograms() < 2:
                raise ValueError("workspace must contain at least 2 spectrum")
            if ws.getDimension(0).getNBins() < 2: # Check number of x bins
                raise ValueError("workspace must contain at least 2 bin")
        elif isinstance(ws, MultipleExperimentInfos):
            if ws.isMDHistoWorkspace():
                if len(ws.getNonIntegratedDimensions()) < 2:
                    raise ValueError("workspace must have at least 2 non-integrated dimensions")
            else:
                if ws.getNumDims() < 2:
                    raise ValueError("workspace must have at least 2 dimensions")
        else:
            raise ValueError("only works for MatrixWorkspace and MDWorkspace")

        self._ws = ws

        if self.get_ws_type() == WS_TYPE.MDE:
            self.get_ws = self.get_ws_MDE
            self.get_data = self.get_data_MDE
        else:
            self.get_ws = self._get_ws
            self.get_data = self.get_data_MDH

    def _get_ws(self):
        return self._ws

    def get_ws_MDE(self, slicepoint, bin_params):
        params = {}
        for n in range(self._get_ws().getNumDims()):
            if slicepoint[n] is None:
                params['AlignedDim{}'.format(n)] = '{},{},{},{}'.format(self._get_ws().getDimension(n).name,
                                                                        self._get_ws().getDimension(n).getMinimum(),
                                                                        self._get_ws().getDimension(n).getMaximum(),
                                                                        bin_params[n])
            else:
                params['AlignedDim{}'.format(n)] = '{},{},{},{}'.format(self._get_ws().getDimension(n).name,
                                                                        slicepoint[n]-bin_params[n]/2,
                                                                        slicepoint[n]+bin_params[n]/2,
                                                                        1)
        return BinMD(InputWorkspace=self._get_ws(),
                     OutputWorkspace=self._get_ws().name()+'_rebinned', **params)

    def get_data_MDH(self, slicepoint, transpose=False):
        indices, _ = get_indices(self.get_ws(), slicepoint=slicepoint)
        if transpose:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices]).T
        else:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices])

    def get_data_MDE(self, slicepoint, bin_params, transpose=False):
        if transpose:
            return np.ma.masked_invalid(self.get_ws_MDE(slicepoint, bin_params).getSignalArray().squeeze()).T
        else:
            return np.ma.masked_invalid(self.get_ws_MDE(slicepoint, bin_params).getSignalArray().squeeze())

    def get_dim_info(self, n):
        """
        returns dict of (minimum, maximun, number_of_bins, width, name, units) for dimension n
        """
        dim = self._get_ws().getDimension(n)
        return {'minimum': dim.getMinimum(),
                'maximum': dim.getMaximum(),
                'number_of_bins': dim.getNBins(),
                'width': dim.getBinWidth(),
                'name': dim.name,
                'units': dim.getUnits(),
                'type': self.get_ws_type().name}

    def get_dimensions_info(self):
        """
        returns a list of dict for each dimension conainting dim_info
        """
        return [self.get_dim_info(n) for n in range(self._get_ws().getNumDims())]

    def get_ws_type(self):
        if isinstance(self._get_ws(), MatrixWorkspace):
            return WS_TYPE.MATRIX
        elif isinstance(self._get_ws(), MultipleExperimentInfos):
            if self._get_ws().isMDHistoWorkspace():
                return WS_TYPE.MDH
            else:
                return WS_TYPE.MDE
        else:
            raise ValueError("Unsupported workspace type")
