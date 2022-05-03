# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.
from mantid.api import IPeaksWorkspace, ITableWorkspace
from mantid.kernel import V3D
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.marked_columns import MarkedColumns
from contextlib import contextmanager


class TableWorkspaceColumnTypeMapping(object):
    """
    Enum can't be used here because the original C++ code maps the types to integers.
    Comparing the integer to a Python enum does not work, as it does not simply compare
    the integer values. So the types are just stored as integers here
    """
    NotSet = -1000
    NoType = 0
    X = 1
    Y = 2
    Z = 3
    XERR = 4
    YERR = 5
    LABEL = 6


@contextmanager
def block_model_replacement(model):
    model.block_model_replace = True
    yield
    model.block_model_replace = False


class TableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'
    EDITABLE_COLUMN_NAMES = ['h', 'k', 'l']

    ALLOWED_WORKSPACE_TYPES = [ITableWorkspace]

    @classmethod
    def supports(cls, ws: ITableWorkspace):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        if not any(isinstance(ws, allowed_type) for allowed_type in cls.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

    def __init__(self, ws: ITableWorkspace):
        """
        Initialise the model with the workspace
        :param ws: Workspace to be used for providing data
        :raises ValueError: if the workspace is not supported
        """
        self.supports(ws)

        self.ws: ITableWorkspace = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()
        self.block_model_replace = False
        # loads the types of the columns
        for col in range(self.ws_num_cols):
            plot_type = self.ws.getPlotType(col)
            if plot_type == TableWorkspaceColumnTypeMapping.X:
                self.marked_columns.add_x(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.Y:
                self.marked_columns.add_y(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.YERR:
                err_for_column = self.ws.getLinkedYCol(col)
                if err_for_column >= 0:
                    self.marked_columns.add_y_err(ErrorColumn(col, err_for_column))

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

    def get_cell(self, row, column):
        return self.ws.cell(row, column)

    def get_number_of_rows(self):
        return self.ws_num_rows

    def get_number_of_columns(self):
        return self.ws_num_cols

    def get_column_header(self, index):
        return self.get_column_headers()[index]

    def is_editable_column(self, icol):
        if self.is_peaks_workspace():
            return self.ws.getColumnNames()[icol] in self.EDITABLE_COLUMN_NAMES
        else:
            return not self.ws.isColumnReadOnly(icol)

    def is_peaks_workspace(self):
        return isinstance(self.ws, IPeaksWorkspace)

    def set_cell_data(self, row, col, data, is_v3d):
        if self.is_peaks_workspace():
            p = self.ws.getPeak(row)
            if self.ws.getColumnNames()[col] == "h":
                p.setH(data)
            elif self.ws.getColumnNames()[col] == "k":
                p.setK(data)
            elif self.ws.getColumnNames()[col] == "l":
                p.setL(data)
        else:
            # if the cell contains V3D data, construct a V3D object
            # from the string to that it can be properly set
            if is_v3d:
                data = self._get_v3d_from_str(data)
            # The False stops the replace workspace ADS event from being triggered
            # The replace event causes the TWD model to be replaced, which in turn
            # deletes the previous table item objects, however this happens
            # at the same time as we are trying to locally update the data in the
            # item object itself, which causes a Qt exception that the object has
            # already been deleted and a crash
            with block_model_replacement(self):
                self.ws.setCell(row, col, data)

    def workspace_equals(self, workspace_name):
        return self.ws.name() == workspace_name

    def delete_rows(self, selected_rows):
        from mantid.simpleapi import DeleteTableRows
        DeleteTableRows(self.ws, selected_rows)

    def get_statistics(self, selected_columns):
        from mantid.simpleapi import StatisticsOfTableWorkspace
        stats = StatisticsOfTableWorkspace(self.ws, selected_columns)
        return stats

    def sort(self, column_index, sort_ascending):
        from mantid.simpleapi import SortPeaksWorkspace, SortTableWorkspace
        column_name = self.ws.getColumnNames()[column_index]
        if self.is_peaks_workspace():
            SortPeaksWorkspace(InputWorkspace=self.ws,
                               OutputWorkspace=self.ws,
                               ColumnNameToSortBy=column_name,
                               SortAscending=sort_ascending)
        else:
            SortTableWorkspace(InputWorkspace=self.ws,
                               OutputWorkspace=self.ws,
                               Columns=column_name,
                               Ascending=sort_ascending)

    def set_column_type(self, col, type, linked_col_index=-1):
        self.ws.setPlotType(col, type, linked_col_index)
