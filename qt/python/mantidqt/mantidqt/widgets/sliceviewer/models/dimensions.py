# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List, Dict

from mantidqt.widgets.sliceviewer.models.workspaceinfo import WorkspaceInfo


class Dimensions:
    def __init__(self, workspace):
        self._ws = workspace

    def get_dimensions_info(self) -> List[Dict]:
        """
        returns a list of dict for each dimension containing dim_info
        """
        return [self.get_dim_info(n) for n in range(self._ws.getNumDims())]

    def get_dim_info(self, n: int) -> dict:
        """
        returns dict of (minimum :float, maximum :float, number_of_bins :int,
                         width :float, name :str, units :str, type :str, can_rebin: bool, qdim: bool) for dimension n
        """
        workspace = self._ws
        dim = workspace.getDimension(n)
        return {
            'minimum': dim.getMinimum(),
            'maximum': dim.getMaximum(),
            'number_of_bins': dim.getNBins(),
            'width': dim.getBinWidth(),
            'name': dim.name,
            'units': dim.getUnits(),
            'type': WorkspaceInfo.get_ws_type(workspace).name,
            'can_rebin': WorkspaceInfo.can_support_dynamic_rebinning(workspace),
            'qdim': dim.getMDFrame().isQ()
        }

    def get_dim_limits(self, slicepoint, transpose):
        """
        Return a xlim, ylim) for the display dimensions where xlim, ylim are tuples
        :param slicepoint: Sequence containing either a float or None where None indicates a display dimension
        :param transpose: A boolean flag indicating if the display dimensions are transposed
        """
        xindex, yindex = WorkspaceInfo.display_indices(slicepoint, transpose)
        workspace = self._ws
        xdim, ydim = workspace.getDimension(xindex), workspace.getDimension(yindex)
        return (xdim.getMinimum(), xdim.getMaximum()), (ydim.getMinimum(), ydim.getMaximum())
