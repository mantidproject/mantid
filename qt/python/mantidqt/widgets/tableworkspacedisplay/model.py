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

from mantid.dataobjects import PeaksWorkspace, TableWorkspace


class TableWorkspaceDisplayModel(object):
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    def __init__(self, ws):
        if not isinstance(ws, TableWorkspace) and not isinstance(ws, PeaksWorkspace):
            raise ValueError("The workspace type is not supported: {0}".format(type(ws)))

        self._ws = ws

    def get_name(self):
        return self._ws.name()

    def get_column_headers(self):
        return self._ws.getColumnNames()

    def get_column(self, index):
        return self._ws.column(index)

    def get_number_of_rows(self):
        return self._ws.rowCount()

    def get_number_of_columns(self):
        return self._ws.columnCount()

    def is_peaks_workspace(self):
        return isinstance(self._ws, PeaksWorkspace)
