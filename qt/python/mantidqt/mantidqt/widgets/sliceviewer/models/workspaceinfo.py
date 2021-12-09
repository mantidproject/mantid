# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from enum import Enum
from typing import Sequence, Optional

from mantid.api import MatrixWorkspace, MultipleExperimentInfos


class WS_TYPE(Enum):
    MDE = 0
    MDH = 1
    MATRIX = 2


class WorkspaceInfo:
    @staticmethod
    def get_ws_type(workspace) -> WS_TYPE:
        if isinstance(workspace, MatrixWorkspace):
            return WS_TYPE.MATRIX
        elif isinstance(workspace, MultipleExperimentInfos):
            if workspace.isMDHistoWorkspace():
                return WS_TYPE.MDH
            else:
                return WS_TYPE.MDE
        else:
            raise ValueError("Unsupported workspace type")

    @staticmethod
    def can_support_dynamic_rebinning(workspace) -> bool:
        """
        Check if the given workspace can multiple BinMD calls.
        """
        ws_type = WorkspaceInfo.get_ws_type(workspace)
        return ws_type == WS_TYPE.MDE or (ws_type == WS_TYPE.MDH and workspace.hasOriginalWorkspace(
            0) and workspace.getOriginalWorkspace(0).getNumDims() == workspace.getNumDims())

    @staticmethod
    def display_indices(slicepoint: Sequence[Optional[float]], transpose: bool = False):
        """
        Given a slicepoint sequence return the indices of the display
        dimensions.
        :param slicepoint: ND sequence of either None or float. A float defines the point
                        in that dimension for the slice.
        :param transpose: If True then swap the indices before return
        """
        xindex = slicepoint.index(None)
        yindex = slicepoint.index(None, xindex + 1)
        if transpose:
            return yindex, xindex
        else:
            return xindex, yindex
