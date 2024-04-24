# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.
from mantid.api import WorkspaceGroup
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.marked_columns import MarkedColumns
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceColumnTypeMapping
from collections import defaultdict


class GroupTableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = "{}-{}"
    BIN_PLOT_LEGEND_STRING = "{}-bin-{}"
    EDITABLE_COLUMN_NAMES = ["h", "k", "l"]

    ALLOWED_WORKSPACE_TYPES = [WorkspaceGroup]

    @classmethod
    def supports(cls, ws: WorkspaceGroup):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        if not any(isinstance(ws, allowed_type) for allowed_type in cls.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

    def __init__(self, ws: WorkspaceGroup):
        """
        Initialise the model with the workspace
        :param ws: Workspace to be used for providing data
        :raises ValueError: if the workspace is not supported
        """
        self.supports(ws)

        self.ws: WorkspaceGroup = ws
        self.ws_num_rows = sum(peakWs.rowCount() for peakWs in ws)
        self.ws_num_cols = self.ws[0].columnCount() + 1
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()
        self.block_model_replace = False
        # loads the types of the columns
        for col in range(1, len(self._original_column_headers)):
            plot_type = self.ws[0].getPlotType(col - 1)
            if plot_type == TableWorkspaceColumnTypeMapping.X:
                self.marked_columns.add_x(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.Y:
                self.marked_columns.add_y(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.YERR:
                err_for_column = self.ws[0].getLinkedYCol(col - 1)
                if err_for_column >= 0:
                    self.marked_columns.add_y_err(ErrorColumn(col, err_for_column))

    def _get_group_and_workspace_indcies(self, row_indicies):
        cumulative_size = 0
        ws_range_limits = []
        for peaksWs in self.ws:
            ws_range_limits.append((cumulative_size, cumulative_size + len(peaksWs)))
            cumulative_size += len(peaksWs)

        row_to_ws_index = defaultdict(list)

        for row_index in list(map(int, row_indicies.split(","))):
            for group_index, ws_range_limit in enumerate(ws_range_limits):
                if row_index >= ws_range_limit[0] and row_index < ws_range_limit[1]:
                    row_to_ws_index[group_index].append(row_index - ws_range_limit[0])
                    break

        return row_to_ws_index

    def original_column_headers(self):
        return self._original_column_headers[:]

    def build_current_labels(self):
        return self.marked_columns.build_labels()

    def get_name(self):
        return self.ws.name()

    def get_column_headers(self):
        return ["Group Index"] + self.ws[0].getColumnNames()

    def get_column(self, index):
        column_data = []

        for i, ws_item in enumerate(self.ws):
            if index == 0:
                column_data.extend([i] * ws_item.rowCount())
            else:
                column_data.extend(ws_item.column(index - 1))

        return column_data

    def get_number_of_rows(self):
        return self.ws_num_rows

    def get_number_of_columns(self):
        return self.ws_num_cols

    def get_column_header(self, index):
        return self.get_column_headers()[index]

    def is_editable_column(self, icol):
        return self.get_column_headers()[icol] in self.EDITABLE_COLUMN_NAMES

    def workspace_equals(self, workspace_name):
        return self.ws.name() == workspace_name

    def set_column_type(self, col, type, linked_col_index=-1):
        for peaksWs in self.ws:
            peaksWs.setPlotType(col, type, linked_col_index)

    def get_cell(self, row, column):
        row_to_ws_index = self._get_group_and_workspace_indcies(f"{row}")

        group_index, ws_index = next(iter(row_to_ws_index.items()))

        return self.ws[group_index][ws_index]

    def set_cell_data(self, row, col, data, is_v3d):
        cumulative_size = 0
        col = col - 1
        for peaksWs in self.ws:
            if cumulative_size + len(peaksWs) > row:
                local_index = row - cumulative_size

                p = peaksWs[local_index]
                if self.ws.getColumnNames()[col] == "h":
                    p.setH(data)
                elif self.ws.getColumnNames()[col] == "k":
                    p.setK(data)
                elif self.ws.getColumnNames()[col] == "l":
                    p.setL(data)

            cumulative_size += len(peaksWs)

    def delete_rows(self, selected_rows):
        from mantid.simpleapi import DeleteTableRows

        row_to_ws_index = self._get_group_and_workspace_indcies(selected_rows)

        for group_index in row_to_ws_index:
            DeleteTableRows(self.ws[group_index], ",".join(map(str, row_to_ws_index[group_index])))

    def get_statistics(self, selected_columns):

        from mantid.simpleapi import StatisticsOfTableWorkspace

        stats = StatisticsOfTableWorkspace(self.ws, selected_columns)
        return stats

    def sort(self, column_index, sort_ascending):
        from mantid.simpleapi import SortPeaksWorkspace

        if column_index == 0:
            return

        column_name = self.get_column_headers()[column_index]

        for peakWs in self.ws:
            SortPeaksWorkspace(
                InputWorkspace=peakWs,
                OutputWorkspace=peakWs,
                ColumnNameToSortBy=column_name,
                SortAscending=sort_ascending,
            )
