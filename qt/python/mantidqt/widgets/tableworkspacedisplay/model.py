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
from mantidqt.widgets.tableworkspacedisplay.marked_columns import MarkedColumns
from mantid.kernel import V3D

import six

if six.PY2:
    from functools32 import lru_cache
else:
    from functools import lru_cache


class TableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    ALLOWED_WORKSPACE_TYPES = [PeaksWorkspace, TableWorkspace]

    def __init__(self, ws):
        if not any(isinstance(ws, allowed_type) for allowed_type in self.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

        self.ws = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()
        self.ws_column_types = self.ws.columnTypes()
        self.convert_types = self.map_from_type_name(self.ws_column_types)
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()

    def _get_bool_from_str(self, string):
        string = string.lower()
        if string == "true":
            return True
        elif string == "false":
            return False
        else:
            raise ValueError("'{}' is not a valid bool string.".format(string))

    def _get_v3d_from_str(self, string):
        if '[' in string and ']' in string:
            string = string[1:-1]
        if ',' in string:
            return V3D(*[float(x) for x in string.split(',')])
        else:
            raise RuntimeError("'{}' is not a valid V3D string.".format(string))

    def map_from_type_name(self, column_types):
        convert_types = []
        for type in column_types:
            type = type.lower()
            if 'int' in type:
                convert_types.append(int)
            elif 'double' in type or 'float' in type:
                convert_types.append(float)
            elif 'string' in type:
                convert_types.append(str)
            elif 'bool' in type:
                convert_types.append(self._get_bool_from_str)
            elif 'v3d' in type:
                convert_types.append(self._get_v3d_from_str)
            else:
                raise ValueError("Trying to set data for unknown column type {}".format(type))

        return convert_types

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

    @lru_cache(maxsize=1)
    def is_peaks_workspace(self):
        return isinstance(self.ws, PeaksWorkspace)

    def set_cell_data(self, row, col, data):
        data = self.convert_types[col](data)
        self.ws.setCell(row, col, data)
