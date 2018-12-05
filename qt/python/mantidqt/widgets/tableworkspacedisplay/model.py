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
from mantid.py3compat import Enum
from mantidqt.widgets.matrixworkspacedisplay.test_helpers.matrixworkspacedisplay_common import MockWorkspace


class TableDisplayColumnType(Enum):
    NUMERIC = 1
    TEST = 123


class TableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    def __init__(self, ws):
        if not isinstance(ws, TableWorkspace) \
                and not isinstance(ws, PeaksWorkspace) \
                and not isinstance(ws, MockWorkspace):
            raise ValueError("The workspace type is not supported: {0}".format(type(ws)))

        self.ws = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()
        self.column_types = [TableDisplayColumnType.NUMERIC] * self.ws_num_cols
        self.set_column_type(0, TableDisplayColumnType.TEST)

        #     TODO figure out a way to store column TYPE
        # set to Numeric by default
        # How can the type be set

    def set_column_type(self, index, type):
        # find a way to check if type is a TableDisplayColumnType?
        self.column_types[index] = type

    def get_column_type(self, index):
        return self.column_types[index]

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
