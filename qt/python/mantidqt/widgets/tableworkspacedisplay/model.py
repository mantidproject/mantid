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
from mantid.kernel import V3D
from mantidqt.widgets.tableworkspacedisplay.marked_columns import MarkedColumns


class TableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    ALLOWED_WORKSPACE_TYPES = [PeaksWorkspace, TableWorkspace]

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

        self.ws = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()
        self.ws_column_types = self.ws.columnTypes()
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()

    def _get_v3d_from_str(self, string):
        if '[' in string and ']' in string:
            string = string[1:-1]
        if ',' in string:
            return V3D(*[float(x) for x in string.split(',')])
        else:
            raise ValueError("'{}' is not a valid V3D string.".format(string))

    def original_column_headers(self):
        return self._original_column_headers[:]

    def build_current_labels(self):
        return self.marked_columns.build_labels()

    def get_name(self):
        return self.ws.name()

    def get_column_headers(self):
        return self.ws.getColumnNames()

    def get_column(self, index):
        return self.ws.column(index)

    def get_number_of_rows(self):
        return self.ws_num_rows

    def get_number_of_columns(self):
        return self.ws_num_cols

    def get_column_header(self, index):
        return self.get_column_headers()[index]

    def is_peaks_workspace(self):
        return isinstance(self.ws, PeaksWorkspace)

    def set_cell_data(self, row, col, data, is_v3d):
        # if the cell contains V3D data, construct a V3D object
        # from the string to that it can be properly set
        if is_v3d:
            data = self._get_v3d_from_str(data)
        self.ws.setCell(row, col, data)
