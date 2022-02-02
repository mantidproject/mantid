# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List, Dict

from mantidqt.widgets.sliceviewer.models.workspaceinfo import WorkspaceInfo


class Dimensions:
    @staticmethod
    def get_dimensions_info(workspace) -> List[Dict]:
        """
        returns a list of dict for each dimension containing dim_info
        """
        return [Dimensions.get_dim_info(workspace, n) for n in range(workspace.getNumDims())]

    @staticmethod
    def get_dim_info(workspace, n: int) -> dict:
        """
        returns dict of (minimum :float, maximum :float, number_of_bins :int,
                         width :float, name :str, units :str, type :str, can_rebin: bool, qdim: bool) for dimension n
        """
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

    @staticmethod
    def get_dim_limits(workspace, slicepoint, transpose):
        """
        Return a xlim, ylim) for the display dimensions where xlim, ylim are tuples
        :param slicepoint: Sequence containing either a float or None where None indicates a display dimension
        :param transpose: A boolean flag indicating if the display dimensions are transposed
        """
        xindex, yindex = WorkspaceInfo.display_indices(slicepoint, transpose)
        xdim, ydim = workspace.getDimension(xindex), workspace.getDimension(yindex)
        return (xdim.getMinimum(), xdim.getMaximum()), (ydim.getMinimum(), ydim.getMaximum())
