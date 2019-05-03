# coding=utf-8
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

from mantid.api import MatrixWorkspace
from mantid.dataobjects import EventWorkspace, Workspace2D
from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModel, \
    MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayModel(object):
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    ALLOWED_WORKSPACE_TYPES = [MatrixWorkspace, Workspace2D, EventWorkspace]

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        if not any(isinstance(ws, allowed_type) for allowed_type in cls.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

    def __init__(self, ws):
        """
        Initialise the model with the workspace
        :param ws: Workspace to be used for providing data
        :raises ValueError: if the workspace is not supported
        """
        self.supports(ws)
        self._ws = ws

    def get_name(self):
        return self._ws.name()

    def get_item_model(self):
        return (MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.x),
                MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.y),
                MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.e))

    def workspace_equals(self, workspace_name):
        return workspace_name == self._ws.name()
