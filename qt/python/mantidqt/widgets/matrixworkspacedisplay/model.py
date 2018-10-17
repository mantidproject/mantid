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

from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModel, \
    MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayModel(object):
    def __init__(self, ws):
        self._ws = ws

    def get_name(self):
        return self._ws.name()

    def get_item_model(self):
        return MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.x), \
               MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.y), \
               MatrixWorkspaceTableViewModel(self._ws, MatrixWorkspaceTableViewModelType.e)
